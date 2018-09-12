//
// Created by marius on 11.09.18.
//

#ifndef GDALCUBES_IMAGE_COLLECTION_H_
#define GDALCUBES_IMAGE_COLLECTION_H_

#include <string>
#include <sqlite3.h>
#include <vector>
#include <ogr_spatialref.h>
#include <gdal.h>
#include <map>
#include "gdal_dataset_pool.h"

template <typename Ta>
struct bounds_2d {
    Ta left, bottom, top, right;
    static bool intersects(bounds_2d<Ta> a, bounds_2d<Ta> b) {
        return (
                a.right >= b.left &&
                a.left <= b.right &&
                a.top >= b.bottom &&
                a.bottom <= b.top);
    }
    static bool within(bounds_2d<Ta> a, bounds_2d<Ta> b) {
        return (
                a.left >= b.left &&
                a.right <= b.right &&
                a.top <= b.top &&
                a.bottom >= b.bottom);
    }
    static bool outside(bounds_2d<Ta> a, bounds_2d<Ta> b) {
        return !intersects(a, b);
    }
    static bounds_2d<Ta> union2d(bounds_2d<Ta> a, bounds_2d<Ta> b) {
        bounds_2d<Ta> out;
        out.left = fmin(a.left, b.left);
        out.right = fmax(a.right, b.right);
        out.bottom = fmin(a.bottom, b.bottom);
        out.top = fmax(a.top, b.top);
        return out;
    }

    static bounds_2d<Ta> intersection(bounds_2d<Ta> a, bounds_2d<Ta> b) {
        bounds_2d<Ta> out;
        out.left = fmax(a.left, b.left);
        out.right = fmin(a.right, b.right);
        out.bottom = fmin(a.bottom, b.bottom);
        out.top = fmax(a.top, b.top);
        return out;
    }

    bounds_2d<Ta> transform(std::string srs_from, std::string srs_to) {
        OGRSpatialReference srs_in;
        OGRSpatialReference srs_out;
        srs_in.SetFromUserInput(srs_from.c_str());
        srs_out.SetFromUserInput(srs_to.c_str());
        if (srs_in.IsSame(&srs_out)) {
            return *this;
        }

        OGRCoordinateTransformation* coord_transform = OGRCreateCoordinateTransformation(&srs_in, &srs_out);

        Ta x[4] = {left, left, right, right};
        Ta y[4] = {top, bottom, top, bottom};

        if (coord_transform == NULL || !coord_transform->Transform(4, x, y)) {
            throw std::string("ERROR: coordinate transformation failed.");
        }

        Ta xmin = std::numeric_limits<Ta>::is_integer ? std::numeric_limits<Ta>::max() : std::numeric_limits<Ta>::max();
        Ta ymin = std::numeric_limits<Ta>::is_integer ? std::numeric_limits<Ta>::max() : std::numeric_limits<Ta>::max();
        Ta xmax = std::numeric_limits<Ta>::is_integer ? std::numeric_limits<Ta>::min() : -std::numeric_limits<Ta>::max();
        Ta ymax = std::numeric_limits<Ta>::is_integer ? std::numeric_limits<Ta>::min() : -std::numeric_limits<Ta>::max();
        for (uint8_t k = 0; k < 4; ++k) {
            if (x[k] < xmin) xmin = x[k];
            if (y[k] < ymin) ymin = y[k];
            if (x[k] > xmax) xmax = x[k];
            if (y[k] > ymax) ymax = y[k];
        }

        bounds_2d<Ta> in_extent;
        left = xmin;
        right = xmax;
        top = ymax;
        bottom = ymin;

        return *this;
    }
};

template <typename T>
struct coords_2d {
    T x, y;
};







class image_band {
public:
    image_band(std::string band_id) : _band_id(band_id),
                                      _band_name(""),
                                      _offset(0),
                                      _scale(1),
                                      _type(GDT_Unknown),
                                      _unit("1") {}

    image_band(std::string band_id, std::string name) : _band_id(band_id),
                                                        _band_name(name),
                                                        _offset(0),
                                                        _scale(1),
                                                        _type(GDT_Unknown),
                                                        _unit("1") {}

    void set_name(std::string name) { _band_name = name; }
    void set_offset(double offset) { _offset = offset; }
    void set_scale(double scale) { _scale = scale; }
    void set_type(GDALDataType type) { _type = type; }
    void set_unit(std::string unit) { _unit = unit; }

