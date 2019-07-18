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
#include "reduce.h"

namespace gdalcubes {

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
 * @brief Implementation of reducer to calculate sum values (for all bands) over time
 */
struct sum_reducer : public reducer {
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
                    if (!std::isnan(v)) {
                        ((double *)a->buf())[ib * a->size()[1] * a->size()[2] * a->size()[3] + ixy] += v;
                    }
                }
            }
        }
    }
    void finalize(std::shared_ptr<chunk_data> a) override {}
};

/**
 * @brief Implementation of reducer to calculate product values (for all bands) over time
 */
struct prod_reducer : public reducer {
    void init(std::shared_ptr<chunk_data> a) override {
        for (uint32_t ibxy = 0; ibxy < a->size()[0] * a->size()[2] * a->size()[3]; ++ibxy) {
            ((double *)a->buf())[ibxy] = 1;
        }
    }

    void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b) override {
        for (uint16_t ib = 0; ib < a->size()[0]; ++ib) {
            for (uint32_t it = 0; it < b->size()[1]; ++it) {
                for (uint32_t ixy = 0; ixy < b->size()[2] * b->size()[3]; ++ixy) {
                    double v = ((double *)b->buf())[ib * b->size()[1] * b->size()[2] * b->size()[3] + it * b->size()[2] * b->size()[3] + ixy];
                    if (!std::isnan(v)) {
                        ((double *)a->buf())[ib * a->size()[1] * a->size()[2] * a->size()[3] + ixy] *= v;
                    }
                }
            }
        }
    }
    void finalize(std::shared_ptr<chunk_data> a) override {}
};

/**
 * @brief Implementation of reducer to calculate mean values (for all bands) over time
 */
