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

#include "cube.h"

#include <gdal_utils.h>  // for GDAL translate
#include <netcdf.h>

#include <algorithm>  // std::transform
#include <fstream>
#include <thread>

#include "build_info.h"
#include "filesystem.h"

#if defined(R_PACKAGE) && defined(__sun) && defined(__SVR4)
#define USE_NCDF4 0
#endif

#ifndef USE_NCDF4
#define USE_NCDF4 1
#endif

namespace gdalcubes {

void cube::write_chunks_gtiff(std::string dir, std::shared_ptr<chunk_processor> p) {
    if (!filesystem::exists(dir)) {
        filesystem::mkdir_recursive(dir);
    }

    if (!filesystem::is_directory(dir)) {
        throw std::string("ERROR in cube::write_chunks_gtiff(): output is not a directory.");
    }

    GDALDriver *gtiff_driver = (GDALDriver *)GDALGetDriverByName("GTiff");
    if (gtiff_driver == NULL) {
        throw std::string("ERROR: cannot find GDAL driver for GTiff.");
    }

    if (!_st_ref->has_regular_space()) {
        throw std::string("ERROR: GeoTIFF export currently does not support irregular spatial dimensions");
    }

    // NOTE: the following will only work as long as all cube st reference types with regular spatial dimensions inherit from  cube_stref_regular class
    std::shared_ptr<cube_stref_regular> stref = std::dynamic_pointer_cast<cube_stref_regular>(_st_ref);

    std::shared_ptr<progress> prg = config::instance()->get_default_progress_bar()->get();
    prg->set(0);  // explicitly set to zero to show progress bar immediately

    std::function<void(chunkid_t, std::shared_ptr<chunk_data>, std::mutex &)> f = [this, dir, prg, gtiff_driver, stref](chunkid_t id, std::shared_ptr<chunk_data> dat, std::mutex &m) {
        bounds_st cextent = this->bounds_from_chunk(id);  // implemented in derived classes
        double affine[6];
        affine[0] = cextent.s.left;
        affine[3] = cextent.s.top;
        affine[1] = stref->dx();
        affine[5] = -stref->dy();
        affine[2] = 0.0;
        affine[4] = 0.0;

        CPLStringList out_co;
        //out_co.AddNameValue("TILED", "YES");
        //out_co.AddNameValue("BLOCKXSIZE", "256");
        // out_co.AddNameValue("BLOCKYSIZE", "256");

        for (uint16_t ib = 0; ib < dat->count_bands(); ++ib) {
            for (uint32_t it = 0; it < dat->size()[1]; ++it) {
                std::string out_file = filesystem::join(dir, (std::to_string(id) + "_" + std::to_string(ib) + "_" + std::to_string(it) + ".tif"));

                GDALDataset *gdal_out = gtiff_driver->Create(out_file.c_str(), dat->size()[3], dat->size()[2], 1, GDT_Float64, out_co.List());
                CPLErr res = gdal_out->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, dat->size()[3], dat->size()[2], ((double *)dat->buf()) + (ib * dat->size()[1] * dat->size()[2] * dat->size()[3] + it * dat->size()[2] * dat->size()[3]), dat->size()[3], dat->size()[2], GDT_Float64, 0, 0, NULL);
                if (res != CE_None) {
                    GCBS_WARN("RasterIO (write) failed for band " + _bands.get(ib).name);
                }
                gdal_out->GetRasterBand(1)->SetNoDataValue(std::stod(_bands.get(ib).no_data_value));
                char *wkt_out;
                OGRSpatialReference srs_out;
                srs_out.SetFromUserInput(_st_ref->srs().c_str());
                srs_out.exportToWkt(&wkt_out);

                GDALSetProjection(gdal_out, wkt_out);
                GDALSetGeoTransform(gdal_out, affine);
                CPLFree(wkt_out);

                GDALClose(gdal_out);
            }
        }
        prg->increment((double)1 / (double)this->count_chunks());
    };

    p->apply(shared_from_this(), f);
    prg->finalize();
}

