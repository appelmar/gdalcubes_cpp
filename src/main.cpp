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


#include <iostream>
#include "collection_format.h"



std::vector<std::string> string_list_from_text_file(std::string filename) {
    std::vector<std::string> out;

    std::string line;
    std::ifstream infile(filename);
    while (std::getline(infile, line))
        out.push_back(line);
    return out;
}



int main() {

//    GDALAllRegister();
//    GDALSetCacheMax(1024 * 1024 * 256);            // 256 MiB
//    CPLSetConfigOption("GDAL_PAM_ENABLED", "NO");  // avoid aux files for PNG tiles

    srand(time(NULL));




    collection_format f("../../test/collection_format_test.json");
    //std::vector<std::string> temp;
    try {
        f.apply(string_list_from_text_file("../../test/test_list.txt"), "test.db");
    }
    catch (std::string e) {
        std::cout << e << std::endl;
    }


}