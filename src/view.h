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

#include <boost/date_time.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include "../include/json.hpp"

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
    enum datetime_unit {
        SECOND = 0,  // not yet implemented
        MINUTE = 1,  // not yet implemented
        HOUR = 2,    // not yet implemented
        DAY = 3,
        WEEK = 4,
        MONTH = 5,
        YEAR = 6
    };

    static cube_view read_json(std::string filename);
    void write_json(std::string filename);

    inline uint32_t& nx() { return _nx; }
    inline uint32_t& ny() { return _ny; }

    inline double dx() { return (_win.right - _win.left) / _nx; }
    inline double dy() { return (_win.top - _win.bottom) / _ny; }

    inline std::string dt() {
        switch (_dt_unit) {
            case DAY:
                return "P" + std::to_string(_dt_interval) + "D";
            case WEEK:
                return "P" + std::to_string(_dt_interval) + "W";
            case MONTH:
                return "P" + std::to_string(_dt_interval) + "M";
            case YEAR:
                return "P" + std::to_string(_dt_interval) + "Y";
        }
        throw std::string("ERROR in cube_view::dt(): time-based resolution (hours, minutes, seconds) is not supported in gdalcubes");
    }

    inline double& left() { return _win.left; }
    inline double& right() { return _win.right; }
    inline double& bottom() { return _win.bottom; }
    inline double& top() { return _win.top; }

    inline bounds_2d<double>& win() { return _win; }

    void set_daily(uint16_t n = 1) {
        _dt_interval = n;
        _dt_unit = DAY;
    }
    void set_monthly(uint16_t n = 1) {
        _dt_interval = n;
        _dt_unit = MONTH;
    }
    void set_yearly(uint16_t n = 1) {
        _dt_interval = n;
        _dt_unit = YEAR;
    }
    void set_quarterly(uint16_t n = 1) {
        _dt_interval = 3 * n;
        _dt_unit = MONTH;
    }
    void set_weekly(uint16_t n = 1) {
        _dt_interval = n;
        _dt_unit = WEEK;
    }

    // TODO: add resampling / aggregation methods

    uint32_t nt() {
        if (_dt_unit == datetime_unit::DAY) {
            return (uint32_t)std::ceil(((_t1.date() - _t0.date()).days() + 1) / (double)_dt_interval);
        } else if (_dt_unit == datetime_unit::WEEK) {
            return (uint32_t)std::ceil(((_t1.date() - _t0.date()).days() + 1) / (7 * (double)_dt_interval));
        } else if (_dt_unit == datetime_unit::MONTH) {
            uint32_t res = 0;
            int yd = _t1.date().year() - _t0.date().year();
            if (yd >= 1) {
                return (uint32_t)std::ceil(((yd - 1) * 12 + _t1.date().month() + (12 - _t0.date().month() + 1)) / (double)_dt_interval);
            }
            return (uint32_t)std::ceil((_t1.date().month() - _t0.date().month() + 1) / (double)_dt_interval);
        } else if (_dt_unit == datetime_unit::YEAR) {
            return (uint32_t)std::ceil((_t1.date().year() - _t0.date().year() + 1) / (double)_dt_interval);
        }
        throw std::string("ERROR in cube_view::nt(): time-based resolution (hours, minutes, seconds) is not supported in gdalcubes");
    }

   protected:
    std::string _proj;
    bounds_2d<double> _win;

    boost::posix_time::ptime _t0;
    boost::posix_time::ptime _t1;

    uint32_t _nx;
    uint32_t _ny;

    uint16_t _dt_interval;  // time size as number +  unit of time or date
    datetime_unit _dt_unit;
};

#endif  //VIEW_H
