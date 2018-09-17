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

    boost::regex rexp("P([0-9]+)([YMWD])");

    boost::cmatch res;
    if (!boost::regex_match(j.at("time").at("dt").get<std::string>().c_str(), res, rexp)) {
        throw std::string("ERROR in cube_view::read_json(): cannot derive date interval from JSON");
    }
    v._dt_interval = std::stoi(res[1]);
    if (res[2] == "Y")
        v._dt_unit = YEAR;
    else if (res[2] == "M")
        v._dt_unit = MONTH;
    else if (res[2] == "W")
        v._dt_unit = WEEK;
    else if (res[2] == "D")
        v._dt_unit = DAY;

    std::string st0 = j.at("time").at("t0").get<std::string>();
    std::string st1 = j.at("time").at("t1").get<std::string>();
    v._t0 = boost::posix_time::ptime(boost::gregorian::from_string(st0)); // Currently no time support
    v._t1 = boost::posix_time::ptime(boost::gregorian::from_string(st1)); // Currently no time support

    v._nx = j.at("space").at("nx").get<uint32_t>();
    v._ny = j.at("space").at("ny").get<uint32_t>();
    v._win.left = j.at("space").at("left").get<double>();
    v._win.right = j.at("space").at("right").get<double>();
    v._win.top = j.at("space").at("top").get<double>();
    v._win.bottom = j.at("space").at("bottom").get<double>();
    v._proj = j.at("space").at("proj").get<std::string>();

    return v;
}

void cube_view::write_json(std::string filename) {
    nlohmann::json j = nlohmann::json{
        {"space", {{"nx", _nx}, {"ny", _ny}, {"left", _win.left}, {"right", _win.right}, {"top", _win.top}, {"bottom", _win.bottom}, {"proj", _proj}}},
        {"time", {{"dt", dt()}, {"t0", boost::gregorian::to_iso_extended_string(_t0.date())}, {"t1", boost::gregorian::to_iso_extended_string(_t1.date())}}}};

    std::ofstream o(filename, std::ofstream::out);
    if (!o.good()) {
        throw std::string("ERROR in cube_view::write_json(): cannot write to file.");
    }

    o << std::setw(4) << j << std::endl;
    o.close();
}