struct mean_reducer : public reducer {
    void init(std::shared_ptr<chunk_data> a) override {
        _count = (uint32_t *)std::calloc(a->size()[0] * a->size()[2] * a->size()[3], sizeof(uint32_t));
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
                    if (!std::isnan(v)) {
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
        std::free(_count);
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
                    if (!std::isnan(v)) {
                        double *w = &(((double *)a->buf())[ib * a->size()[1] * a->size()[2] * a->size()[3] + ixy]);
                        if (std::isnan(*w))
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
                    if (!std::isnan(v)) {
                        double *w = &(((double *)a->buf())[ib * a->size()[1] * a->size()[2] * a->size()[3] + ixy]);
                        if (std::isnan(*w))
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
                    if (!std::isnan(v)) {
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
        //_m_buckets = (std::vector<double> *)std::calloc(a->size()[0] * a->size()[2] * a->size()[3], sizeof(std::vector<double>));
        _m_buckets.resize(a->size()[0] * a->size()[2] * a->size()[3], std::vector<double>());
    }

    void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b) override {
        for (uint16_t ib = 0; ib < a->size()[0]; ++ib) {
            for (uint32_t it = 0; it < b->size()[1]; ++it) {
                for (uint32_t ixy = 0; ixy < b->size()[2] * b->size()[3]; ++ixy) {
                    double v = ((double *)b->buf())[ib * b->size()[1] * b->size()[2] * b->size()[3] + it * b->size()[2] * b->size()[3] + ixy];
                    if (!std::isnan(v)) {
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
 * @brief Implementation of reducer to calculate variance values (for all bands) over time using Welford's Online algorithm
 */
struct var_reducer : public reducer {
    void init(std::shared_ptr<chunk_data> a) override {
        _count = (uint32_t *)std::calloc(a->size()[0] * a->size()[2] * a->size()[3], sizeof(uint32_t));
        _mean = (double *)std::calloc(a->size()[0] * a->size()[2] * a->size()[3], sizeof(double));
        for (uint32_t ibxy = 0; ibxy < a->size()[0] * a->size()[2] * a->size()[3]; ++ibxy) {
            _count[ibxy] = 0;
            _mean[ibxy] = 0;
            ((double *)a->buf())[ibxy] = 0;
        }
    }

    void combine(std::shared_ptr<chunk_data> a, std::shared_ptr<chunk_data> b) override {
        for (uint16_t ib = 0; ib < a->size()[0]; ++ib) {
            for (uint32_t it = 0; it < b->size()[1]; ++it) {
                for (uint32_t ixy = 0; ixy < b->size()[2] * b->size()[3]; ++ixy) {
                    double &v = ((double *)b->buf())[ib * b->size()[1] * b->size()[2] * b->size()[3] + it * b->size()[2] * b->size()[3] + ixy];
                    if (!std::isnan(v)) {
                        double &mean = _mean[ib * a->size()[2] * a->size()[3] + ixy];
                        uint32_t &count = _count[ib * a->size()[2] * a->size()[3] + ixy];
                        ++count;
                        double delta = v - mean;
                        mean += delta / count;
                        ((double *)a->buf())[ib * a->size()[1] * a->size()[2] * a->size()[3] + ixy] += delta * (v - mean);
                    }
                }
            }
        }
    }

    virtual void finalize(std::shared_ptr<chunk_data> a) override {
        // divide by count - 1;
        for (uint32_t ibxy = 0; ibxy < a->size()[0] * a->size()[2] * a->size()[3]; ++ibxy) {
            ((double *)a->buf())[ibxy] = _count[ibxy] > 1 ? ((double *)a->buf())[ibxy] / (_count[ibxy] - 1) : NAN;
        }
        std::free(_count);
        std::free(_mean);
    }

   protected:
    uint32_t *_count;
    double *_mean;
};

/**
 * @brief Implementation of reducer to calculate standard deviation values (for all bands) over time using Welford's Online algorithm
 */
struct sd_reducer : public var_reducer {
    void finalize(std::shared_ptr<chunk_data> a) override {
        // divide by count - 1;
        for (uint32_t ibxy = 0; ibxy < a->size()[0] * a->size()[2] * a->size()[3]; ++ibxy) {
            ((double *)a->buf())[ibxy] = _count[ibxy] > 1 ? sqrt(((double *)a->buf())[ibxy] / (_count[ibxy] - 1)) : NAN;
        }
        std::free(_count);
        std::free(_mean);
    }
};

std::shared_ptr<chunk_data> reduce_cube::read_chunk(chunkid_t id) {
    GCBS_TRACE("reduce_cube::read_chunk(" + std::to_string(id) + ")");
    std::shared_ptr<chunk_data> out = std::make_shared<chunk_data>();
    if (id >= count_chunks())
        return out;  // chunk is outside of the view, we don't need to read anything.

    // If input cube is already "reduced", simply return corresponding input chunk
    if (_in_cube->size_t() == 1) {
        return _in_cube->read_chunk(id);
    }

    coords_nd<uint32_t, 3> size_tyx = chunk_size(id);
    coords_nd<uint32_t, 4> size_btyx = {_bands.count(), 1, size_tyx[1], size_tyx[2]};
    out->size(size_btyx);

    // Fill buffers accordingly
    out->buf(std::calloc(size_btyx[0] * size_btyx[1] * size_btyx[2] * size_btyx[3], sizeof(double)));
    double *begin = (double *)out->buf();
    double *end = ((double *)out->buf()) + size_btyx[0] * size_btyx[1] * size_btyx[2] * size_btyx[3];
    std::fill(begin, end, NAN);

    reducer *r = nullptr;
    if (_reducer == "min") {
        r = new min_reducer();
    } else if (_reducer == "max") {
        r = new max_reducer();
    } else if (_reducer == "mean") {
        r = new mean_reducer();
    } else if (_reducer == "median") {
        r = new median_reducer();
    } else if (_reducer == "sum") {
        r = new sum_reducer();
    } else if (_reducer == "count") {
        r = new count_reducer();
    } else if (_reducer == "prod") {
        r = new prod_reducer();
    } else if (_reducer == "var") {
        r = new var_reducer();
    } else if (_reducer == "sd") {
        r = new sd_reducer();
    } else
        throw std::string("ERROR in reduce_cube::read_chunk(): Unknown reducer given");

    r->init(out);

    // iterate over all chunks that must be read from the input cube to compute this chunk
    for (chunkid_t i = id; i < _in_cube->count_chunks(); i += _in_cube->count_chunks_x() * _in_cube->count_chunks_y()) {
        std::shared_ptr<chunk_data> x = _in_cube->read_chunk(i);
        r->combine(out, x);
    }

    r->finalize(out);
    if (r != nullptr) delete r;

    return out;
}

void reduce_cube::write_gdal_image(std::string path, std::string format, std::vector<std::string> co, std::shared_ptr<chunk_processor> p) {
    std::shared_ptr<progress> prg = config::instance()->get_default_progress_bar()->get();
    prg->set(0);  // explicitly set to zero to show progress bar immediately
    GDALDriver *drv = (GDALDriver *)GDALGetDriverByName(format.c_str());
    if (!drv) {
        throw std::string("ERROR in reduce_cube::write_gdal_image(): Cannot find GDAL driver for given format.");
    }
    // TODO: Check whether driver supports Create()

    CPLStringList out_co;
    for (uint16_t i = 0; i < co.size(); ++i) {
        out_co.AddString(co[i].c_str());
    }

    GDALDataset *gdal_out = drv->Create(path.c_str(), size_x(), size_y(), bands().count(), GDT_Float64, out_co.List());
    if (!gdal_out) {
        throw std::string("ERROR in reduce_cube::write_gdal_image(): cannot create output image");
    }

    OGRSpatialReference proj_out;
    proj_out.SetFromUserInput(_st_ref->srs().c_str());
    char *out_wkt;
    proj_out.exportToWkt(&out_wkt);

    double affine[6];
    affine[0] = _st_ref->win().left;
    affine[3] = _st_ref->win().top;
    affine[1] = _st_ref->dx();
    affine[5] = -_st_ref->dy();
    affine[2] = 0.0;
    affine[4] = 0.0;

    gdal_out->SetProjection(out_wkt);
    gdal_out->SetGeoTransform(affine);
    CPLFree(out_wkt);

    // The following loop seems to be needed for some drivers only
    for (uint16_t b = 0; b < _bands.count(); ++b) {  //            gdal_out->GetRasterBand(b+1)->SetNoDataValue(NAN);
        if (!_bands.get(b).no_data_value.empty()) {
            gdal_out->GetRasterBand(b + 1)->Fill(std::stod(_bands.get(b).no_data_value));
            gdal_out->GetRasterBand(b + 1)->SetNoDataValue(std::stod(_bands.get(b).no_data_value));
        }
    }

    GDALClose(gdal_out);
    std::function<void(chunkid_t, std::shared_ptr<chunk_data>, std::mutex &)> f = [this, &path, prg](chunkid_t id, std::shared_ptr<chunk_data> dat, std::mutex &m) {
        if (dat->empty()) {
            GCBS_WARN("Output GDAL image contains empty chunk " + std::to_string(id));
            prg->increment((double)1 / (double)this->count_chunks());
            return;
        }
        m.lock();
        GDALDataset *gdal_out = (GDALDataset *)GDALOpen(path.c_str(), GA_Update);
        m.unlock();
        //bounds_nd<uint32_t, 3> cb = chunk_limits(id);
        chunk_coordinate_tyx ct = chunk_coords_from_id(id);
        for (uint16_t b = 0; b < _bands.count(); ++b) {
            uint32_t yoff = std::max(0, ((int)size_y() - ((int)ct[1] + 1) * (int)_chunk_size[1]));
            uint32_t xoff = ct[2] * _chunk_size[2];
            uint32_t xsize = dat->size()[3];
            uint32_t ysize = dat->size()[2];
            m.lock();
            CPLErr res = gdal_out->GetRasterBand(b + 1)->RasterIO(GF_Write, xoff, yoff, xsize,
                                                                  ysize, ((double *)dat->buf()) + b * dat->size()[2] * dat->size()[3], dat->size()[3], dat->size()[2],
                                                                  GDT_Float64, 0, 0, NULL);
            if (res != CE_None) {
                GCBS_WARN("RasterIO (write) failed for " + std::string(gdal_out->GetDescription()));
            }
            m.unlock();
        }
        m.lock();
        GDALClose(gdal_out);
        m.unlock();
        prg->increment((double)1 / (double)this->count_chunks());
    };

    p->apply(shared_from_this(), f);
    prg->finalize();
}

}  // namespace gdalcubes