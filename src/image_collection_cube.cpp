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

#include "image_collection_cube.h"

#include <gdal_utils.h>
#include <map>
#include "error.h"
#include "utils.h"

image_collection_cube::image_collection_cube(std::shared_ptr<image_collection> ic, cube_view v) : cube(std::make_shared<cube_view>(v)), _collection(ic), _input_bands(), _mask(nullptr), _mask_band("") { load_bands(); }
image_collection_cube::image_collection_cube(std::string icfile, cube_view v) : cube(std::make_shared<cube_view>(v)), _collection(std::make_shared<image_collection>(icfile)), _input_bands(), _mask(nullptr), _mask_band("") { load_bands(); }
image_collection_cube::image_collection_cube(std::shared_ptr<image_collection> ic, std::string vfile) : cube(std::make_shared<cube_view>(cube_view::read_json(vfile))), _collection(ic), _input_bands(), _mask(nullptr), _mask_band("") { load_bands(); }
image_collection_cube::image_collection_cube(std::string icfile, std::string vfile) : cube(std::make_shared<cube_view>(cube_view::read_json(vfile))), _collection(std::make_shared<image_collection>(icfile)), _input_bands(), _mask(nullptr), _mask_band("") { load_bands(); }
image_collection_cube::image_collection_cube(std::shared_ptr<image_collection> ic) : cube(), _collection(ic), _input_bands(), _mask(nullptr), _mask_band("") {
    st_reference(std::make_shared<cube_view>(image_collection_cube::default_view(_collection)));
    load_bands();
}

image_collection_cube::image_collection_cube(std::string icfile) : cube(), _collection(std::make_shared<image_collection>(icfile)), _input_bands(), _mask(nullptr), _mask_band("") {
    st_reference(std::make_shared<cube_view>(image_collection_cube::default_view(_collection)));
    load_bands();
}

std::string image_collection_cube::to_string() {
    std::stringstream out;
    std::shared_ptr<cube_view> x = std::dynamic_pointer_cast<cube_view>(_st_ref);
    out << "GDAL IMAGE COLLECTION CUBE with (x,y,t)=(" << view()->nx() << "," << view()->ny() << "," << view()->nt() << ") cells in " << count_chunks() << " chunks." << std::endl;
    return out.str();
}

struct aggregation_state {
   public:
    aggregation_state(coords_nd<uint32_t, 4> size_btyx) : _size_btyx(size_btyx) {}
    virtual ~aggregation_state() {}

    virtual void init() = 0;
    virtual void update(void *chunk_buf, void *img_buf, uint32_t t) = 0;
    virtual void finalize(void *buf) = 0;

   protected:
    coords_nd<uint32_t, 4> _size_btyx;
};

struct aggregation_state_mean : public aggregation_state {
    aggregation_state_mean(coords_nd<uint32_t, 4> size_btyx) : aggregation_state(size_btyx), _img_count(), _val_count() {}

    ~aggregation_state_mean() {}

    void init() override {}

