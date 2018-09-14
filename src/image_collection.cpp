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

#include "image_collection.h"

#include <boost/regex.hpp>

image_collection::image_collection(collection_format format) : _format(format), _filename(""), _db(nullptr) {
    if (sqlite3_open("", &_db) != SQLITE_OK) {
        std::string msg = "ERROR in image_collection::create(): cannot create temporary image collection file.";
        throw msg;
    }

    // Enable foreign key constraints
    sqlite3_db_config(_db, SQLITE_DBCONFIG_ENABLE_FKEY, 1, NULL);

    // Create tables

    // key value metadata table
    std::string sql_schema_md = "CREATE TABLE md(key TEXT PRIMARY KEY, value TEXT);";
    if (sqlite3_exec(_db, sql_schema_md.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
        throw std::string("ERROR in image_collection::create(): cannot create image collection schema (i).");
    }
    std::string sql_insert_format = "INSERT INTO md(key, value) VALUES('collection_format','" + _format.json().dump() + "');";
    if (sqlite3_exec(_db, sql_insert_format.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
        throw std::string("ERROR in image_collection::create(): cannot insert collection format to database.");
    }

    // Create Bands
    std::string sql_schema_bands = "CREATE TABLE bands (id INTEGER PRIMARY KEY, name TEXT, type VARCHAR(16), offset NUMERIC, scale NUMERIC, unit VARCHAR(16));";
    if (sqlite3_exec(_db, sql_schema_bands.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
        throw std::string("ERROR in collection_format::apply(): cannot create image collection schema (ii).");
    }
    if (!_format.json().count("bands")) {
        throw std::string("ERROR in collection_format::apply(): image collection format does not contain any bands.");
    }

    if (_format.json()["bands"].size() == 0) {
        throw std::string("ERROR in collection_format::apply(): image collection format does not contain any bands.");
    }

    uint16_t band_id = 0;
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

    std::string sql_schema_gdalrefs =
        "CREATE TABLE gdalrefs (image_id INTEGER, band_id INTEGER, descriptor TEXT, band_num INTEGER, FOREIGN KEY (image_id) REFERENCES images(id) ON DELETE CASCADE, PRIMARY KEY (image_id, band_id), FOREIGN KEY (band_id) REFERENCES bands(id) ON DELETE CASCADE);"
        "CREATE INDEX idx_gdalrefs_bandid ON gdalrefs(band_id);"
        "CREATE INDEX idx_gdalrefs_imageid ON gdalrefs(image_id);";
    if (sqlite3_exec(_db, sql_schema_gdalrefs.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
        throw std::string("ERROR in collection_format::apply(): cannot create image collection schema (iv).");
    }
}

image_collection::image_collection(std::string filename) : _format(), _filename(filename), _db(nullptr) {
    if (sqlite3_open(filename.c_str(), &_db) != SQLITE_OK) {
        std::string msg = "ERROR in image_collection::image_collection(): cannot open existing image collection file.";
        throw msg;
    }
    // Enable foreign key constraints
    sqlite3_db_config(_db, SQLITE_DBCONFIG_ENABLE_FKEY, 1, NULL);

    // load format from database
    std::string sql_select_format = "SELECT value FROM md WHERE key='collection_format';";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(_db, sql_select_format.c_str(), -1, &stmt, NULL);
    if (!stmt) {
        std::string msg = "ERROR in image_collection::image_collection(): cannot extract collection format from existing image collection file.";
        throw msg;
    }
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        std::string msg = "ERROR in image_collection::image_collection(): cannot extract collection format from existing image collection file.";
        throw msg;
    } else {
        _format.load_string(std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))));
    }
    sqlite3_finalize(stmt);
}

image_collection image_collection::create(collection_format format, std::vector<std::string> descriptors, bool strict) {
    image_collection o(format);

    o.add(descriptors, strict);

    return o;
}

struct image_band {
    GDALDataType type;
    std::string unit;
    double scale;
    double offset;
};