void cube::write_tif_collection(std::string dir, std::string prefix,
                                bool overviews, bool cog,
                                std::map<std::string, std::string> creation_options,
                                std::string overview_resampling,
                                packed_export packing,
                                bool drop_empty_slices,
                                std::shared_ptr<chunk_processor> p) {
    if (!overviews && cog) {
        overviews = true;
    }

    if (!filesystem::exists(dir)) {
        filesystem::mkdir_recursive(dir);
    }
    if (!filesystem::is_directory(dir)) {
        throw std::string("ERROR in cube::write_tif_collection(): invalid output directory.");
    }

    GDALDataType ot = GDT_Float64;
    if (packing.type != packed_export::packing_type::PACK_NONE) {
        if (packing.type == packed_export::packing_type::PACK_UINT8) {
            ot = GDT_Byte;
        } else if (packing.type == packed_export::packing_type::PACK_UINT16) {
            ot = GDT_UInt16;
        } else if (packing.type == packed_export::packing_type::PACK_UINT32) {
            ot = GDT_UInt32;
        } else if (packing.type == packed_export::packing_type::PACK_INT16) {
            ot = GDT_Int16;
        } else if (packing.type == packed_export::packing_type::PACK_INT32) {
            ot = GDT_Int32;
        } else if (packing.type == packed_export::packing_type::PACK_FLOAT32) {
            ot = GDT_Float32;
            packing.offset = {0.0};
            packing.scale = {1.0};
            packing.nodata = {std::numeric_limits<float>::quiet_NaN()};
        }

        if (!(packing.scale.size() == 1 || packing.scale.size() == size_bands())) {
            std::string msg;
            if (size_bands() == 1) {
                msg = "Packed export needs exactly 1 scale and offset value.";
            } else {
                msg = "Packed export needs either n or 1 scale / offset values for n bands.";
            }
            GCBS_ERROR(msg);
            throw(msg);
        }
        if (packing.scale.size() != packing.offset.size()) {
            std::string msg = "Unequal number of scale and offset values provided for packed export.";
            GCBS_ERROR(msg);
            throw(msg);
        }
        if (packing.scale.size() != packing.nodata.size()) {
            std::string msg = "Unequal number of scale and nodata values provided for packed export.";
            GCBS_ERROR(msg);
            throw(msg);
        }
    }

    GDALDriver *gtiff_driver = (GDALDriver *)GDALGetDriverByName("GTiff");
    if (gtiff_driver == NULL) {
        throw std::string("ERROR: cannot find GDAL driver for GTiff.");
    }

    if (!_st_ref->has_regular_space()) {
        throw std::string("ERROR: GeoTIFF export currently does not support irregular spatial dimensions");
    }

    // NOTE: the following will only work as long as all cube st reference types with regular spatial dimensions inherit from  cube_stref_regular class
    std::shared_ptr<cube_stref_regular> stref = std::dynamic_pointer_cast<cube_stref_regular>(_st_ref);

    std::shared_ptr<progress> prg = config::instance()->get_default_progress_bar()->get();
    prg->set(0);  // explicitly set to zero to show progress bar immediately

    // avoid parallel RasterIO calls to the same file
    std::map<uint32_t, std::mutex> mtx;  // time_slice_index -> mutex

    CPLStringList out_co;
    out_co.AddNameValue("TILED", "YES");
    if (creation_options.find("BLOCKXSIZE") != creation_options.end()) {
        out_co.AddNameValue("BLOCKXSIZE", creation_options["BLOCKXSIZE"].c_str());
    } else {
        out_co.AddNameValue("BLOCKXSIZE", "256");
    }
    if (creation_options.find("BLOCKYSIZE") != creation_options.end()) {
        out_co.AddNameValue("BLOCKYSIZE", creation_options["BLOCKYSIZE"].c_str());
    } else {
        out_co.AddNameValue("BLOCKYSIZE", "256");
    }

    for (auto it = creation_options.begin(); it != creation_options.end(); ++it) {
        std::string key = it->first;
        std::transform(key.begin(), key.end(), key.begin(), (int (*)(int))std::toupper);
        if (key == "TILED") {
            GCBS_WARN("Setting" + it->first + "=" + it->second + "is not allowed, ignoring GeoTIFF creation option.");
            continue;
        }
        if (key == "BLOCKXSIZE" || key == "BLOCKYSIZE") continue;
        out_co.AddNameValue(it->first.c_str(), it->second.c_str());
    }

    // create all datasets
    for (uint32_t it = 0; it < size_t(); ++it) {
        std::string name = cog ? filesystem::join(dir, prefix + st_reference()->datetime_at_index(it).to_string() + "_temp.tif") : filesystem::join(dir, prefix + st_reference()->datetime_at_index(it).to_string() + ".tif");

        GDALDataset *gdal_out = gtiff_driver->Create(name.c_str(), size_x(), size_y(), size_bands(), ot, out_co.List());
        char *wkt_out;
        OGRSpatialReference srs_out;
        srs_out.SetFromUserInput(_st_ref->srs().c_str());
        srs_out.exportToWkt(&wkt_out);
        GDALSetProjection(gdal_out, wkt_out);

        double affine[6];
        affine[0] = st_reference()->left();
        affine[3] = st_reference()->top();
        affine[1] = stref->dx();
        affine[5] = -stref->dy();
        affine[2] = 0.0;
        affine[4] = 0.0;
        GDALSetGeoTransform(gdal_out, affine);
        CPLFree(wkt_out);

        // Setting NoData value seems to be not needed for Float64 GeoTIFFs
        //gdal_out->GetRasterBand(1)->SetNoDataValue(NAN); // GeoTIFF supports only one NoData value for all bands
        if (packing.type != packed_export::packing_type::PACK_NONE) {
            if (packing.scale.size() > 1) {
                for (uint16_t ib = 0; ib < size_bands(); ++ib) {
                    gdal_out->GetRasterBand(ib + 1)->SetNoDataValue(packing.nodata[ib]);
                    gdal_out->GetRasterBand(ib + 1)->SetOffset(packing.offset[ib]);
                    gdal_out->GetRasterBand(ib + 1)->SetScale(packing.scale[ib]);
                    gdal_out->GetRasterBand(ib + 1)->Fill(packing.nodata[ib]);
                }
            } else {
                for (uint16_t ib = 0; ib < size_bands(); ++ib) {
                    gdal_out->GetRasterBand(ib + 1)->SetNoDataValue(packing.nodata[0]);
                    gdal_out->GetRasterBand(ib + 1)->SetOffset(packing.offset[0]);
                    gdal_out->GetRasterBand(ib + 1)->SetScale(packing.scale[0]);
                    gdal_out->GetRasterBand(ib + 1)->Fill(packing.nodata[0]);
                }
            }
        } else {
            // Fill nodata value
            for (uint16_t ib = 0; ib < size_bands(); ++ib) {
                gdal_out->GetRasterBand(ib + 1)->Fill(NAN);
            }
        }

        GDALClose((GDALDatasetH)gdal_out);
    }

    std::function<void(chunkid_t, std::shared_ptr<chunk_data>, std::mutex &)> f = [this, dir, prg, &mtx, &prefix, &packing, cog, overviews](chunkid_t id, std::shared_ptr<chunk_data> dat, std::mutex &m) {
        if (!dat->empty()) {
            for (uint32_t it = 0; it < dat->size()[1]; ++it) {
                uint32_t cur_t_index = chunk_limits(id).low[0] + it;
                std::string name = cog ? filesystem::join(dir, prefix + st_reference()->datetime_at_index(cur_t_index).to_string() + "_temp.tif") : filesystem::join(dir, prefix + st_reference()->datetime_at_index(cur_t_index).to_string() + ".tif");

                mtx[cur_t_index].lock();
                GDALDataset *gdal_out = (GDALDataset *)GDALOpen(name.c_str(), GA_Update);
                if (!gdal_out) {
                    GCBS_WARN("GDAL failed to open " + name);
                    mtx[cur_t_index].unlock();
                    continue;
                }

                // apply packing
                if (packing.type != packed_export::packing_type::PACK_NONE) {
                    for (uint16_t ib = 0; ib < size_bands(); ++ib) {
                        double cur_scale;
                        double cur_offset;
                        double cur_nodata;
                        if (packing.scale.size() == size_bands()) {
                            cur_scale = packing.scale[ib];
                            cur_offset = packing.offset[ib];
                            cur_nodata = packing.nodata[ib];
                        } else {
                            cur_scale = packing.scale[0];
                            cur_offset = packing.offset[0];
                            cur_nodata = packing.nodata[0];
                        }

                        /*
                         * If band of cube already has scale + offset, we do not apply this before.
                         * As a consequence, provided scale and offset values refer to actual data values
                         * but ignore band metadata. The following commented code would apply the
                         * unpacking before
                         */
                        /*
                        if (bands().get(ib).scale != 1 || bands().get(ib).offset != 0) {
                            for (uint32_t i = 0; i < dat->size()[2] * dat->size()[3]; ++i) {
                                double &v = ((double *)(dat->buf()))[ib * dat->size()[1] * dat->size()[2] * dat->size()[3] +
                                                                     it * dat->size()[2] * dat->size()[3]  + i];
                                v = v * bands().get(ib).scale + bands().get(ib).offset;
                            }
                        } */

                        for (uint32_t i = 0; i < dat->size()[2] * dat->size()[3]; ++i) {
                            double &v = ((double *)(dat->buf()))[ib * dat->size()[1] * dat->size()[2] * dat->size()[3] +
                                                                 it * dat->size()[2] * dat->size()[3] + i];
                            if (std::isnan(v)) {
                                v = cur_nodata;
                            } else {
                                v = std::round((v - cur_offset) / cur_scale);  // use std::round to avoid truncation bias
                            }
                        }
                    }
                }  // if packing

                for (uint16_t ib = 0; ib < size_bands(); ++ib) {
                    CPLErr res = gdal_out->GetRasterBand(ib + 1)->RasterIO(GF_Write, chunk_limits(id).low[2], size_y() - chunk_limits(id).high[1] - 1, dat->size()[3], dat->size()[2],
                                                                           ((double *)dat->buf()) + (ib * dat->size()[1] * dat->size()[2] * dat->size()[3] + it * dat->size()[2] * dat->size()[3]),
                                                                           dat->size()[3], dat->size()[2], GDT_Float64, 0, 0, NULL);
                    if (res != CE_None) {
                        GCBS_WARN("RasterIO (write) failed for " + name);
                        break;
                    }
                }

                GDALClose(gdal_out);
                mtx[cur_t_index].unlock();
            }
        }

        if (overviews) {
            prg->increment((double)0.5 / (double)this->count_chunks());
        } else {
            prg->increment((double)1 / (double)this->count_chunks());
        }
    };

    p->apply(shared_from_this(), f);

    // build overviews and convert to COG (with IFDs of overviews at the beginning of the file)
    // TODO: use multiple threads

    if (overviews) {
        for (uint32_t it = 0; it < size_t(); ++it) {
            std::string name = cog ? filesystem::join(dir, prefix + st_reference()->datetime_at_index(it).to_string() + "_temp.tif") : filesystem::join(dir, prefix + st_reference()->datetime_at_index(it).to_string() + ".tif");

            GDALDataset *gdal_out = (GDALDataset *)GDALOpen(name.c_str(), GA_Update);
            if (!gdal_out) {
                continue;
            }

            int n_overviews = (int)std::ceil(std::log2(std::fmax(double(size_x()), double(size_y())) / 256));
            std::vector<int> overview_list;
            for (int i = 1; i <= n_overviews; ++i) {
                overview_list.push_back(std::pow(2, i));
            }

            if (!overview_list.empty()) {
                CPLErr res = GDALBuildOverviews(gdal_out, overview_resampling.c_str(), n_overviews, overview_list.data(), 0, NULL, NULL, nullptr);
                if (res != CE_None) {
                    GCBS_WARN("GDALBuildOverviews failed for " + name);
                    GDALClose(gdal_out);
                    continue;
                }
            }

            if (cog) {
                CPLStringList translate_args;
                translate_args.AddString("-of");
                translate_args.AddString("GTiff");
                translate_args.AddString("-co");
                translate_args.AddString("COPY_SRC_OVERVIEWS=YES");
                translate_args.AddString("-co");
                translate_args.AddString("TILED=YES");

                if (creation_options.find("BLOCKXSIZE") != creation_options.end()) {
                    translate_args.AddString("-co");
                    translate_args.AddString(("BLOCKXSIZE=" + creation_options["BLOCKXSIZE"]).c_str());
                } else {
                    translate_args.AddString("-co");
                    translate_args.AddString("BLOCKXSIZE=256");
                }
                if (creation_options.find("BLOCKYSIZE") != creation_options.end()) {
                    translate_args.AddString("-co");
                    translate_args.AddString(("BLOCKYSIZE=" + creation_options["BLOCKYSIZE"]).c_str());
                } else {
                    translate_args.AddString("-co");
                    translate_args.AddString("BLOCKYSIZE=256");
                }
                for (auto it = creation_options.begin(); it != creation_options.end(); ++it) {
                    std::string key = it->first;
                    std::transform(key.begin(), key.end(), key.begin(), (int (*)(int))std::toupper);
                    if (key == "TILED") {
                        GCBS_WARN("Setting" + it->first + "=" + it->second + "is not allowed, ignoring GeoTIFF creation option.");
                        continue;
                    }
                    if (key == "BLOCKXSIZE" || key == "BLOCKYSIZE") continue;
                    if (key == "COPY_SRC_OVERVIEWS") {
                        GCBS_WARN("Setting" + it->first + "=" + it->second + "is not allowed, ignoring GeoTIFF creation option.");
                        continue;
                    }
                    out_co.AddNameValue(it->first.c_str(), it->second.c_str());
                    translate_args.AddString("-co");
                    translate_args.AddString((it->first + "=" + it->second).c_str());
                }

                GDALTranslateOptions *trans_options = GDALTranslateOptionsNew(translate_args.List(), NULL);
                if (trans_options == NULL) {
                    GCBS_ERROR("ERROR in cube::write_tif_collection(): Cannot create gdal_translate options.");
                    throw std::string("ERROR in cube::write_tif_collection(): Cannot create gdal_translate options.");
                }
                std::string cogname = filesystem::join(dir, prefix + st_reference()->datetime_at_index(it).to_string() + ".tif");
                GDALDatasetH gdal_cog = GDALTranslate(cogname.c_str(), (GDALDatasetH)gdal_out, trans_options, NULL);

                GDALClose((GDALDatasetH)gdal_out);
                filesystem::remove(name);
                GDALClose((GDALDatasetH)gdal_cog);
                GDALTranslateOptionsFree(trans_options);
            }

            prg->increment((double)0.5 / (double)size_t());
        }
    }

    prg->set(1.0);
    prg->finalize();
}

