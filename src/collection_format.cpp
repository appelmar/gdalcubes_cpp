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


#include "collection_format.h"

#include <boost/regex.hpp>
#include <boost/date_time.hpp>






sqlite3* collection_format::apply(const std::vector<std::string> &descriptors, const std::string& db_filename, bool strict) {


    sqlite3 *db;
    if (sqlite3_open("", &db) != SQLITE_OK) {
        std::string msg = "ERROR in collection_format::apply(): cannot create temporary image collection file.";
        throw msg;
    }



    // Create Bands
    std::string sql_schema_bands =  "CREATE TABLE bands (id INTEGER PRIMARY KEY, name TEXT, type VARCHAR(16), offset NUMERIC, scale NUMERIC, unit VARCHAR(16));";
    if (sqlite3_exec(db, sql_schema_bands.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
        throw std::string("ERROR in collection_format::apply(): cannot create image collection schema (i).");
    }
    if (!_j.count("bands")) {
        throw std::string("ERROR in collection_format::apply(): image collection format does not contain any bands.");
    }

    if (_j["bands"].size() == 0) {
        throw std::string("ERROR in collection_format::apply(): image collection format does not contain any bands.");
    }

    std::vector<boost::regex> regex_band_pattern;
    std::vector<std::string> band_name;
    std::vector<uint16_t> band_num;
    std::vector<uint16_t> band_ids;

    uint16_t band_id=0;
    for (nlohmann::json::iterator it = _j["bands"].begin(); it != _j["bands"].end(); ++it) {
        band_name.push_back(it.key());
        regex_band_pattern.push_back(boost::regex(it.value()["pattern"].get<std::string>()));
        if (it.value().count("band")) {
            band_num.push_back(it.value()["band"].get<int>());
        }
        else {
            band_num.push_back(1);
        }

        std::string sql_insert_band = "INSERT INTO bands(id, name) VALUES(" + std::to_string(band_id) + ",'" + it.key() + "');";
        band_ids.push_back(band_id);
        ++band_id;
        if (sqlite3_exec(db, sql_insert_band.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
            throw std::string("ERROR in collection_format::apply(): cannot insert bands to collection database.");
        }
    }



    // Create image table
    std::string sql_schema_images = "CREATE TABLE images (id INTEGER PRIMARY KEY, name TEXT, left NUMERIC, top NUMERIC, bottom NUMERIC, right NUMERIC, datetime TEXT, proj TEXT, UNIQUE(name));CREATE INDEX idx_image_names ON images(name);";
    if (sqlite3_exec(db, sql_schema_images.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
        throw std::string("ERROR in collection_format::apply(): cannot create image collection schema (ii).");
    }

    std::string sql_schema_gdalrefs = "CREATE TABLE gdalrefs (image_id INTEGER, band_id INTEGER, descriptor TEXT, band_num INTEGER, FOREIGN KEY (image_id) REFERENCES images(id), PRIMARY KEY (image_id, band_id), FOREIGN KEY (band_id) REFERENCES bands(id));"
            "CREATE INDEX idx_gdalrefs_bandid ON gdalrefs(band_id);"
            "CREATE INDEX idx_gdalrefs_imageid ON gdalrefs(image_id);";
    std::cout << sql_schema_gdalrefs << std::endl;
    if (sqlite3_exec(db, sql_schema_gdalrefs.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
        throw std::string("ERROR in collection_format::apply(): cannot create image collection schema (iii).");
    }


    std::string global_pattern = "";
    if (_j.count("pattern")) global_pattern = _j["pattern"].get<std::string>();
    boost::regex regex_global_pattern(global_pattern);


    if (!_j["images"].count("pattern"))
        throw std::string("ERROR in collection_format::apply(): image collection format does not contain a composition rule for images.");
    boost::regex regex_images(_j["images"]["pattern"].get<std::string>());



    // @TODO: Make datetime optional, e.g., for DEMs
    std::string datetime_format = "%Y%m%d";
    if(!_j.count("datetime") || !_j["datetime"].count("pattern")) {
        throw std::string("ERROR in collection_format::apply(): image collection format does not contain a rule to derive date/time.");
    }
    boost::regex regex_datetime(_j["datetime"]["pattern"].get<std::string>());
    if (_j["datetime"].count("format")) {
        datetime_format = _j["datetime"]["format"].get<std::string>();
    }


    uint32_t image_inc_id = 0;
    uint32_t image_id = 0;
    for (auto it = descriptors.begin(); it != descriptors.end(); ++it)
    {
        if (!global_pattern.empty()) {
            if (!boost::regex_match(*it,regex_global_pattern)) {
                std::cout << "ignoring " << *it << std::endl;
                continue;
            }
        }

        boost::cmatch res_image;
        if (!boost::regex_match(it->c_str(), res_image, regex_images)) {
            if (strict) throw std::string("ERROR in collection_format::apply(): image composition rule failed for " + std::string(*it));
            std::cout << "WARNING: skipping  " << *it  << " due to failed image composition rule" << std::endl;
            continue;
        }


        std::string sql_select_image  = "SELECT id FROM images WHERE name='" + res_image[1] + "'";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql_select_image.c_str(), -1, &stmt, NULL);
        if (!stmt) {
            // @TODO do error check here
        }
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            // Empty result --> image has not been added before

            image_id = image_inc_id;
            ++image_inc_id;


            std::string sql;




            // @TODO: Shall we check that all files óf the same image have the same date / time? Currently we don't do.

            // Extract datetime
            boost::cmatch res_datetime;
            if (!boost::regex_match(it->c_str(), res_datetime, regex_datetime)) { // not sure to continue or throw an exception here...
                if (strict) throw std::string("ERROR in collection_format::apply(): datetime rule failed for " + std::string(*it));
                std::cout << "WARNING: skipping  " << *it  << " due to failed datetime rule" << std::endl;
                continue;
            }

            std::istringstream is(res_datetime[1]);
            is.imbue(std::locale(std::locale::classic(),new boost::posix_time::time_input_facet(datetime_format)));
            boost::posix_time::ptime pt;
            is >> pt;
            if (pt.is_not_a_date_time()) {
                if (strict) throw std::string("ERROR in collection_format::apply(): cannot derive datetime from " + *it);
                std::cout << "WARNING: skipping  " << *it  << " due to failed datetime rule" << std::endl;
                continue;
            }
            std::cout << boost::posix_time::to_iso_string(pt) << std::endl;

            std::string sql_insert_image = "INSERT OR IGNORE INTO images(id, name, datetime) VALUES(" + std::to_string(image_id) + ",'" + res_image[1] + "','" + boost::posix_time::to_iso_string(pt) + "')";
            if (sqlite3_exec(db, sql_insert_image.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
                if (strict) throw std::string("ERROR in collection_format::apply(): cannot add image to images table.");
                std::cout << "WARNING: skipping  " << *it  << " due to failed image table insert" << std::endl;
                continue;
            }

        }
        else {
            image_id = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);


        // Insert into gdalrefs table
        for (uint16_t i=0; i<band_name.size(); ++i) {
            if (boost::regex_match(*it, regex_band_pattern[i] )) {
                std::string sql_insert_gdalref = "INSERT INTO gdalrefs(descriptor, image_id, band_id, band_num) VALUES('" + *it + "'," + std::to_string(image_id) + "," +  std::to_string(band_ids[i]) + "," +  std::to_string(band_num[i]) + ");";
                if (sqlite3_exec(db, sql_insert_gdalref.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
                    if (strict) throw std::string("ERROR in collection_format::apply(): cannot add dataset to gdalrefs table.");
                    std::cout << "WARNING: skipping  " << *it  << " due to failed gdalrefs insert" << std::endl;
                    break; // break only works because there is nothing after the loop.
                }
            }
        }


    }


    if (db_filename.empty()) {
        return db;
    }



    // Store DB
    sqlite3_backup *db_backup;
    sqlite3 *out_db;

    if (sqlite3_open(db_filename.c_str(),&out_db) != SQLITE_OK) {
        throw std::string("ERROR in collection_format::apply(): cannot create output database file.");
    }
    db_backup = sqlite3_backup_init(out_db, "main", db, "main");
    if (!db_backup) {
        throw std::string("ERROR in collection_format::apply(): cannot create temporary database backup object.");
    }
    sqlite3_backup_step(db_backup, -1);
    sqlite3_backup_finish(db_backup);
    sqlite3_close(db);

    return out_db;

}