void image_collection::add(std::vector<std::string> descriptors, bool strict) {
    std::vector<boost::regex> regex_band_pattern;

    /* TODO: The following would fail if other applications create image collections and assign ids to bands differently. A better solution would be to load band ids, names, and nums from the database bands table directly
     */

    std::vector<std::string> band_name;
    std::vector<uint16_t> band_num;
    std::vector<uint16_t> band_ids;
    std::vector<bool> band_complete;

    uint16_t band_id = 0;
    for (nlohmann::json::iterator it = _format.json()["bands"].begin(); it != _format.json()["bands"].end(); ++it) {
        band_name.push_back(it.key());
        regex_band_pattern.push_back(boost::regex(it.value()["pattern"].get<std::string>()));
        if (it.value().count("band")) {
            band_num.push_back(it.value()["band"].get<int>());
        } else {
            band_num.push_back(1);
        }
        band_ids.push_back(band_id);
        band_complete.push_back(false);
        ++band_id;
    }

    std::string global_pattern = "";
    if (_format.json().count("pattern")) global_pattern = _format.json()["pattern"].get<std::string>();
    boost::regex regex_global_pattern(global_pattern);

    if (!_format.json()["images"].count("pattern"))
        throw std::string("ERROR in image_collection::add(): image collection format does not contain a composition rule for images.");
    boost::regex regex_images(_format.json()["images"]["pattern"].get<std::string>());

    // @TODO: Make datetime optional, e.g., for DEMs
    std::string datetime_format = "%Y-%m-%d";
    if (!_format.json().count("datetime") || !_format.json()["datetime"].count("pattern")) {
        throw std::string("ERROR in image_collection::add(): image collection format does not contain a rule to derive date/time.");
    }
    boost::regex regex_datetime(_format.json()["datetime"]["pattern"].get<std::string>());
    if (_format.json()["datetime"].count("format")) {
        datetime_format = _format.json()["datetime"]["format"].get<std::string>();
    }

    for (auto it = descriptors.begin(); it != descriptors.end(); ++it) {
        if (!global_pattern.empty()) {  // prevent unnecessary GDALOpen calls
            if (!boost::regex_match(*it, regex_global_pattern)) {
                // std::cout << "ignoring " << *it << std::endl;
                continue;
            }
        }

        // Read GDAL metadata
        GDALDataset* dataset = (GDALDataset*)GDALOpen((*it).c_str(), GA_ReadOnly);
        if (!dataset) {
            if (strict) throw std::string("ERROR in image_collection::add(): GDAL cannot open '" + *it + "'.");
            std::cout << "WARNING in image_collection::add(): GDAL cannot open '" << *it << "'.";
            continue;
        }
        // if check = false, the following is not really needed if image is already in the database due to another file.
        double affine_in[6] = {0, 0, 1, 0, 0, 1};
        bounds_2d<double> bbox;
        char* proj4;
        if (dataset->GetGeoTransform(affine_in) != CE_None) {
            // No affine transformation, maybe GCPs?
            GDALClose((GDALDatasetH)dataset);
            if (strict) throw std::string("ERROR in image_collection::add(): GDAL cannot derive affine transformation for '" + *it + "'. GCPs or unreferenced images are currently not supported.");
            std::cout << "WARNING in image_collection::add(): GDAL cannot derive affine transformation for  '" << *it << "'.";
            continue;
        } else {
            bbox.left = affine_in[0];
            bbox.right = affine_in[0] + affine_in[1] * dataset->GetRasterXSize() + affine_in[2] * dataset->GetRasterYSize();
            bbox.top = affine_in[3];
            bbox.bottom = affine_in[3] + affine_in[4] * dataset->GetRasterXSize() + affine_in[5] * dataset->GetRasterYSize();
            OGRSpatialReference srs_in;
            srs_in.SetFromUserInput(dataset->GetProjectionRef());
            srs_in.exportToProj4(&proj4);
            bbox.transform(proj4, "EPSG:4326");
        }

        std::vector<image_band> bands;
        for (uint16_t i = 0; i < dataset->GetRasterCount(); ++i) {
            image_band b;
            b.type = dataset->GetRasterBand(i + 1)->GetRasterDataType();
            b.offset = dataset->GetRasterBand(i + 1)->GetOffset();
            b.scale = dataset->GetRasterBand(i + 1)->GetScale();
            b.unit = dataset->GetRasterBand(i + 1)->GetUnitType();
            bands.push_back(b);
        }
        GDALClose((GDALDatasetH)dataset);

        // TODO: check consistency for all files of an image?!
        // -> add parameter checks=true / false

        boost::cmatch res_image;
        if (!boost::regex_match(it->c_str(), res_image, regex_images)) {
            if (strict) throw std::string("ERROR in image_collection::add(): image composition rule failed for " + std::string(*it));
            std::cout << "WARNING: skipping  " << *it << " due to failed image composition rule" << std::endl;
            continue;
        }

        uint32_t image_id;

        std::string sql_select_image = "SELECT id FROM images WHERE name='" + res_image[1] + "'";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(_db, sql_select_image.c_str(), -1, &stmt, NULL);
        if (!stmt) {
            // @TODO do error check here
        }
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            // Empty result --> image has not been added before

            // @TODO: Shall we check that all files óf the same image have the same date / time? Currently we don't do.

            // Extract datetime
            boost::cmatch res_datetime;
            if (!boost::regex_match(it->c_str(), res_datetime, regex_datetime)) {  // not sure to continue or throw an exception here...
                if (strict) throw std::string("ERROR in image_collection::add(): datetime rule failed for " + std::string(*it));
                std::cout << "WARNING: skipping  " << *it << " due to failed datetime rule" << std::endl;
                continue;
            }

            std::istringstream is(res_datetime[1]);
            is.imbue(std::locale(std::locale::classic(), new boost::posix_time::time_input_facet(datetime_format)));
            boost::posix_time::ptime pt;
            is >> pt;
            if (pt.is_not_a_date_time()) {
                if (strict) throw std::string("ERROR in image_collection::add(): cannot derive datetime from " + *it);
                std::cout << "WARNING: skipping  " << *it << " due to failed datetime rule" << std::endl;
                continue;
            }

            // Convert to ISO string including separators (boost::to_iso_string or boost::to_iso_extended_string do not work with SQLite datetime functions)
            std::stringstream os;
            os.imbue(std::locale(std::locale::classic(), new boost::posix_time::time_facet("%Y-%m-%dT%H:%M:%S")));  // ISO8601 with separators to make this readable for SQlite
            os << pt;
            std::string sql_insert_image = "INSERT OR IGNORE INTO images(name, datetime, left, top, bottom, right, proj) VALUES('" + res_image[1] + "','" +
                                           os.str() + "'," +
                                           std::to_string(bbox.left) + "," + std::to_string(bbox.top) + "," + std::to_string(bbox.bottom) + "," + std::to_string(bbox.right) + ",'" + proj4 + "')";
            if (sqlite3_exec(_db, sql_insert_image.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
                if (strict) throw std::string("ERROR in image_collection::add(): cannot add image to images table.");
                std::cout << "WARNING: skipping  " << *it << " due to failed image table insert" << std::endl;
                continue;
            }
            image_id = sqlite3_last_insert_rowid(_db);  // take care of race conditions if things run parallel at some point

        } else {
            image_id = sqlite3_column_int(stmt, 0);
            // TODO: if checks, compare l,r,b,t, datetime,proj4 from images table with current GDAL dataset
        }
        sqlite3_finalize(stmt);

        // Insert into gdalrefs table

        for (uint16_t i = 0; i < band_name.size(); ++i) {
            if (boost::regex_match(*it, regex_band_pattern[i])) {
                // TODO: if checks, check whether bandnum exists in GDALdataset
                // TODO: if checks, compare band type, offset, scale, unit, etc. with current GDAL dataset

                if (!band_complete[i]) {
                    std::string sql_band_update = "UPDATE bands SET type='" + std::to_string(bands[band_num[i] - 1].type) + "'," +
                                                  "scale=" + std::to_string(bands[band_num[i] - 1].scale) + "," +
                                                  "offset=" + std::to_string(bands[band_num[i] - 1].offset) + "," +
                                                  "unit='" + bands[band_num[i] - 1].unit + "' WHERE name='" + band_name[i] + "';";
                    if (sqlite3_exec(_db, sql_band_update.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
                        if (strict) throw std::string("ERROR in image_collection::add(): cannot update band table.");
                        std::cout << "WARNING: skipping  " << *it << " due to failed band table update" << std::endl;
                        continue;
                    }
                    band_complete[i] = true;
                }

                std::string sql_insert_gdalref = "INSERT INTO gdalrefs(descriptor, image_id, band_id, band_num) VALUES('" + *it + "'," + std::to_string(image_id) + "," + std::to_string(band_ids[i]) + "," + std::to_string(band_num[i]) + ");";
                if (sqlite3_exec(_db, sql_insert_gdalref.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
                    if (strict) throw std::string("ERROR in image_collection::add(): cannot add dataset to gdalrefs table.");
                    std::cout << "WARNING: skipping  " << *it << " due to failed gdalrefs insert" << std::endl;
                    break;  // break only works because there is nothing after the loop.
                }
            }
        }
    }
}

void image_collection::add(std::string descriptor) {
    std::vector<std::string> x{descriptor};
    return add(x);
}

void image_collection::write(const std::string filename) {
    if (_filename.compare(filename) == 0) {
        // nothing to do
        return;
    }

    if (!_db) {
        throw std::string("ERROR in image_collection::write(): database handle is not open");
    }
    _filename = filename;

    // Store DB
    sqlite3_backup* db_backup;
    sqlite3* out_db;

    if (sqlite3_open(_filename.c_str(), &out_db) != SQLITE_OK) {
        throw std::string("ERROR in image_collection::write(): cannot create output database file.");
    }
    db_backup = sqlite3_backup_init(out_db, "main", _db, "main");
    if (!db_backup) {
        throw std::string("ERROR in image_collection::write(): cannot create temporary database backup object.");
    }
    sqlite3_backup_step(db_backup, -1);
    sqlite3_backup_finish(db_backup);

    sqlite3_close(_db);
    _db = out_db;

    // Enable foreign key constraints
    sqlite3_db_config(_db, SQLITE_DBCONFIG_ENABLE_FKEY, 1, NULL);  // this is important!
}

uint16_t image_collection::count_bands() {
    std::string sql = "SELECT COUNT(*) FROM bands;";

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, NULL);
    if (!stmt) {
        throw std::string("ERROR in image_collection::count_bands(): cannot read query result");
    }
    sqlite3_step(stmt);
    uint16_t out = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return out;
}

uint32_t image_collection::count_images() {
    std::string sql = "SELECT COUNT(*) FROM images;";

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, NULL);
    if (!stmt) {
        throw std::string("ERROR in image_collection::count_images(): cannot read query result");
    }
    sqlite3_step(stmt);
    uint16_t out = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return out;
}

