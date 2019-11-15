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

/**
 * THIS FILE IS JUST FOR TESTING WITH SOME LOCAL FILES INCLUDING ABSOLUTE PATHS.
 * IT WILL NOT RUN ON YOUR MACHINE:
 */

#include <iostream>
#include "cube_factory.h"
#include "gdalcubes.h"
#include "image_collection_ops.h"

using namespace gdalcubes;

std::vector<std::string> string_list_from_text_file(std::string filename) {
    std::vector<std::string> out;

    std::string line;
    std::ifstream infile(filename);
    while (std::getline(infile, line))
        out.push_back(line);
    return out;
}

int main(int argc, char* argv[]) {
    config::instance()->gdalcubes_init();
    config::instance()->set_error_handler(error_handler::error_handler_debug);
    config::instance()->set_default_progress_bar(std::make_shared<progress_simple_stdout_with_time>());
    config::instance()->set_default_chunk_processor(std::make_shared<chunk_processor_multithread>(8));

    //    auto prsts = collection_format::list_presets();
    //    for (auto it = prsts.begin(); it != prsts.end(); ++it) {
    //        std::cout << it->first << "    " << it->second << std::endl;
    //    }

    try {
        timer t0;

        /**************************************************************************/
        // Test create image collection
        {
            //            collection_format f("Sentinel2_L2A");
            //            auto ic = image_collection::create(f, image_collection::unroll_archives(string_list_from_text_file("/home/marius/eodata/Sentinel2/file_list.txt")), false);
            //            ic->write("test.db");
            //            std::cout << ic->to_string() << std::endl;
            //            std::dynamic_pointer_cast<cube_view>(image_collection_cube::create(ic)->st_reference())->write_json("view_default.json");
        }
        /**************************************************************************/

        /**************************************************************************/
        // Test addo
        {
            //            auto ic = std::make_shared<image_collection>("test.db");
            //            image_collection_ops::create_overviews(ic);
        }
        /**************************************************************************/

        cube_view v = cube_view::read_json("view.json");

        /**************************************************************************/
        // test fill_time
        //        {
        //            auto c = image_collection_cube::create("test.db", v);
        //            auto cb = select_bands_cube::create(c, std::vector<std::string>{"B04", "B08"});
        //            cb->write_netcdf_file("test_fill_time_in.nc");
        //            auto cf = fill_time_cube::create(cb, "linear");
        //            cf->write_netcdf_file("test_fill_time_linear.nc");
        //        }
        /**************************************************************************/

        /**************************************************************************/
        // test reduction
        //        {
        //            auto c = image_collection_cube::create("test.db", v);
        //            auto cb = select_bands_cube::create(c, std::vector<std::string>{"B04", "B08"});
        //            cb->write_netcdf_file("band_select.nc");
        //            auto cr = reduce_cube::create(cb, "median");
        //            cr->write_gdal_image("test_A.tif");
        //
        //            c = image_collection_cube::create("test.db", v);
        //            cr = reduce_cube::create(c, "max");
        //            cb = select_bands_cube::create(cr, std::vector<std::string>{"B04_max", "B08_max"});
        //            reduce_cube::create(cb, "max")->write_gdal_image("test_B.tif");
        //        }
        /**************************************************************************/

        /**************************************************************************/
        // test packed export
        {
            //            auto c = image_collection_cube::create("test.db", v);

            //            auto cb = select_bands_cube::create(c, std::vector<std::string>{"B04", "B08"});
            //            cb->write_tif_collection("/home/marius/Desktop/test_pack1",
            //                                     "", true, true, std::map<std::string, std::string>(), "NEAREST", packed_export::make_uint8(1, 0));
        }
        /**************************************************************************/

        /**************************************************************************/
        // test masking
        //        {
        //            cube_view w;
        //            w.left() = 300000.000;
        //            w.top() = 5800020.000;
        //            w.bottom() = 5690220.000;
        //            w.right() = 409800.000;
        //            w.srs() = "EPSG:32632";
        //            w.nx() = 500;
        //            w.ny() = 500;
        //            w.dt(duration::from_string("P1D"));
        //            w.t0() = datetime::from_string("2018-06-14");
        //            w.t1() = datetime::from_string("2018-06-14");
        //
        //            auto c = image_collection_cube::create("test.db", w);
        //            std::shared_ptr<image_mask> mask = std::make_shared<value_mask>(std::unordered_set<double>{8, 9});
        //            c->set_mask("SCL", mask);
        //            auto cb = select_bands_cube::create(c, std::vector<std::string>{"SCL", "B08"});
        //
        //            cb->write_netcdf_file("mask.nc");
        //        }

        v = cube_view::read_json("view.json");

        /**************************************************************************/

        /**************************************************************************/
        // test filter predicate over time
        //        {
        //            auto c = image_collection_cube::create("test.db", v);
        //            auto cb = select_bands_cube::create(c, std::vector<std::string>{"B04", "B08"});
        //            auto cf = filter_pixel_cube::create(cb, "B04 < 1000");
        //            auto cr = reduce_time_cube::create(cf, {{"max", "B04"}, {"count", "B04"}});
        //            cr->write_gdal_image("test_filter_predicate_2.tif");
        //        }
        /**************************************************************************/

        //        /**************************************************************************/
        //        // test reduction over space
        //        {
        //            auto c = image_collection_cube::create("test.db", v);
        //            auto cb = select_bands_cube::create(c, std::vector<std::string>{"B04", "B08"});
        //            auto cr = reduce_space_cube::create(cb, {{"count", "B04"}, {"mean", "B04"}});
        //            //auto cr = reduce_time_cube::create(cb, {{"min", "B04"}, {"max", "B04"}, {"median", "B04"}, {"var", "B04"}, {"which_min", "B04"}, {"which_max", "B04"}});
        //            cr->write_netcdf_file("test_reduce_space.nc");
        //        }
        //        /**************************************************************************/

        //        /**************************************************************************/
        //        // test window time over space
        //        {
        //            auto c = image_collection_cube::create("test.db", v);
        //            c->st_reference()->nt(5);
        //            auto cb = select_bands_cube::create(c, std::vector<std::string>{"B04"});
        //            auto cw = window_time_cube::create(cb, {{"mean", "B04"}}, 1, 1);
        //            //   cw->write_netcdf_file("test_window_time_reduce.nc");
        //
        //            auto cw2 = window_time_cube::create(cb, {-1.0, 2, -1.0}, 1, 1);
        //            cw2->write_netcdf_file("test_window_time_kernel.nc");
        //        }
        //      /**************************************************************************/

        /**************************************************************************/
        // Test apply_pixel
        //                {
        //                                auto c = image_collection_cube::create("test.db", v);
        //                                //auto capply_err = apply_pixel_cube::create(select_bands_cube::create(c, std::vector<std::string>({"B04", "B08"})), {"(B08 - B04)/(B08 + B04 -c Bsss)"});
        //                                //auto capply = apply_pixel_cube::create(select_bands_cube::create(c, std::vector<std::string>({"B04", "B08"})), {"(B08 - B04)/(B08 + B04)"});
        //                                //auto capply = apply_pixel_cube::create(select_bands_cube::create(c, std::vector<std::string>({"B02", "B03", "B04"})), {"sqrt((B02+B03+B04)^2)"});
        //                                // auto capply = apply_pixel_cube::create(select_bands_cube::create(c, std::vector<std::string>({"B02", "B03", "B04"})), {"B02/B03"});
        //
        //                                auto capply = apply_pixel_cube::create(c, {"(B08 - B04)/(B08 + B04)"});
        //
        //                                auto cr = reduce_cube::create(capply, "median");
        //                                // cr->write_gdal_image("test_apply_reduce.tif");
        //                                cr->write_netcdf_file("test_apply_reduce.nc");
        //                }

        // Test apply_pixel
        //        {
        //            auto c = dummy_cube::create(v, 1, 1.0);
        //            auto capply = apply_pixel_cube::create(c, {"day(t0)"});
        //            auto cr = reduce_cube::create(capply, "median");
        //            cr->write_netcdf_file("test_apply_reduce.nc");
        //        }

        // Test query_points
        //        {
        //            auto c = image_collection_cube::create("test.db", v);
        //            //c->set_chunk_size(1000, 1000, 1000);  // single chunk
        //            auto cb = select_bands_cube::create(c, std::vector<std::string>{"B04"});
        //
        //            auto ca = apply_pixel_cube::create(cb, {"left", "top"}, {"x", "y"}, true);
        //            //cb->write_netcdf_file("cube.nc");
        //            auto cr = reduce_time_cube::create(ca, {{"median", "B04"}, {"median", "x"}, {"median", "y"}});
        //            cr->write_netcdf_file("cube_red_xy_1.nc");
        //
        //            std::vector<double> ux = {653469.0, 693953.1, 734437.2, 774921.3, 815405.4, 855889.6, 896373.7, 936857.8, 977341.9, 1017826.0};
        //            std::vector<double> uy = {6670781, 6692220, 6713658, 6735097, 6756536, 6777974, 6799413, 6820852, 6842290, 6863729};
        //            std::vector<std::string> ut = {"2018-06-04", "2018-06-12", "2018-06-20", "2018-06-30", "2018-07-10"};
        //
        //            std::vector<double> x;
        //            std::vector<double> y;
        //            std::vector<std::string> t;
        //
        //            for (uint16_t ix = 0; ix < ux.size(); ++ix) {
        //                for (uint16_t iy = 0; iy < uy.size(); ++iy) {
        //                    for (uint16_t it = 0; it < ut.size(); ++it) {
        //                        x.push_back(ux[ix]);
        //                        y.push_back(uy[iy]);
        //                        t.push_back(ut[it]);
        //                    }
        //                }
        //            }
        //            std::string srs = "EPSG:3857";
        //
        //            auto res = vector_queries::query_points(cr, x, y, t, srs);
        //
        //            //std::cout << std::fixed << std::setprecision(std::numeric_limits<double>::digits10 + 1);
        //            for (uint32_t ip = 0; ip < x.size(); ++ip) {
        //                std::cout << ip + 1 << "\t";
        //                std::cout << x[ip] << "\t";
        //                std::cout << y[ip] << "\t";
        //                std::cout << t[ip] << "\t";
        //                for (uint32_t ib = 0; ib < res.size(); ++ib) {
        //                    std::cout << res[ib][ip] << "\t";
        //                }
        //                std::cout << x[ip] - res[1][ip] << "\t";
        //                std::cout << y[ip] - res[2][ip] << "\t";
        //                std::cout << std::endl;
        //            }
        //        }

        // test zonal statistics
        {
            cube_view w;
            w.left() = -180;
            w.top() = 50;
            w.bottom() = -50;
            w.right() = 180;
            w.srs() = "EPSG:4326";
            w.dx(0.2);
            w.dy(0.2);
            w.t0() = datetime::from_string("2019-01-01");
            w.t1() = datetime::from_string("2019-01-01");
            w.dt(duration::from_string("P1D"));
            w.resampling_method() = resampling::resampling_type::RSMPL_AVERAGE;

            //            auto ch = _helper_cube::create(w);
            //            ch->set_chunk_size(1,100,100);
            //            ch->write_netcdf_file("/home/marius/Desktop/gdalcubes_model.nc");

            auto c = dummy_cube::create(w, 1, 1.0);

            //vector_queries::zonal_statistics(c, "/home/marius/sciebo/global_grid_5deg.gpkg", {{"count", "band1"}}, "/tmp/zonal_stats", true);

            auto c1 = dummy_cube::create(w, 1, 1.0);
            auto c2 = apply_pixel_cube::create(c1, {"left", "top"}, {"left", "top"}, false);

            //vector_queries::zonal_statistics(c2,"/home/marius/sciebo/test_features.gpkg",{{"min","left"},{"max","left"},{"mean","left"},{"min","top"},{"max","top"},{"mean","top"}}, "/tmp/zonal_stats_coords_");

            // Real world data (CHIRPS)
            collection_format f("/home/marius/github/collection_formats/formats/CHIRPS_v2_0_daily_p05_tif.json");
            std::vector<std::string> files;
            filesystem::iterate_directory("/home/marius/eodata/CHIRPS/", [&files](const std::string& s) {
                if (s.find(".tif") != s.npos) {
                    files.push_back(s);
                }
            });
            auto ic = image_collection::create(f, files, false);
            //ic->write("CHIRPS.db");

            w.t0() = datetime::from_string("2018-01-01");
            w.t1() = datetime::from_string("2018-01-04");

            auto chirps_cube = image_collection_cube::create("CHIRPS.db", w);
            chirps_cube->set_chunk_size(10, 256, 256);

            // chirps_cube->write_netcdf_file("/home/marius/sciebo/chirps.nc");

            //vector_queries::zonal_statistics(chirps_cube, "/home/marius/sciebo/world_polygons.gpkg", {{"mean", "precipitation"}}, "/tmp/zonal_stats_chirps2.gpkg", true);

            std::ifstream i("/tmp/cube.json");
            nlohmann::json j;
            i >> j;
            auto cube = cube_factory::instance()->create_from_json(j);

            vector_queries::zonal_statistics(cube, "/home/marius/sciebo/ms_flurstuecke_filtered_larger_1ha.gpkg", {{"mean", "NDVI_median"}}, "/tmp/zonal_stats_NDVI.gpkg", true);
        }

        /**************************************************************************/
        // test streaming
        //        {
        //            // test streaming
        //            auto c = image_collection_cube::create("test.db", v);
        //            auto sc = stream_cube::create(c, "Rscript --vanilla stream_example.R", "stdout");
        //            auto cr = reduce_cube::create(sc, "median");
        //            cr->write_gdal_image("test_stream.tif");
        //        }

        //
        //        /******************************************/
        //        // Test CHIRPS dataset and update_view()
        //        {
        //            chdir("/home/marius/Desktop/CHIRPS/");
        //            config::instance()->set_default_chunk_processor(std::make_shared<chunk_processor_multithread>(1));
        //            auto x = image_collection_cube::create("/home/marius/Desktop/CHIRPS/CHIRPS.db", "/home/marius/Desktop/CHIRPS/view_debug.json");
        //            std::cout << x->view()->write_json_string() << std::endl;
        //            auto xmax = reduce_cube::create(x, "max");
        //            std::shared_ptr<cube_st_reference> vv = x->st_reference();
        //            vv->nt(1);
        //            vv->nx() = 100;
        //            vv->ny() = 100;
        //            xmax->update_st_reference(vv);
        //            xmax->write_gdal_image("test_max.tif");
        //        }
        //        /******************************************/

        //
        //        /******************************************/
        // Test NetCDF export
        {
            //            chdir("/home/marius/Desktop/MODIS/MOD13A3.A2018");
            //            auto cc = image_collection_cube::create("MOD13A3.db");
            //            cc->view()->aggregation_method() = aggregation::aggregation_type::AGG_MEDIAN;
            //            cc->write_netcdf_file("full.nc");
        }

        /******************************************/

    } catch (std::string e) {
        std::cout << e << std::endl;
    }

    config::instance()->gdalcubes_cleanup();
}