void cube::write_netcdf_file(std::string path, uint8_t compression_level, bool with_VRT, bool write_bounds,
                             packed_export packing, bool drop_empty_slices, std::shared_ptr<chunk_processor> p) {
    std::string op = filesystem::make_absolute(path);

    if (filesystem::is_directory(op)) {
        throw std::string("ERROR in cube::write_netcdf_file(): output already exists and is a directory.");
    }
    if (filesystem::is_regular_file(op)) {
        GCBS_INFO("Existing file '" + op + "' will be overwritten for NetCDF export");
    }

    if (!filesystem::exists(filesystem::parent(op))) {
        filesystem::mkdir_recursive(filesystem::parent(op));
    }

    if (!_st_ref->has_regular_space()) {
        throw std::string("ERROR: netCDF export currently does not support irregular spatial dimensions");
    }

    // NOTE: the following will only work as long as all cube st reference types with regular spatial dimensions inherit from  cube_stref_regular class
    std::shared_ptr<cube_stref_regular> stref = std::dynamic_pointer_cast<cube_stref_regular>(_st_ref);

    std::shared_ptr<progress> prg = config::instance()->get_default_progress_bar()->get();
    prg->set(0);  // explicitly set to zero to show progress bar immediately

    int ot = NC_DOUBLE;
    if (packing.type != packed_export::packing_type::PACK_NONE) {
        if (packing.type == packed_export::packing_type::PACK_UINT8) {
            ot = NC_UBYTE;
        } else if (packing.type == packed_export::packing_type::PACK_UINT16) {
            ot = NC_USHORT;
        } else if (packing.type == packed_export::packing_type::PACK_UINT32) {
            ot = NC_UINT;
        } else if (packing.type == packed_export::packing_type::PACK_INT16) {
            ot = NC_SHORT;
        } else if (packing.type == packed_export::packing_type::PACK_INT32) {
            ot = NC_INT;
        } else if (packing.type == packed_export::packing_type::PACK_FLOAT32) {
            ot = NC_FLOAT;
            packing.offset = {0.0};
            packing.scale = {1.0};
            packing.nodata = {std::numeric_limits<float>::quiet_NaN()};
        }

        if (!(packing.scale.size() == 1 || packing.scale.size() == size_bands())) {
            std::string msg;
            if (size_bands() == 1) {
                msg = "Packed export needs exactly 1 scale and offset value.";
            } else {
                msg = "Packed export needs either n or 1 scale / offset values for n bands.";
            }
            GCBS_ERROR(msg);
            throw(msg);
        }
        if (packing.scale.size() != packing.offset.size()) {
            std::string msg = "Unequal number of scale and offset values provided for packed export.";
            GCBS_ERROR(msg);
            throw(msg);
        }
        if (packing.scale.size() != packing.nodata.size()) {
            std::string msg = "Unequal number of scale and nodata values provided for packed export.";
            GCBS_ERROR(msg);
            throw(msg);
        }
    }

    double *dim_x = (double *)std::calloc(size_x(), sizeof(double));
    double *dim_y = (double *)std::calloc(size_y(), sizeof(double));
    int *dim_t = (int *)std::calloc(size_t(), sizeof(int));

    double *dim_x_bnds = nullptr;
    double *dim_y_bnds = nullptr;
    int *dim_t_bnds = nullptr;

    if (write_bounds) {
        dim_x_bnds = (double *)std::calloc(size_x() * 2, sizeof(double));
        dim_y_bnds = (double *)std::calloc(size_y() * 2, sizeof(double));
        dim_t_bnds = (int *)std::calloc(size_t() * 2, sizeof(int));
    }

    if (stref->dt().dt_unit == datetime_unit::WEEK) {
        stref->dt_unit(datetime_unit::DAY);
        stref->dt_interval(stref->dt_interval() * 7);  // UDUNIT does not support week
    }

    if (stref->has_regular_time()) {
        for (uint32_t i = 0; i < size_t(); ++i) {
            dim_t[i] = (i * stref->dt().dt_interval);
        }
    } else {
        for (uint32_t i = 0; i < size_t(); ++i) {
            dim_t[i] = (stref->datetime_at_index(i) - stref->t0()).dt_interval;
        }
    }

    for (uint32_t i = 0; i < size_y(); ++i) {
        dim_y[i] = stref->win().bottom + size_y() * stref->dy() - (i + 0.5) * stref->dy();  // cell center
    }
    for (uint32_t i = 0; i < size_x(); ++i) {
        dim_x[i] = stref->win().left + (i + 0.5) * stref->dx();
    }

    if (write_bounds) {
        if (stref->has_regular_time()) {
            for (uint32_t i = 0; i < size_t(); ++i) {
                dim_t_bnds[2 * i] = (i * stref->dt().dt_interval);
                dim_t_bnds[2 * i + 1] = ((i + 1) * stref->dt().dt_interval);
            }
        } else {
            for (uint32_t i = 0; i < size_t(); ++i) {
                dim_t_bnds[2 * i] = (stref->datetime_at_index(i) - stref->t0()).dt_interval;
                dim_t_bnds[2 * i + 1] = dim_t_bnds[2 * i] + stref->dt_interval();
            }
        }

        for (uint32_t i = 0; i < size_y(); ++i) {
            dim_y_bnds[2 * i] = stref->win().bottom + size_y() * stref->dy() - (i)*stref->dy();
            dim_y_bnds[2 * i + 1] = stref->win().bottom + size_y() * stref->dy() - (i + 1) * stref->dy();
        }
        for (uint32_t i = 0; i < size_x(); ++i) {
            dim_x_bnds[2 * i] = stref->win().left + (i + 0) * stref->dx();
            dim_x_bnds[2 * i + 1] = stref->win().left + (i + 1) * stref->dx();
        }
    }

    OGRSpatialReference srs = st_reference()->srs_ogr();
    std::string yname = srs.IsProjected() ? "y" : "latitude";
    std::string xname = srs.IsProjected() ? "x" : "longitude";

    int ncout;

#if USE_NCDF4 == 1
    nc_create(op.c_str(), NC_NETCDF4, &ncout);
#else
    nc_create(op.c_str(), NC_CLASSIC_MODEL, &ncout);
#endif

    int d_t, d_y, d_x;
    nc_def_dim(ncout, "time", size_t(), &d_t);
    nc_def_dim(ncout, yname.c_str(), size_y(), &d_y);
    nc_def_dim(ncout, xname.c_str(), size_x(), &d_x);

    int d_bnds = -1;
    if (write_bounds) {
        nc_def_dim(ncout, "nv", 2, &d_bnds);
    }

    int v_t, v_y, v_x;
    nc_def_var(ncout, "time", NC_INT, 1, &d_t, &v_t);
    nc_def_var(ncout, yname.c_str(), NC_DOUBLE, 1, &d_y, &v_y);
    nc_def_var(ncout, xname.c_str(), NC_DOUBLE, 1, &d_x, &v_x);

    int v_tbnds, v_ybnds, v_xbnds;
    int d_tbnds[] = {d_t, d_bnds};
    int d_ybnds[] = {d_y, d_bnds};
    int d_xbnds[] = {d_x, d_bnds};
    if (write_bounds) {
        nc_def_var(ncout, "time_bnds", NC_INT, 2, d_tbnds, &v_tbnds);
        nc_def_var(ncout, "y_bnds", NC_DOUBLE, 2, d_ybnds, &v_ybnds);
        nc_def_var(ncout, "x_bnds", NC_DOUBLE, 2, d_xbnds, &v_xbnds);
    }

    std::string att_source = "gdalcubes " + std::to_string(GDALCUBES_VERSION_MAJOR) + "." + std::to_string(GDALCUBES_VERSION_MINOR) + "." + std::to_string(GDALCUBES_VERSION_PATCH);

    nc_put_att_text(ncout, NC_GLOBAL, "Conventions", strlen("CF-1.6"), "CF-1.6");
    nc_put_att_text(ncout, NC_GLOBAL, "source", strlen(att_source.c_str()), att_source.c_str());

    // write json graph as metadata
    std::string j = make_constructible_json().dump();
    nc_put_att_text(ncout, NC_GLOBAL, "process_graph", j.length(), j.c_str());

    char *wkt;
    srs.exportToWkt(&wkt);

    //double geoloc_array[6] = {stref->left(), stref->dx(), 0.0, stref->top(), 0.0, stref->dy()};
    std::string geoloc_array_str = utils::dbl_to_string(stref->left()) + " " + utils::dbl_to_string(stref->dx()) + " 0 " + utils::dbl_to_string(stref->top()) + " 0 " + utils::dbl_to_string(-stref->dy());
    //nc_put_att_text(ncout, NC_GLOBAL, "spatial_ref", strlen(wkt), wkt);
    //nc_put_att_double(ncout, NC_GLOBAL, "GeoTransform", NC_DOUBLE, 6, geoloc_array);

    std::string dtunit_str;
    if (stref->dt().dt_unit == datetime_unit::YEAR) {
        dtunit_str = "years";  // WARNING: UDUNITS defines a year as 365.2425 days
    } else if (stref->dt().dt_unit == datetime_unit::MONTH) {
        dtunit_str = "months";  // WARNING: UDUNITS defines a month as 1/12 year
    } else if (stref->dt().dt_unit == datetime_unit::DAY) {
        dtunit_str = "days";
    } else if (stref->dt().dt_unit == datetime_unit::HOUR) {
        dtunit_str = "hours";
    } else if (stref->dt().dt_unit == datetime_unit::MINUTE) {
        dtunit_str = "minutes";
    } else if (stref->dt().dt_unit == datetime_unit::SECOND) {
        dtunit_str = "seconds";
    }
    dtunit_str += " since ";
    dtunit_str += stref->t0().to_string(datetime_unit::SECOND);

    nc_put_att_text(ncout, v_t, "standard_name", strlen("time"), "time");
    nc_put_att_text(ncout, v_t, "long_name", strlen("time"), "time");
    nc_put_att_text(ncout, v_t, "units", strlen(dtunit_str.c_str()), dtunit_str.c_str());
    nc_put_att_text(ncout, v_t, "calendar", strlen("gregorian"), "gregorian");
    nc_put_att_text(ncout, v_t, "axis", strlen("T"), "T");  // this avoids GDAL warnings

    if (srs.IsProjected()) {
        // GetLinearUnits(char **) is deprecated since GDAL 2.3.0
#if GDAL_VERSION_MAJOR > 2 || (GDAL_VERSION_MAJOR == 2 && GDAL_VERSION_MINOR >= 3)
        const char *unit = nullptr;
#else
        char *unit = nullptr;
#endif
        srs.GetLinearUnits(&unit);

        nc_put_att_text(ncout, v_y, "standard_name", strlen("projection_y_coordinate"), "projection_y_coordinate");
        nc_put_att_text(ncout, v_y, "long_name", strlen("y coordinate of projection"), "y coordinate of projection");
        nc_put_att_text(ncout, v_y, "units", strlen(unit), unit);
        nc_put_att_text(ncout, v_y, "axis", strlen("Y"), "Y");
        nc_put_att_text(ncout, v_x, "standard_name", strlen("projection_x_coordinate"), "projection_x_coordinate");
        nc_put_att_text(ncout, v_x, "long_name", strlen("x coordinate of projection"), "x coordinate of projection");
        nc_put_att_text(ncout, v_x, "units", strlen(unit), unit);
        nc_put_att_text(ncout, v_x, "axis", strlen("X"), "X");

        int v_crs;
        nc_def_var(ncout, "crs", NC_CHAR, 0, NULL, &v_crs);
        //nc_put_att_text(ncout, v_crs, "grid_mapping_name", strlen("easting_northing"), "easting_northing");
        nc_put_att_text(ncout, v_crs, "spatial_ref", strlen(wkt), wkt);
        //nc_put_att_double(ncout, v_crs, "GeoTransform", NC_DOUBLE, 6, geoloc_array);
        nc_put_att_text(ncout, v_crs, "GeoTransform", strlen(geoloc_array_str.c_str()), geoloc_array_str.c_str());

    } else {
        // char* unit;
        // double scale = srs.GetAngularUnits(&unit);
        nc_put_att_text(ncout, v_y, "units", strlen("degrees_north"), "degrees_north");
        nc_put_att_text(ncout, v_y, "long_name", strlen("latitude"), "latitude");
        nc_put_att_text(ncout, v_y, "standard_name", strlen("latitude"), "latitude");
        nc_put_att_text(ncout, v_y, "axis", strlen("Y"), "Y");

        nc_put_att_text(ncout, v_x, "units", strlen("degrees_east"), "degrees_east");
        nc_put_att_text(ncout, v_x, "long_name", strlen("longitude"), "longitude");
        nc_put_att_text(ncout, v_x, "standard_name", strlen("longitude"), "longitude");
        nc_put_att_text(ncout, v_x, "axis", strlen("X"), "X");

        int v_crs;
        //nc_put_att_text(ncout, v_crs, "grid_mapping_name", strlen("latitude_longitude"), "latitude_longitude");
        //nc_put_att_text(ncout, v_crs, "crs_wkt", strlen(wkt), wkt);
        nc_def_var(ncout, "crs", NC_CHAR, 0, NULL, &v_crs);
        //nc_put_att_text(ncout, v_crs, "grid_mapping_name", strlen("easting_northing"), "easting_northing");
        nc_put_att_text(ncout, v_crs, "spatial_ref", strlen(wkt), wkt);
        //nc_put_att_double(ncout, v_crs, "GeoTransform", NC_DOUBLE, 6, geoloc_array);
        nc_put_att_text(ncout, v_crs, "GeoTransform", strlen(geoloc_array_str.c_str()), geoloc_array_str.c_str());
    }
    CPLFree(wkt);
    int d_all[] = {d_t, d_y, d_x};

    std::vector<int> v_bands;

    for (uint16_t i = 0; i < bands().count(); ++i) {
        int v;
        nc_def_var(ncout, bands().get(i).name.c_str(), ot, 3, d_all, &v);
        std::size_t csize[3] = {_chunk_size[0], _chunk_size[1], _chunk_size[2]};
#if USE_NCDF4 == 1
        nc_def_var_chunking(ncout, v, NC_CHUNKED, csize);
#endif
        if (compression_level > 0) {
#if USE_NCDF4 == 1
            nc_def_var_deflate(ncout, v, 1, 1, compression_level);  // TODO: experiment with shuffling
#else
            GCBS_WARN("gdalcubes has been built with support for netCDF-3 classic model only; compression will be ignored.");
#endif
        }

        if (!bands().get(i).unit.empty())
            nc_put_att_text(ncout, v, "units", strlen(bands().get(i).unit.c_str()), bands().get(i).unit.c_str());

        double pscale = bands().get(i).scale;
        double poff = bands().get(i).offset;
        double pNAN = NAN;

        if (packing.type != packed_export::packing_type::PACK_NONE) {
            if (packing.scale.size() > 1) {
                pscale = packing.scale[i];
                poff = packing.offset[i];
                pNAN = packing.nodata[i];
            } else {
                pscale = packing.scale[0];
                poff = packing.offset[0];
                pNAN = packing.nodata[0];
            }
        }

        nc_put_att_double(ncout, v, "scale_factor", NC_DOUBLE, 1, &pscale);
        nc_put_att_double(ncout, v, "add_offset", NC_DOUBLE, 1, &poff);
        nc_put_att_text(ncout, v, "type", strlen(bands().get(i).type.c_str()), bands().get(i).type.c_str());
        nc_put_att_text(ncout, v, "grid_mapping", strlen("crs"), "crs");

        // this doesn't seem to solve missing spatial reference for multitemporal nc files
        //        nc_put_att_text(ncout, v, "spatial_ref", strlen(wkt), wkt);
        //        nc_put_att_double(ncout, v, "GeoTransform", NC_DOUBLE, 6, geoloc_array);

        nc_put_att_double(ncout, v, "_FillValue", ot, 1, &pNAN);

        v_bands.push_back(v);
    }

    nc_enddef(ncout);  ////////////////////////////////////////////////////

    nc_put_var(ncout, v_t, (void *)dim_t);
    nc_put_var(ncout, v_y, (void *)dim_y);
    nc_put_var(ncout, v_x, (void *)dim_x);

    if (write_bounds) {
        nc_put_var(ncout, v_tbnds, (void *)dim_t_bnds);
        nc_put_var(ncout, v_ybnds, (void *)dim_y_bnds);
        nc_put_var(ncout, v_xbnds, (void *)dim_x_bnds);
    }

    if (dim_t) std::free(dim_t);
    if (dim_y) std::free(dim_y);
    if (dim_x) std::free(dim_x);

    if (write_bounds) {
        if (dim_t_bnds) std::free(dim_t_bnds);
        if (dim_y_bnds) std::free(dim_y_bnds);
        if (dim_x_bnds) std::free(dim_x_bnds);
    }

    std::function<void(chunkid_t, std::shared_ptr<chunk_data>, std::mutex &)> f = [this, op, prg, &v_bands, ncout, &packing](chunkid_t id, std::shared_ptr<chunk_data> dat, std::mutex &m) {
        chunk_size_btyx csize = dat->size();
        bounds_nd<uint32_t, 3> climits = chunk_limits(id);
        std::size_t startp[] = {climits.low[0], size_y() - climits.high[1] - 1, climits.low[2]};
        std::size_t countp[] = {csize[1], csize[2], csize[3]};

        for (uint16_t i = 0; i < bands().count(); ++i) {
            if (packing.type != packed_export::packing_type::PACK_NONE) {
                double cur_scale;
                double cur_offset;
                double cur_nodata;
                if (packing.scale.size() == size_bands()) {
                    cur_scale = packing.scale[i];
                    cur_offset = packing.offset[i];
                    cur_nodata = packing.nodata[i];
                } else {
                    cur_scale = packing.scale[0];
                    cur_offset = packing.offset[0];
                    cur_nodata = packing.nodata[0];
                }

                /*
               * If band of cube already has scale + offset, we do not apply this before.
               * As a consequence, provided scale and offset values refer to actual data values
               * but ignore band metadata. The following commented code would apply the
               * unpacking before
               */
                /*
              if (bands().get(i).scale != 1 || bands().get(i).offset != 0) {
                  for (uint32_t i = 0; i < dat->size()[1] * dat->size()[2] * dat->size()[3]; ++i) {
                      double &v = ((double *)(dat->buf()))[i * dat->size()[1] * dat->size()[2] * dat->size()[3] + i];
                      v = v * bands().get(i).scale + bands().get(i).offset;
                  }
              } */

                uint8_t *packedbuf = nullptr;

                if (packing.type == packed_export::packing_type::PACK_UINT8) {
                    packedbuf = (uint8_t *)std::malloc(dat->size()[1] * dat->size()[2] * dat->size()[3] * sizeof(uint8_t));
                    for (uint32_t iv = 0; iv < dat->size()[1] * dat->size()[2] * dat->size()[3]; ++iv) {
                        double &v = ((double *)(dat->buf()))[i * dat->size()[1] * dat->size()[2] * dat->size()[3] + iv];
                        if (std::isnan(v)) {
                            v = cur_nodata;
                        } else {
                            v = std::round((v - cur_offset) / cur_scale);  // use std::round to avoid truncation bias
                        }
                        ((uint8_t *)(packedbuf))[iv] = v;
                    }
                    m.lock();
                    nc_put_vara(ncout, v_bands[i], startp, countp, (void *)(packedbuf));
                    m.unlock();
                } else if (packing.type == packed_export::packing_type::PACK_UINT16) {
                    packedbuf = (uint8_t *)std::malloc(dat->size()[1] * dat->size()[2] * dat->size()[3] * sizeof(uint16_t));
                    for (uint32_t iv = 0; iv < dat->size()[1] * dat->size()[2] * dat->size()[3]; ++iv) {
                        double &v = ((double *)(dat->buf()))[i * dat->size()[1] * dat->size()[2] * dat->size()[3] + iv];
                        if (std::isnan(v)) {
                            v = cur_nodata;
                        } else {
                            v = std::round((v - cur_offset) / cur_scale);  // use std::round to avoid truncation bias
                        }
                        ((uint16_t *)(packedbuf))[iv] = v;
                    }
                    m.lock();
                    nc_put_vara(ncout, v_bands[i], startp, countp, (void *)(packedbuf));
                    m.unlock();
                } else if (packing.type == packed_export::packing_type::PACK_UINT32) {
                    packedbuf = (uint8_t *)std::malloc(dat->size()[1] * dat->size()[2] * dat->size()[3] * sizeof(uint32_t));
                    for (uint32_t iv = 0; iv < dat->size()[1] * dat->size()[2] * dat->size()[3]; ++iv) {
                        double &v = ((double *)(dat->buf()))[i * dat->size()[1] * dat->size()[2] * dat->size()[3] + iv];
                        if (std::isnan(v)) {
                            v = cur_nodata;
                        } else {
                            v = std::round((v - cur_offset) / cur_scale);  // use std::round to avoid truncation bias
                        }
                        ((uint32_t *)(packedbuf))[iv] = v;
                    }
                    m.lock();
                    nc_put_vara(ncout, v_bands[i], startp, countp, (void *)(packedbuf));
                    m.unlock();
                } else if (packing.type == packed_export::packing_type::PACK_INT16) {
                    packedbuf = (uint8_t *)std::malloc(dat->size()[1] * dat->size()[2] * dat->size()[3] * sizeof(int16_t));
                    for (uint32_t iv = 0; iv < dat->size()[1] * dat->size()[2] * dat->size()[3]; ++iv) {
                        double &v = ((double *)(dat->buf()))[i * dat->size()[1] * dat->size()[2] * dat->size()[3] + iv];
                        if (std::isnan(v)) {
                            v = cur_nodata;
                        } else {
                            v = std::round((v - cur_offset) / cur_scale);  // use std::round to avoid truncation bias
                        }
                        ((int16_t *)(packedbuf))[iv] = v;
                    }
                    m.lock();
                    nc_put_vara(ncout, v_bands[i], startp, countp, (void *)(packedbuf));
                    m.unlock();
                } else if (packing.type == packed_export::packing_type::PACK_INT32) {
                    packedbuf = (uint8_t *)std::malloc(dat->size()[1] * dat->size()[2] * dat->size()[3] * sizeof(int32_t));
                    for (uint32_t iv = 0; iv < dat->size()[1] * dat->size()[2] * dat->size()[3]; ++iv) {
                        double &v = ((double *)(dat->buf()))[i * dat->size()[1] * dat->size()[2] * dat->size()[3] + iv];
                        if (std::isnan(v)) {
                            v = cur_nodata;
                        } else {
                            v = std::round((v - cur_offset) / cur_scale);  // use std::round to avoid truncation bias
                        }
                        ((int32_t *)(packedbuf))[iv] = v;
                    }
                    m.lock();
                    nc_put_vara(ncout, v_bands[i], startp, countp, (void *)(packedbuf));
                    m.unlock();
                } else if (packing.type == packed_export::packing_type::PACK_FLOAT32) {
                    packedbuf = (uint8_t *)std::malloc(dat->size()[1] * dat->size()[2] * dat->size()[3] * sizeof(float));
                    for (uint32_t iv = 0; iv < dat->size()[1] * dat->size()[2] * dat->size()[3]; ++iv) {
                        double &v = ((double *)(dat->buf()))[i * dat->size()[1] * dat->size()[2] * dat->size()[3] + iv];
                        ((float *)(packedbuf))[iv] = v;
                    }
                    m.lock();
                    nc_put_vara(ncout, v_bands[i], startp, countp, (void *)(packedbuf));
                    m.unlock();
                }
                if (packedbuf) std::free(packedbuf);
            } else {
                m.lock();
                nc_put_vara(ncout, v_bands[i], startp, countp, (void *)(((double *)dat->buf()) + (int)i * (int)csize[1] * (int)csize[2] * (int)csize[3]));
                m.unlock();
            }
        }
        prg->increment((double)1 / (double)this->count_chunks());
    };

    p->apply(shared_from_this(), f);
    nc_close(ncout);
    prg->finalize();

    // netCDF is now written, write additional per-time-slice VRT datasets if needed

    if (with_VRT) {
        for (uint32_t it = 0; it < size_t(); ++it) {
            std::string dir = filesystem::directory(path);
            std::string outfile = dir.empty() ? filesystem::stem(path) + +"_" + st_reference()->datetime_at_index(it).to_string() + ".vrt" : filesystem::join(dir, filesystem::stem(path) + "_" + st_reference()->datetime_at_index(it).to_string() + ".vrt");

            std::ofstream fout(outfile);
            fout << "<VRTDataset rasterXSize=\"" << size_x() << "\" rasterYSize=\"" << size_y() << "\">" << std::endl;
            fout << "<SRS>" << st_reference()->srs() << "</SRS>" << std::endl;  // TODO: if SRS is WKT, it must be escaped with ampersand sequences
            fout << "<GeoTransform>" << utils::dbl_to_string(st_reference()->left()) << ", " << utils::dbl_to_string(stref->dx()) << ", "
                 << "0.0"
                 << ", " << utils::dbl_to_string(stref->top()) << ", "
                 << "0.0"
                 << ", "
                 << "-" << utils::dbl_to_string(stref->dy()) << "</GeoTransform>" << std::endl;

            for (uint16_t ib = 0; ib < size_bands(); ++ib) {
                fout << "<VRTRasterBand dataType=\"Float64\" band=\"" << ib + 1 << "\">" << std::endl;
                fout << "<SimpleSource>" << std::endl;

                fout << "<NoDataValue>" << std::stod(_bands.get(ib).no_data_value) << "</NoDataValue>" << std::endl;
                fout << "<UnitType>" << _bands.get(ib).unit << "</UnitType>" << std::endl;
                fout << "<Offset>" << _bands.get(ib).offset << "</Offset>" << std::endl;
                fout << "<Scale>" << _bands.get(ib).scale << "</Scale>" << std::endl;

                std::string in_dataset = "NETCDF:\"" + filesystem::filename(path) + "\":" + _bands.get(ib).name;
                fout << "<SourceFilename relativeToVRT=\"1\">" << in_dataset << "</SourceFilename>" << std::endl;
                fout << "<SourceBand>" << it + 1 << "</SourceBand>" << std::endl;
                fout << "<SrcRect xOff=\"0\" yOff=\"0\" xSize=\"" << size_x() << "\" ySize=\"" << size_y() << "\"/>" << std::endl;
                fout << "<DstRect xOff=\"0\" yOff=\"0\" xSize=\"" << size_x() << "\" ySize=\"" << size_y() << "\"/>" << std::endl;
                fout << "</SimpleSource>" << std::endl;
                fout << "</VRTRasterBand>" << std::endl;
            }
            fout << "</VRTDataset>" << std::endl;
            fout.close();
        }
    }
}