uint32_t image_collection::count_gdalrefs() {
    std::string sql = "SELECT COUNT(*) FROM gdalrefs;";

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, NULL);
    if (!stmt) {
        throw std::string("ERROR in image_collection::count_gdalrefs(): cannot read query result");
    }
    sqlite3_step(stmt);
    uint16_t out = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return out;
}

std::string image_collection::to_string() {
    std::string out = "IMAGE COLLECTION '" + _filename + "':\n";
    out += "\t " + std::to_string(count_images()) + " images \n";
    out += "\t " + std::to_string(count_bands()) + " bands\n";
    out += "\t " + std::to_string(count_gdalrefs()) + " GDAL dataset references\n";
    return out;
}

void image_collection::filter_bands(std::vector<std::string> bands) {
    // This implementation requires a foreign key constraint for gdalrefs table with cascade delete

    if (bands.empty()) {
        throw std::string("ERROR in image_collection::filter_bands(): no bands selected");
    }
    if (bands.size() == count_bands())
        return;

    std::string bandlist;
    for (uint16_t i = 0; i < bands.size() - 1; ++i) {
        bandlist += "'" + bands[i] + "',";
    }
    bandlist += "'" + bands[bands.size() - 1] + "'";

    std::string sql = "DELETE FROM bands WHERE name NOT IN (" + bandlist + ");";
    if (sqlite3_exec(_db, sql.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
        throw std::string("ERROR in image_collection::filter_bands(): cannot remove bands from collection.");
    }
}

void image_collection::filter_datetime_range(boost::posix_time::ptime start, boost::posix_time::ptime end) {
    // This implementation requires a foreign key constraint for the gdalrefs table with cascade delete

    // TODO: write a helper function to convert ptime to string with given format
    std::ostringstream os;
    os.imbue(std::locale(std::locale::classic(), new boost::posix_time::time_facet("%Y-%m-%dT%H:%M:%S")));  // ISO8601 with separators to make this readable for SQlite
    os << start;
    std::string start_str = os.str();
    os.clear();
    os << end;
    std::string end_str = os.str();

    std::string sql = "DELETE FROM images WHERE datetime(images.datetime) < datetime('" + start_str + "')  OR datetime(images.datetime) > datetime('" + end_str + "');";
    if (sqlite3_exec(_db, sql.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
        throw std::string("ERROR in image_collection::filter_datetime_range(): cannot remove images from collection.");
    }
}

void image_collection::filter_spatial_range(bounds_2d<double> range, std::string proj) {
    // This implementation requires a foreign key constraint for the gdalrefs table with cascade delete

    range.transform(proj, "EPSG:4326");
    std::string sql = "DELETE FROM images WHERE images.right < " + std::to_string(range.left) + " OR images.left > " + std::to_string(range.right) + "OR images.bottom > " + std::to_string(range.top) + "OR images.top < " + std::to_string(range.bottom) + ";";

    if (sqlite3_exec(_db, sql.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
        throw std::string("ERROR in image_collection::filter_spatial_range(): cannot remove images from collection.");
    }
}