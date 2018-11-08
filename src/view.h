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

#ifndef VIEW_H
#define VIEW_H

#include <algorithm>
#include "datetime.h"
#include "external/json.hpp"
#include "image_collection.h"

/**
 * A utility structure to work with different aggregation
 * algorithms
 */
struct aggregation {
    enum aggregation_type {
        NONE,
        MIN,
        MAX,
        MEAN,
        MEDIAN,
        FIRST,
        LAST
    };

    static aggregation_type from_string(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        if (s == "none") {
            return NONE;
        } else if (s == "min") {
            return MIN;
        } else if (s == "max") {
            return MAX;
        } else if (s == "mean") {
            return MEAN;
        } else if (s == "median") {
            return MEDIAN;
        } else if (s == "first") {
            return FIRST;
        } else if (s == "last") {
            return LAST;
        }
        return NONE;
    }

    static std::string to_string(aggregation_type a) {
        switch (a) {
            case NONE:
                return "none";
            case MIN:
                return "min";
            case MAX:
                return "max";
            case MEAN:
                return "mean";
            case MEDIAN:
                return "median";
            case FIRST:
                return "first";
            case LAST:
                return "last";
            default:
                return "none";
        }
    }
};

/**
 * @brief Utility structure to work with different resampling
 * algorithms and their different types in GDAL
 */
struct resampling {
    /**
     * @brief An enumeration listing all available resampling types
     */
    enum resampling_type {
        NEAR,
        BILINEAR,
        CUBIC,
        CUBICSPLINE,
        LANCZOS,
        AVERAGE,
        MODE,
        MAX,
        MIN,
        MED,
        Q1,
        Q3
    };

    /**
     * @brief Get the resampling type from its name
     * @param s string representation of a resampling algorithm
     * @return the corresponding resampling type entry
     */
    static resampling_type from_string(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        if (s == "near" || s == "nearest") {
            return NEAR;
        } else if (s == "bilinear") {
            return BILINEAR;
        } else if (s == "cubic") {
            return CUBIC;
        } else if (s == "cubicspline") {
            return CUBICSPLINE;
        } else if (s == "lanczos") {
            return LANCZOS;
        } else if (s == "average" || s == "mean") {
            return AVERAGE;
        } else if (s == "mode") {
            return MODE;
        } else if (s == "max") {
            return MAX;
        } else if (s == "min") {
            return MIN;
        } else if (s == "med" || s == "median") {
            return MED;
        } else if (s == "q1") {
            return Q1;
        } else if (s == "q3") {
            return Q3;
        }
        return NEAR;
    }

    /**
     * @brief Get the name of a resampling method
     * @param r resampling type
     * @return name string of the resampling method
     */
    static std::string to_string(resampling_type r) {
        switch (r) {
            case NEAR:
                return "near";
            case BILINEAR:
                return "bilinear";
            case CUBIC:
                return "cubic";
            case CUBICSPLINE:
                return "cubicspline";
            case LANCZOS:
                return "lanczos";
            case AVERAGE:
                return "average";
            case MODE:
                return "mode";
            case MAX:
                return "max";
            case MIN:
                return "min";
            case MED:
                return "med";
            case Q1:
                return "q1";
            case Q3:
                return "q3";
            default:
                return "near";
        }
    }

    /**
     * @brief Convert a resampling type to the corresponding GDAL type to be used in RasterIO
     * @note RasterIO does not support all available resampling types
     * @param r resampling type
     * @return GDALRIOResampleAlg
     */
    static GDALRIOResampleAlg to_gdal_rasterio(resampling_type r) {
        switch (r) {
            case NEAR:
                return GRIORA_NearestNeighbour;
            case BILINEAR:
                return GRIORA_Bilinear;
            case CUBIC:
                return GRIORA_Cubic;
            case CUBICSPLINE:
                return GRIORA_CubicSpline;
            case LANCZOS:
                return GRIORA_Lanczos;
            case AVERAGE:
                return GRIORA_Average;
            case MODE:
                return GRIORA_Mode;
            case MAX:
            case MIN:
            case MED:
            case Q1:
            case Q3:
            default:
                return GRIORA_NearestNeighbour;  // Not yet defined in gdal.h
        }
    }
};

