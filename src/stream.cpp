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

#include "stream.h"
#include <iostream>
#include <thread>

std::shared_ptr<chunk_data> stream_cube::read_chunk(chunkid_t id) {
    std::shared_ptr<chunk_data> out = std::make_shared<chunk_data>();
    if (id < 0 || id >= count_chunks())
        return out;  // chunk is outside of the cube, we don't need to read anything.
    return stream_chunk_stdin(_in_cube->read_chunk(id));
}

std::shared_ptr<chunk_data> stream_cube::stream_chunk_stdin(std::shared_ptr<chunk_data> data) {
    std::shared_ptr<chunk_data> out = std::make_shared<chunk_data>();

    int size[] = {data->size()[0], data->size()[1], data->size()[2], data->size()[3]};
    if (size[0] * size[1] * size[2] * size[3] == 0) {
        return out;
    }

    boost::process::opstream in;

    boost::asio::io_service ios;
    std::future<std::vector<char>> outdata;
    std::future<std::vector<char>> outerr;


    boost::process::child c(_cmd, boost::process::std_out > outdata, ios, boost::process::std_in < in, boost::process::std_err > outerr,  boost::process::env["GDALCUBES_STREAMING"] = "1");


    std::string proj = _in_cube->st_reference().proj();

    in.write((char*)(size), sizeof(int) * 4);

    for (uint16_t i = 0; i < _in_cube->bands().count(); ++i) {
        int str_size = _in_cube->bands().get(i).name.size();
        in.write((char*)(&str_size), sizeof(int));
        in.write(_in_cube->bands().get(i).name.c_str(), sizeof(char) * str_size);
    }
    double* dims = (double*)calloc(size[1] + size[2] + size[3], sizeof(double));
    for (uint32_t i = 0; i < size[1]; ++i) {
        dims[i] = (_in_cube->st_reference().t0() + _in_cube->st_reference().dt() * i).to_double();
    }
    for (uint32_t i = size[1]; i < size[1] + size[2]; ++i) {
        dims[i] = _in_cube->st_reference().win().bottom + i * _in_cube->st_reference().dy();
    }
    for (uint32_t i = size[1] + size[2]; i < size[1] + size[2] + size[3]; ++i) {
        dims[i] = _in_cube->st_reference().win().left + i * _in_cube->st_reference().dx();
    }
    in.write((char*)(dims), sizeof(double) * (size[1] + size[2] + size[3]));
    free(dims);

    int str_size = proj.size();
    in.write((char*)(&str_size), sizeof(int));
    in.write(proj.c_str(), sizeof(char) * str_size);
    in.write(((char*)(data->buf())), sizeof(double) * data->size()[0] * data->size()[1] * data->size()[2] * data->size()[3]);
    in.flush();

    boost::system::error_code result;
    ios.run(result);
    if (result.value() != 0)
        throw std::string("ERROR in stream_cube::read_chunk(): external program returned status (" + std::to_string(result.value()) + ") --> " + result.message());




    std::vector<char> odat = outdata.get();
    if (!_log_output.empty()) {
        if (_log_output == "stdout") {
            std::cout << outerr.get().data() << std::endl;
        } else if (_log_output == "stderr") {
            std::cerr << outerr.get().data() << std::endl;
        }
        else {
            std::ofstream flog(_log_output, std::ios_base::out | std::ios_base::app);
            if (flog.fail()) {
                std::cout << "WARNING in tream_cube::stream_chunk_stdin(): cannot open file'" << _log_output  << "' for writing output of streaming.";
            }
            else {
                flog << outerr.get().data();
                flog.close();
            }
        }
    }

    chunk_size_btyx out_size = {(uint32_t)(((int*)odat.data())[0]), (uint32_t)(((int*)odat.data())[1]), (uint32_t)(((int*)odat.data())[2]), (uint32_t)(((int*)odat.data())[3])};
    out->size(out_size);

    // Fill buffers accordingly
    out->buf(calloc(out_size[0] * out_size[1] * out_size[2] * out_size[3], sizeof(double)));
    memcpy(out->buf(), odat.data() + (4 * sizeof(int)), sizeof(double) * out_size[0] * out_size[1] * out_size[2] * out_size[3]);

    return out;
}