    void update(void *chunk_buf, void *img_buf, uint32_t t) override {
        for (uint32_t ib = 0; ib < _size_btyx[0]; ++ib) {
            uint32_t chunk_buf_offset =
                ib * _size_btyx[1] * _size_btyx[2] * _size_btyx[3] + t * _size_btyx[2] * _size_btyx[3];
            uint32_t img_buf_offset = ib * _size_btyx[2] * _size_btyx[3];
            if (_img_count.find(ib) == _img_count.end()) {
                _img_count[ib] = std::unordered_map<uint32_t, uint16_t>();
            }
            if (_img_count[ib].find(t) == _img_count[ib].end()) {
                memcpy(chunk_buf, img_buf,
                       sizeof(double) * _size_btyx[2] * _size_btyx[3]);  // TODO: make sure that b is zero-based!!!
                _img_count[ib][t] = 1;
            } else {
                _img_count[ib][t]++;
                if (_val_count.find(ib) == _val_count.end()) {
                    _val_count[ib] = std::unordered_map<uint32_t, uint16_t *>();
                }
                if (_val_count[ib].find(t) == _val_count[ib].end()) {
                    _val_count[ib][t] = (uint16_t *)(std::calloc(_size_btyx[2] * _size_btyx[3], sizeof(uint16_t)));
                    // TODO: fill with _img_count[b][t]?????
                    for (uint32_t i = 0; i < _size_btyx[2] * _size_btyx[3]; ++i) {
                        _val_count[ib][t][i] = _img_count[ib][t];
                    }
                }

                // iterate over all pixels
                for (uint32_t i = 0; i < _size_btyx[2] * _size_btyx[3]; ++i) {
                    if (std::isnan(((double *)img_buf)[img_buf_offset + i])) continue;
                    if (std::isnan(((double *)chunk_buf)[chunk_buf_offset + i])) {
                        ((double *)chunk_buf)[chunk_buf_offset + i] = ((double *)img_buf)[img_buf_offset + i];
                    } else {
                        double sum = ((double *)chunk_buf)[chunk_buf_offset + i] * _val_count[ib][t][i] + ((double *)chunk_buf)[chunk_buf_offset + i];
                        _val_count[ib][t][i]++;
                        ((double *)chunk_buf)[chunk_buf_offset + i] = sum / _val_count[ib][t][i];
                    }
                }
            }
        }
    }

    void finalize(void *buf) override {
        for (auto it = _val_count.begin(); it != _val_count.end(); ++it) {
            for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                if (it2->second) std::free(it2->second);
            }
        }
        _val_count.clear();
        _img_count.clear();
    }

   private:
    std::unordered_map<uint16_t, std::unordered_map<uint32_t, uint16_t>> _img_count;
    std::unordered_map<uint16_t, std::unordered_map<uint32_t, uint16_t *>> _val_count;
};

struct aggregation_state_median : public aggregation_state {
    aggregation_state_median(coords_nd<uint32_t, 4> size_btyx) : aggregation_state(size_btyx) {}

    void init() override {
        _m_buckets.resize(_size_btyx[0] * _size_btyx[1] * _size_btyx[2] * _size_btyx[3]);
    }

    void update(void *chunk_buf, void *img_buf, uint32_t t) override {
        // iterate over all pixels
        for (uint32_t ib = 0; ib < _size_btyx[0]; ++ib) {
            // uint32_t chunk_buf_offset = ib * _size_btyx[1] * _size_btyx[2] * _size_btyx[3] + t * _size_btyx[2] * _size_btyx[3];
            uint32_t img_buf_offset = ib * _size_btyx[2] * _size_btyx[3];
            for (uint32_t i = 0; i < _size_btyx[2] * _size_btyx[3]; ++i) {
                if (std::isnan(((double *)img_buf)[img_buf_offset + i]))
                    continue;
                else {
                    _m_buckets[ib * _size_btyx[1] * _size_btyx[2] * _size_btyx[3] + t * _size_btyx[2] * _size_btyx[3] +
                               i]
                        .push_back(((double *)img_buf)[img_buf_offset + i]);
                }
            }
        }
    }

    void finalize(void *buf) override {
        for (uint32_t i = 0; i < _size_btyx[0] * _size_btyx[1] * _size_btyx[2] * _size_btyx[3]; ++i) {
            std::vector<double> &list = _m_buckets[i];
            std::sort(list.begin(), list.end());
            if (list.size() == 0) {
                ((double *)buf)[i] = NAN;
            } else if (list.size() % 2 == 1) {
                ((double *)buf)[i] = list[list.size() / 2];
            } else {
                ((double *)buf)[i] = (list[list.size() / 2] + list[list.size() / 2 - 1]) / ((double)2);
            }
        }
    }

   private:
    std::vector<std::vector<double>> _m_buckets;
};

struct aggregation_state_first : public aggregation_state {
    aggregation_state_first(coords_nd<uint32_t, 4> size_btyx) : aggregation_state(size_btyx) {}

    void init() override {}