    std::string get_id() const { return _band_id; }
    std::string get_name() const { return _band_name; }
    double get_offset() const { return _offset; }
    double get_scale() const { return _scale; }
    GDALDataType get_type() const { return _type; }
    std::string get_unit() const { return _unit; }

protected:
    std::string _band_id;
    std::string _band_name;
    double _offset;
    double _scale;
    GDALDataType _type;
    std::string _unit;
};







class image_band_collection {
public:
    image_band_collection() : _band_idx(), _bands() {}

    void add(image_band band) {
        if (_band_idx.find(band.get_id()) == _band_idx.end()) {
            _bands.push_back(band);
            _band_idx[band.get_id()] = _bands.size() - 1;
        } else
            throw std::string("ERROR: band with the same identifier already exists");
    }

    image_band get(std::string band_id) {
        if (_band_idx.find(band_id) != _band_idx.end()) {
            return _bands[_band_idx[band_id]];
        } else
            throw std::string("ERROR: band with given identifier does not exists in collection");
    }

    bool has(std::string band_id) {
        return (_band_idx.find(band_id) != _band_idx.end()) ? true : false;
    }

    image_band get(uint16_t band_idx) {
        if (band_idx >= 0 && band_idx < _bands.size()) {
            return _bands[band_idx];
        } else
            throw std::string("ERROR: band with given index number does not exists in collection");
    }

    uint16_t get_index(std::string band_id) {
        if (_band_idx.find(band_id) != _band_idx.end()) {
            return _band_idx[band_id];
        } else {
            throw std::string("ERROR: image collection has no band '" + band_id + "'.");
        }
    }

    bool empty() const { return _bands.size() == 0; }

    uint16_t count() const { return _bands.size(); }

    friend bool operator==(const image_band_collection& l, const image_band_collection& r) {
        if (l._bands.size() != r._bands.size()) return false;
        for (uint16_t i = 0; i < l._bands.size(); ++i) {
            const image_band a = l._bands[i];
            const image_band b = r._bands[i];
            if (a.get_id().compare(b.get_id()) != 0 ||
                a.get_type() != b.get_type()) return false;
        }
        return true;
    }

    friend bool operator!=(const image_band_collection& l, const image_band_collection& r) {
        return !(l == r);
    }

protected:
    std::map<std::string, uint16_t> _band_idx;
    std::vector<image_band> _bands;
};






class image {
public:
    image(std::string path);
    bounds_2d<double> get_bbox() const { return _bbox; }
    image_band_collection get_bands() const { return _bands; }

    gdal_dataset* get_gdaldataset() {
        // the gdal_dataset_pool takes care of whether the file is already open or not
        return _gdal_obj;
    }

    std::string get_path() const {
        return _gdal_obj->path();
    }


protected:
    gdal_dataset* _gdal_obj;
    bounds_2d<double> _bbox;
    image_band_collection _bands;

};





class image_collection {

public:

    image_collection() : _db(nullptr), _has_open_transaction(false) {
        if (sqlite3_open("should_be_temporary.sqlite", &_db) != SQLITE_OK) {
            std::string msg = "ERROR: cannot create temporary image collection file.";
            throw msg;
        }
        create_schema();
    }

    image_collection(std::string filename) : _db(nullptr), _has_open_transaction(false) {

        if (sqlite3_open(filename.c_str(), &_db) != SQLITE_OK) {
            std::string msg = "ERROR: cannot open image collection from '" + filename + "'.";
            throw msg;
        }
        //TODO: validate schema
    }

    // empty temporary database
    void create();


    /**
     * Create an image collection from files in a directory
     * @param dir
     * @param recursive
     * @todo add optional pattern argument
     * @todo replace recursive with recursive depth integer
     */
    void create(std::string dir, bool recursive = false);



    void create(std::vector<std::string> path);




    void add(std::string path);





    void open(std::string path);


    bool is_aligned();



    /**
     * Checks whehter a collection is empty, i.e., does not contain any image
     * @return true if the collection is empty, false if not.
     */
    bool is_empty();

    /**
     * Create a textual summary of an image collection
     * @return std::string
     */
    std::string to_string();




    /**
     * Counts the number of images in a collection
     * @return the number of images
     */
    uint32_t count();



    /**
     * Checks whether all images can be accessed
     * @return false if one or more image links are not accessible, true otherwise.
     */
    bool check();


    void write();


public: // TODO: make protected

    /**
     * Creates the database schema for new collections, requires an existing valid database connection (_db must not be NULL).
     */
    void create_schema();

    bool validate_chema();




    sqlite3 *_db; // SQLite database connection

    bool _has_open_transaction;







};


#endif //GDALCUBES_IMAGE_COLLECTION_H_
