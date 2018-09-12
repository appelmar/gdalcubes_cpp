//
// Created by marius on 11.09.18.
//

#include "image_collection.h"

#include <sqlite3.h>
#include <iostream>
#include "boost/filesystem.hpp"

void image_collection::create_schema() {



    if (!_db) {
        throw std::string("Error in image_collection::create_schema(). No database connection, create SQLite database first.");
    }

    // if tables already exist
    std::string sql_schema_bands =  "CREATE TABLE bands  (id INTEGER PRIMARY KEY, name TEXT, type VARCHAR(16), offset NUMERIC, scale NUMERIC, unit VARCHAR(16)); ";
    //TODO:  is the foreign key constraint needed? Joins of those tables are most likely not used much
    std::string sql_schema_images =
            "CREATE TABLE images (id INTEGER PRIMARY KEY, url TEXT, gdal_id TEXT, left NUMERIC, top NUMERIC, bottom NUMERIC, "
                    "right NUMERIC, datetime TEXT, proj TEXT, band_id INTEGER, FOREIGN KEY(band_id) REFERENCES bands(id)); "
                    "CREATE INDEX idx_image_bands ON images(band_id);";



    std::string sql_schema_md = "CREATE TABLE md(k TEXT UNIQUE, v BLOB)";





    //std::cout << gpkg_libversion();
}



void image_collection::create(std::string dir, bool recursive) {


    namespace fs = boost::filesystem;
    fs::path pdir{dir};
    fs::directory_iterator end;
    for (fs::directory_iterator i(pdir); i != end; ++i) {
        try {
            add((*i).path().string());
        } catch (...) {
            std::cout << "WARNING: image '" << (*i).path().string() << "' was skipped because it is not readable by GDAL." << std::endl;
            continue;
        }
    }
}




void image_collection::add(std::string path) {

//    try {
//        image x(path);
//        if (!_bands.empty()) {
//            if (_bands != x.get_bands()) {
//                throw std::string("ERROR: bands in image '" + path + "' are not compatible with bands from the collection.");
//            }
//
//        } else {
//            _bands = x.get_bands();
//        }
//        _bbox = bounds_2d<double>::union2d(_bbox, x.get_bbox());
//        _images.push_back(x);
//
//    } catch (...) {
//        throw;
//    }


}