void cube::write_single_chunk_netcdf(gdalcubes::chunkid_t id, std::string path, uint8_t compression_level) {
    std::string fname = path;  // TODO: check for existence etc.

    if (!_st_ref->has_regular_space()) {
        throw std::string("ERROR: netCDF export does not supported irregular spatial dimensions");
    }

    // NOTE: the following will only work as long as all cube st reference types with regular spatial dimensions inherit from  cube_stref_regular class
    std::shared_ptr<cube_stref_regular> stref = std::dynamic_pointer_cast<cube_stref_regular>(_st_ref);

    std::shared_ptr<chunk_data> dat = this->read_chunk(id);

    double *dim_x = (double *)std::calloc(dat->size()[3], sizeof(double));
    double *dim_y = (double *)std::calloc(dat->size()[2], sizeof(double));
    int *dim_t = (int *)std::calloc(dat->size()[1], sizeof(int));

    if (stref->dt().dt_unit == datetime_unit::WEEK) {
        stref->dt_unit(datetime_unit::DAY);
        stref->dt_interval(stref->dt_interval() * 7);  // UDUNIT does not support week
    }
    bounds_st bbox = this->bounds_from_chunk(id);

    if (stref->has_regular_time()) {
        for (uint32_t i = 0; i < size_t(); ++i) {
            dim_t[i] = (i * stref->dt().dt_interval);
        }
    } else {
        for (uint32_t i = 0; i < size_t(); ++i) {
            dim_t[i] = (stref->datetime_at_index(i) - stref->t0()).dt_interval;
        }
    }
    for (uint32_t i = 0; i < dat->size()[2]; ++i) {
        dim_y[i] = bbox.s.top - (i + 0.5) * stref->dy();
        //dim_y[i] = st_reference()->win().bottom + size_y() * st_reference()->dy() - (i + 0.5) * st_reference()->dy();  // cell center
    }
    for (uint32_t i = 0; i < dat->size()[3]; ++i) {
        dim_x[i] = bbox.s.left + (i + 0.5) * stref->dx();
        //dim_x[i] = st_reference()->win().left + (i + 0.5) * st_reference()->dx();
    }

    OGRSpatialReference srs = st_reference()->srs_ogr();
    std::string yname = srs.IsProjected() ? "y" : "latitude";
    std::string xname = srs.IsProjected() ? "x" : "longitude";

    int ncout;

#if USE_NCDF4 == 1
    nc_create(fname.c_str(), NC_NETCDF4, &ncout);
#else
    nc_create(fname.c_str(), NC_CLASSIC_MODEL, &ncout);
#endif

    int d_t, d_y, d_x;
    nc_def_dim(ncout, "time", dat->size()[1], &d_t);
    nc_def_dim(ncout, yname.c_str(), dat->size()[2], &d_y);
    nc_def_dim(ncout, xname.c_str(), dat->size()[3], &d_x);

    int v_t, v_y, v_x;
    nc_def_var(ncout, "time", NC_INT, 1, &d_t, &v_t);
    nc_def_var(ncout, yname.c_str(), NC_DOUBLE, 1, &d_y, &v_y);
    nc_def_var(ncout, xname.c_str(), NC_DOUBLE, 1, &d_x, &v_x);

    std::string att_source = "gdalcubes " + std::to_string(GDALCUBES_VERSION_MAJOR) + "." + std::to_string(GDALCUBES_VERSION_MINOR) + "." + std::to_string(GDALCUBES_VERSION_PATCH);
    nc_put_att_text(ncout, NC_GLOBAL, "Conventions", strlen("CF-1.6"), "CF-1.6");
    nc_put_att_text(ncout, NC_GLOBAL, "source", strlen(att_source.c_str()), att_source.c_str());


    std::string att_t =  stref->has_regular_time() ? "regular" : "labeled";
    nc_put_att_text(ncout, NC_GLOBAL, "gdalcubes_datetime_type", strlen(att_t.c_str()), att_t.c_str());
    att_t =  stref->t0().to_string();
    nc_put_att_text(ncout, NC_GLOBAL, "gdalcubes_datetime_t0", strlen(att_t.c_str()), att_t.c_str());
    att_t =  stref->t1().to_string();
    nc_put_att_text(ncout, NC_GLOBAL, "gdalcubes_datetime_t1", strlen(att_t.c_str()), att_t.c_str());
    att_t =  stref->dt().to_string();
    nc_put_att_text(ncout, NC_GLOBAL, "gdalcubes_datetime_dt", strlen(att_t.c_str()), att_t.c_str());


    // write json graph as metadata
    std::string j = make_constructible_json().dump();
    nc_put_att_text(ncout, NC_GLOBAL, "process_graph", j.length(), j.c_str());

    char *wkt;
    srs.exportToWkt(&wkt);

    //double geoloc_array[6] = {bbox.s.left, stref->dx(), 0.0, bbox.s.top, 0.0, -stref->dy()};
    std::string geoloc_array_str = utils::dbl_to_string(bbox.s.left) + " " + utils::dbl_to_string(stref->dx()) + " 0 " + utils::dbl_to_string(bbox.s.top) + " 0 " + utils::dbl_to_string(-stref->dy());

    //nc_put_att_text(ncout, NC_GLOBAL, "spatial_ref", strlen(wkt), wkt);
    //nc_put_att_double(ncout, NC_GLOBAL, "GeoTransform", NC_DOUBLE, 6, geoloc_array);

    std::string dtunit_str;
    if (stref->dt().dt_unit == datetime_unit::YEAR) {
        dtunit_str = "years";  // WARNING: UDUNITS defines a year as 365.2425 days
    } else if (stref->dt().dt_unit == datetime_unit::MONTH) {
        dtunit_str = "months";  // WARNING: UDUNITS defines a month as 1/12 year
    } else if (stref->dt().dt_unit == datetime_unit::DAY) {
        dtunit_str = "days";
    } else if (stref->dt().dt_unit == datetime_unit::HOUR) {
        dtunit_str = "hours";
    } else if (stref->dt().dt_unit == datetime_unit::MINUTE) {
        dtunit_str = "minutes";
    } else if (stref->dt().dt_unit == datetime_unit::SECOND) {
        dtunit_str = "seconds";
    }
    dtunit_str += " since ";
    dtunit_str += bbox.t0.to_string(datetime_unit::SECOND);

    nc_put_att_text(ncout, v_t, "standard_name", strlen("time"), "time");
    nc_put_att_text(ncout, v_t, "long_name", strlen("time"), "time");
    nc_put_att_text(ncout, v_t, "units", strlen(dtunit_str.c_str()), dtunit_str.c_str());
    nc_put_att_text(ncout, v_t, "calendar", strlen("gregorian"), "gregorian");
    nc_put_att_text(ncout, v_t, "axis", strlen("T"), "T");  // this avoids GDAL warnings

    if (srs.IsProjected()) {
        // GetLinearUnits(char **) is deprecated since GDAL 2.3.0
#if GDAL_VERSION_MAJOR > 2 || (GDAL_VERSION_MAJOR == 2 && GDAL_VERSION_MINOR >= 3)
        const char *unit = nullptr;
#else
        char *unit = nullptr;
#endif
        srs.GetLinearUnits(&unit);

        nc_put_att_text(ncout, v_y, "standard_name", strlen("projection_y_coordinate"), "projection_y_coordinate");
        nc_put_att_text(ncout, v_y, "long_name", strlen("y coordinate of projection"), "y coordinate of projection");
        nc_put_att_text(ncout, v_y, "units", strlen(unit), unit);
        nc_put_att_text(ncout, v_y, "axis", strlen("Y"), "Y");
        nc_put_att_text(ncout, v_x, "standard_name", strlen("projection_x_coordinate"), "projection_x_coordinate");
        nc_put_att_text(ncout, v_x, "long_name", strlen("x coordinate of projection"), "x coordinate of projection");
        nc_put_att_text(ncout, v_x, "units", strlen(unit), unit);
        nc_put_att_text(ncout, v_x, "axis", strlen("X"), "X");

        int v_crs;
        nc_def_var(ncout, "crs", NC_CHAR, 0, NULL, &v_crs);
        //nc_put_att_text(ncout, v_crs, "grid_mapping_name", strlen("easting_northing"), "easting_northing");
        nc_put_att_text(ncout, v_crs, "spatial_ref", strlen(wkt), wkt);
        //nc_put_att_double(ncout, v_crs, "GeoTransform", NC_DOUBLE, 6, geoloc_array);
        nc_put_att_text(ncout, v_crs, "GeoTransform", strlen(geoloc_array_str.c_str()), geoloc_array_str.c_str());

    } else {
        // char* unit;
        // double scale = srs.GetAngularUnits(&unit);
        nc_put_att_text(ncout, v_y, "units", strlen("degrees_north"), "degrees_north");
        nc_put_att_text(ncout, v_y, "long_name", strlen("latitude"), "latitude");
        nc_put_att_text(ncout, v_y, "standard_name", strlen("latitude"), "latitude");
        nc_put_att_text(ncout, v_y, "axis", strlen("Y"), "Y");

        nc_put_att_text(ncout, v_x, "units", strlen("degrees_east"), "degrees_east");
        nc_put_att_text(ncout, v_x, "long_name", strlen("longitude"), "longitude");
        nc_put_att_text(ncout, v_x, "standard_name", strlen("longitude"), "longitude");
        nc_put_att_text(ncout, v_x, "axis", strlen("X"), "X");

        int v_crs;
        //nc_put_att_text(ncout, v_crs, "grid_mapping_name", strlen("latitude_longitude"), "latitude_longitude");
        //nc_put_att_text(ncout, v_crs, "crs_wkt", strlen(wkt), wkt);
        nc_def_var(ncout, "crs", NC_CHAR, 0, NULL, &v_crs);
        //nc_put_att_text(ncout, v_crs, "grid_mapping_name", strlen("easting_northing"), "easting_northing");
        nc_put_att_text(ncout, v_crs, "spatial_ref", strlen(wkt), wkt);
        //nc_put_att_double(ncout, v_crs, "GeoTransform", NC_DOUBLE, 6, geoloc_array);
        nc_put_att_text(ncout, v_crs, "GeoTransform", strlen(geoloc_array_str.c_str()), geoloc_array_str.c_str());
    }
    CPLFree(wkt);
    int d_all[] = {d_t, d_y, d_x};

    std::vector<int> v_bands;

    for (uint16_t i = 0; i < bands().count(); ++i) {
        int v;
        nc_def_var(ncout, bands().get(i).name.c_str(), NC_DOUBLE, 3, d_all, &v);
        std::size_t csize[3] = {_chunk_size[0], _chunk_size[1], _chunk_size[2]};
#if USE_NCDF4 == 1
        nc_def_var_chunking(ncout, v, NC_CHUNKED, csize);
#endif
        if (compression_level > 0) {
#if USE_NCDF4 == 1
            nc_def_var_deflate(ncout, v, 1, 1, compression_level);  // TODO: experiment with shuffling
#else
            GCBS_WARN("gdalcubes has been built to write netCDF-3 classic model files, compression will be ignored.");
#endif
        }

        if (!bands().get(i).unit.empty())
            nc_put_att_text(ncout, v, "units", strlen(bands().get(i).unit.c_str()), bands().get(i).unit.c_str());

        double pscale = bands().get(i).scale;
        double poff = bands().get(i).offset;
        double pNAN = NAN;

        nc_put_att_double(ncout, v, "scale_factor", NC_DOUBLE, 1, &pscale);
        nc_put_att_double(ncout, v, "add_offset", NC_DOUBLE, 1, &poff);
        nc_put_att_text(ncout, v, "type", strlen(bands().get(i).type.c_str()), bands().get(i).type.c_str());
        nc_put_att_text(ncout, v, "grid_mapping", strlen("crs"), "crs");

        nc_put_att_double(ncout, v, "_FillValue", NC_DOUBLE, 1, &pNAN);

        v_bands.push_back(v);
    }

    nc_enddef(ncout);  ////////////////////////////////////////////////////

    nc_put_var(ncout, v_t, (void *)dim_t);
    nc_put_var(ncout, v_y, (void *)dim_y);
    nc_put_var(ncout, v_x, (void *)dim_x);

    if (dim_t) std::free(dim_t);
    if (dim_y) std::free(dim_y);
    if (dim_x) std::free(dim_x);

    std::size_t startp[] = {0, 0, 0};
    std::size_t countp[] = {dat->size()[1], dat->size()[2], dat->size()[3]};

    for (uint16_t i = 0; i < bands().count(); ++i) {
        nc_put_vara(ncout, v_bands[i], startp, countp, (void *)(((double *)dat->buf()) + (int)i * (int)dat->size()[1] * (int)dat->size()[2] * (int)dat->size()[3]));
    }
    nc_close(ncout);
}

