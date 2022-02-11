/*
    MIT License

    Copyright (c) 2021 Marius Appel <marius.appel@uni-muenster.de>

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
#include "../gdalcubes.h"

using namespace gdalcubes;

TEST_CASE("aggregate_time_create", "[aggregate_time]") {

    cube_view r;
    r.srs("EPSG:3857");
    r.left(-6180000);
    r.right(-6080000);
    r.top(-450000);
    r.bottom(-550000);
    r.dx(1000);
    r.dy(1000);
    r.t0(datetime::from_string("2014-01-01"));
    r.t1(datetime::from_string("2014-12-31"));
    r.dt(duration::from_string("P1D"));

    auto c = dummy_cube::create(r, 1, 1.0);
    auto ca = aggregate_time_cube::create(c, "P1M", "mean");
    REQUIRE(ca->st_reference()->nt() == 12);



}