    void update(void *chunk_buf, void *img_buf, uint32_t t) override {
        for (uint32_t ib = 0; ib < _size_btyx[0]; ++ib) {
            uint32_t chunk_buf_offset =
                ib * _size_btyx[1] * _size_btyx[2] * _size_btyx[3] + t * _size_btyx[2] * _size_btyx[3];
            uint32_t img_buf_offset = ib * _size_btyx[2] * _size_btyx[3];
            // iterate over all pixels
            for (uint32_t i = 0; i < _size_btyx[2] * _size_btyx[3]; ++i) {
                if (std::isnan(((double *)img_buf)[img_buf_offset + i])) continue;
                if (!std::isnan(((double *)chunk_buf)[chunk_buf_offset + i]))
                    continue;
                else {
                    ((double *)chunk_buf)[chunk_buf_offset + i] = ((double *)img_buf)[img_buf_offset + i];
                }
            }
        }
    }

    void finalize(void *buf) override {}
};

struct aggregation_state_last : public aggregation_state {
    aggregation_state_last(coords_nd<uint32_t, 4> size_btyx) : aggregation_state(size_btyx) {}

    void init() override {}

    void update(void *chunk_buf, void *img_buf, uint32_t t) override {
        for (uint32_t ib = 0; ib < _size_btyx[0]; ++ib) {
            uint32_t chunk_buf_offset = ib * _size_btyx[1] * _size_btyx[2] * _size_btyx[3] + t * _size_btyx[2] * _size_btyx[3];
            uint32_t img_buf_offset = ib * _size_btyx[2] * _size_btyx[3];
            // iterate over all pixels
            for (uint32_t i = 0; i < _size_btyx[2] * _size_btyx[3]; ++i) {
                if (std::isnan(((double *)img_buf)[img_buf_offset + i])) continue;
                ((double *)chunk_buf)[chunk_buf_offset + i] = ((double *)img_buf)[img_buf_offset + i];
            }
        }
    }

    void finalize(void *buf) override {}
};

struct aggregation_state_min : public aggregation_state {
    aggregation_state_min(coords_nd<uint32_t, 4> size_btyx) : aggregation_state(size_btyx) {}

    void init() override {}

    void update(void *chunk_buf, void *img_buf, uint32_t t) override {
        for (uint32_t ib = 0; ib < _size_btyx[0]; ++ib) {
            uint32_t chunk_buf_offset = ib * _size_btyx[1] * _size_btyx[2] * _size_btyx[3] + t * _size_btyx[2] * _size_btyx[3];
            uint32_t img_buf_offset = ib * _size_btyx[2] * _size_btyx[3];
            // iterate over all pixels
            for (uint32_t i = 0; i < _size_btyx[2] * _size_btyx[3]; ++i) {
                if (std::isnan(((double *)img_buf)[img_buf_offset + i])) continue;
                if (std::isnan(((double *)chunk_buf)[chunk_buf_offset + i])) {
                    ((double *)chunk_buf)[chunk_buf_offset + i] = ((double *)img_buf)[img_buf_offset + i];
                } else {
                    ((double *)chunk_buf)[chunk_buf_offset + i] = std::min(((double *)chunk_buf)[chunk_buf_offset + i], ((double *)img_buf)[img_buf_offset + i]);
                }
            }
        }
    }

    void finalize(void *buf) override {}
};

struct aggregation_state_max : public aggregation_state {
    aggregation_state_max(coords_nd<uint32_t, 4> size_btyx) : aggregation_state(size_btyx) {}

    void init() override {}

    void update(void *chunk_buf, void *img_buf, uint32_t t) override {
        for (uint32_t ib = 0; ib < _size_btyx[0]; ++ib) {
            uint32_t chunk_buf_offset = ib * _size_btyx[1] * _size_btyx[2] * _size_btyx[3] + t * _size_btyx[2] * _size_btyx[3];
            uint32_t img_buf_offset = ib * _size_btyx[2] * _size_btyx[3];
            // iterate over all pixels
            for (uint32_t i = 0; i < _size_btyx[2] * _size_btyx[3]; ++i) {
                if (std::isnan(((double *)img_buf)[img_buf_offset + i])) continue;
                if (std::isnan(((double *)chunk_buf)[chunk_buf_offset + i])) {
                    ((double *)chunk_buf)[chunk_buf_offset + i] = ((double *)img_buf)[img_buf_offset + i];
                } else {
                    ((double *)chunk_buf)[chunk_buf_offset + i] = std::max(((double *)chunk_buf)[chunk_buf_offset + i], ((double *)img_buf)[img_buf_offset + i]);
                }
            }
        }
    }

