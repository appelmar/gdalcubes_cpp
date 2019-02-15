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

#include <string>
#include "../external/catch.hpp"
#include "../view.h"

TEST_CASE("Create", "[view]") {
    cube_view v;
    v.srs() = "EPSG:4326";
    v.left() = -110;
    v.right() = 110;

    v.dx(0.5);
    REQUIRE(v.nx() == 2 * 110 / 0.5);
    REQUIRE(v.left() == -110);
    REQUIRE(v.right() == 110);

    v.nx() = 440;
    REQUIRE(v.dx() == 0.5);
    REQUIRE(v.left() == -110);
    REQUIRE(v.right() == 110);

    v.bottom() = -50;
    v.top() = 50;

    v.ny() = 200;
    REQUIRE(v.dy() == 0.5);
    REQUIRE(v.bottom() == -50);
    REQUIRE(v.top() == 50);

    v.dy(0.5);
    REQUIRE(v.ny() == 200);
    REQUIRE(v.bottom() == -50);
    REQUIRE(v.top() == 50);

    v.t0() = datetime::from_string("2018-01-01");
    v.t1() = datetime::from_string("2018-01-10");

    v.nt(10);
    REQUIRE(v.t0() == datetime::from_string("2018-01-01"));
    REQUIRE(v.t1() == datetime::from_string("2018-01-10"));
    REQUIRE(v.nt() == 10);
    REQUIRE(v.dt() == duration::from_string("P1D"));

    v.dt(duration::from_string("P3D"));
    REQUIRE(v.t0() == datetime::from_string("2018-01-01"));
    REQUIRE(v.t1() == datetime::from_string("2018-01-12"));
    REQUIRE(v.nt() == 4);
    REQUIRE(v.dt() == duration::from_string("P3D"));
}
