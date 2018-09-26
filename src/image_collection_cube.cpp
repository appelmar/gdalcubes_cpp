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
#include "utils.h"


image_collection_cube::image_collection_cube(std::shared_ptr<image_collection> ic, cube_view v) : _collection(ic),  cube(std::make_shared<cube_view>(v)) {}
image_collection_cube::image_collection_cube(std::string icfile, cube_view v) : _collection(std::make_shared<image_collection>(icfile)), cube(std::make_shared<cube_view>(v))  {}
image_collection_cube::image_collection_cube(std::shared_ptr<image_collection> ic, std::string vfile) : _collection(ic), cube(std::make_shared<cube_view>(cube_view::read_json(vfile)))  {}
image_collection_cube::image_collection_cube(std::string icfile, std::string vfile) : _collection(std::make_shared<image_collection>(icfile)),  cube(std::make_shared<cube_view>(cube_view::read_json(vfile))) {}



std::string image_collection_cube::to_string() {
    std::stringstream out;
    std::shared_ptr<cube_view> x = std::dynamic_pointer_cast<cube_view>(_st_ref);
    out << "GDAL IMAGE COLLECTION CUBE with (x,y,t)=(" << view()->nx() << "," << view()->ny() << "," << view()->nt() << ") cells in " << count_chunks() << " chunks." << std::endl;
    return out.str();
}



void image_collection_cube::write_gtiff_directory(std::string dir) {

    namespace fs = boost::filesystem;
    fs::path op(dir);

    if (!fs::exists(dir)) {
        fs::create_directories(op);
    }

    if (!fs::is_directory(dir)) {
        throw std::string("ERROR in chunking::write_gtiff_directory(): output is not a directory.");
    }

    uint32_t nchunks = count_chunks();
    for (uint32_t i=0; i<nchunks; ++i) {

        GDALDriver *gtiff_driver = (GDALDriver *)GDALGetDriverByName("GTiff");
        if (gtiff_driver == NULL) {
            throw std::string("ERROR: cannot find GDAL driver for GTiff.");
        }

        CPLStringList out_co(NULL);
        //out_co.AddNameValue("TILED", "YES");
        //out_co.AddNameValue("BLOCKXSIZE", "256");
        // out_co.AddNameValue("BLOCKYSIZE", "256");

        bounds_st cextent = this->bounds_from_chunk(i);  // implemented in derived classes
        double affine[6];
        affine[0] = cextent.s.left;
        affine[3] = cextent.s.top;
        affine[1] = _st_ref->dx();
        affine[5] = -_st_ref->dy();
        affine[2] = 0.0;
        affine[4] = 0.0;

        std::shared_ptr<chunk_data> dat = read_chunk(i);

        for (uint16_t ib = 0; ib < dat->count_bands(); ++ib) {
            for (uint32_t it = 0; it < dat->size()[1]; ++it) {
                fs::path out_file = op / (std::to_string(i) + "_" + std::to_string(ib) + "_" + std::to_string(it) + ".tif");

                GDALDataset *gdal_out = gtiff_driver->Create(out_file.string().c_str(), dat->size()[3], dat->size()[2], 1, GDT_Float64, out_co.List());
                gdal_out->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, dat->size()[3], dat->size()[2], ((double*)dat->buf())  + (ib * dat->size()[1] * dat->size()[2] * dat->size()[3] + it * dat->size()[2] * dat->size()[3]), dat->size()[3], dat->size()[2], GDT_Float64, 0, 0, NULL);
                gdal_out->GetRasterBand(1)->SetNoDataValue(NAN);
                char *wkt_out;
                OGRSpatialReference srs_out;
                srs_out.SetFromUserInput(_st_ref->proj().c_str());
                srs_out.exportToWkt(&wkt_out);

                GDALSetProjection(gdal_out, wkt_out);
                GDALSetGeoTransform(gdal_out, affine);

                GDALClose(gdal_out);
            }
        }
    }

}


struct aggregation_state {
public:
    aggregation_state(coords_nd<uint32_t, 4> size_btyx) : _size_btyx(size_btyx) {}

    virtual void init() = 0;
    virtual void update(void *chunk_buf, void *img_buf, uint32_t b, uint32_t t) = 0;
    virtual void finalize(void *buf) = 0;

protected:
    coords_nd<uint32_t, 4> _size_btyx;
};

