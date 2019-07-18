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

#include <string>
#include "../external/catch.hpp"
#include "../view.h"

using namespace gdalcubes;

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

    cube_view v1;
    v1.srs() = "EPSG:3857";
    v1.win().bottom = 6831918;
    v1.win().top = 7027881;
    v1.win().left = 2500790;
    v1.win().right = 2858522;
    v1.t0() = datetime::from_string("2018-03-26T09:40:29");
    v1.t1() = datetime::from_string("2018-11-08T09:32:09");
    v1.nx() = 700;
    v1.ny() = 700;
    v1.nt(4);

    datetime t0 = v1.t0();
    datetime t1 = v1.t1();
    REQUIRE((t1 - t0).dt_interval > 0);
}
