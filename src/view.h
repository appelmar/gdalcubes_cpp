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

#include "../include/json.hpp"
#include "datetime.h"
#include "image_collection.h"

/**
 * This class defines a view how to look at the data including which resolution, which projection,
 * which spatial / temporal window we are intereseted in following analyses.
 *
 * The class uses "simplified" date calculations, i.e., date differences are based on a
 * given maximum unit / granularity that we are interested in. For instance, the difference between 2018-04-01 and 2016-06-05 can
 * be 2 years, 23 months, or 666 days. Time intervals are currently not implemented.
 */

class cube_view {
   public:
    static cube_view read_json(std::string filename);
    void write_json(std::string filename);

    inline uint32_t& nx() { return _nx; }
    inline uint32_t& ny() { return _ny; }

    inline double dx() { return (_win.right - _win.left) / _nx; }
    inline double dy() { return (_win.top - _win.bottom) / _ny; }

    inline double& left() { return _win.left; }
    inline double& right() { return _win.right; }
    inline double& bottom() { return _win.bottom; }
    inline double& top() { return _win.top; }

    inline datetime& t0() { return _t0; }
    inline datetime& t1() { return _t1; }

    inline bounds_2d<double>& win() { return _win; }

    inline duration& dt() { return _dt; }

    void set_daily(uint16_t n = 1) {
        _dt = duration(n, DAY);
    }
    void set_monthly(uint16_t n = 1) {
        _dt = duration(n, MONTH);
    }
    void set_yearly(uint16_t n = 1) {
        _dt = duration(n, YEAR);
    }
    void set_quarterly(uint16_t n = 1) {
        _dt = duration(3 * n, MONTH);
    }
    void set_weekly(uint16_t n = 1) {
        _dt = duration(n, WEEK);
    }

    // TODO: add resampling / aggregation methods

    uint32_t nt() {
        duration d = (_t1 - _t0) + 1;
        return (d % _dt == 0) ? d / _dt : (1 + d / _dt);
    }

    /**
     * Convert integer view-based coordinates to map coordinates
     * @note view-based coordinates are in the order (t,y,x), (0,0,0) corresponds to the earliest date (t0) for the
     * lower left pixel.
     * @note Output map coordinates will have the same projection as the cube view
     * @see cube_view::view_coords()
     * @param p
     * @return
     */
    coords_st map_coords(coords_nd<uint32_t, 3> p) {
        coords_st s;
        s.s.x = _win.left + p[2] * dx();
        s.s.y = _win.bottom + p[1] * dy();
        s.t = _t0 + _dt * p[0];
    }

    /**
     *  Convert map coordinates to integer view-based coordinates
     * @note view-based coordinates are in the order (t,y,x), (0,0,0) corresponds to the earliest date (t0) for the
     * lower left pixel.
     * @note the function assumes input map coordinates have the same projection as the cube view
     * @see cube_view::map_coords()
     * @param p
     * @return
     */
    coords_nd<uint32_t, 3> view_coords(coords_st p) {
        coords_nd<uint32_t, 3> s;
        s[2] = (uint32_t)((p.s.x - _win.left) / dx());
        s[1] = (uint32_t)((p.s.y - _win.bottom) / dy());
        s[0] = (uint32_t)((p.t - _t0) / _dt);
        return s;
    }

   protected:
    std::string _proj;
    bounds_2d<double> _win;

    datetime _t0;
    datetime _t1;

    uint32_t _nx;
    uint32_t _ny;

    duration _dt;
};

#endif  //VIEW_H