struct aggregation_state_mean : public aggregation_state {
    aggregation_state_mean(coords_nd<uint32_t, 4> size_btyx) : aggregation_state(size_btyx), _val_count(), _img_count() {}

    ~aggregation_state_mean() {}

    void init() override {}

    void update(void *chunk_buf, void *img_buf, uint32_t b, uint32_t t) override {
        if (_img_count.find(b) == _img_count.end()) {
            _img_count[b] = std::unordered_map<uint32_t, uint16_t>();
        }
        if (_img_count[b].find(t) == _img_count[b].end()) {
            memcpy(chunk_buf, img_buf, sizeof(double) * _size_btyx[2] * _size_btyx[3]);  // TODO: make sure that b is zero-based!!!
            _img_count[b][t] = 1;
        } else {
            _img_count[b][t]++;
            if (_val_count.find(b) == _val_count.end()) {
                _val_count[b] = std::unordered_map<uint32_t, uint16_t *>();
            }
            if (_val_count[b].find(t) == _val_count[b].end()) {
                _val_count[b][t] = (uint16_t *)(calloc(_size_btyx[2] * _size_btyx[3], sizeof(uint16_t)));
                // TODO: fill with _img_count[b][t]?????
                for (uint32_t i = 0; i < _size_btyx[2] * _size_btyx[3]; ++i) {
                    _val_count[b][t][i] = _img_count[b][t];
                }
            }

            // iterate over all pixels
            for (uint32_t i = 0; i < _size_btyx[2] * _size_btyx[3]; ++i) {
                if (isnan(((double *)img_buf)[i])) continue;
                if (isnan(((double *)chunk_buf)[i])) {
                    ((double *)chunk_buf)[i] = ((double *)img_buf)[i];
                } else {
                    double sum = ((double *)chunk_buf)[i] * _val_count[b][t][i] + ((double *)chunk_buf)[i];
                    _val_count[b][t][i]++;
                    ((double *)chunk_buf)[i] = sum / _val_count[b][t][i];
                }
            }
        }
    }

    void finalize(void *buf) override {
        for (auto it = _val_count.begin(); it != _val_count.end(); ++it) {
            for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                if (it2->second) free(it2->second);
            }
        }
        _val_count.clear();
        _img_count.clear();
    }

protected:
    std::unordered_map<uint16_t, std::unordered_map<uint32_t, uint16_t>> _img_count;
    std::unordered_map<uint16_t, std::unordered_map<uint32_t, uint16_t *>> _val_count;
};

struct aggregation_state_median : public aggregation_state {
    aggregation_state_median(coords_nd<uint32_t, 4> size_btyx) : aggregation_state(size_btyx) {}
};

struct aggregation_state_min : public aggregation_state {
    aggregation_state_min(coords_nd<uint32_t, 4> size_btyx) : aggregation_state(size_btyx) {}

    void init() override {}

    void update(void *chunk_buf, void *img_buf, uint32_t b, uint32_t t) override {
        // iterate over all pixels
        for (uint32_t i = 0; i < _size_btyx[2] * _size_btyx[3]; ++i) {
            if (isnan(((double *)img_buf)[i])) continue;
            if (isnan(((double *)chunk_buf)[i])) {
                ((double *)chunk_buf)[i] = ((double *)img_buf)[i];
            } else {
                ((double *)chunk_buf)[i] = std::min(((double *)chunk_buf)[i], ((double *)img_buf)[i]);
            }
        }
    }

    void finalize(void *buf) override {}
};

struct aggregation_state_max : public aggregation_state {
    aggregation_state_max(coords_nd<uint32_t, 4> size_btyx) : aggregation_state(size_btyx) {}

    void init() override {}

    void update(void *chunk_buf, void *img_buf, uint32_t b, uint32_t t) override {
        // iterate over all pixels
        for (uint32_t i = 0; i < _size_btyx[2] * _size_btyx[3]; ++i) {
            if (isnan(((double *)img_buf)[i])) continue;
            if (isnan(((double *)chunk_buf)[i])) {
                ((double *)chunk_buf)[i] = ((double *)img_buf)[i];
            } else {
                ((double *)chunk_buf)[i] = std::max(((double *)chunk_buf)[i], ((double *)img_buf)[i]);
            }
        }
    }

    void finalize(void *buf) override {}
};