    void finalize(void *buf) override {}
};

struct aggregation_state_none : public aggregation_state {
    aggregation_state_none(coords_nd<uint32_t, 4> size_btyx) : aggregation_state(size_btyx) {}

    void init() override {}
    void update(void *chunk_buf, void *img_buf, uint32_t t) override {
        for (uint32_t ib = 0; ib < _size_btyx[0]; ++ib) {
            uint32_t chunk_buf_offset = ib * _size_btyx[1] * _size_btyx[2] * _size_btyx[3] + t * _size_btyx[2] * _size_btyx[3];
            uint32_t img_buf_offset = ib * _size_btyx[2] * _size_btyx[3];
            memcpy(((double *)chunk_buf) + chunk_buf_offset, ((double *)img_buf) + img_buf_offset, sizeof(double) * _size_btyx[2] * _size_btyx[3]);
        }
    }
    void finalize(void *buf) override {}
};

/*
 * The procedure to read data for a chunk is the following:
 * 1. Exclude images that are completely ouside the spatiotemporal chunk boundaries
 * 2. create a temporary in-memory VRT dataset which crops images at the boundary of the corresponding chunks and selects its bands
 * 3. use gdal warp to reproject the VRT dataset to an in-memory GDAL dataset (this will take most of the time)
 * 4. use RasterIO to read from the dataset
 */
