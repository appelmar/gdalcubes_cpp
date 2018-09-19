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
#include "../../include/catch.hpp"
#include "../datetime.h"
TEST_CASE("Deriving datetime unit from string", "[datetime]") {
    REQUIRE(datetime::from_string("2002-03-04 12:13:14").unit() == datetime_unit::SECOND);
    REQUIRE(datetime::from_string("2002-03-04 12:13").unit() == datetime_unit::MINUTE);
    REQUIRE(datetime::from_string("2002-03-04 12").unit() == datetime_unit::HOUR);
    REQUIRE(datetime::from_string("2002-03-04").unit() == datetime_unit::DAY);
    REQUIRE(datetime::from_string("2002-03").unit() == datetime_unit::MONTH);
    REQUIRE(datetime::from_string("2002").unit() == datetime_unit::YEAR);
}