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

#include "view.h"
#include <boost/regex.hpp>

cube_view cube_view::read_json(std::string filename) {
    namespace fs = boost::filesystem;
    if (!fs::exists(filename))
        throw std::string("ERROR in cube_view::read_json(): cube view file does not exist.");
    std::ifstream i(filename);
    nlohmann::json j;
    i >> j;
    cube_view v;

    v._dt = duration::from_string(j.at("time").at("dt").get<std::string>());

    std::string st0 = j.at("time").at("t0").get<std::string>();
    std::string st1 = j.at("time").at("t1").get<std::string>();

    v._t0 = datetime::from_string(st0);
    v._t1 = datetime::from_string(st1);

    // No matter how the start and end datetime are given, use the unit of the datetime interval!
    v._t0.unit() = v._dt.dt_unit;
    v._t1.unit() = v._dt.dt_unit;

    v._nx = j.at("space").at("nx").get<uint32_t>();
    v._ny = j.at("space").at("ny").get<uint32_t>();
    v._win.left = j.at("space").at("left").get<double>();
    v._win.right = j.at("space").at("right").get<double>();
    v._win.top = j.at("space").at("top").get<double>();
    v._win.bottom = j.at("space").at("bottom").get<double>();
    v._proj = j.at("space").at("proj").get<std::string>();

    if (j.count("resampling") == 0) {
        v._resampling = resampling::resampling_type::NEAR;
    }
    else {
        v._resampling = resampling::from_string(j.at("resampling").get<std::string>());
    }

    if (j.count("aggregation") == 0) {
        v._aggregation = aggregation::aggregation_type::NONE;
    }
    else {
        v._aggregation = aggregation::from_string(j.at("aggregation").get<std::string>());
    }

    return v;
}

void cube_view::write_json(std::string filename) {
    nlohmann::json j = nlohmann::json{
        {"space", {{"nx", _nx}, {"ny", _ny}, {"left", _win.left}, {"right", _win.right}, {"top", _win.top}, {"bottom", _win.bottom}, {"proj", _proj}}},
        {"time", {{"dt", dt().to_string()}, {"t0", _t0.to_string()}, {"t1", _t1.to_string()}}},
        {"aggregation", aggregation::to_string(_aggregation)}, {"resampling", resampling::to_string(_resampling)}};

    std::ofstream o(filename, std::ofstream::out);
    if (!o.good()) {
        throw std::string("ERROR in cube_view::write_json(): cannot write to file.");
    }

    o << std::setw(4) << j << std::endl;
    o.close();
}