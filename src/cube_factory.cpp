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

#include "cube_factory.h"

#include "external/json.hpp"
#include "image_collection_cube.h"
#include "reduce.h"
#include "stream.h"

std::shared_ptr<cube> cube_factory::create_from_json(nlohmann::json j) {
    // TODO: move the map to somwhere else but not in this function, alternatives could be a singleton implementation of this class
    std::map<std::string, std::function<std::shared_ptr<cube>(nlohmann::json&)>> cube_generators;
    cube_generators.insert(std::make_pair<std::string, std::function<std::shared_ptr<cube>(nlohmann::json&)>>(
        "reduce", [](nlohmann::json& j) {
            std::shared_ptr<reduce_cube> x = std::make_shared<reduce_cube>(create_from_json(j["in_cube"]), j["reducer"].get<std::string>());
            return x;
        }));

    cube_generators.insert(std::make_pair<std::string, std::function<std::shared_ptr<cube>(nlohmann::json&)>>(
        "stream", [](nlohmann::json& j) {
            // input MUST be an image_collection_cube
            std::shared_ptr<image_collection_cube> cin = std::dynamic_pointer_cast<image_collection_cube>(create_from_json(j["in_cube"]));
            std::shared_ptr<stream_cube> x = std::make_shared<stream_cube>(cin, j["command"].get<std::string>());
            return x;
        }));

    cube_generators.insert(std::make_pair<std::string, std::function<std::shared_ptr<cube>(nlohmann::json&)>>(
        "image_collection", [](nlohmann::json& j) {
            if (!boost::filesystem::exists(j["file"].get<std::string>())) {
                throw std::string("ERROR in cube_generators[\"image_collection\"](): image collection file does not exist.");
            }
            cube_view v = cube_view::read_json_string(j["view"].dump());
            std::shared_ptr<image_collection_cube> x = std::make_shared<image_collection_cube>(j["file"].get<std::string>(), v);
            x->set_chunk_size(j["chunk_size"][0].get<uint32_t>(), j["chunk_size"][1].get<uint32_t>(), j["chunk_size"][2].get<uint32_t>());
            return x;
        }));

    if (!j.count("cube_type") > 0) {
        throw std::string("ERROR in cube_factory::create_from_json(): invalid object, missing cube_type key.");
    }

    std::string cube_type = j["cube_type"];

    return (cube_generators[cube_type](j));  //recursive creation
}