/*
   Copyright 2018 Marius Appel <marius.appel@uni-muenster.de>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef STREAM_H
#define STREAM_H

#include <boost/process.hpp>
#include "image_collection_cube.h"

//constexpr chunk_size_btyx udf_type_aggregate_t(chunk_size_btyx in) {
//    return chunk_size_btyx{in[0],1,in[2],in[3]};
//}
//constexpr chunk_size_btyx udf_type_aggregate_s(chunk_size_btyx in) {
//    return chunk_size_btyx{in[0],in[1],1,1};
//}
//constexpr chunk_size_btyx udf_type_apply_pixel(chunk_size_btyx in) {
//    return chunk_size_btyx{1,in[1],in[2],in[3]};
//}

/**
 * Assumptions for the implementation below:
 * 1. the stream operation will not change the spatial / temporal extent of a chunk
 * 2. except boundary chunks, all result chunks will have the same dimensions
 * 3. the size of the output chunk is a function of the size of an input chunk, the function is linear in each dimension
 */

/**
 * TODO:
 *  - stream axes + bands
 *  - demo dataset online (COG GTiff) + case study
 *  - parallel execution
 */

class stream_cube : public cube {
   public:
    stream_cube(std::shared_ptr<image_collection_cube> in_cube, std::string cmd) : _in_cube(in_cube), _cmd(cmd), cube(std::make_shared<cube_st_reference>(in_cube->st_reference())) {
        // Test CMD and find out what size comes out.
        cube_size_tyx tmp = _in_cube->chunk_size(0);
        cube_size_btyx csize_in = {_in_cube->bands().count(), tmp[0], tmp[1], tmp[2]};

        void* test_buf = calloc(csize_in[0] * csize_in[1] * csize_in[2] * csize_in[3], sizeof(double));

        boost::process::opstream in;
        boost::asio::io_service ios;
        std::future<std::vector<char>> outdata;
        boost::process::child c(cmd, boost::process::std_out > outdata, ios, boost::process::std_in < in, boost::process::env["GDALCUBES_STREAMING"] = "1");
        // TODO: Error handling, check whether command can be executed

        int size[] = {csize_in[0], csize_in[1], csize_in[2], csize_in[3]};
        in.write((char*)(size), sizeof(int) * 4);

        // TODO: write band names?
        for (uint16_t i = 0; i < in_cube->bands().count(); ++i) {
            int str_size = in_cube->bands().get(i).name.size();
            in.write((char*)(&str_size), sizeof(int));
            in.write(in_cube->bands().get(i).name.c_str(), sizeof(char) * str_size);
        }
        double* dims = (double*)calloc(csize_in[1] + csize_in[2] + csize_in[3], sizeof(double));
        for (uint32_t i = 0; i < csize_in[1]; ++i) {
            dims[i] = (in_cube->st_reference().t0() + in_cube->st_reference().dt() * i).to_double();
        }
        for (uint32_t i = csize_in[1]; i < csize_in[1] + csize_in[2]; ++i) {
            dims[i] = in_cube->st_reference().win().bottom + i * in_cube->st_reference().dy();
        }
        for (uint32_t i = csize_in[1] + csize_in[2]; i < csize_in[1] + csize_in[2] + csize_in[3]; ++i) {
            dims[i] = in_cube->st_reference().win().left + i * in_cube->st_reference().dx();
        }
        in.write((char*)(dims), sizeof(double) * (csize_in[1] + csize_in[2] + csize_in[3]));
        free(dims);
        int str_size = in_cube->st_reference().proj().size();
        in.write((char*)(&str_size), sizeof(int));
        in.write(_st_ref->proj().c_str(), sizeof(char) * str_size);
        in.write(((char*)test_buf), sizeof(double) * csize_in[0] * csize_in[1] * csize_in[2] * csize_in[3]);
        in.flush();

        boost::system::error_code result;
        ios.run(result);
        if (result.value() != 0)
            throw std::string("ERROR in stream_cube::read_chunk(): external program returned status (" + std::to_string(result.value()) + ") --> " + result.message());

        auto out = outdata.get();

        cube_size_btyx csize_out_1 = {((int*)out.data())[0], ((int*)out.data())[1], ((int*)out.data())[2], ((int*)out.data())[3]};
        free(test_buf);

        for (uint16_t ib = 0; ib < csize_out_1[0]; ++ib) {
            band b("band" + std::to_string(ib + 1));
            b.no_data_value = std::to_string(NAN);
            b.type = "float64";
            _bands.add(b);
        }

        _chunk_size = {csize_out_1[1], csize_out_1[2], csize_out_1[3]};

        _size[0] = csize_out_1[0];

        // Make an optimistic guess
        if (csize_out_1[1] == 1) {
            _size[1] = in_cube->count_chunks_t();
            _st_ref->nt(_size[1]);
        } else if (csize_out_1[1] == csize_in[1]) {
            _size[1] = in_cube->size()[1];
        } else
            throw std::string("ERROR in stream_cube::stream_cube(): could not derive size of result cube");

        if (csize_out_1[2] == 1) {
            _size[2] = in_cube->count_chunks_t();
            _st_ref->ny() = _size[2];
        } else if (csize_out_1[2] == csize_in[2]) {
            _size[2] = in_cube->size()[2];
        } else
            throw std::string("ERROR in stream_cube::stream_cube(): could not derive size of result cube");

        if (csize_out_1[3] == 1) {
            _size[3] = in_cube->count_chunks_t();
            _st_ref->ny() = _size[3];
        } else if (csize_out_1[3] == csize_in[3]) {
            _size[3] = in_cube->size()[3];
        } else
            throw std::string("ERROR in stream_cube::stream_cube(): could not derive size of result cube");
    }

    std::shared_ptr<chunk_data> read_chunk(chunkid_t id) override;

    nlohmann::json make_constructible_json() override {
        nlohmann::json out;
        out["cube_type"] = "stream";
        out["command"] = _cmd;
        out["in_cube"] = _in_cube->make_constructible_json();
        return out;
    }

   protected:
    std::shared_ptr<image_collection_cube> _in_cube;
    std::string _cmd;
};

#endif  //STREAM_H