void cube::write_chunks_netcdf(std::string dir, std::string name, uint8_t compression_level, std::shared_ptr<chunk_processor> p) {
    if (name.empty()) {
        name = utils::generate_unique_filename();
    }

    dir = filesystem::make_absolute(dir);

    if (filesystem::exists(dir)) {
        if (!filesystem::is_directory(dir)) {
            throw std::string("ERROR in cube::write_chunks_netcdf(): output is not a directory");
        }
    } else {
        filesystem::mkdir_recursive(dir);
    }

    std::shared_ptr<progress> prg = config::instance()->get_default_progress_bar()->get();
    prg->set(0);  // explicitly set to zero to show progress bar immediately

    std::function<void(chunkid_t, std::shared_ptr<chunk_data>, std::mutex &)> f = [this, prg, &compression_level, &name, &dir](chunkid_t id, std::shared_ptr<chunk_data> dat, std::mutex &m) {
        std::string fname = filesystem::join(dir, name + "_" + std::to_string(id) + ".nc");

        if (!_st_ref->has_regular_space()) {
            throw std::string("ERROR: netCDF export currently does not support irregular spatial dimensions");
        }

        // NOTE: the following will only work as long as all cube st reference types with regular spatial dimensions inherit from  cube_stref_regular class
        std::shared_ptr<cube_stref_regular> stref = std::dynamic_pointer_cast<cube_stref_regular>(_st_ref);

        double *dim_x = (double *)std::calloc(dat->size()[3], sizeof(double));
        double *dim_y = (double *)std::calloc(dat->size()[2], sizeof(double));
        int *dim_t = (int *)std::calloc(dat->size()[1], sizeof(int));

        if (stref->dt().dt_unit == datetime_unit::WEEK) {
            stref->dt_unit(datetime_unit::DAY);
            stref->dt_interval(stref->dt_interval() * 7);  // UDUNIT does not support week
        }
        bounds_st bbox = this->bounds_from_chunk(id);

        if (stref->has_regular_time()) {
            for (uint32_t i = 0; i < size_t(); ++i) {
                dim_t[i] = (i * stref->dt().dt_interval);
            }
        } else {
            for (uint32_t i = 0; i < size_t(); ++i) {
                dim_t[i] = (stref->datetime_at_index(i) - stref->t0()).dt_interval;
            }
        }
        for (uint32_t i = 0; i < dat->size()[2]; ++i) {
            dim_y[i] = bbox.s.top - (i + 0.5) * stref->dy();
            //dim_y[i] = st_reference()->win().bottom + size_y() * st_reference()->dy() - (i + 0.5) * st_reference()->dy();  // cell center
        }
        for (uint32_t i = 0; i < dat->size()[3]; ++i) {
            dim_x[i] = bbox.s.left + (i + 0.5) * stref->dx();
            //dim_x[i] = st_reference()->win().left + (i + 0.5) * st_reference()->dx();
        }

        OGRSpatialReference srs = st_reference()->srs_ogr();
        std::string yname = srs.IsProjected() ? "y" : "latitude";
        std::string xname = srs.IsProjected() ? "x" : "longitude";

        int ncout;

#if USE_NCDF4 == 1
        nc_create(fname.c_str(), NC_NETCDF4, &ncout);
#else
        nc_create(fname.c_str(), NC_CLASSIC_MODEL, &ncout);
#endif

        int d_t, d_y, d_x;
        nc_def_dim(ncout, "time", dat->size()[1], &d_t);
        nc_def_dim(ncout, yname.c_str(), dat->size()[2], &d_y);
        nc_def_dim(ncout, xname.c_str(), dat->size()[3], &d_x);

        int v_t, v_y, v_x;
        nc_def_var(ncout, "time", NC_INT, 1, &d_t, &v_t);
        nc_def_var(ncout, yname.c_str(), NC_DOUBLE, 1, &d_y, &v_y);
        nc_def_var(ncout, xname.c_str(), NC_DOUBLE, 1, &d_x, &v_x);

        std::string att_source = "gdalcubes " + std::to_string(GDALCUBES_VERSION_MAJOR) + "." + std::to_string(GDALCUBES_VERSION_MINOR) + "." + std::to_string(GDALCUBES_VERSION_PATCH);
        nc_put_att_text(ncout, NC_GLOBAL, "Conventions", strlen("CF-1.6"), "CF-1.6");
        nc_put_att_text(ncout, NC_GLOBAL, "source", strlen(att_source.c_str()), att_source.c_str());

        // write json graph as metadata
        std::string j = make_constructible_json().dump();
        nc_put_att_text(ncout, NC_GLOBAL, "process_graph", j.length(), j.c_str());

        char *wkt;
        srs.exportToWkt(&wkt);
        //double geoloc_array[6] = {bbox.s.left, stref->dx(), 0.0, bbox.s.top, 0.0, -stref->dy()};
        std::string geoloc_array_str = utils::dbl_to_string(bbox.s.left) + " " + utils::dbl_to_string(stref->dx()) + " 0 " + utils::dbl_to_string(bbox.s.top) + " 0 " + utils::dbl_to_string(-stref->dy());
        //       nc_put_att_text(ncout, NC_GLOBAL, "spatial_ref", strlen(wkt), wkt);
        //       nc_put_att_double(ncout, NC_GLOBAL, "GeoTransform", NC_DOUBLE, 6, geoloc_array);

        std::string dtunit_str;
        if (stref->dt().dt_unit == datetime_unit::YEAR) {
            dtunit_str = "years";  // WARNING: UDUNITS defines a year as 365.2425 days
        } else if (stref->dt().dt_unit == datetime_unit::MONTH) {
            dtunit_str = "months";  // WARNING: UDUNITS defines a month as 1/12 year
        } else if (stref->dt().dt_unit == datetime_unit::DAY) {
            dtunit_str = "days";
        } else if (stref->dt().dt_unit == datetime_unit::HOUR) {
            dtunit_str = "hours";
        } else if (stref->dt().dt_unit == datetime_unit::MINUTE) {
            dtunit_str = "minutes";
        } else if (stref->dt().dt_unit == datetime_unit::SECOND) {
            dtunit_str = "seconds";
        }
        dtunit_str += " since ";
        dtunit_str += bbox.t0.to_string(datetime_unit::SECOND);

        nc_put_att_text(ncout, v_t, "standard_name", strlen("time"), "time");
        nc_put_att_text(ncout, v_t, "long_name", strlen("time"), "time");
        nc_put_att_text(ncout, v_t, "units", strlen(dtunit_str.c_str()), dtunit_str.c_str());
        nc_put_att_text(ncout, v_t, "calendar", strlen("gregorian"), "gregorian");
        nc_put_att_text(ncout, v_t, "axis", strlen("T"), "T");  // this avoids GDAL warnings

        if (srs.IsProjected()) {
            // GetLinearUnits(char **) is deprecated since GDAL 2.3.0
#if GDAL_VERSION_MAJOR > 2 || (GDAL_VERSION_MAJOR == 2 && GDAL_VERSION_MINOR >= 3)
            const char *unit = nullptr;
#else
            char *unit = nullptr;
#endif
            srs.GetLinearUnits(&unit);

            nc_put_att_text(ncout, v_y, "standard_name", strlen("projection_y_coordinate"), "projection_y_coordinate");
            nc_put_att_text(ncout, v_y, "long_name", strlen("y coordinate of projection"), "y coordinate of projection");
            nc_put_att_text(ncout, v_y, "units", strlen(unit), unit);
            nc_put_att_text(ncout, v_y, "axis", strlen("Y"), "Y");
            nc_put_att_text(ncout, v_x, "standard_name", strlen("projection_x_coordinate"), "projection_x_coordinate");
            nc_put_att_text(ncout, v_x, "long_name", strlen("x coordinate of projection"), "x coordinate of projection");
            nc_put_att_text(ncout, v_x, "units", strlen(unit), unit);
            nc_put_att_text(ncout, v_x, "axis", strlen("X"), "X");

            int v_crs;
            nc_def_var(ncout, "crs", NC_CHAR, 0, NULL, &v_crs);
            //nc_put_att_text(ncout, v_crs, "grid_mapping_name", strlen("easting_northing"), "easting_northing");
            nc_put_att_text(ncout, v_crs, "spatial_ref", strlen(wkt), wkt);
            //nc_put_att_double(ncout, v_crs, "GeoTransform", NC_DOUBLE, 6, geoloc_array);
            nc_put_att_text(ncout, v_crs, "GeoTransform", strlen(geoloc_array_str.c_str()), geoloc_array_str.c_str());

        } else {
            // char* unit;
            // double scale = srs.GetAngularUnits(&unit);
            nc_put_att_text(ncout, v_y, "units", strlen("degrees_north"), "degrees_north");
            nc_put_att_text(ncout, v_y, "long_name", strlen("latitude"), "latitude");
            nc_put_att_text(ncout, v_y, "standard_name", strlen("latitude"), "latitude");
            nc_put_att_text(ncout, v_y, "axis", strlen("Y"), "Y");

            nc_put_att_text(ncout, v_x, "units", strlen("degrees_east"), "degrees_east");
            nc_put_att_text(ncout, v_x, "long_name", strlen("longitude"), "longitude");
            nc_put_att_text(ncout, v_x, "standard_name", strlen("longitude"), "longitude");
            nc_put_att_text(ncout, v_x, "axis", strlen("X"), "X");

            int v_crs;
            //nc_put_att_text(ncout, v_crs, "grid_mapping_name", strlen("latitude_longitude"), "latitude_longitude");
            //nc_put_att_text(ncout, v_crs, "crs_wkt", strlen(wkt), wkt);
            nc_def_var(ncout, "crs", NC_CHAR, 0, NULL, &v_crs);
            //nc_put_att_text(ncout, v_crs, "grid_mapping_name", strlen("easting_northing"), "easting_northing");
            nc_put_att_text(ncout, v_crs, "spatial_ref", strlen(wkt), wkt);
            //nc_put_att_double(ncout, v_crs, "GeoTransform", NC_DOUBLE, 6, geoloc_array);
            nc_put_att_text(ncout, v_crs, "GeoTransform", strlen(geoloc_array_str.c_str()), geoloc_array_str.c_str());
        }
        CPLFree(wkt);
        int d_all[] = {d_t, d_y, d_x};

        std::vector<int> v_bands;

        for (uint16_t i = 0; i < bands().count(); ++i) {
            int v;
            nc_def_var(ncout, bands().get(i).name.c_str(), NC_DOUBLE, 3, d_all, &v);
            std::size_t csize[3] = {_chunk_size[0], _chunk_size[1], _chunk_size[2]};
#if USE_NCDF4 == 1
            nc_def_var_chunking(ncout, v, NC_CHUNKED, csize);
#endif
            if (compression_level > 0) {
#if USE_NCDF4 == 1
                nc_def_var_deflate(ncout, v, 1, 1, compression_level);  // TODO: experiment with shuffling
#else
                GCBS_WARN("gdalcubes has been built to write netCDF-3 classic model files, compression will be ignored.");
#endif
            }

            if (!bands().get(i).unit.empty())
                nc_put_att_text(ncout, v, "units", strlen(bands().get(i).unit.c_str()), bands().get(i).unit.c_str());

            double pscale = bands().get(i).scale;
            double poff = bands().get(i).offset;
            double pNAN = NAN;

            nc_put_att_double(ncout, v, "scale_factor", NC_DOUBLE, 1, &pscale);
            nc_put_att_double(ncout, v, "add_offset", NC_DOUBLE, 1, &poff);
            nc_put_att_text(ncout, v, "type", strlen(bands().get(i).type.c_str()), bands().get(i).type.c_str());
            nc_put_att_text(ncout, v, "grid_mapping", strlen("crs"), "crs");

            nc_put_att_double(ncout, v, "_FillValue", NC_DOUBLE, 1, &pNAN);

            v_bands.push_back(v);
        }

        nc_enddef(ncout);  ////////////////////////////////////////////////////

        nc_put_var(ncout, v_t, (void *)dim_t);
        nc_put_var(ncout, v_y, (void *)dim_y);
        nc_put_var(ncout, v_x, (void *)dim_x);

        if (dim_t) std::free(dim_t);
        if (dim_y) std::free(dim_y);
        if (dim_x) std::free(dim_x);

        std::size_t startp[] = {0, 0, 0};
        std::size_t countp[] = {dat->size()[1], dat->size()[2], dat->size()[3]};

        for (uint16_t i = 0; i < bands().count(); ++i) {
            nc_put_vara(ncout, v_bands[i], startp, countp, (void *)(((double *)dat->buf()) + (int)i * (int)dat->size()[1] * (int)dat->size()[2] * (int)dat->size()[3]));
        }
        nc_close(ncout);
        prg->increment((double)1 / (double)this->count_chunks());
    };

    p->apply(shared_from_this(), f);
    prg->finalize();
}