struct aggregation_state_none : public aggregation_state {
    aggregation_state_none(coords_nd<uint32_t, 4> size_btyx) : aggregation_state(size_btyx) {}

    void init() override {}
    void update(void *chunk_buf, void *img_buf, uint32_t b, uint32_t t) override {
        memcpy(chunk_buf, img_buf, sizeof(double) * _size_btyx[2] * _size_btyx[3]);
    }
    void finalize(void *buf) {}
};

/**
 *
 * The procedure to read data for a chunk is the following:
 * 1. Exclude images that are completely ouside the spatiotemporal chunk boundaries
 * 2. create a temporary in-memory VRT dataset which crops images at the boundary of the corresponding chunks and selects its bands
 * 3. use gdal warp to reproject the VRT dataset to an in-memory GDAL dataset (this will take most of the time)
 * 4. use RasterIO to read from the dataset
 * @param id
 * @return
 */

std::shared_ptr<chunk_data> image_collection_cube::read_chunk(chunkid_t id) {
    std::shared_ptr<chunk_data> out = std::make_shared<chunk_data>();
    if (id < 0 || id >= count_chunks())
        return out;  // chunk is outside of the view, we don't need to read anything.

    // Access image collection and fetch band information
    std::vector<image_collection::band_info_row> bands = _collection->get_bands();

    // create a map to relate band names to integer band coordinates
    std::unordered_map<std::string, uint16_t> band_idx;
    for (uint16_t ib=0; ib<bands.size(); ++ib) {
        band_idx[bands[ib].name] = ib;
    }

    // Derive how many pixels the chunk has (this varies for chunks at the boundary of the view)
    coords_nd<uint32_t, 3> size_tyx = chunk_size(id);
    coords_nd<uint32_t, 4> size_btyx = {bands.size(), size_tyx[0], size_tyx[1], size_tyx[2]};
    out->size(size_btyx);

    // Fill buffers accordingly
    out->buf(calloc(size_btyx[0] * size_btyx[1] * size_btyx[2] * size_btyx[3], sizeof(double)));

    // Find intersecting images from collection and iterate over these
    bounds_st cextent = bounds_from_chunk(id);  // is this->bounds_from_chunk(id) needed?
    std::vector<image_collection::find_range_st_row> datasets = _collection->find_range_st(cextent, "gdalrefs.descriptor");

    if (datasets.empty()) {
        return std::make_shared<chunk_data>();  // empty chunk data
    }

    //    CPLStringList out_co(NULL);
    //    out_co.AddNameValue("TILED", "YES");
    //    out_co.AddNameValue("BLOCKXSIZE", "256");
    //    out_co.AddNameValue("BLOCKYSIZE", "256");
    GDALDriver *gtiff_driver = (GDALDriver *)GDALGetDriverByName("GTiff");
    GDALDriver *vrt_driver = (GDALDriver *)GDALGetDriverByName("VRT");
    GDALDriver *mem_driver = (GDALDriver *)GDALGetDriverByName("MEM");

    double affine[6];
    affine[0] = cextent.s.left;
    affine[3] = cextent.s.top;
    affine[1] = _st_ref->dx();
    affine[5] = -_st_ref->dy();
    affine[2] = 0.0;
    affine[4] = 0.0;

    CPLStringList warp_args(NULL);
    warp_args.AddString("-of");
    warp_args.AddString("GTiff");  // TODO: make this configurable depending on the output size, maybe GTiff is faster? Alternatively a temporary directory might be given as configuration option that can be a ramdisk

    warp_args.AddString("-t_srs");
    warp_args.AddString(_st_ref->proj().c_str());

    warp_args.AddString("-te");  // xmin ymin xmax ymax
    warp_args.AddString(std::to_string(cextent.s.left).c_str());
    warp_args.AddString(std::to_string(cextent.s.bottom).c_str());
    warp_args.AddString(std::to_string(cextent.s.right).c_str());
    warp_args.AddString(std::to_string(cextent.s.top).c_str());

    warp_args.AddString("-dstnodata");
    warp_args.AddString("nan");

    warp_args.AddString("-srcnodata");  // <-- for testing only
    warp_args.AddString("0");           // <-- for testing only

    warp_args.AddString("-ot");
    warp_args.AddString("Float64");

    warp_args.AddString("-te_srs");
    warp_args.AddString(_st_ref->proj().c_str());

    warp_args.AddString("-ts");
    warp_args.AddString(std::to_string(size_btyx[3]).c_str());
    warp_args.AddString(std::to_string(size_btyx[2]).c_str());

    warp_args.AddString("-r");
    warp_args.AddString(resampling::to_string(view()->resampling_method()).c_str());

    // TODO: add resampling

    warp_args.AddString("-overwrite");
    //warp_args.AddString("-dstalpha"); // TODO: depend on data type?

    GDALWarpAppOptions *warp_opts = GDALWarpAppOptionsNew(warp_args.List(), NULL);
    if (warp_opts == NULL) {
        throw std::string(" default_chunking::read(): cannot create gdalwarp options.");
    }

    OGRSpatialReference proj_out;
    proj_out.SetFromUserInput(_st_ref->proj().c_str());
    char *out_wkt;
    proj_out.exportToWkt(&out_wkt);

    aggregation_state *agg = nullptr;
    if (view()->aggregation_method() == aggregation::MEAN) {
        agg = new aggregation_state_mean(size_btyx);
    } else if (view()->aggregation_method() == aggregation::MIN) {
        agg = new aggregation_state_min(size_btyx);
    } else if (view()->aggregation_method() == aggregation::MAX) {
        agg = new aggregation_state_max(size_btyx);
    }
        //    else if (->view()->aggregation_method() == aggregation::MEDIAN) {
        //        agg = new aggregation_state_median(size_btyx);
        //    }
    else
        agg = new aggregation_state_none(size_btyx);

    agg->init();

    void *img_buf = calloc(size_btyx[3] * size_btyx[2], sizeof(double));

    // For each image, call gdal_warp if projection is different than view or gdaltranslate if possible otherwise
    uint32_t i = 0;
    while (i < datasets.size()) {
        // ASSUMPTION: datasets is ordered by gdal_refs.descriptor, i.e., the GDAL dataset identifier.
        std::string descriptor_name = datasets[i].descriptor;
        std::vector<std::tuple<std::string, uint16_t>> band_rels;
        // std::vector
        while (datasets[i].descriptor == descriptor_name && i < datasets.size()) {
            band_rels.push_back(std::tuple<std::string, uint16_t>(datasets[i].band_name, datasets[i].band_num));
            ++i;
        }

        // find band data type (must be the same for all bands in the same GDALDataset / descriptor
//        GDALDataType cur_type;
//        for (uint16_t b = 0; b < bands.size(); ++b) {
//            if (bands[b].name == std::get<0>(band_rels[0])) {  // take datatype from first match
//                cur_type = bands[b].type;
//                break;
//            }
//        }

        GDALDataset *g = (GDALDataset *)GDALOpen(descriptor_name.c_str(), GA_ReadOnly);
        if (!g) {
            throw std::string(" default_chunking::read(): cannot open'" + datasets[i].descriptor + "'");
        }
        OGRSpatialReference srs_in(g->GetProjectionRef());
        double affine_in[6];
        g->GetGeoTransform(affine_in);

        // applying gdal_translate before yields better performance but (depending on parameters) causes additional resampling
        CPLStringList translate_args(NULL);
        translate_args.AddString("-of");
        translate_args.AddString("VRT");

        translate_args.AddString("-b");

        for (uint16_t b = 0; b < band_rels.size(); ++b) {
            translate_args.AddString(std::to_string(std::get<1>(band_rels[b])).c_str());
        }


        // translate_args.AddString("-projwin");

        //translate_args.AddString(std::to_string(cextent.s.left).c_str());
        // translate_args.AddString(std::to_string(cextent.s.top).c_str());
        //translate_args.AddString(std::to_string(cextent.s.right).c_str());
        //translate_args.AddString(std::to_string(cextent.s.bottom).c_str());
        //translate_args.AddString("-projwin_srs");
        //translate_args.AddString(_c->view().proj().c_str());
        //
//                translate_args.AddString("-tr");
//                translate_args.AddString(
//                        std::to_string(_c->view().dx()).c_str());
//                translate_args.AddString(
//                        std::to_string(_c->view().dy()).c_str());

        //        translate_args.AddString("-outsize");
        //        translate_args.AddString(std::to_string(  _c->view().nx()).c_str());
        //        translate_args.AddString(std::to_string( _c->view().ny()).c_str());

        GDALTranslateOptions *trans_options = GDALTranslateOptionsNew(translate_args.List(), NULL);
        if (trans_options == NULL) {
            throw std::string("default_chunking::read(): cannot create gdal_translate options.");
        }

        // TODO: Can we use gdal_translate instead gdalwarp if the conditions below are met?
        //        if (srs_in.IsSame(&srs_out) && affine_in[2] == affine[2] && affine_in[4] == affine[4] &&
        //            affine_in[0] == affine[0] && affine_in[3] == affine[3] && affine_in[1] == affine[1] &&
        //            affine_in[5] == affine[5]) {}

        GDALDatasetH cropped_vrt = GDALTranslate("", g, trans_options, NULL);
        GDALTranslateOptionsFree(trans_options);
        //GDALDatasetH cropped_vrt = GDALTranslate((std::to_string(i)  + ".vrt").c_str(), g, trans_options, NULL);
        if (!cropped_vrt) {
            // TODO: Error handling
            std::cout << "ERROR in gdal_translate for " << i << std::endl;
        }

        GDALDataset* gdal_out = mem_driver->Create("", size_btyx[3],size_btyx[2], band_rels.size(), GDT_Float64, NULL); // mask band?
        //GDALDataset *gdal_out = gtiff_driver->Create((std::to_string(i) + ".tif").c_str(), size_btyx[3], size_btyx[2], band_rels.size(), GDT_Float64, NULL);  // mask band?
        if (!gdal_out) {
            // TODO: Error handling
            std::cout << "ERROR in gdalwarp for " << i << std::endl;
        }                                                                                                                                                      //        for (uint16_t b=0; b<band_rels.size(); ++b) {


        //gdal_out->SetProjection(_c->view().proj().c_str());
        gdal_out->SetProjection(out_wkt);
        gdal_out->SetGeoTransform(affine);

        // The following loop with filling the buffer is needed for MEM datasets but not for GTiff
        for (uint16_t b = 0; b < band_rels.size(); ++b) {                                                                                                                                  //            gdal_out->GetRasterBand(b+1)->SetNoDataValue(NAN);
            gdal_out->GetRasterBand(b + 1)->Fill(NAN);
            gdal_out->GetRasterBand(b + 1)->SetNoDataValue(NAN); // why is the no data flag not available in the resulting files?
        }

        (GDALDataset *)GDALWarp(NULL, gdal_out, 1, &cropped_vrt, warp_opts, NULL);


        GDALClose(cropped_vrt);

        // Find coordinates for date of the image
        datetime dt = datetime::from_string(datasets[i - 1].datetime);  // Assumption here is that the dattime of all bands within a gdal dataset is the same, which should be OK in practice
        dt.unit() = _st_ref->dt().dt_unit;                            // explicit datetime unit cast
        int it = (dt - cextent.t0) / _st_ref->dt();
//
//        std::cout << "T=" << it << "    " << dt.to_string() << std::endl;

        // For each band, call RasterIO to read and copy data to the right position in the buffers
        for (uint16_t b = 0; b < band_rels.size(); ++b) {

            uint16_t b_internal = (band_idx[std::get<0>(band_rels[b])]);

            void *cbuf = ((double*)out->buf()) +  (b_internal * size_btyx[1] * size_btyx[2] * size_btyx[3] + it * size_btyx[2] * size_btyx[3]);

            // optimization if aggregation method = NONE, avoid copy and directly write to the chunk buffer, is this really useful?
            if (view()->aggregation_method() == aggregation::NONE) {
                gdal_out->GetRasterBand(b+1)->RasterIO(GF_Read, 0, 0, size_btyx[3], size_btyx[2], cbuf, size_btyx[3], size_btyx[2], GDT_Float64, 0, 0, NULL);
            } else {
                gdal_out->GetRasterBand(b+1)->RasterIO(GF_Read, 0, 0, size_btyx[3], size_btyx[2], img_buf, size_btyx[3], size_btyx[2], GDT_Float64, 0, 0, NULL);
                agg->update(cbuf, img_buf, b_internal, it);
            }
        }

        GDALClose(gdal_out);
    }

    agg->finalize(out->buf());
    delete agg;

    free(img_buf);

    GDALWarpAppOptionsFree(warp_opts);

    return out;
}

