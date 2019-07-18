/*
    MIT License

    Copyright (c) 2019 Marius Appel <marius.appel@uni-muenster.de>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include "rechunk_merge_time.h"

namespace gdalcubes {

std::shared_ptr<chunk_data> rechunk_merge_time_cube::read_chunk(chunkid_t id) {
    GCBS_TRACE("rechunk_merge_time_cube::read_chunk(" + std::to_string(id) + ")");
    std::shared_ptr<chunk_data> out = std::make_shared<chunk_data>();
    if (id >= count_chunks())
        return out;  // chunk is outside of the view, we don't need to read anything.

    coords_nd<uint32_t, 3> size_tyx = chunk_size(id);
    coords_nd<uint32_t, 4> size_btyx = {_in_cube->size_bands(), _in_cube->size_t(), size_tyx[1], size_tyx[2]};
    out->size(size_btyx);

    // Fill buffers accordingly
    out->buf(std::calloc(size_btyx[0] * size_btyx[1] * size_btyx[2] * size_btyx[3], sizeof(double)));
    double *begin = (double *)out->buf();
    double *end = ((double *)out->buf()) + size_btyx[0] * size_btyx[1] * size_btyx[2] * size_btyx[3];
    std::fill(begin, end, NAN);

    uint32_t ichunk = 0;
    for (chunkid_t i = id;
         i < _in_cube->count_chunks(); i += _in_cube->count_chunks_x() * _in_cube->count_chunks_y()) {
        std::shared_ptr<chunk_data> x = _in_cube->read_chunk(i);
        for (uint16_t ib = 0; ib < x->size()[0]; ++ib) {
            for (uint32_t it = 0; it < x->size()[1]; ++it) {
                for (uint32_t ixy = 0; ixy < x->size()[2] * x->size()[3]; ++ixy) {
                    ((double *)out->buf())[ib * x->size()[1] * x->size()[2] * x->size()[3] +
                                           (it + ichunk * x->size()[1]) * x->size()[2] * x->size()[3] + ixy] =
                        ((double *)x->buf())[ib * x->size()[1] * x->size()[2] * x->size()[3] +
                                             it * x->size()[2] * x->size()[3] + ixy];
                }
            }
        }
        ++ichunk;
    }

    return out;
}

}  // namespace gdalcubes