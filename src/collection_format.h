//
// Created by marius on 12.09.18.
//

#ifndef COLLECTION_FORMAT_H_
#define COLLECTION_FORMAT_H_

#include <string>
#include <iostream>
#include <fstream>
#include <sqlite3.h>
#include <boost/filesystem.hpp>
#include "../include/json.hpp"

class collection_format {

public:
    collection_format(std::string filename) {

        namespace fs = boost::filesystem;
        if (!fs::exists(filename))
            throw std::string("ERROR in collection_format::collection_format(): image collection format file does not exist.");
        std::ifstream i(filename);
        i >> _j;
    }


    void test() {
        std::cout << _j.count("pattern") << std::endl;
        std::cout << _j["images"].count("pattern");
    }

    /**
     * @TODO
     * @return
     */
    bool validate();



    /**
     * Apply the collection format to a list of filenames or other GDALataset descriptors and bulid the basic image collection data structure in
     * an SQLite database. This function will not open any files.
     * @param descriptors
     */
    void apply(const std::vector<std::string>& descriptors);





    inline nlohmann::json get() {return _j;}

private:


    nlohmann::json _j;

};


#endif COLLECTION_FORMAT_H_