std::shared_ptr<chunk_data> image_collection_cube::read_chunk(chunkid_t id) {
    GCBS_TRACE("image_collection_cube::read_chunk(" + std::to_string(id) + ")");
    std::shared_ptr<chunk_data> out = std::make_shared<chunk_data>();
    if (id < 0 || id >= count_chunks()) {
        // chunk is outside of the cube, we don't need to read anything.
        GCBS_WARN("Chunk id " + std::to_string(id) + " is out of range");
        return out;
    }

    // Find intersecting images from collection and iterate over these
    // Note that these are ordered by image id and descriptor
    bounds_st cextent = bounds_from_chunk(id);
    std::vector<image_collection::find_range_st_row> datasets = _collection->find_range_st(cextent, _st_ref->srs(), "gdalrefs.image_id, gdalrefs.descriptor");

    if (datasets.empty()) {
        GCBS_DEBUG("Chunk " + std::to_string(id) + " does not intersect with any image from the image_collection_cube");
        return out;  // empty chunk data
    }

    // Derive how many pixels the chunk has (this varies for chunks at the boundary of the view)
    coords_nd<uint32_t, 3> size_tyx = chunk_size(id);
    coords_nd<uint32_t, 4> size_btyx = {_bands.count(), size_tyx[0], size_tyx[1], size_tyx[2]};
    out->size(size_btyx);

    if (size_btyx[0] * size_btyx[1] * size_btyx[2] * size_btyx[3] == 0)
        return out;

    // Fill buffers accordingly
    out->buf(std::calloc(size_btyx[0] * size_btyx[1] * size_btyx[2] * size_btyx[3], sizeof(double)));
    double *begin = (double *)out->buf();
    double *end = ((double *)out->buf()) + size_btyx[0] * size_btyx[1] * size_btyx[2] * size_btyx[3];
    std::fill(begin, end, NAN);

    // In case /vsimem/ with GTiff is used, create options might be used to improve the performance
    //    CPLStringList out_co(NULL);
    //    out_co.AddNameValue("TILED", "YES");
    //    out_co.AddNameValue("BLOCKXSIZE", "256");
    //    out_co.AddNameValue("BLOCKYSIZE", "256");

    //    double affine[6];
    //    affine[0] = cextent.s.left;
    //    affine[3] = cextent.s.top;
    //    affine[1] = _st_ref->dx();
    //    affine[5] = -_st_ref->dy();
    //    affine[2] = 0.0;
    //    affine[4] = 0.0;

    OGRSpatialReference proj_out;
    proj_out.SetFromUserInput(_st_ref->srs().c_str());

    aggregation_state *agg = nullptr;
    if (view()->aggregation_method() == aggregation::aggregation_type::AGG_MEAN) {
        agg = new aggregation_state_mean(size_btyx);
    } else if (view()->aggregation_method() == aggregation::aggregation_type::AGG_MIN) {
        agg = new aggregation_state_min(size_btyx);
    } else if (view()->aggregation_method() == aggregation::aggregation_type::AGG_MAX) {
        agg = new aggregation_state_max(size_btyx);
    } else if (view()->aggregation_method() == aggregation::aggregation_type::AGG_FIRST) {
        agg = new aggregation_state_first(size_btyx);
    } else if (view()->aggregation_method() == aggregation::aggregation_type::AGG_LAST) {
        agg = new aggregation_state_last(size_btyx);
    } else if (view()->aggregation_method() == aggregation::aggregation_type::AGG_MEDIAN) {
        agg = new aggregation_state_median(size_btyx);
    } else
        agg = new aggregation_state_none(size_btyx);

    agg->init();

    void *img_buf = std::calloc(size_btyx[0] * size_btyx[3] * size_btyx[2], sizeof(double));
    void *mask_buf = nullptr;
    if (_mask) {
        mask_buf = std::calloc(size_btyx[3] * size_btyx[2], sizeof(double));
    }

    uint32_t i = 0;
    while (i < datasets.size()) {
        std::pair<std::string, uint16_t> mask_dataset_band;
        mask_dataset_band.first = "";
        mask_dataset_band.second = 0;

        uint32_t image_id = datasets[i].image_id;
        std::string image_name = datasets[i].image_name;
        datetime dt = datetime::from_string(datasets[i].datetime);
        dt.unit() = _st_ref->dt_unit();  // explicit datetime unit cast
        duration temp_dt = _st_ref->dt();
        int itime = (dt - cextent.t0) / temp_dt;  // time index, at which time slice of the chunk buffer will this image be written?

        // map: gdal dataset descriptor -> list of contained bands (name and number)
        std::unordered_map<std::string, std::vector<std::tuple<std::string, uint16_t>>> image_datasets;
        while (i < datasets.size() && datasets[i].image_id == image_id) {
            std::string descriptor_name = datasets[i].descriptor;
            while (i < datasets.size() && datasets[i].image_id == image_id && datasets[i].descriptor == descriptor_name) {
                if (_mask) {
                    if (datasets[i].band_name == _mask_band) {
                        mask_dataset_band.first = descriptor_name;
                        mask_dataset_band.second = datasets[i].band_num;
                    }
                }
                if (_bands.has(datasets[i].band_name)) {
                    image_datasets[descriptor_name].push_back(std::tuple<std::string, uint16_t>(datasets[i].band_name, datasets[i].band_num));
                }
                ++i;
            }
        }
        if (image_datasets.empty()) {
            continue;
        }
        if (itime < 0 && itime >= (int)(out->size()[1])) {
            continue;  // image would be written outside of the chunk buffer
        }

        // refill for all images
        std::fill((double *)img_buf, ((double *)img_buf) + size_btyx[0] * size_btyx[3] * size_btyx[2], NAN);

        for (auto it = image_datasets.begin(); it != image_datasets.end(); ++it) {
            GDALDataset *g = (GDALDataset *)GDALOpen(it->first.c_str(), GA_ReadOnly);
            if (!g) {
                throw std::string("ERROR in image_collection_cube::read_chunk(): GDAL cannot open'" + it->first + "'");
            }

            OGRSpatialReference srs_in(g->GetProjectionRef());
            double affine_in[6];
            g->GetGeoTransform(affine_in);

            CPLStringList warp_args;
            warp_args.AddString("-of");
            warp_args.AddString("MEM");  // TODO: Check whether /vsimem/GTiff is faster?

            warp_args.AddString("-t_srs");
            warp_args.AddString(_st_ref->srs().c_str());

            warp_args.AddString("-te");  // xmin ymin xmax ymax
            warp_args.AddString(std::to_string(cextent.s.left).c_str());
            warp_args.AddString(std::to_string(cextent.s.bottom).c_str());
            warp_args.AddString(std::to_string(cextent.s.right).c_str());
            warp_args.AddString(std::to_string(cextent.s.top).c_str());

            warp_args.AddString("-dstnodata");
            warp_args.AddString("nan");

            warp_args.AddString("-wo");
            warp_args.AddString("INIT_DEST=nan");

            std::string nodata_value_list = "";
            uint16_t hasnodata_count = 0;
            for (uint16_t b = 0; b < it->second.size(); ++b) {
                if (!_input_bands.get(std::get<0>(it->second[b])).no_data_value.empty()) {
                    ++hasnodata_count;
                    nodata_value_list += _input_bands.get(std::get<0>(it->second[b])).no_data_value;
                    if (b < it->second.size() - 1) nodata_value_list += " ";
                }
            }
            if (hasnodata_count == it->second.size() || hasnodata_count == 1) {
                warp_args.AddString("-srcnodata");
                warp_args.AddString(("\"" + nodata_value_list + "\"").c_str());
            } else if (hasnodata_count != 0) {
                // What if nodata value is only defined for some of the bands?
                GCBS_WARN("Missing nodata value(s) for " + it->first + ", no nodata value will be used");
            }

            warp_args.AddString("-ot");
            warp_args.AddString("Float64");

            warp_args.AddString("-te_srs");
            warp_args.AddString(_st_ref->srs().c_str());

            warp_args.AddString("-ts");
            warp_args.AddString(std::to_string(size_btyx[3]).c_str());
            warp_args.AddString(std::to_string(size_btyx[2]).c_str());

            warp_args.AddString("-r");
            warp_args.AddString(resampling::to_string(view()->resampling_method()).c_str());

            // warp_args.AddString("-ovr");
            //warp_args.AddString("none");

            warp_args.AddString("-wo");
            warp_args.AddString(("NUM_THREADS=" + std::to_string(config::instance()->get_gdal_num_threads())).c_str());

            GDALWarpAppOptions *warp_opts = GDALWarpAppOptionsNew(warp_args.List(), NULL);
            if (warp_opts == NULL) {
                GDALWarpAppOptionsFree(warp_opts);
                throw std::string("ERROR in image_collection_cube::read_chunk(): cannot create gdalwarp options.");
            }

            // log gdalwarp call
            //            std::stringstream ss;
            //            ss << "Running gdalwarp ";
            //            for (uint16_t iws = 0; iws < warp_args.size(); ++iws) {
            //                ss << warp_args[iws] << " ";
            //            }
            //            ss << it->first;
            //            GCBS_TRACE(ss.str());

            GDALDataset *gdal_out = (GDALDataset *)GDALWarp("", NULL, 1, (GDALDatasetH *)(&g), warp_opts, NULL);

            // GDALDataset *gdal_out = (GDALDataset *)GDALWarp(("/vsimem/" + std::to_string(id) + "_" + std::to_string(i) + ".tif").c_str(), NULL, 1, (GDALDatasetH *)(&g), warp_opts, NULL);
            GDALWarpAppOptionsFree(warp_opts);

            // Find coordinates for date of the image

            // For each band, call RasterIO to read and copy data to the right position in the buffers
            for (uint16_t b = 0; b < it->second.size(); ++b) {
                uint16_t b_internal = _bands.get_index(std::get<0>(it->second[b]));

                // Make sure that b_internal is valid in order to prevent buffer overflows
                if (b_internal < 0 || b_internal >= out->size()[0])
                    continue;

                CPLErr res = gdal_out->GetRasterBand(b + 1)->RasterIO(GF_Read, 0, 0, size_btyx[3], size_btyx[2], ((double *)img_buf) + b_internal * size_btyx[2] * size_btyx[3], size_btyx[3], size_btyx[2], GDT_Float64, 0, 0, NULL);
                if (res != CE_None) {
                    GCBS_WARN("RasterIO (read) failed for " + std::string(gdal_out->GetDescription()));
                }
            }

            GDALClose(g);
            GDALClose(gdal_out);
        }

        // now, we have filled img_buf with data from all available bands

        if (_mask) {
            // if we apply a mask, we again read the mask band with NN / MODE resampling
            // read mask again (with NN

            // find out, which dataset has mask band
            if (mask_dataset_band.first.empty()) {
                GCBS_WARN("Missing mask band for image '" + image_name + "', mask will be ignored");
            } else {
                GDALDataset *g = (GDALDataset *)GDALOpen(mask_dataset_band.first.c_str(), GA_ReadOnly);
                if (!g) {
                    throw std::string("ERROR in image_collection_cube::read_chunk(): GDAL cannot open'" + mask_dataset_band.first + "'");
                }

                OGRSpatialReference srs_in(g->GetProjectionRef());
                double affine_in[6];
                g->GetGeoTransform(affine_in);

                CPLStringList warp_args;
                warp_args.AddString("-of");
                warp_args.AddString("MEM");

                warp_args.AddString("-t_srs");
                warp_args.AddString(_st_ref->srs().c_str());

                warp_args.AddString("-te");  // xmin ymin xmax ymax
                warp_args.AddString(std::to_string(cextent.s.left).c_str());
                warp_args.AddString(std::to_string(cextent.s.bottom).c_str());
                warp_args.AddString(std::to_string(cextent.s.right).c_str());
                warp_args.AddString(std::to_string(cextent.s.top).c_str());

                warp_args.AddString("-dstnodata");
                warp_args.AddString("nan");

                warp_args.AddString("-wo");
                warp_args.AddString("INIT_DEST=nan");

                warp_args.AddString("-ot");
                warp_args.AddString("Float64");

                warp_args.AddString("-te_srs");
                warp_args.AddString(_st_ref->srs().c_str());

                warp_args.AddString("-ts");
                warp_args.AddString(std::to_string(size_btyx[3]).c_str());
                warp_args.AddString(std::to_string(size_btyx[2]).c_str());

                warp_args.AddString("-r");
                warp_args.AddString("near");

                warp_args.AddString("-wo");
                warp_args.AddString(("NUM_THREADS=" + std::to_string(config::instance()->get_gdal_num_threads())).c_str());

                GDALWarpAppOptions *warp_opts = GDALWarpAppOptionsNew(warp_args.List(), NULL);
                if (warp_opts == NULL) {
                    GDALWarpAppOptionsFree(warp_opts);
                    throw std::string("ERROR in image_collection_cube::read_chunk(): cannot create gdalwarp options.");
                }

                GDALDataset *gdal_out = (GDALDataset *)GDALWarp("", NULL, 1, (GDALDatasetH *)(&g), warp_opts, NULL);

                GDALWarpAppOptionsFree(warp_opts);

                CPLErr res = gdal_out->GetRasterBand(mask_dataset_band.second)->RasterIO(GF_Read, 0, 0, size_btyx[3], size_btyx[2], mask_buf, size_btyx[3], size_btyx[2], GDT_Float64, 0, 0, NULL);
                if (res != CE_None) {
                    GCBS_WARN("RasterIO (read) failed for " + std::string(gdal_out->GetDescription()));
                }
                GDALClose(g);
                GDALClose(gdal_out);
                _mask->apply((double *)mask_buf, (double *)img_buf, size_btyx[0], size_btyx[2], size_btyx[3]);
            }
        }

        // feed the aggregator
        agg->update(out->buf(), img_buf, itime);
    }

    agg->finalize(out->buf());
    delete agg;

    std::free(img_buf);
    if (mask_buf) std::free(mask_buf);

    return out;
}

