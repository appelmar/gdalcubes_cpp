//
// Created by marius on 10.09.18.
//


#include <iostream>
#include "image_collection.h"
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

    GDALAllRegister();
    GDALSetCacheMax(1024 * 1024 * 256);            // 256 MiB
    CPLSetConfigOption("GDAL_PAM_ENABLED", "NO");  // avoid aux files for PNG tiles

    srand(time(NULL));



    image_collection ic;
    ic.create_schema();
    std::cout << "Test" << std::endl;

    collection_format f("../../test/collection_format_test.json");
    //std::vector<std::string> temp;
    try {
        f.apply(string_list_from_text_file("../../test/test_list.txt"));
    }
    catch (std::string e) {
        std::cout << e << std::endl;
    }


}