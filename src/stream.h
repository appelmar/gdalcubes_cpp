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

/**
 * Assumptions for the implementation below:
 * 1. the stream operation will not change the spatial / temporal extent of a chunk
 * 2. except boundary chunks, all result chunks will have the same dimensions
 * 3. the size of the output chunk is a function of the size of an input chunk, the function is linear in each dimension
 */

class stream_cube : public cube {
   public:
    stream_cube(std::shared_ptr<image_collection_cube> in_cube, std::string cmd, std::string log_output = "") : _in_cube(in_cube), _cmd(cmd), _log_output(log_output), cube(std::make_shared<cube_st_reference>(in_cube->st_reference())) {
        // Test CMD and find out what size comes out.
        cube_size_tyx tmp = _in_cube->chunk_size(0);
        cube_size_btyx csize_in = {_in_cube->bands().count(), tmp[0], tmp[1], tmp[2]};

        // do not read original chunk data (which can be expensive) but simply stream a dummy chunk with proper size here
        std::shared_ptr<chunk_data> dummy_chunk = std::make_shared<chunk_data>();
        dummy_chunk->size(csize_in);
        dummy_chunk->buf(calloc(csize_in[0] * csize_in[1] * csize_in[2] * csize_in[3], sizeof(double)));

        std::shared_ptr<chunk_data> c0 = stream_chunk_stdin(dummy_chunk);

        for (uint16_t ib = 0; ib < c0->size()[0]; ++ib) {
            band b("band" + std::to_string(ib + 1));
            b.no_data_value = std::to_string(NAN);
            b.type = "float64";
            _bands.add(b);
        }

        _chunk_size = {c0->size()[1], c0->size()[2], c0->size()[3]};

        _size[0] = c0->size()[0];

        // Make an optimistic guess
        if (c0->size()[1] == 1) {
            _size[1] = in_cube->count_chunks_t();
            _st_ref->nt(_size[1]);
        } else if (c0->size()[1] == csize_in[1]) {
            _size[1] = in_cube->size()[1];
        } else
            throw std::string("ERROR in stream_cube::stream_cube(): could not derive size of result cube");

        if (c0->size()[2] == 1) {
            _size[2] = in_cube->count_chunks_t();
            _st_ref->ny() = _size[2];
        } else if (c0->size()[2] == csize_in[2]) {
            _size[2] = in_cube->size()[2];
        } else
            throw std::string("ERROR in stream_cube::stream_cube(): could not derive size of result cube");

        if (c0->size()[3] == 1) {
            _size[3] = in_cube->count_chunks_t();
            _st_ref->ny() = _size[3];
        } else if (c0->size()[3] == csize_in[3]) {
            _size[3] = in_cube->size()[3];
        } else
            throw std::string("ERROR in stream_cube::stream_cube(): could not derive size of result cube");
    }

    virtual std::shared_ptr<chunk_data> read_chunk(chunkid_t id) override;

    virtual nlohmann::json make_constructible_json() override {
        nlohmann::json out;
        out["cube_type"] = "stream";
        out["command"] = _cmd;
        out["in_cube"] = _in_cube->make_constructible_json();
        return out;
    }

   protected:
    std::shared_ptr<image_collection_cube> _in_cube;
    std::string _cmd;
    std::string _log_output;

   private:
    std::shared_ptr<chunk_data> stream_chunk_stdin(std::shared_ptr<chunk_data> data);
};

#endif  //STREAM_H