void image_collection_cube::load_bands() {
    // Access image collection and fetch band information
    std::vector<image_collection::bands_row> band_info = _collection->get_bands();

    // this is the band information of the cube, not of the original image bands
    for (uint16_t ib = 0; ib < band_info.size(); ++ib) {
        band bout(band_info[ib].name);
        band bin(band_info[ib].name);
        bout.unit = band_info[ib].unit;
        bin.unit = band_info[ib].unit;
        bout.type = "float64";
        bin.type = utils::string_from_gdal_type(band_info[ib].type);
        bout.scale = band_info[ib].scale;
        bin.scale = band_info[ib].scale;
        bout.offset = band_info[ib].offset;
        bin.offset = band_info[ib].offset;
        bout.no_data_value = std::to_string(NAN);
        bin.no_data_value = band_info[ib].nodata;
        _bands.add(bout);
        _input_bands.add(bin);
    }
}

cube_view image_collection_cube::default_view(std::shared_ptr<image_collection> ic) {
    bounds_st extent = ic->extent();

    cube_view out;

    std::string srs = ic->distinct_srs();
    if (srs.empty()) {
        out.srs() = "EPSG:3857";
    } else {
        out.srs() = srs;
    }

    // Transform WGS84 boundaries to target srs
    bounds_2d<double> ext_transformed = extent.s.transform("EPSG:4326", out.srs().c_str());
    out.left() = ext_transformed.left;
    out.right() = ext_transformed.right;
    out.top() = ext_transformed.top;
    out.bottom() = ext_transformed.bottom;

    uint32_t ncells_space = 512 * 512;
    double asp_ratio = (out.right() - out.left()) / (out.top() - out.bottom());
    out.nx() = (uint32_t)std::fmax((uint32_t)sqrt(ncells_space * asp_ratio), 1.0);
    out.ny() = (uint32_t)std::fmax((uint32_t)sqrt(ncells_space * 1 / asp_ratio), 1.0);

    out.t0() = extent.t0;
    out.t1() = extent.t1;

    duration d = out.t1() - out.t0();

    if (out.t0() == out.t1()) {
        out.dt_unit() = datetime_unit::DAY;
        out.dt_interval() = 1;
    } else {
        if (d.convert(datetime_unit::YEAR).dt_interval > 4) {
            out.dt_unit() = datetime_unit::YEAR;
        } else if (d.convert(datetime_unit::MONTH).dt_interval > 4) {
            out.dt_unit() = datetime_unit::MONTH;
        } else if (d.convert(datetime_unit::DAY).dt_interval > 4) {
            out.dt_unit() = datetime_unit::DAY;
        } else if (d.convert(datetime_unit::HOUR).dt_interval > 4) {
            out.dt_unit() = datetime_unit::HOUR;
        } else if (d.convert(datetime_unit::MINUTE).dt_interval > 4) {
            out.dt_unit() = datetime_unit::MINUTE;
        } else if (d.convert(datetime_unit::SECOND).dt_interval > 4) {
            out.dt_unit() = datetime_unit::SECOND;
        } else {
            out.dt_unit() = datetime_unit::DAY;
        }
        out.t0().unit() = out.dt().dt_unit;
        out.t1().unit() = out.dt().dt_unit;
        out.nt(4);
    }

    out.aggregation_method() = aggregation::aggregation_type::AGG_FIRST;
    out.resampling_method() = resampling::resampling_type::RSMPL_NEAR;

    return out;
}

void image_collection_cube::select_bands(std::vector<std::string> bands) {
    if (bands.empty()) {
        load_bands();  // restore band selection from original image collection
        return;
    }
    // Check that all given bands exist
    for (uint16_t i = 0; i < bands.size(); ++i) {
        if (!_bands.has(bands[i])) {
            GCBS_ERROR("Band '" + bands[i] + "' does not exist in image collection");
            return;
        }
    }
    band_collection sel;
    for (uint16_t i = 0; i < bands.size(); ++i) {
        band b = _bands.get(bands[i]);
        sel.add(b);
    }
    _bands = sel;
}

void image_collection_cube::select_bands(std::vector<uint16_t> bands) {
    if (bands.empty()) {
        load_bands();  // restore band selection from original image collection
        return;
    }
    // Check that all given bands exist
    for (uint16_t i = 0; i < bands.size(); ++i) {
        if (!(bands[i] >= 0 && bands[i] < _bands.count())) {
            GCBS_ERROR("Band '" + std::to_string(bands[i]) + "' does not exist in image collection");
            return;
        }
    }
    band_collection sel;
    for (uint16_t i = 0; i < bands.size(); ++i) {
        band b = _bands.get(bands[i]);
        sel.add(b);
    }
    _bands = sel;
}