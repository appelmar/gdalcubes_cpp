//
// Created by marius on 13.09.18.
//

#include "image_collection.h"

#include <boost/regex.hpp>
#include <boost/date_time.hpp>
#include <gdal_priv.h>
#include <ogr_spatialref.h>


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

    uint16_t band_id=0;
    for (nlohmann::json::iterator it = _format.json()["bands"].begin(); it != _format.json()["bands"].end(); ++it) {
        band_name.push_back(it.key());
        regex_band_pattern.push_back(boost::regex(it.value()["pattern"].get<std::string>()));
        if (it.value().count("band")) {
            band_num.push_back(it.value()["band"].get<int>());
        }
        else {
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
    std::string datetime_format = "%Y%m%d";
    if(!_format.json().count("datetime") || !_format.json()["datetime"].count("pattern")) {
        throw std::string("ERROR in image_collection::add(): image collection format does not contain a rule to derive date/time.");
    }
    boost::regex regex_datetime(_format.json()["datetime"]["pattern"].get<std::string>());
    if (_format.json()["datetime"].count("format")) {
        datetime_format = _format.json()["datetime"]["format"].get<std::string>();
    }


    for (auto it = descriptors.begin(); it != descriptors.end(); ++it)
    {
        if (!global_pattern.empty()) { // prevent unnecessary GDALOpen calls
            if (!boost::regex_match(*it,regex_global_pattern)) {
               // std::cout << "ignoring " << *it << std::endl;
                continue;
            }
        }



        // Read GDAL metadata
        GDALDataset *dataset = (GDALDataset *)GDALOpen((*it).c_str(), GA_ReadOnly);
        if (!dataset) {
            if (strict) throw std::string("ERROR in image_collection::add(): GDAL cannot open '" + *it + "'.");
            std::cout << "WARNING in image_collection::add(): GDAL cannot open '" << *it <<"'.";
            continue;
        }
        // if check = false, the following is not really needed if image is already in the database due to another file.
        double affine_in[6] = {0,0,1,0,0,1};
        double l,r,b,t;
        char *proj4;
        if (dataset->GetGeoTransform(affine_in) != CE_None) {
            // No affine transformation, maybe GCPs?
            GDALClose((GDALDatasetH)dataset);
            if (strict) throw std::string("ERROR in image_collection::add(): GDAL cannot derive affine transformation for '" + *it + "'. GCPs or unreferenced images are currently not supported.");
            std::cout << "WARNING in image_collection::add(): GDAL cannot derive affine transformation for  '" << *it <<"'.";
            continue;
        }
        else {
            l = affine_in[0];
            r = affine_in[0] + affine_in[1] *  dataset->GetRasterXSize() + affine_in[2] * dataset->GetRasterYSize();
            t = affine_in[3];
            b = affine_in[3] + affine_in[4] *  dataset->GetRasterXSize() + affine_in[5] *  dataset->GetRasterYSize();
            //_bbox.transform( _gdal_obj->get()->GetProjectionRef(), "EPSG:4326");
            OGRSpatialReference srs_in;
            srs_in.SetFromUserInput(dataset->GetProjectionRef());
            srs_in.exportToProj4(&proj4);
        }

        std::vector<image_band> bands;
        for (uint16_t i=0; i<dataset->GetRasterCount(); ++i) {
            image_band b;
            b.type = dataset->GetRasterBand(i+1)->GetRasterDataType();
            b.offset = dataset->GetRasterBand(i+1)->GetOffset();
            b.scale = dataset->GetRasterBand(i+1)->GetScale();
            b.unit = dataset->GetRasterBand(i+1)->GetUnitType();
            bands.push_back(b);
        }
        GDALClose((GDALDatasetH)dataset);


        // TODO: check consistency for all files of an image?!
        // -> add parameter checks=true / false




        boost::cmatch res_image;
        if (!boost::regex_match(it->c_str(), res_image, regex_images)) {
            if (strict) throw std::string("ERROR in image_collection::add(): image composition rule failed for " + std::string(*it));
            std::cout << "WARNING: skipping  " << *it  << " due to failed image composition rule" << std::endl;
            continue;
        }

        uint32_t image_id;

        std::string sql_select_image  = "SELECT id FROM images WHERE name='" + res_image[1] + "'";
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
            if (!boost::regex_match(it->c_str(), res_datetime, regex_datetime)) { // not sure to continue or throw an exception here...
                if (strict) throw std::string("ERROR in image_collection::add(): datetime rule failed for " + std::string(*it));
                std::cout << "WARNING: skipping  " << *it  << " due to failed datetime rule" << std::endl;
                continue;
            }

            std::istringstream is(res_datetime[1]);
            is.imbue(std::locale(std::locale::classic(),new boost::posix_time::time_input_facet(datetime_format)));
            boost::posix_time::ptime pt;
            is >> pt;
            if (pt.is_not_a_date_time()) {
                if (strict) throw std::string("ERROR in image_collection::add(): cannot derive datetime from " + *it);
                std::cout << "WARNING: skipping  " << *it  << " due to failed datetime rule" << std::endl;
                continue;
            }
            std::string sql_insert_image = "INSERT OR IGNORE INTO images(name, datetime, left, top, bottom, right, proj) VALUES('"
                                           + res_image[1] + "','" +
                                            boost::posix_time::to_iso_string(pt) + "'," +
                                            std::to_string(l) + "," + std::to_string(t) + "," + std::to_string(b) + "," + std::to_string(r) + ",'" + proj4  + "')";
            if (sqlite3_exec(_db, sql_insert_image.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
                if (strict) throw std::string("ERROR in image_collection::add(): cannot add image to images table.");
                std::cout << "WARNING: skipping  " << *it  << " due to failed image table insert" << std::endl;
                continue;
            }
            image_id = sqlite3_last_insert_rowid(_db); // take care of race conditions if things run parallel at some point

        }
        else {
            image_id = sqlite3_column_int(stmt, 0);
            // TODO: if checks, compare l,r,b,t, datetime,proj4 from images table with current GDAL dataset
        }
        sqlite3_finalize(stmt);


        // Insert into gdalrefs table

        for (uint16_t i=0; i<band_name.size(); ++i) {
            if (boost::regex_match(*it, regex_band_pattern[i] )) {
                // TODO: if checks, check whether bandnum exists in GDALdataset
                // TODO: if checks, compare band type, offset, scale, unit, etc. with current GDAL dataset

                if (!band_complete[i]) {
                    std::string sql_band_update = "UPDATE bands SET type='" + std::to_string(bands[band_num[i]-1].type) + "'," +
                                                                   "scale=" + std::to_string(bands[band_num[i]-1].scale) + "," +
                                                                   "offset=" + std::to_string(bands[band_num[i]-1].offset) + "," +
                                                                   "unit='" +  bands[band_num[i]-1].unit + "' WHERE name='" + band_name[i] + "';";
                    if (sqlite3_exec(_db, sql_band_update.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
                        if (strict) throw std::string("ERROR in image_collection::add(): cannot update band table.");
                        std::cout << "WARNING: skipping  " << *it  << " due to failed band table update" << std::endl;
                        continue;
                    }
                    band_complete[i] = true;
                }

                std::string sql_insert_gdalref = "INSERT INTO gdalrefs(descriptor, image_id, band_id, band_num) VALUES('" + *it + "'," + std::to_string(image_id) + "," +  std::to_string(band_ids[i]) + "," +  std::to_string(band_num[i]) + ");";
                if (sqlite3_exec(_db, sql_insert_gdalref.c_str(), NULL, NULL, NULL) != SQLITE_OK) {
                    if (strict) throw std::string("ERROR in image_collection::add(): cannot add dataset to gdalrefs table.");
                    std::cout << "WARNING: skipping  " << *it  << " due to failed gdalrefs insert" << std::endl;
                    break; // break only works because there is nothing after the loop.
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
    sqlite3_backup *db_backup;
    sqlite3 *out_db;

    if (sqlite3_open(_filename.c_str(),&out_db) != SQLITE_OK) {
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
}



std::string image_collection::to_string() {
    std::string out = "IMAGE COLLECTION '" + _filename + "':\n";



    std::string sql = "SELECT COUNT(*) FROM images UNION SELECT COUNT(*) FROM bands UNION SELECT COUNT(*) FROM gdalrefs";



    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(_db, sql.c_str(), -1, &stmt, NULL);
    if (!stmt) {
        // @TODO do error check here
    }
    sqlite3_step(stmt);
    out += "\t " + std::to_string(sqlite3_column_int(stmt,0)) + " images \n";
    sqlite3_step(stmt);
    out += "\t " + std::to_string(sqlite3_column_int(stmt,0)) + " bands\n";
    sqlite3_step(stmt);
    out += "\t " + std::to_string(sqlite3_column_int(stmt,0)) + " GDAL dataset references\n";

    return out;

}