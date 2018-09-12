//
// Created by marius on 12.09.18.
//

#include "collection_format.h"

#include "boost/regex.hpp"







void collection_format::apply(const std::vector<std::string> &descriptors) {


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
    uint16_t nbands = _j["bands"].size();
    std::cout << "Collection format describes " << nbands << " bands." << std::endl;


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

    std::string sql_schema_gdalrefs = "CREATE TABLE gdalrefs (descriptor TEXT, image_id INTEGER, band_id INTEGER, band_num INTEGER, FOREIGN KEY (image_id) REFERENCES images(id), PRIMARY KEY (image_id, band_id), FOREIGN KEY (band_id) REFERENCES bands(id));"
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

        boost::cmatch res;
        if (!boost::regex_match(it->c_str(), res, regex_images)) { // not sure to continue or throw an exception here...
            std::cout << regex_images << std::endl;
            throw std::string("ERROR in collection_format::apply(): image composition rule failed for " + std::string(*it));
        }


        std::string sql_select_image  = "SELECT id FROM images WHERE name='" + res[1] + "'";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql_select_image.c_str(), -1, &stmt, NULL);
        if (!stmt) {

        }
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            image_id = image_inc_id;
            ++image_inc_id;
            // Empty result --> image has not been added before
            std::string sql_insert_image = "INSERT OR IGNORE INTO images(id, name) VALUES(" + std::to_string(image_id) + ",'" + res[1] + "')";
            if (sqlite3_exec(db, sql_insert_image.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
                throw std::string("ERROR in collection_format::apply(): cannot add image to images table.");
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
                    throw std::string("ERROR in collection_format::apply(): cannot add dataset to gdalrefs table.");
                }
            }
        }
    }

    // Store DB
    sqlite3_backup *db_backup;
    sqlite3 *out_db;

    if (sqlite3_open("temp.sqlite",&out_db) != SQLITE_OK) {
        throw std::string("ERROR in collection_format::apply(): cannot create output database file.");
    }
    db_backup = sqlite3_backup_init(out_db, "main", db, "main");
    if (!db_backup) {
        throw std::string("ERROR in collection_format::apply(): cannot create temporary database backup object.");
    }
    sqlite3_backup_step(db_backup, -1);
    sqlite3_backup_finish(db_backup);
    sqlite3_close(db);
    sqlite3_close(out_db);

}
