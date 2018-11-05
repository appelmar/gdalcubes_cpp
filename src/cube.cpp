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

#include "cube.h"
#include <thread>

#include <netcdf>
#include "build_info.h"

void cube::write_gtiff_directory(std::string dir, std::shared_ptr<chunk_processor> p) {
    namespace fs = boost::filesystem;
    fs::path op(dir);

    if (!fs::exists(dir)) {
        fs::create_directories(op);
    }

    if (!fs::is_directory(dir)) {
        throw std::string("ERROR in chunking::write_gtiff_directory(): output is not a directory.");
    }

    uint32_t nchunks = count_chunks();
    for (uint32_t i = 0; i < nchunks; ++i) {
        GDALDriver *gtiff_driver = (GDALDriver *)GDALGetDriverByName("GTiff");
        if (gtiff_driver == NULL) {
            throw std::string("ERROR: cannot find GDAL driver for GTiff.");
        }

        CPLStringList out_co;
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
                CPLErr res = gdal_out->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, dat->size()[3], dat->size()[2], ((double *)dat->buf()) + (ib * dat->size()[1] * dat->size()[2] * dat->size()[3] + it * dat->size()[2] * dat->size()[3]), dat->size()[3], dat->size()[2], GDT_Float64, 0, 0, NULL);
                if (res != CE_None) {
                    GCBS_WARN("RasterIO (write) failed for band " + _bands.get(ib).name);
                }
                gdal_out->GetRasterBand(1)->SetNoDataValue(std::stod(_bands.get(ib).no_data_value));
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

/**
 * TODO: Use CF conventions (http://cfconventions.org/Data/cf-conventions/cf-conventions-1.7/cf-conventions.html#_abstract)
 * @param dir
 */
void cube::write_netcdf_directory(std::string dir, std::shared_ptr<chunk_processor> p) {
    namespace fs = boost::filesystem;
    fs::path op(dir);

    if (!fs::exists(dir)) {
        fs::create_directories(op);
    }

    if (!fs::is_directory(dir)) {
        throw std::string("ERROR in chunking::write_netcdf_directory(): output is not a directory.");
    }

    std::shared_ptr<progress> prg = config::instance()->get_default_progress_bar()->get();
    prg->set(0); // explicitly set to zero to show progress bar immediately

    std::function<void(chunkid_t, std::shared_ptr<chunk_data>, std::mutex &)> f = [this, op, prg](chunkid_t id, std::shared_ptr<chunk_data> dat, std::mutex &m) {
        fs::path out_file = op / (std::to_string(id) + ".nc");

        chunk_size_btyx csize = dat->size();
        bounds_nd<uint32_t, 3> climits = chunk_limits(id);

        //        for (uint16_t i = 0; i < bands().count(); ++i) {
        //            int str_size = bands().get(i).name.size();
        //        }

        double *dim_x = (double *)calloc(csize[3], sizeof(double));
        double *dim_y = (double *)calloc(csize[2], sizeof(double));
        int *dim_t = (int *)calloc(csize[2], sizeof(int));

        if (_st_ref->dt().dt_unit == WEEK) {
            _st_ref->dt().dt_unit = DAY;
            _st_ref->dt().dt_interval *= 7;  // UDUNIT does not support week
        }
        for (uint32_t i = 0; i < csize[1]; ++i) {
            dim_t[i] = climits.low[0] + (i * st_reference().dt().dt_interval);
        }
        for (uint32_t i = 0; i < csize[2]; ++i) {
            dim_y[i] = st_reference().win().bottom + climits.high[1] * st_reference().dy() - (i)*st_reference().dy();  // or i +1 ?
        }
        for (uint32_t i = 0; i < csize[3]; ++i) {
            dim_x[i] = st_reference().win().left + (i + climits.low[2]) * st_reference().dx();
        }

        OGRSpatialReference srs = st_reference().proj_ogr();
        std::string yname = srs.IsProjected() ? "y" : "latitude";
        std::string xname = srs.IsProjected() ? "x" : "longitude";

        netCDF::NcFile ncout(out_file.c_str(), netCDF::NcFile::newFile);
        //netCDF::NcDim d_band = ncout.addDim("band", _bands.count());
        netCDF::NcDim d_t = ncout.addDim("time", csize[1]);
        netCDF::NcDim d_y = ncout.addDim(yname.c_str(), csize[2]);
        netCDF::NcDim d_x = ncout.addDim(xname.c_str(), csize[3]);

        netCDF::NcVar v_t = ncout.addVar("time", netCDF::ncInt, d_t);
        netCDF::NcVar v_y = ncout.addVar(yname.c_str(), netCDF::ncDouble, d_y);
        netCDF::NcVar v_x = ncout.addVar(xname.c_str(), netCDF::ncDouble, d_x);

        ncout.putAtt("Conventions", "CF-1.6");
        ncout.putAtt("source", ("gdalcubes " + std::to_string(GDALCUBES_VERSION_MAJOR) + "." + std::to_string(GDALCUBES_VERSION_MINOR) + "." + std::to_string(GDALCUBES_VERSION_PATCH)).c_str());

        v_t.putVar(dim_t);
        v_y.putVar(dim_y);
        v_x.putVar(dim_x);

        std::string dtunit_str;
        if (_st_ref->dt().dt_unit == YEAR) {
            dtunit_str = "years";  // WARNING: UDUNITS defines a year as 365.2425 days
        } else if (_st_ref->dt().dt_unit == MONTH) {
            dtunit_str = "months";  // WARNING: UDUNITS defines a month as 1/12 year
        } else if (_st_ref->dt().dt_unit == DAY) {
            dtunit_str = "days";
        } else if (_st_ref->dt().dt_unit == HOUR) {
            dtunit_str = "hours";
        } else if (_st_ref->dt().dt_unit == MINUTE) {
            dtunit_str = "minutes";
        } else if (_st_ref->dt().dt_unit == SECOND) {
            dtunit_str = "seconds";
        }
        dtunit_str += " since ";
        dtunit_str += _st_ref->t0().to_string(SECOND);

        v_t.putAtt("units", dtunit_str.c_str());
        v_t.putAtt("calendar", "gregorian");
        v_t.putAtt("long_name", "time");
        v_t.putAtt("standard_name", "time");

        if (srs.IsProjected()) {
            char *unit = nullptr;
            srs.GetLinearUnits(&unit);
            v_y.putAtt("units", unit);
            v_x.putAtt("units", unit);

            char *wkt;
            srs.exportToWkt(&wkt);
            netCDF::NcVar v_crs = ncout.addVar("crs", netCDF::ncInt);
            v_crs.putAtt("grid_mapping_name", "easting_northing");
            v_crs.putAtt("crs_wkt", wkt);
        } else {
            // char* unit;
            // double scale = srs.GetAngularUnits(&unit);
            v_y.putAtt("units", "degrees_north");
            v_y.putAtt("long_name", "latitude");
            v_y.putAtt("standard_name", "latitude");
            v_x.putAtt("units", "degrees_east");
            v_x.putAtt("long_name", "longitude");
            v_x.putAtt("standard_name", "longitude");

            char *wkt;
            srs.exportToWkt(&wkt);
            netCDF::NcVar v_crs = ncout.addVar("crs", netCDF::ncInt);
            v_crs.putAtt("grid_mapping_name", "latitude_longitude");
            v_crs.putAtt("crs_wkt", wkt);
        }

        std::vector<netCDF::NcVar> v_bands;
        std::vector<netCDF::NcDim> d_all;

        d_all.push_back(d_t);
        d_all.push_back(d_y);
        d_all.push_back(d_x);

        for (uint16_t i = 0; i < bands().count(); ++i) {
            netCDF::NcVar v = ncout.addVar(bands().get(i).name, netCDF::ncDouble, d_all);
            if (!bands().get(i).unit.empty())
                v.putAtt("units", bands().get(i).unit.c_str());
            v.putAtt("scale_factor", netCDF::ncDouble, bands().get(i).scale);
            v.putAtt("add_offset", netCDF::ncDouble, bands().get(i).offset);
            //v.putAtt("nodata", std::to_string(bands().get(i).no_data_value).c_str());
            v.putAtt("type", bands().get(i).type.c_str());
            v.putAtt("grid_mapping", "crs");
            v.putAtt("_FillValue", netCDF::ncDouble, NAN);
            v_bands.push_back(v);
        }

        for (uint16_t i = 0; i < bands().count(); ++i) {
            v_bands[i].putVar(((double *)dat->buf()) + (int)i * (int)size_t() * (int)size_y() * (int)size_x());
        }
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
                std::shared_ptr<chunk_data> dat = c->read_chunk(i);
                f(i, dat, mutex);
            }
        }));
    }
    for (uint16_t it = 0; it < _nthreads; ++it) {
        workers[it].join();
    }
}

//void cube::apply(std::function<void(chunkid_t, std::shared_ptr<chunk_data>, std::mutex &)> f, uint16_t nthreads) {
//    if (nthreads == 1) {
//        std::mutex mutex;
//        uint32_t nchunks = count_chunks();
//        for (uint32_t i = 0; i < nchunks; ++i) {
//            std::shared_ptr<chunk_data> dat = read_chunk(i);
//            f(i, dat, mutex);
//        }
//    } else {
//        std::mutex mutex;
//        std::vector<std::thread> workers;
//        for (uint16_t it = 0; it < nthreads; ++it) {
//            workers.push_back(std::thread([this, &f, it, nthreads, &mutex](void) {
//                for (uint32_t i = it; i < count_chunks(); i += nthreads) {
//                    std::shared_ptr<chunk_data> dat = read_chunk(i);
//                    f(i, dat, mutex);
//                }
//            }));
//        }
//        for (uint16_t it = 0; it < nthreads; ++it) {
//            workers[it].join();
//        }
//    }
//}