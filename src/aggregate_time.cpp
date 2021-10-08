/*
    MIT License

    Copyright (c) 2021 Marius Appel <marius.appel@uni-muenster.de>

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
#include "aggregate_time.h"



struct aggregator_time_slice_singleband {
    virtual ~aggregator_time_slice_singleband() {}

    virtual void init(double *out, uint32_t size_x, uint32_t size_y) = 0;
    virtual void combine(double* out, double* in, uint32_t size_x, uint32_t size_y) = 0;
    virtual void finalize(double *out, uint32_t size_x, uint32_t size_y) = 0;
};




/**
 * @brief Implementation of reducer to calculate mean values over time
 */
struct mean_aggregtor_time_slice_singleband : public aggregator_time_slice_singleband {
    void init(double *out, uint32_t size_x, uint32_t size_y) override {

        _count = (uint32_t *)std::calloc(size_x * size_y, sizeof(uint32_t));
        for (uint32_t ixy = 0; ixy < size_x *size_y; ++ixy) {
            _count[ixy] = 0;
            out[ixy] = 0;
        }
    }

    void combine(double* out, double* in, uint32_t size_x, uint32_t size_y) override {
        for (uint32_t ixy = 0; ixy < size_x * size_y; ++ixy) {
            double v = in[ixy];
            if (!std::isnan(v)) {
                out[ixy] += v;
                ++_count[ixy];
            }
        }
    }

    void finalize(double *out, uint32_t size_x, uint32_t size_y) override {
        // divide by count;
        for (uint32_t ixy = 0; ixy < size_x * size_y; ++ixy) {
            out[ixy] =  _count[ixy] > 0 ? out[ixy] / _count[ixy] : NAN;
        }
        std::free(_count);
    }

   private:
    uint32_t *_count;
};