/**
 * @brief Spatial and temporal reference for data cubes
 */
class cube_st_reference {
   public:
    virtual ~cube_st_reference() {
    }

    /**
     * Get the number of cells in x dimension
     * @return number of cells in x dimension
     */
    inline uint32_t& nx() { return _nx; }

    /**
    * Get the number of cells in y dimension
    * @return number of cells in y dimension
    */
    inline uint32_t& ny() { return _ny; }

    /**
    * Get the size of cells in x dimension
    * @return size of cells in x dimension
    */
    inline double dx() { return (_win.right - _win.left) / _nx; }

    /**
   * Get the size of cells in y dimension
   * @return size of cells in y dimension
   */
    inline double dy() { return (_win.top - _win.bottom) / _ny; }

    /**
     * Get the lower limit in x dimension / left boundary of the cube's extent
     * @return left boundary of the cube's extent
     */
    inline double& left() { return _win.left; }

    /**
     * Get the upper limit in x dimension / right boundary of the cube's extent
     * @return right boundary of the cube's extent
     */
    inline double& right() { return _win.right; }

    /**
     * Get the lower limit in y dimension / bottom boundary of the cube's extent
     * @return bottom boundary of the cube's extent
     */
    inline double& bottom() { return _win.bottom; }

    /**
     * Get the upper limit in y dimension / top boundary of the cube's extent
     * @return top boundary of the cube's extent
     */
    inline double& top() { return _win.top; }

    /**
     * Get or set the spatial reference system / projection
     * @return string (reference) with projection / SRS information that is understandable by GDAL / OGR
     */
    inline std::string& proj() { return _proj; }

    /**
     * Return the spatial reference system / projection
     * @return OGRSpatialReference object
     */
    inline OGRSpatialReference proj_ogr() {
        OGRSpatialReference s;
        s.SetFromUserInput(proj().c_str());
        return s;
    }

    /**
     * Getter / setter for the lower boundary of the cube's temporal extent (start datetime)
     * @return reference to the object's t0 object
     */
    inline datetime& t0() { return _t0; }

    /**
     * Getter / setter for the upper boundary of the cube's temporal extent (end datetime)
     * @return reference to the object's t1 object
     */
    inline datetime& t1() { return _t1; }

    /**
     * Get the numbers of cells in the time dimension
     * @return integer number of cells
     */
    uint32_t nt() {
        duration d = (_t1 - _t0) + 1;
        return (d % _dt == 0) ? d / _dt : (1 + (d / _dt));
    }

    /**
     * Set the numbers of cells in the time dimension. This method will automatically derive a
     * datetime duration of cells based on the current unit
     */
    void nt(uint32_t n) {
        duration d = (_t1 - _t0) + 1;
        dt().dt_interval = (int32_t)std::ceil((double)d.dt_interval / (double)n);
        assert(nt() == n);
    }

    /**
     * Get the spatial extent / window of a cube view
     * @return spatial extent, coordinates are expressed in the view's projection / SRS
     */
    inline bounds_2d<double>& win() { return _win; }

    /**
     * Get or set the temporal site / duration of one cube cell
     * @return a reference to the view's dt field
     */
    inline duration& dt() { return _dt; }

    /**
     * Set the temporal size of cube cells as n days
     * @param n duration / temporal size of one cell as number of days
     */
    void set_daily(uint16_t n = 1) {
        _dt = duration(n, DAY);
    }

    /**
    * Set the temporal size of cube cells as n months
    * @param n duration / temporal size of one cell as number of months
    */
    void set_monthly(uint16_t n = 1) {
        _dt = duration(n, MONTH);
    }

    /**
    * Set the temporal size of cube cells as n years
    * @param n duration / temporal size of one cell as number of years
    */
    void set_yearly(uint16_t n = 1) {
        _dt = duration(n, YEAR);
    }

