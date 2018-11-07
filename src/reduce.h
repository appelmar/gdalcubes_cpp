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
    virtual ~reducer() {}

    /**
     * @brief Initialization function for reducers that is automatically called before reading data from the input cube
     * @param a chunk data where reduction results are written to later
     */
    virtual void init(std::shared_ptr<chunk_data> a) = 0;

    /**
     * @brief Combines a chunk of data from the input cube with the current state of the result chunk according to the specific reducer
     * @param a output chunk of the reduction
     * @param b one input chunk of the input cube, which is aligned with the output chunk in space
     */
    virtual void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b) = 0;

    /**
     * @brief Finallizes the reduction, i.e., frees additional buffers and postprocesses the result (e.g. dividing by n for mean reducer)
     * @param a result chunk
     */
    virtual void finalize(std::shared_ptr<chunk_data> a) = 0;
};

/**
 * @brief Implementation of reducer to calculate mean values (for all bands) over time
 */
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

   private:
    uint32_t *_count;
};

/**
 * @brief Implementation of reducer to calculate minimum values (for all bands) over time
 */
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

/**
 * @brief Implementation of reducer to calculate maximum values (for all bands) over time
 */
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

struct count_reducer : public reducer {
    void init(std::shared_ptr<chunk_data> a) override {
        for (uint32_t ibxy = 0; ibxy < a->size()[0] * a->size()[2] * a->size()[3]; ++ibxy) {
            ((double *)a->buf())[ibxy] = 0;
        }
    }

    void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b) override {
        for (uint16_t ib = 0; ib < a->size()[0]; ++ib) {
            for (uint32_t it = 0; it < b->size()[1]; ++it) {
                for (uint32_t ixy = 0; ixy < b->size()[2] * b->size()[3]; ++ixy) {
                    double v = ((double *)b->buf())[ib * b->size()[1] * b->size()[2] * b->size()[3] + it * b->size()[2] * b->size()[3] + ixy];
                    if (!isnan(v)) {
                        double *w = &(((double *)a->buf())[ib * a->size()[1] * a->size()[2] * a->size()[3] + ixy]);
                        *w += 1;
                    }
                }
            }
        }
    }

    void finalize(std::shared_ptr<chunk_data> a) override {}
};

/**
 * @brief Implementation of reducer to calculate median values (for all bands) over time
 * @note Calculating the exact median has a strong memory overhead, approximate median reducers might be implemented in the future
 */
struct median_reducer : public reducer {
    void init(std::shared_ptr<chunk_data> a) override {
        //_m_buckets = (std::vector<double> *)calloc(a->size()[0] * a->size()[2] * a->size()[3], sizeof(std::vector<double>));
        _m_buckets.resize(a->size()[0] * a->size()[2] * a->size()[3], std::vector<double>());
    }

    void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b) override {
        for (uint16_t ib = 0; ib < a->size()[0]; ++ib) {
            for (uint32_t it = 0; it < b->size()[1]; ++it) {
                for (uint32_t ixy = 0; ixy < b->size()[2] * b->size()[3]; ++ixy) {
                    double v = ((double *)b->buf())[ib * b->size()[1] * b->size()[2] * b->size()[3] + it * b->size()[2] * b->size()[3] + ixy];
                    if (!isnan(v)) {
                        _m_buckets[ib * a->size()[2] * a->size()[3] + ixy].push_back(v);
                    }
                }
            }
        }
    }

    void finalize(std::shared_ptr<chunk_data> a) override {
        // divide by count;
        for (uint32_t ibxy = 0; ibxy < a->size()[0] * a->size()[2] * a->size()[3]; ++ibxy) {
            std::vector<double> &list = _m_buckets[ibxy];
            std::sort(list.begin(), list.end());
            if (list.size() == 0) {
                ((double *)a->buf())[ibxy] = NAN;
            } else if (list.size() % 2 == 1) {
                ((double *)a->buf())[ibxy] = list[list.size() / 2];
            } else {
                ((double *)a->buf())[ibxy] = (list[list.size() / 2] + list[list.size() / 2 - 1]) / ((double)2);
            }
        }
    }

   private:
    std::vector<std::vector<double>> _m_buckets;
};

/**
 * @brief A data cube that applies a reducer function to another data cube over time
 */
class reduce_cube : public cube {
   public:
    reduce_cube(std::shared_ptr<cube> in, std::string reducer = "mean") : cube(std::make_shared<cube_st_reference>(in->st_reference())), _in_cube(in), _reducer(reducer) {
        _st_ref->dt() = _st_ref->t1() - _st_ref->t0();
        _st_ref->t1() = _st_ref->t0();  // set nt=1
        _size[1] = 1;
        _chunk_size[0] = 1;
        _chunk_size[1] = _in_cube->chunk_size()[1];
        _chunk_size[2] = _in_cube->chunk_size()[2];
        _size[0] = _in_cube->bands().count();

        for (uint16_t ib = 0; ib < in->bands().count(); ++ib) {
            band b = in->bands().get(ib);
            if (in->size_t() > 1) {
                b.name = b.name + "_" + reducer;  // Change name only if input is not yet reduced
            }
            _bands.add(b);
        }

        if (!(reducer == "min" ||
              reducer == "max" ||
              reducer == "mean" ||
              reducer == "median" ||
              reducer == "count"))
            throw std::string("ERROR in reduce_cube::reduce_cube(): Unknown reducer given");
    }

    ~reduce_cube() {}

    std::shared_ptr<chunk_data> read_chunk(chunkid_t id) override;

    /**
 * Combines all chunks and produces a single GDAL image
 * @param path path to output image file
 * @param format GDAL format (see https://www.gdal.org/formats_list.html)
 * @param co GDAL create options
     * @param p chunk processor instance, defaults to the current global configuration in config::instance()->get_default_chunk_processor()
 */
    void write_gdal_image(std::string path, std::string format = "GTiff", std::vector<std::string> co = std::vector<std::string>(), std::shared_ptr<chunk_processor> p = config::instance()->get_default_chunk_processor());

    nlohmann::json make_constructible_json() override {
        nlohmann::json out;
        out["cube_type"] = "reduce";
        out["reducer"] = _reducer;
        out["in_cube"] = _in_cube->make_constructible_json();
        return out;
    }

   private:
    std::shared_ptr<cube> _in_cube;
    std::string _reducer;
};

#endif  //REDUCE_H
