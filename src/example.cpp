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

/**
 * THIS FILE IS JUST FOR TESTING WITH SOME LOCAL FILES INCLUDING ABSOLUTE PATHS.
 * IT WILL NOT RUN ON YOUR MACHINE:
 */

#include <gdal_priv.h>
#include <iostream>
#include "apply_pixel.h"
#include "collection_format.h"
#include "cube_factory.h"
#include "error.h"
#include "filter_predicate.h"
#include "image_collection.h"
#include "image_collection_cube.h"
#include "reduce.h"
#include "reduce_space.h"
#include "reduce_time.h"
#include "select_bands.h"
#include "stream.h"
#include "window_time.h"

std::vector<std::string> string_list_from_text_file(std::string filename) {
    std::vector<std::string> out;

    std::string line;
    std::ifstream infile(filename);
    while (std::getline(infile, line))
        out.push_back(line);
    return out;
}

int main(int argc, char *argv[]) {
    config::instance()->gdalcubes_init();
    config::instance()->set_error_handler(error_handler::error_handler_debug);
    config::instance()->set_default_progress_bar(std::make_shared<progress_simple_stdout_with_time>());

    datetime ttt = datetime::from_string("2018-11-08T09:32:09");
    std::cout << ttt.to_double() << std::endl;

    auto prsts = collection_format::list_presets();
    for (auto it = prsts.begin(); it != prsts.end(); ++it) {
        std::cout << it->first << "    " << it->second << std::endl;
    }

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

        cube_view v = cube_view::read_json("view.json");

        /**************************************************************************/
        // test reduction
        {
            auto c = image_collection_cube::create("test.db", v);
            auto cb = select_bands_cube::create(c, std::vector<std::string>{"B04", "B08"});
            cb->write_netcdf_file("band_select.nc");
            auto cr = reduce_cube::create(cb, "median");
            cr->write_gdal_image("test_A.tif");

            c = image_collection_cube::create("test.db", v);
            cr = reduce_cube::create(c, "max");
            cb = select_bands_cube::create(cr, std::vector<std::string>{"B04_max", "B08_max"});
            reduce_cube::create(cb, "max")->write_gdal_image("test_B.tif");
        }
        /**************************************************************************/

        /**************************************************************************/
        // test filter predicate over time
        {
            auto c = image_collection_cube::create("test.db", v);
            auto cb = select_bands_cube::create(c, std::vector<std::string>{"B04", "B08"});
            auto cf = filter_predicate_cube::create(cb, "B04 < 1000");
            auto cr = reduce_time_cube::create(cf, {{"max", "B04"}, {"count", "B04"}});
            cr->write_gdal_image("test_filter_predicate_2.tif");
        }
        /**************************************************************************/

        //        /**************************************************************************/
        //        // test reduction over time
        {
            auto c = image_collection_cube::create("test.db", v);
            auto cb = select_bands_cube::create(c, std::vector<std::string>{"B04", "B08"});
            // auto cr = reduce_time_cube::create(cb, {{"var", "B04"}});
            auto cr = reduce_time_cube::create(cb, {{"min", "B04"}, {"max", "B04"}, {"median", "B04"}, {"count", "B04"}});
            cr->write_gdal_image("test_reduce_new.tif");
        }
        //        /**************************************************************************/

        //        /**************************************************************************/
        //        // test reduction over space
        {
            auto c = image_collection_cube::create("test.db", v);
            auto cb = select_bands_cube::create(c, std::vector<std::string>{"B04", "B08"});
            auto cr = reduce_space_cube::create(cb, {{"count", "B04"}, {"mean", "B04"}});
            //auto cr = reduce_time_cube::create(cb, {{"min", "B04"}, {"max", "B04"}, {"median", "B04"}, {"var", "B04"}, {"which_min", "B04"}, {"which_max", "B04"}});
            cr->write_netcdf_file("test_reduce_space.nc");
        }
        //        /**************************************************************************/

        //        /**************************************************************************/
        //        // test window time over space
        {
            auto c = image_collection_cube::create("test.db", v);
            auto cb = select_bands_cube::create(c, std::vector<std::string>{"B04"});
            auto cw = window_time_cube::create(cb, {{"mean", "B04"}}, 1, 1);
            cw->write_netcdf_file("test_window_time.nc");
        }
        //      /**************************************************************************/

        /**************************************************************************/
        // Test apply_pixel
        {
            auto c = image_collection_cube::create("test.db", v);
            //auto capply_err = apply_pixel_cube::create(select_bands_cube::create(c, std::vector<std::string>({"B04", "B08"})), {"(B08 - B04)/(B08 + B04 -c Bsss)"});
            //auto capply = apply_pixel_cube::create(select_bands_cube::create(c, std::vector<std::string>({"B04", "B08"})), {"(B08 - B04)/(B08 + B04)"});
            //auto capply = apply_pixel_cube::create(select_bands_cube::create(c, std::vector<std::string>({"B02", "B03", "B04"})), {"sqrt((B02+B03+B04)^2)"});
            // auto capply = apply_pixel_cube::create(select_bands_cube::create(c, std::vector<std::string>({"B02", "B03", "B04"})), {"B02/B03"});

            auto capply = apply_pixel_cube::create(c, {"(B08 - B04)/(B08 + B04)"});

            auto cr = reduce_cube::create(capply, "median");
            // cr->write_gdal_image("test_apply_reduce.tif");
            cr->write_netcdf_file("test_apply_reduce.nc");
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