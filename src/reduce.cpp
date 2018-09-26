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

#include "reduce.h"



std::shared_ptr<chunk_data> reduce_cube::read_chunk(chunkid_t id) {

    std::shared_ptr<chunk_data> out = std::make_shared<chunk_data>();
    if (id < 0 || id >= count_chunks())
        return out;  // chunk is outside of the view, we don't need to read anything.




    coords_nd<uint32_t, 3> size_tyx = chunk_size(id);
    coords_nd<uint32_t, 4> size_btyx = {_bands.count(), 1, size_tyx[1], size_tyx[2]};
    out->size(size_btyx);

    // Fill buffers accordingly
    out->buf(calloc(size_btyx[0] * size_btyx[1] * size_btyx[2] * size_btyx[3], sizeof(double)));






    reducer* r = new min_reducer(); // = new ..._reducer() // TODO: select reducer based on parameter


    r->init(out);




    // iterate over all chunks that must be read from the input cube to compute this chunk
    for (chunkid_t i=id; i<_in_cube->count_chunks(); i += _in_cube->count_chunks_x()*_in_cube->count_chunks_y()) {
        // read chunk i from input cube

        std::shared_ptr<chunk_data> x = _in_cube->read_chunk(i);
        r->combine(out, x);

    }


    r->finalize(out);


    delete r;

    return out;
}