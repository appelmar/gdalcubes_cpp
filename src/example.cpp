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

#include <gdal_priv.h>
#include <iostream>
#include "collection_format.h"
#include "image_collection.h"
#include "image_collection_cube.h"
#include "reduce.h"
#include "stream.h"
#include "timer.h"
#include "view.h"

std::vector<std::string> string_list_from_text_file(std::string filename) {
    std::vector<std::string> out;

    std::string line;
    std::ifstream infile(filename);
    while (std::getline(infile, line))
        out.push_back(line);
    return out;
}

int main(int argc, char *argv[]) {
    GDALAllRegister();
    GDALSetCacheMax(1024 * 1024 * 256);            // 256 MiB
    CPLSetConfigOption("GDAL_PAM_ENABLED", "NO");  // avoid aux files for PNG tiles

    srand(time(NULL));

    collection_format fmt("../../test/collection_format_test.json");
    //std::vector<std::string> temp;
    try {
        timer t0;
        image_collection x = image_collection::create(fmt, string_list_from_text_file("../../test/test_list.txt"));
        x.write("test.db");
        std::cout << "DONE (" << t0.time() << "s)" << std::endl;
        std::cout << x.to_string();

        x.temp_copy();
        timer t1;
        x.filter_datetime_range("20170101T000000", "20180101T000000");
        auto b = std::vector<std::string>{"B02", "B03", "B04"};
        x.filter_bands(b);
        x.write("test2.db");
        std::cout << "DONE (" << t1.time() << "s)" << std::endl;
        std::cout << x.to_string();

        cube_view v = cube_view::read_json("../../test/view.json");
        v.write_json("out_view.json");

        image_collection x2("test.db");

        bounds_st box;
        box.t0 = datetime::from_string("20170101");
        box.t1 = datetime::from_string("20180101");
        box.s.left = 22;
        box.s.right = 24;
        box.s.top = -18;
        box.s.bottom = -20;

        //        std::vector<image_collection::find_range_st_row> results = x2.find_range_st(box);
        //        for (uint32_t i=0; i<results.size(); ++i) {
        //            std::cout << results[i].image_name << " " << results[i].datetime << " " << results[i].band_name << " -> " << results[i].descriptor << " " << results[i].band_num << std::endl;
        //        }

        //
        //        std::shared_ptr<cube_st_reference> ref = std::make_shared<cube_view>(cube_view::read_json("../../test/view.json"));
        //        std::shared_ptr<cube_view> vvv = std::dynamic_pointer_cast<cube_view>(ref);
        //        std::cout << vvv->proj() << std::endl;
        //

        image_collection_cube c("test.db", "../../test/view2.json");

        std::cout << std::endl
                  << c.to_string() << std::endl;

        //        t0.start();
        //         c.write_gtiff_directory("test");
        //        std::cout << "DONE (" << t0.time() << "s)" << std::endl;

        //        reduce_cube cr(std::make_shared<image_collection_cube>(c), "median");
        //        t0.start();
        //        cr.write_gdal_image("test.tif");
        //        std::cout << "DONE (" << t0.time() << "s)" << std::endl;

        //stream_cube s(std::make_shared<image_collection_cube>(c), "Rscript --vanilla -e \"require(gdalcubes); summary(read_stream_as_vector()); write_stream_from_vector();\"");

        stream_cube s(std::make_shared<image_collection_cube>(c), "Rscript --vanilla ../../test/stream_example.R");

        reduce_cube cstream(std::make_shared<stream_cube>(s), "min");
        cstream.set_threads(1);
        t0.start();
        //        cstream.write_gdal_image("test_stream.tif");
        //cstream.write_netcdf_directory("testnetcdf");
        //cstream.write_gtiff_directory("testgtif");
        std::cout << "DONE (" << t0.time() << "s)" << std::endl;

        std::cout << cstream.make_constructible_json().dump(2) << std::endl;

    } catch (std::string e) {
        std::cout << e << std::endl;
    }
}