namespace gdalcubes {
//
//struct reducer_singleband {
//    virtual ~reducer_singleband() {}
//
//    /**
//     * @brief Initialization function for reducers that is automatically called before reading data from the input cube
//     * @param a chunk data where reduction results are written to later
//     * @param band_idx_in over which band of the chunk data (zero-based index) shall the reducer be applied?
//     * @param band_idx_out to which band of the result chunk (zero-based index) shall the reducer write?
//     */
//    virtual void init(std::shared_ptr<chunk_data> a, uint16_t band_idx_in, uint16_t band_idx_out, std::shared_ptr<cube> in_cube) = 0;
//
//    /**
//     * @brief Combines a chunk of data from the input cube with the current state of the result chunk according to the specific reducer
//     * @param a output chunk of the reduction
//     * @param b one input chunk of the input cube, which is aligned with the output chunk in space
//     */
//    virtual void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b, chunkid_t chunk_id) = 0;
//
//    /**
//     * @brief Finallizes the reduction, i.e., frees additional buffers and postprocesses the result (e.g. dividing by n for mean reducer)
//     * @param a result chunk
//     */
//    virtual void finalize(std::shared_ptr<chunk_data> a) = 0;
//};
//
///**
// * @brief Implementation of reducer to calculate sum values over time
// */
//struct sum_reducer_singleband : public reducer_singleband {
//    void init(std::shared_ptr<chunk_data> a, uint16_t band_idx_in, uint16_t band_idx_out, std::shared_ptr<cube> in_cube) override {
//        _band_idx_in = band_idx_in;
//        _band_idx_out = band_idx_out;
//        for (uint32_t ixy = 0; ixy < a->size()[2] * a->size()[3]; ++ixy) {
//            ((double *)a->buf())[_band_idx_out * a->size()[2] * a->size()[3] + ixy] = 0;
//        }
//    }
//
//    void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b, chunkid_t chunk_id) override {
//        for (uint32_t it = 0; it < b->size()[1]; ++it) {
//            for (uint32_t ixy = 0; ixy < b->size()[2] * b->size()[3]; ++ixy) {
//                double v = ((double *)b->buf())[_band_idx_in * b->size()[1] * b->size()[2] * b->size()[3] + it * b->size()[2] * b->size()[3] + ixy];
//                if (!std::isnan(v)) {
//                    ((double *)a->buf())[_band_idx_out * a->size()[1] * a->size()[2] * a->size()[3] + ixy] += v;
//                }
//            }
//        }
//    }
//    void finalize(std::shared_ptr<chunk_data> a) override {}
//
//   private:
//    uint16_t _band_idx_in;
//    uint16_t _band_idx_out;
//};
//
///**
// * @brief Implementation of reducer to calculate product values over time
// */
//struct prod_reducer_singleband : public reducer_singleband {
//    void init(std::shared_ptr<chunk_data> a, uint16_t band_idx_in, uint16_t band_idx_out, std::shared_ptr<cube> in_cube) override {
//        _band_idx_in = band_idx_in;
//        _band_idx_out = band_idx_out;
//        for (uint32_t ixy = 0; ixy < a->size()[2] * a->size()[3]; ++ixy) {
//            ((double *)a->buf())[_band_idx_out * a->size()[2] * a->size()[3] + ixy] = 1;
//        }
//    }
//
//    void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b, chunkid_t chunk_id) override {
//        for (uint32_t it = 0; it < b->size()[1]; ++it) {
//            for (uint32_t ixy = 0; ixy < b->size()[2] * b->size()[3]; ++ixy) {
//                double v = ((double *)b->buf())[_band_idx_in * b->size()[1] * b->size()[2] * b->size()[3] + it * b->size()[2] * b->size()[3] + ixy];
//                if (!std::isnan(v)) {
//                    ((double *)a->buf())[_band_idx_out * a->size()[1] * a->size()[2] * a->size()[3] + ixy] *= v;
//                }
//            }
//        }
//    }
//    void finalize(std::shared_ptr<chunk_data> a) override {}
//
//   private:
//    uint16_t _band_idx_in;
//    uint16_t _band_idx_out;
//};
//
///**
// * @brief Implementation of reducer to calculate mean values over time
// */
//struct mean_reducer_singleband : public reducer_singleband {
//    void init(std::shared_ptr<chunk_data> a, uint16_t band_idx_in, uint16_t band_idx_out, std::shared_ptr<cube> in_cube) override {
//        _band_idx_in = band_idx_in;
//        _band_idx_out = band_idx_out;
//        _count = (uint32_t *)std::calloc(a->size()[2] * a->size()[3], sizeof(uint32_t));
//        for (uint32_t ixy = 0; ixy < a->size()[2] * a->size()[3]; ++ixy) {
//            _count[ixy] = 0;
//            ((double *)a->buf())[_band_idx_out * a->size()[2] * a->size()[3] + ixy] = 0;
//        }
//    }
//
//    void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b, chunkid_t chunk_id) override {
//        for (uint32_t it = 0; it < b->size()[1]; ++it) {
//            for (uint32_t ixy = 0; ixy < b->size()[2] * b->size()[3]; ++ixy) {
//                double v = ((double *)b->buf())[_band_idx_in * b->size()[1] * b->size()[2] * b->size()[3] + it * b->size()[2] * b->size()[3] + ixy];
//                if (!std::isnan(v)) {
//                    ((double *)a->buf())[_band_idx_out * a->size()[1] * a->size()[2] * a->size()[3] + ixy] += v;
//                    ++_count[ixy];
//                }
//            }
//        }
//    }
//
//    void finalize(std::shared_ptr<chunk_data> a) override {
//        // divide by count;
//        for (uint32_t ixy = 0; ixy < a->size()[2] * a->size()[3]; ++ixy) {
//            ((double *)a->buf())[_band_idx_out * a->size()[2] * a->size()[3] + ixy] = _count[ixy] > 0 ? ((double *)a->buf())[_band_idx_out * a->size()[2] * a->size()[3] + ixy] / _count[ixy] : NAN;
//        }
//        std::free(_count);
//    }
//
//   private:
//    uint32_t *_count;
//    uint16_t _band_idx_in;
//    uint16_t _band_idx_out;
//};
//
///**
// * @brief Implementation of reducer to calculate minimum values over time
// */
//struct min_reducer_singleband : public reducer_singleband {
//    void init(std::shared_ptr<chunk_data> a, uint16_t band_idx_in, uint16_t band_idx_out, std::shared_ptr<cube> in_cube) override {
//        _band_idx_in = band_idx_in;
//        _band_idx_out = band_idx_out;
//        for (uint32_t ixy = 0; ixy < a->size()[2] * a->size()[3]; ++ixy) {
//            ((double *)a->buf())[_band_idx_out * a->size()[2] * a->size()[3] + ixy] = NAN;
//        }
//    }
//
//    void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b, chunkid_t chunk_id) override {
//        for (uint32_t it = 0; it < b->size()[1]; ++it) {
//            for (uint32_t ixy = 0; ixy < b->size()[2] * b->size()[3]; ++ixy) {
//                double v = ((double *)b->buf())[_band_idx_in * b->size()[1] * b->size()[2] * b->size()[3] + it * b->size()[2] * b->size()[3] + ixy];
//                if (!std::isnan(v)) {
//                    double *w = &(((double *)a->buf())[_band_idx_out * a->size()[1] * a->size()[2] * a->size()[3] + ixy]);
//                    if (std::isnan(*w))
//                        *w = v;
//                    else
//                        *w = std::min(*w, v);
//                }
//            }
//        }
//    }
//
//    void finalize(std::shared_ptr<chunk_data> a) override {}
//
//   private:
//    uint16_t _band_idx_in;
//    uint16_t _band_idx_out;
//};
//
///**
// * @brief Implementation of reducer to calculate the date of the minimum over time
// */
//struct which_min_reducer_singleband : public reducer_singleband {
//    void init(std::shared_ptr<chunk_data> a, uint16_t band_idx_in, uint16_t band_idx_out, std::shared_ptr<cube> in_cube) override {
//        _band_idx_in = band_idx_in;
//        _band_idx_out = band_idx_out;
//        _cur_min = (double *)std::calloc(a->size()[2] * a->size()[3], sizeof(double));
//        _in_cube = in_cube;
//        for (uint32_t ixy = 0; ixy < a->size()[2] * a->size()[3]; ++ixy) {
//            _cur_min[ixy] = NAN;
//        }
//        for (uint32_t ixy = 0; ixy < a->size()[2] * a->size()[3]; ++ixy) {
//            ((double *)a->buf())[_band_idx_out * a->size()[2] * a->size()[3] + ixy] = NAN;
//        }
//    }
//
//    void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b, chunkid_t chunk_id) override {
//        std::shared_ptr<cube> in = _in_cube.lock();
//        // we don't check if pointer is expired here since the reducers live only within the read_chunk function of the reducer cube object that has shared ownership with the input cube
//
//        for (uint32_t it = 0; it < b->size()[1]; ++it) {
//            for (uint32_t ixy = 0; ixy < b->size()[2] * b->size()[3]; ++ixy) {
//                double v = ((double *)b->buf())[_band_idx_in * b->size()[1] * b->size()[2] * b->size()[3] + it * b->size()[2] * b->size()[3] + ixy];
//                if (!std::isnan(v)) {
//                    //double *w = &(((double *)a->buf())[_band_idx_out * a->size()[1] * a->size()[2] * a->size()[3] + ixy]);
//                    double *w = &(_cur_min[ixy]);
//                    if (std::isnan(*w)) {
//                        *w = v;
//                        // set date in output chunk
//                        ((double *)a->buf())[_band_idx_out * a->size()[1] * a->size()[2] * a->size()[3] + ixy] = (in->bounds_from_chunk(chunk_id).t0 + (in->st_reference()->dt() * it)).to_double();
//                    } else {
//                        if (v < *w) {
//                            *w = v;
//                            ((double *)a->buf())[_band_idx_out * a->size()[1] * a->size()[2] * a->size()[3] + ixy] = (in->bounds_from_chunk(chunk_id).t0 + (in->st_reference()->dt() * it)).to_double();
//                        }
//                    }
//                }
//            }
//        }
//    }
//
//    void finalize(std::shared_ptr<chunk_data> a) override {
//        std::free(_cur_min);
//    }
//
//   private:
//    uint16_t _band_idx_in;
//    uint16_t _band_idx_out;
//    double *_cur_min;
//    std::weak_ptr<cube> _in_cube;
//};
//
///**
// * @brief Implementation of reducer to calculate maximum values over time
// */
//struct max_reducer_singleband : public reducer_singleband {
//    void init(std::shared_ptr<chunk_data> a, uint16_t band_idx_in, uint16_t band_idx_out, std::shared_ptr<cube> in_cube) override {
//        _band_idx_in = band_idx_in;
//        _band_idx_out = band_idx_out;
//        for (uint32_t ixy = 0; ixy < a->size()[2] * a->size()[3]; ++ixy) {
//            ((double *)a->buf())[_band_idx_out * a->size()[2] * a->size()[3] + ixy] = NAN;
//        }
//    }
//
//    void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b, chunkid_t chunk_id) override {
//        for (uint32_t it = 0; it < b->size()[1]; ++it) {
//            for (uint32_t ixy = 0; ixy < b->size()[2] * b->size()[3]; ++ixy) {
//                double v = ((double *)b->buf())[_band_idx_in * b->size()[1] * b->size()[2] * b->size()[3] + it * b->size()[2] * b->size()[3] + ixy];
//                if (!std::isnan(v)) {
//                    double *w = &(((double *)a->buf())[_band_idx_out * a->size()[1] * a->size()[2] * a->size()[3] + ixy]);
//                    if (std::isnan(*w))
//                        *w = v;
//                    else
//                        *w = std::max(*w, v);
//                }
//            }
//        }
//    }
//
//    void finalize(std::shared_ptr<chunk_data> a) override {}
//
//   private:
//    uint16_t _band_idx_in;
//    uint16_t _band_idx_out;
//};
//
///**
// * @brief Implementation of reducer to calculate the date of the minimum over time
// */
//struct which_max_reducer_singleband : public reducer_singleband {
//    void init(std::shared_ptr<chunk_data> a, uint16_t band_idx_in, uint16_t band_idx_out, std::shared_ptr<cube> in_cube) override {
//        _band_idx_in = band_idx_in;
//        _band_idx_out = band_idx_out;
//        _cur_max = (double *)std::calloc(a->size()[2] * a->size()[3], sizeof(double));
//        _in_cube = in_cube;
//        for (uint32_t ixy = 0; ixy < a->size()[2] * a->size()[3]; ++ixy) {
//            _cur_max[ixy] = NAN;
//        }
//        for (uint32_t ixy = 0; ixy < a->size()[2] * a->size()[3]; ++ixy) {
//            ((double *)a->buf())[_band_idx_out * a->size()[2] * a->size()[3] + ixy] = NAN;
//        }
//    }
//
//    void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b, chunkid_t chunk_id) override {
//        std::shared_ptr<cube> in = _in_cube.lock();
//        // we don't check if pointer is expired here since the reducers live only within the read_chunk function of the reducer cube object that has shared ownership with the input cube
//
//        for (uint32_t it = 0; it < b->size()[1]; ++it) {
//            for (uint32_t ixy = 0; ixy < b->size()[2] * b->size()[3]; ++ixy) {
//                double v = ((double *)b->buf())[_band_idx_in * b->size()[1] * b->size()[2] * b->size()[3] + it * b->size()[2] * b->size()[3] + ixy];
//                if (!std::isnan(v)) {
//                    //double *w = &(((double *)a->buf())[_band_idx_out * a->size()[1] * a->size()[2] * a->size()[3] + ixy]);
//                    double *w = &(_cur_max[ixy]);
//                    if (std::isnan(*w)) {
//                        *w = v;
//                        // set date in output chunk
//                        ((double *)a->buf())[_band_idx_out * a->size()[1] * a->size()[2] * a->size()[3] + ixy] = (in->bounds_from_chunk(chunk_id).t0 + (in->st_reference()->dt() * it)).to_double();
//                    } else {
//                        if (v > *w) {
//                            *w = v;
//                            ((double *)a->buf())[_band_idx_out * a->size()[1] * a->size()[2] * a->size()[3] + ixy] = (in->bounds_from_chunk(chunk_id).t0 + (in->st_reference()->dt() * it)).to_double();
//                        }
//                    }
//                }
//            }
//        }
//    }
//
//    void finalize(std::shared_ptr<chunk_data> a) override {
//        std::free(_cur_max);
//    }
//
//   private:
//    uint16_t _band_idx_in;
//    uint16_t _band_idx_out;
//    double *_cur_max;
//    std::weak_ptr<cube> _in_cube;
//};
//
//struct count_reducer_singleband : public reducer_singleband {
//    void init(std::shared_ptr<chunk_data> a, uint16_t band_idx_in, uint16_t band_idx_out, std::shared_ptr<cube> in_cube) override {
//        _band_idx_in = band_idx_in;
//        _band_idx_out = band_idx_out;
//        for (uint32_t ixy = 0; ixy < a->size()[2] * a->size()[3]; ++ixy) {
//            ((double *)a->buf())[_band_idx_out * a->size()[2] * a->size()[3] + ixy] = 0;
//        }
//    }
//
//    void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b, chunkid_t chunk_id) override {
//        for (uint32_t it = 0; it < b->size()[1]; ++it) {
//            for (uint32_t ixy = 0; ixy < b->size()[2] * b->size()[3]; ++ixy) {
//                double v = ((double *)b->buf())[_band_idx_in * b->size()[1] * b->size()[2] * b->size()[3] + it * b->size()[2] * b->size()[3] + ixy];
//                if (!std::isnan(v)) {
//                    double *w = &(((double *)a->buf())[_band_idx_out * a->size()[1] * a->size()[2] * a->size()[3] + ixy]);
//                    *w += 1;
//                }
//            }
//        }
//    }
//
//    void finalize(std::shared_ptr<chunk_data> a) override {}
//
//   private:
//    uint16_t _band_idx_in;
//    uint16_t _band_idx_out;
//};
//
///**
// * @brief Implementation of reducer to calculate median values over time
// * @note Calculating the exact median has a strong memory overhead, approximate median reducers might be implemented in the future
// */
//struct median_reducer_singleband : public reducer_singleband {
//    void init(std::shared_ptr<chunk_data> a, uint16_t band_idx_in, uint16_t band_idx_out, std::shared_ptr<cube> in_cube) override {
//        _band_idx_in = band_idx_in;
//        _band_idx_out = band_idx_out;
//        _m_buckets.resize(a->size()[2] * a->size()[3], std::vector<double>());
//    }
//
//    void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b, chunkid_t chunk_id) override {
//        for (uint32_t it = 0; it < b->size()[1]; ++it) {
//            for (uint32_t ixy = 0; ixy < b->size()[2] * b->size()[3]; ++ixy) {
//                double v = ((double *)b->buf())[_band_idx_in * b->size()[1] * b->size()[2] * b->size()[3] + it * b->size()[2] * b->size()[3] + ixy];
//                if (!std::isnan(v)) {
//                    _m_buckets[ixy].push_back(v);
//                }
//            }
//        }
//    }
//
//    void finalize(std::shared_ptr<chunk_data> a) override {
//        for (uint32_t ixy = 0; ixy < a->size()[2] * a->size()[3]; ++ixy) {
//            std::vector<double> &list = _m_buckets[ixy];
//            std::sort(list.begin(), list.end());
//            if (list.size() == 0) {
//                ((double *)a->buf())[_band_idx_out * a->size()[2] * a->size()[3] + ixy] = NAN;
//            } else if (list.size() % 2 == 1) {
//                ((double *)a->buf())[_band_idx_out * a->size()[2] * a->size()[3] + ixy] = list[list.size() / 2];
//            } else {
//                ((double *)a->buf())[_band_idx_out * a->size()[2] * a->size()[3] + ixy] = (list[list.size() / 2] + list[list.size() / 2 - 1]) / ((double)2);
//            }
//        }
//    }
//
//   private:
//    std::vector<std::vector<double>> _m_buckets;
//    uint16_t _band_idx_in;
//    uint16_t _band_idx_out;
//};
//
///**
// * @brief Implementation of reducer to calculate variance values over time using Welford's Online algorithm
// */
//struct var_reducer_singleband : public reducer_singleband {
//    void init(std::shared_ptr<chunk_data> a, uint16_t band_idx_in, uint16_t band_idx_out, std::shared_ptr<cube> in_cube) override {
//        _band_idx_in = band_idx_in;
//        _band_idx_out = band_idx_out;
//        _count = (uint32_t *)std::calloc(a->size()[2] * a->size()[3], sizeof(uint32_t));
//        _mean = (double *)std::calloc(a->size()[2] * a->size()[3], sizeof(double));
//        for (uint32_t ixy = 0; ixy < a->size()[2] * a->size()[3]; ++ixy) {
//            _count[ixy] = 0;
//            _mean[ixy] = 0;
//            ((double *)a->buf())[_band_idx_out * a->size()[2] * a->size()[3] + ixy] = 0;
//        }
//    }
//
//    void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b, chunkid_t chunk_id) override {
//        for (uint32_t it = 0; it < b->size()[1]; ++it) {
//            for (uint32_t ixy = 0; ixy < b->size()[2] * b->size()[3]; ++ixy) {
//                double &v = ((double *)b->buf())[_band_idx_in * b->size()[1] * b->size()[2] * b->size()[3] + it * b->size()[2] * b->size()[3] + ixy];
//                if (!std::isnan(v)) {
//                    double &mean = _mean[ixy];
//                    uint32_t &count = _count[ixy];
//                    ++count;
//                    double delta = v - mean;
//                    mean += delta / count;
//                    ((double *)a->buf())[_band_idx_out * a->size()[1] * a->size()[2] * a->size()[3] + ixy] += delta * (v - mean);
//                }
//            }
//        }
//    }
//
//    virtual void finalize(std::shared_ptr<chunk_data> a) override {
//        // divide by count - 1;
//        for (uint32_t ixy = 0; ixy < a->size()[2] * a->size()[3]; ++ixy) {
//            ((double *)a->buf())[_band_idx_out * a->size()[2] * a->size()[3] + ixy] = _count[ixy] > 1 ? ((double *)a->buf())[_band_idx_out * a->size()[2] * a->size()[3] + ixy] / (_count[ixy] - 1) : NAN;
//        }
//        std::free(_count);
//        std::free(_mean);
//    }
//
//   protected:
//    uint32_t *_count;
//    double *_mean;
//    uint16_t _band_idx_in;
//    uint16_t _band_idx_out;
//};
//
///**
// * @brief Implementation of reducer to calculate standard deviation values over time using Welford's Online algorithm
// */
//struct sd_reducer_singleband : public var_reducer_singleband {
//    void finalize(std::shared_ptr<chunk_data> a) override {
//        // divide by count - 1;
//        for (uint32_t ixy = 0; ixy < a->size()[2] * a->size()[3]; ++ixy) {
//            ((double *)a->buf())[_band_idx_out * a->size()[2] * a->size()[3] + ixy] = _count[ixy] > 1 ? sqrt(((double *)a->buf())[_band_idx_out * a->size()[2] * a->size()[3] + ixy] / (_count[ixy] - 1)) : NAN;
//        }
//        std::free(_count);
//        std::free(_mean);
//    }
//};

std::shared_ptr<chunk_data> aggregate_time_cube::read_chunk(chunkid_t id) {
    GCBS_TRACE("aggregate_time_cube::read_chunk(" + std::to_string(id) + ")");
    std::shared_ptr<chunk_data> out = std::make_shared<chunk_data>();
    if (id >= count_chunks())
        return out;  // chunk is outside of the view, we don't need to read anything.


    coords_nd<uint32_t, 3> size_tyx = chunk_size(id);
    coords_nd<uint32_t, 4> size_btyx = {uint32_t(_bands.count()), size_tyx[0], size_tyx[1], size_tyx[2]};
    out->size(size_btyx);

    // Fill buffers accordingly
    out->buf(std::calloc(size_btyx[0] * size_btyx[1] * size_btyx[2] * size_btyx[3], sizeof(double)));
    double *begin = (double *)out->buf();
    double *end = ((double *)out->buf()) + size_btyx[0] * size_btyx[1] * size_btyx[2] * size_btyx[3];
    std::fill(begin, end, NAN);


    /*
    std::vector<reducer_singleband *> reducers;
    for (uint16_t i = 0; i < _reducer_bands.size(); ++i) {
        reducer_singleband *r = nullptr;
        if (_reducer_bands[i].first == "min") {
            r = new min_reducer_singleband();
        } else if (_reducer_bands[i].first == "max") {
            r = new max_reducer_singleband();
        } else if (_reducer_bands[i].first == "mean") {
            r = new mean_reducer_singleband();
        } else if (_reducer_bands[i].first == "median") {
            r = new median_reducer_singleband();
        } else if (_reducer_bands[i].first == "sum") {
            r = new sum_reducer_singleband();
        } else if (_reducer_bands[i].first == "count") {
            r = new count_reducer_singleband();
        } else if (_reducer_bands[i].first == "prod") {
            r = new prod_reducer_singleband();
        } else if (_reducer_bands[i].first == "var") {
            r = new var_reducer_singleband();
        } else if (_reducer_bands[i].first == "sd") {
            r = new sd_reducer_singleband();
        } else if (_reducer_bands[i].first == "which_min") {
            r = new which_min_reducer_singleband();
        } else if (_reducer_bands[i].first == "which_max") {
            r = new which_max_reducer_singleband();
        } else
            throw std::string("ERROR in reduce_time_cube::read_chunk(): Unknown reducer given");

        uint16_t band_idx_in = _in_cube->bands().get_index(_reducer_bands[i].second);
        r->init(out, band_idx_in, i, _in_cube);

        reducers.push_back(r);
    }*/

    auto climits = chunk_limits(id);
    auto ccoords = chunk_coords_from_id(id);

    std::map<chunkid_t, std::shared_ptr<chunk_data>> chunk_cache;


    // TODO: create agg based on func
    std::vector<aggregator_time_slice_singleband*> agg;
    for (uint16_t ib=0; ib<_bands.count(); ++ib) {
        agg.push_back(new mean_aggregtor_time_slice_singleband());
    }



    for (uint32_t it=0; it<size_tyx[0]; ++it) {

        datetime t_cur = _st_ref->datetime_at_index(climits.low[0] + it);
        datetime t_next =  _st_ref->datetime_at_index(climits.low[0] + it + 1);

        uint32_t first = 0;
        uint32_t last = 0;

        t_cur.unit(_in_cube->st_reference()->dt_unit());
        t_next.unit( _in_cube->st_reference()->dt_unit());

        if (cube_stref::type_string(_in_cube->st_reference()) == "cube_stref_regular") {
            first = _in_cube->st_reference()->index_at_datetime(t_cur);
            last = _in_cube->st_reference()->index_at_datetime(t_next) - 1;
        }
        else if (cube_stref::type_string(_in_cube->st_reference()) == "cube_stref_labeled_time") {
            auto p = std::dynamic_pointer_cast<cube_stref_labeled_time>(_in_cube->st_reference());
            while (p->datetime_at_index(first) < t_cur) {
                ++first;
            }
            if (p->datetime_at_index(first) >= t_next) {
                continue; // labeled time axis of input cube as a gap larger than new time duration of cells
            }
            last = first;
            while (p->datetime_at_index(last) < t_next) {
                ++last;
            }
            --last;
        }
        if (last < first) {
            // TODO: exception or empty time slice if labeled time axis?!
            GCBS_DEBUG("Aggregation state points to invalid input time points, ignoring time slice");
            continue;
        }

        for (uint16_t ib=0; ib<_bands.count(); ++ib) {
            agg[ib]->init(((double*)out->buf()) + ib * size_tyx[0] * size_tyx[1] * size_tyx[2] + it * size_tyx[1] * size_tyx[2],
                              size_tyx[1], size_tyx[2]);
        }
        for (uint32_t i=first; i<=last; ++i) {

            if (i >= _in_cube->st_reference()->nt()) {
                break;
            }

            // Now, find out which chunk from input cube needs to be read
            chunk_coordinate_tyx in_ccords = ccoords;
            in_ccords[0] = i / _in_cube->chunk_size()[0];
            chunkid_t cur_in_chunk = _in_cube->chunk_id_from_coords(in_ccords);

            if (chunk_cache.find(cur_in_chunk) == chunk_cache.end()) {
                chunk_cache[cur_in_chunk] = _in_cube->read_chunk(cur_in_chunk);
                // TODO: remove old chunk from "cache" and use a single pointer instead of map?!
            }

            std::shared_ptr<chunk_data> in_chunk = chunk_cache[cur_in_chunk];
            if (in_chunk->empty()) {
                continue;
            }

            // feed aggregator function for each band
            for (uint16_t ib=0; ib<_bands.count(); ++ib) {
                agg[ib]->combine(((double*)out->buf()) + ib * size_tyx[0] * size_tyx[1] * size_tyx[2] + it * size_tyx[1] * size_tyx[2],
                             ((double*)in_chunk->buf()) +  ib * in_chunk->size()[1] * in_chunk->size()[2] * in_chunk->size()[3] + (i % _in_cube->chunk_size()[0]) * in_chunk->size()[2] * in_chunk->size()[3],
                             size_tyx[1], size_tyx[2]);
            }
        }
        for (uint16_t ib=0; ib<_bands.count(); ++ib) {
            agg[ib]->finalize(((double*)out->buf()) + ib * size_tyx[0] * size_tyx[1] * size_tyx[2] + it * size_tyx[1] * size_tyx[2],
                              size_tyx[1], size_tyx[2]);
        }
    }

    for (uint16_t ib=0; ib<_bands.count(); ++ib) {
        if (agg[ib] != nullptr) delete agg[ib];
    }
    return out;
}
}  // namespace gdalcubes