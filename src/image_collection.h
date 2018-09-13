//
// Created by marius on 13.09.18.
//

#ifndef GDALCUBES_IMAGE_COLLECTION_H
#define GDALCUBES_IMAGE_COLLECTION_H

#include "collection_format.h"


class image_collection {
public:


    /**
     * Constructs an empty image collection with given format
     * @param format
     */
    image_collection(collection_format format) : _format(format), _filename(""), _db(nullptr) {
        if (sqlite3_open("", &_db) != SQLITE_OK) {
            std::string msg = "ERROR in image_collection::create(): cannot create temporary image collection file.";
            throw msg;
        }


        // Create tables

        // key value metadata table
        std::string sql_schema_md= "CREATE TABLE md(key TEXT PRIMARY KEY, value TEXT);";
        if (sqlite3_exec(_db, sql_schema_md.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
            throw std::string("ERROR in image_collection::create(): cannot create image collection schema (i).");
        }
        std::string sql_insert_format= "INSERT INTO md(key, value) VALUES('collection_format','" +  _format.json().dump()  + "');";
        if (sqlite3_exec(_db, sql_insert_format.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
            throw std::string("ERROR in image_collection::create(): cannot insert collection format to database.");
        }

        // Create Bands
        std::string sql_schema_bands =  "CREATE TABLE bands (id INTEGER PRIMARY KEY, name TEXT, type VARCHAR(16), offset NUMERIC, scale NUMERIC, unit VARCHAR(16));";
        if (sqlite3_exec(_db, sql_schema_bands.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
            throw std::string("ERROR in collection_format::apply(): cannot create image collection schema (ii).");
        }
        if (!_format.json().count("bands")) {
            throw std::string("ERROR in collection_format::apply(): image collection format does not contain any bands.");
        }

        if (_format.json()["bands"].size() == 0) {
            throw std::string("ERROR in collection_format::apply(): image collection format does not contain any bands.");
        }

        uint16_t band_id=0;
        for (auto it = _format.json()["bands"].begin(); it != _format.json()["bands"].end(); ++it) {
            std::string sql_insert_band = "INSERT INTO bands(id, name) VALUES(" + std::to_string(band_id) + ",'" + it.key() + "');";
            ++band_id;
            if (sqlite3_exec(_db, sql_insert_band.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
                throw std::string("ERROR in collection_format::apply(): cannot insert bands to collection database.");
            }
        }

        // Create image table
        std::string sql_schema_images = "CREATE TABLE images (id INTEGER PRIMARY KEY, name TEXT, left NUMERIC, top NUMERIC, bottom NUMERIC, right NUMERIC, datetime TEXT, proj TEXT, UNIQUE(name));CREATE INDEX idx_image_names ON images(name);";
        if (sqlite3_exec(_db, sql_schema_images.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
            throw std::string("ERROR in collection_format::apply(): cannot create image collection schema (iii).");
        }

        std::string sql_schema_gdalrefs = "CREATE TABLE gdalrefs (image_id INTEGER, band_id INTEGER, descriptor TEXT, band_num INTEGER, FOREIGN KEY (image_id) REFERENCES images(id), PRIMARY KEY (image_id, band_id), FOREIGN KEY (band_id) REFERENCES bands(id));"
                "CREATE INDEX idx_gdalrefs_bandid ON gdalrefs(band_id);"
                "CREATE INDEX idx_gdalrefs_imageid ON gdalrefs(image_id);";
        if (sqlite3_exec(_db, sql_schema_gdalrefs.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
            throw std::string("ERROR in collection_format::apply(): cannot create image collection schema (iv).");
        }

    }


    /**
     * Opens an existing image collection from a file
     * @param filename
     */
    image_collection(std::string filename) : _format(), _filename(filename), _db(nullptr) {
        if (sqlite3_open(filename.c_str(), &_db) != SQLITE_OK) {
            std::string msg = "ERROR in image_collection::image_collection(): cannot open existing image collection file.";
            throw msg;
        }

        // load format from database
        std::string sql_select_format  = "SELECT value FROM md WHERE key='collection_format';";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(_db, sql_select_format.c_str(), -1, &stmt, NULL);
        if (!stmt) {
            std::string msg = "ERROR in image_collection::image_collection(): cannot extract collection format from existing image collection file.";
            throw msg;
        }
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            std::string msg = "ERROR in image_collection::image_collection(): cannot extract collection format from existing image collection file.";
            throw msg;
        }
        else {
           _format.load_string(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt,0))));
        }
        sqlite3_finalize(stmt);








    }

    ~image_collection() {
        if (_db) {
            sqlite3_close(_db);
            _db = nullptr;
        }
    }


    static image_collection create(collection_format format, std::vector<std::string> descriptors, bool strict=true);
    static image_collection create_from_dir(collection_format format, std::string path, bool recursive=true);




    std::string to_string();


    void add(std::vector<std::string> descriptors, bool strict=true);
    void add(std::string descriptor);


    void write (const std::string filename);






protected:

    collection_format _format;
    std::string _filename;
    sqlite3 *_db;

};


#endif //GDALCUBES_IMAGE_COLLECTION_H