void chunk_processor_singlethread::apply(std::shared_ptr<cube> c,
                                         std::function<void(chunkid_t, std::shared_ptr<chunk_data>, std::mutex &)> f) {
    std::mutex mutex;
    uint32_t nchunks = c->count_chunks();
    for (uint32_t i = 0; i < nchunks; ++i) {
        std::shared_ptr<chunk_data> dat = c->read_chunk(i);
        f(i, dat, mutex);
    }
}

void chunk_processor_multithread::apply(std::shared_ptr<cube> c,
                                        std::function<void(chunkid_t, std::shared_ptr<chunk_data>, std::mutex &)> f) {
    std::mutex mutex;
    std::vector<std::thread> workers;
    for (uint16_t it = 0; it < _nthreads; ++it) {
        workers.push_back(std::thread([this, &c, f, it, &mutex](void) {
            for (uint32_t i = it; i < c->count_chunks(); i += _nthreads) {
                try {
                    std::shared_ptr<chunk_data> dat = c->read_chunk(i);
                    f(i, dat, mutex);
                } catch (std::string s) {
                    GCBS_ERROR(s);
                    continue;
                } catch (...) {
                    GCBS_ERROR("unexpected exception while processing chunk " + std::to_string(i));
                    continue;
                }
            }
        }));
    }
    for (uint16_t it = 0; it < _nthreads; ++it) {
        workers[it].join();
    }
}

}  // namespace gdalcubes
