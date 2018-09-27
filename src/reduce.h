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

#ifndef REDUCE_H
#define REDUCE_H

#include "cube.h"

struct reducer {
    virtual void init(std::shared_ptr<chunk_data> a) = 0;
    virtual void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b) = 0;
    virtual void finalize(std::shared_ptr<chunk_data> a) = 0;
};

struct mean_reducer : public reducer {
    void init(std::shared_ptr<chunk_data> a) override {
        _count = (uint32_t *)calloc(a->size()[0] * a->size()[2] * a->size()[3], sizeof(uint32_t));
        for (uint32_t ibxy = 0; ibxy < a->size()[0] * a->size()[2] * a->size()[3]; ++ibxy) {
            _count[ibxy] = 0;
            ((double *)a->buf())[ibxy] = 0;
        }
    }

    void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b) override {
        for (uint16_t ib = 0; ib < a->size()[0]; ++ib) {
            for (uint32_t it = 0; it < b->size()[1]; ++it) {
                for (uint32_t ixy = 0; ixy < b->size()[2] * b->size()[3]; ++ixy) {
                    double v = ((double *)b->buf())[ib * b->size()[1] * b->size()[2] * b->size()[3] + it * b->size()[2] * b->size()[3] + ixy];
                    if (!isnan(v)) {
                        ((double *)a->buf())[ib * a->size()[1] * a->size()[2] * a->size()[3] + ixy] += v;
                        ++_count[ib * a->size()[2] * a->size()[3] + ixy];
                    }
                }
            }
        }
    }

    void finalize(std::shared_ptr<chunk_data> a) override {
        // divide by count;
        for (uint32_t ibxy = 0; ibxy < a->size()[0] * a->size()[2] * a->size()[3]; ++ibxy) {
            ((double *)a->buf())[ibxy] = _count[ibxy] > 0 ? ((double *)a->buf())[ibxy] / _count[ibxy] : NAN;
        }
        free(_count);
    }

   protected:
    uint32_t *_count;
};

// TODO: handle nodata values
struct min_reducer : public reducer {
    void init(std::shared_ptr<chunk_data> a) override {
        for (uint32_t ibxy = 0; ibxy < a->size()[0] * a->size()[2] * a->size()[3]; ++ibxy) {
            ((double *)a->buf())[ibxy] = NAN;
        }
    }

    void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b) override {
        for (uint16_t ib = 0; ib < a->size()[0]; ++ib) {
            for (uint32_t it = 0; it < b->size()[1]; ++it) {
                for (uint32_t ixy = 0; ixy < b->size()[2] * b->size()[3]; ++ixy) {
                    double v = ((double *)b->buf())[ib * b->size()[1] * b->size()[2] * b->size()[3] + it * b->size()[2] * b->size()[3] + ixy];
                    if (!isnan(v)) {
                        double *w = &(((double *)a->buf())[ib * a->size()[1] * a->size()[2] * a->size()[3] + ixy]);
                        if (isnan(*w))
                            *w = v;
                        else
                            *w = std::min(*w, v);
                    }
                }
            }
        }
    }

    void finalize(std::shared_ptr<chunk_data> a) override {}
};

struct max_reducer : public reducer {
    void init(std::shared_ptr<chunk_data> a) override {
        for (uint32_t ibxy = 0; ibxy < a->size()[0] * a->size()[2] * a->size()[3]; ++ibxy) {
            ((double *)a->buf())[ibxy] = NAN;
        }
    }

    void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b) override {
        for (uint16_t ib = 0; ib < a->size()[0]; ++ib) {
            for (uint32_t it = 0; it < b->size()[1]; ++it) {
                for (uint32_t ixy = 0; ixy < b->size()[2] * b->size()[3]; ++ixy) {
                    double v = ((double *)b->buf())[ib * b->size()[1] * b->size()[2] * b->size()[3] + it * b->size()[2] * b->size()[3] + ixy];
                    if (!isnan(v)) {
                        double *w = &(((double *)a->buf())[ib * a->size()[1] * a->size()[2] * a->size()[3] + ixy]);
                        if (isnan(*w))
                            *w = v;
                        else
                            *w = std::max(*w, v);
                    }
                }
            }
        }
    }

    void finalize(std::shared_ptr<chunk_data> a) override {}
};

class reduce_cube : public cube {
   public:
    reduce_cube(std::shared_ptr<cube> in, std::string reducer = "mean") : _in_cube(in), _r(nullptr), cube(std::make_shared<cube_st_reference>(in->st_reference())) {
        _st_ref->dt() = _st_ref->t1() - _st_ref->t0();
        _st_ref->t1() = _st_ref->t0();  // set nt=1
        _size[1] = 1;
        _chunk_size[0] = 1;
        _chunk_size[1] = _in_cube->chunk_size()[1];
        _chunk_size[2] = _in_cube->chunk_size()[2];
        _size[0] = _in_cube->bands().count();  // TODO: add with band information

        for (uint16_t ib = 0; ib < _in_cube->bands().count(); ++ib) {
            band b = _in_cube->bands().get(ib);
            b.name = b.name + "_reduce";  // TODO: replace with mean / min / max / ...
            _bands.add(b);
        }

        if (reducer == "min") {
            _r = new min_reducer();
        } else if (reducer == "max") {
            _r = new max_reducer();
        } else if (reducer == "mean") {
            _r = new mean_reducer();
        } else
            throw std::string("ERROR in reduce_cube::reduce_cube(): Unknown reducer given");
    }

    ~reduce_cube() {
        if (_r) delete _r;
    }

    std::shared_ptr<chunk_data> read_chunk(chunkid_t id) override;

    /**
 * Combines all chunks and produces a single GDAL image
 * @param path path to output image file
 * @param format GDAL format (see https://www.gdal.org/formats_list.html)
 * @param co GDAL create options
 */
    void write_gdal_image(std::string path, std::string format = "GTiff", std::vector<std::string> co = std::vector<std::string>());

   protected:
    std::shared_ptr<cube> _in_cube;
    reducer *_r;
};

#endif  //REDUCE_H
