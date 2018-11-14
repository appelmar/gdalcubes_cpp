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
#include "error.h"
#include "image_collection.h"
#include "image_collection_cube.h"
#include "reduce.h"
#include "stream.h"
#include "swarm.h"
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
    config::instance()->gdalcubes_init();

    config::instance()->set_error_handler(error_handler::error_handler_debug);

    collection_format fmt("../../test/collection_format_test.json");
    //std::vector<std::string> temp;
    try {
        timer t0;
        //        image_collection x = image_collection::create(fmt, string_list_from_text_file("../../test/test_list.txt"));
        //        x.write("test.db");
        //        std::cout << "DONE (" << t0.time() << "s)" << std::endl;
        //        std::cout << x.to_string();
        //
        //        x.temp_copy();
        //        timer t1;
        //        x.filter_datetime_range("20170101T000000", "20180101T000000");
        //        auto b = std::vector<std::string>{"B02", "B03", "B04"};
        //        x.filter_bands(b);
        //        x.write("test2.db");
        //        std::cout << "DONE (" << t1.time() << "s)" << std::endl;
        //        std::cout << x.to_string();
        //
        //        cube_view v = cube_view::read_json("../../test/view.json");
        //        v.write_json("out_view.json");
        //
        //        image_collection x2("test.db");
        //
        //        bounds_st box;
        //        box.t0 = datetime::from_string("20170101");
        //        box.t1 = datetime::from_string("20180101");
        //        box.s.left = 22;
        //        box.s.right = 24;
        //        box.s.top = -18;
        //        box.s.bottom = -20;
        //
        //        //        std::vector<image_collection::find_range_st_row> results = x2.find_range_st(box);
        //        //        for (uint32_t i=0; i<results.size(); ++i) {
        //        //            std::cout << results[i].image_name << " " << results[i].datetime << " " << results[i].band_name << " -> " << results[i].descriptor << " " << results[i].band_num << std::endl;
        //        //        }
        //
        //        //
        //        //        std::shared_ptr<cube_st_reference> ref = std::make_shared<cube_view>(cube_view::read_json("../../test/view.json"));
        //        //        std::shared_ptr<cube_view> vvv = std::dynamic_pointer_cast<cube_view>(ref);
        //        //        std::cout << vvv->proj() << std::endl;
        //        //

        cube_view v = cube_view::read_json("../../test/view2.json");
        v.proj() = "EPSG:3857";
        v.win() = v.win().transform("EPSG:4326", "EPSG:3857");
        std::cout << v.write_json_string() << std::endl;

        //   image_collection_cube c("test.db", v);

        //image_collection_cube c("test.db");

        //std::cout << image_collection_cube::default_view(c.collection()).write_json_string() << std::endl;

        //std::shared_ptr<reduce_cube> cr = std::make_shared<reduce_cube>(std::make_shared<image_collection_cube>(c), "max");
        // t0.start();
        // cr->write_gdal_image("test.tif");
        //std::cout << "DONE (" << t0.time() << "s)" << std::endl;

        //stream_cube s(std::make_shared<image_collection_cube>(c), "Rscript --vanilla -e \"require(gdalcubes); summary(read_stream_as_vector()); write_stream_from_vector();\"");

        //        std::shared_ptr<stream_cube> s = std::make_shared<stream_cube>(std::make_shared<image_collection_cube>(c), "Rscript --vanilla stream_example.R", "");
        //
        //        std::shared_ptr<reduce_cube> cstream = std::make_shared<reduce_cube>(s, "min");
        //        t0.start();
        //        cstream->write_gdal_image("test_stream.tif");
        //        std::cout << "DONE (" << t0.time() << "s)" << std::endl;

        //  std::cout << cstream.make_constructible_json().dump(2) << std::endl;

        //        std::vector<std::string> servers;
        //        servers.push_back("http://localhost:1112/gdalcubes/api");
        //        std::shared_ptr<gdalcubes_swarm> swarm = std::make_shared<gdalcubes_swarm>(servers);
        //        config::instance()->set_default_chunk_processor(swarm);
        //        cstream->write_gdal_image("test_swarm.tif");

        chdir("/home/marius/Desktop/CHIRPS/");
        config::instance()->set_default_chunk_processor(std::make_shared<chunk_processor_multithread>(1));
        std::shared_ptr<image_collection_cube> x = std::make_shared<image_collection_cube>("/home/marius/Desktop/CHIRPS/CHIRPS.db", "/home/marius/Desktop/CHIRPS/view_debug.json");
        std::cout << x->view()->write_json_string() << std::endl;
        std::shared_ptr<reduce_cube> xmax = std::make_shared<reduce_cube>(x, "max");
        xmax->write_gdal_image("test_max");

    } catch (std::string e) {
        std::cout << e << std::endl;
    }

    config::instance()->gdalcubes_cleanup();
}