    /**
    * Set the temporal size of cube cells as n quarter years
    * @param n duration / temporal size of one cell as number of quarter years
    */
    void set_quarterly(uint16_t n = 1) {
        _dt = duration(3 * n, MONTH);
    }

    /**
    * Set the temporal size of cube cells as n weeks
    * @param n duration / temporal size of one cell as number of weeks
    */
    void set_weekly(uint16_t n = 1) {
        _dt = duration(n, WEEK);
    }

    /**
    * Convert integer cube-based coordinates to spacetime coordinates
    * @note cube-based coordinates are in the order (t,y,x), (0,0,0) corresponds to the earliest date (t0) for the
    * lower left pixel.
    * @note Output coordinates will have the projection / SRS as in cube_st_reference::proj()
    * @see cube_st_reference::view_coords()
    * @param p cube-based coordinates
    * @return spacetime coordinates
    */
    coords_st map_coords(coords_nd<uint32_t, 3> p) {
        coords_st s;
        s.s.x = _win.left + p[2] * dx();
        s.s.y = _win.bottom + p[1] * dy();
        s.t = _t0 + _dt * p[0];
        return s;
    }

    /**
     * Convert spacetime coordinates to integer cube-based coordinates
     * @note cube-based coordinates are in the order (t,y,x), (0,0,0) corresponds to the earliest date (t0) for the
     * lower left pixel.
     * @note the function assumes input coordinates have the  projection / SRS as in cube_st_reference::proj()
     * @see cube_st_reference::map_coords()
     * @param p spacetime coordinates
     * @return cube-based coordinates
     */
    coords_nd<uint32_t, 3> cube_coords(coords_st p) {
        coords_nd<uint32_t, 3> s;
        s[2] = (uint32_t)((p.s.x - _win.left) / dx());
        s[1] = (uint32_t)((p.s.y - _win.bottom) / dy());
        s[0] = (uint32_t)((p.t - _t0) / _dt);
        return s;
    }

   protected:
    /**
     * @brief Spatial reference system / projection
     *
     * The string must be readable for OGRSpatialReference::SetFromUserInput, i.e.,
     * it can be "EPSG:xxx", WKT, or PROJ.4
     *
     */
    std::string _proj;

    /**
     * @brief Spatial window
     */
    bounds_2d<double> _win;

    datetime _t0;
    datetime _t1;
    uint32_t _nx;
    uint32_t _ny;

    duration _dt;
};

/**
 * A data cube view includes the spacetime reference of a cube (extent, resolution, projection) and
 * optional resampling and aggregation algorithms that are applied when original images are
 * read from an image_collection_cube. Aggregation refers to how multiple values for the same
 * cube cell from different images are combined whereas resampling refers to the algorithm used
 * to warp / reproject images to the cube geometry.
 */
class cube_view : public cube_st_reference {
   public:
    /**
     * Deserializes a cube_view object from a JSON file.
     * @param filename Path to the json file on disk
     * @return A cube_view object
     */
    static cube_view read_json(std::string filename);

    /**
    * Deserializes a cube_view object from a JSON file.
    * @param str JSON string
    * @return A cube_view object
    */
    static cube_view read_json_string(std::string str);

    /**
    * Serializes a cube_view object as a JSON file.
    * @param filename output file
    */
    void write_json(std::string filename);

    /**
      * Serializes a cube_view object as a JSON string.
      * @return JSON string
      */
    std::string write_json_string();

    /**
     * Getter / setter for aggregation method
     * @return reference to the object's aggregation field
     */
    inline aggregation::aggregation_type& aggregation_method() { return _aggregation; }

    /**
    * Getter / setter for resampling method
    * @return reference to the object's resampling field
    */
    inline resampling::resampling_type& resampling_method() { return _resampling; }

   private:
    static cube_view read(nlohmann::json j);
    resampling::resampling_type _resampling;
    aggregation::aggregation_type _aggregation;
};

#endif  //VIEW_H
