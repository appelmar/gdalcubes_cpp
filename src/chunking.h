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

#ifndef CHUNKING_H
#define CHUNKING_H

#include "image_collection.h"
#include "view.h"

/**
 * A chunk stores a spatial and temporal contiguous part of an image collection
 * as four-dimensional array.
 */

class chunk {
   public:
    bounds_2d<double> bounds();
    int64_t id();

    // read()
    // id()

   protected:
    // spatial extent
    // temporal extent
    // bands
};

typedef uint32_t chunkid;

class chunking {
   public:
    class chunk_iterator;
};

class default_chunking : public chunking {
   public:
    default_chunking(cube_view v) : _v(v), _size_x(256), _size_y(256), _size_t(16) {
    }

    inline uint32_t& size_x() { return _size_x; }
    inline uint32_t& size_y() { return _size_y; }
    inline uint32_t& size_t() { return _size_t; }

    inline uint32_t count_chunks() {
        return std::ceil((double)_v.nx() / (double)_size_x) * std::ceil((double)_v.ny() / (double)_size_y) * std::ceil((double)_v.nt() / (double)_size_t);
    }

    chunkid find_chunk_that_contains(coords_st p) {
        uint32_t cumprod = 1;
        chunkid id = 0;

        // 1. Convert map coordinates to view-based coordinates
        coords_nd<uint32_t, 3> s = _v.view_coords(p);

        // 2. Find out which chunk contains the integer view coordinates
        id += cumprod * (s[3] / _size_x);
        cumprod *= (uint32_t)std::ceil(((double)_v.nx()) / ((double)_size_x));
        id += cumprod * (s[2] / _size_y);
        cumprod *= (uint32_t)std::ceil(((double)(_v.ny()) / ((double)_size_y)));
        id += cumprod * (s[1] / _size_t);
        //cumprod *= (uint32_t)std::ceil(((double)(_v.nt()) / ((double)_size_t);

        return id;
    }

    bounds_st bounds_from_chunk(chunkid id) {
        coords_nd<uint32_t, 3> out_vcoords_low;
        coords_nd<uint32_t, 3> out_vcoords_high;
        bounds_st out_st;

        // Transform to global coordinates based on chunk id
        uint32_t cumprod = 1;
        int32_t n;

        n = (uint32_t)std::ceil(((double)(_v.nx())) / ((double)_size_x));
        out_vcoords_low[2] = (id / cumprod) % n;
        out_vcoords_low[2] *= _size_x;
        out_vcoords_high[2] = out_vcoords_low[2] + _size_x;
        cumprod *= n;

        n = (uint32_t)std::ceil(((double)(_v.ny())) / ((double)_size_y));
        out_vcoords_low[1] = (id / cumprod) % n;
        out_vcoords_low[1] *= _size_y;
        out_vcoords_high[1] = out_vcoords_low[1] + _size_y;
        cumprod *= n;

        n = (uint32_t)std::ceil(((double)(_v.nt())) / ((double)_size_t));
        out_vcoords_low[0] = (id / cumprod) % n;
        out_vcoords_low[0] *= _size_t;
        out_vcoords_high[0] = out_vcoords_low[0] + _size_t;
        cumprod *= n;

        // Shrink to view
        if (out_vcoords_high[0] < 0)
            out_vcoords_high[0] = 0;
        else if (out_vcoords_high[0] > _v.nt())
            out_vcoords_high[0] = _v.nt();
        if (out_vcoords_low[0] < 0)
            out_vcoords_low[0] = 0;
        else if (out_vcoords_low[0] > _v.nt())
            out_vcoords_low[0] = _v.nt();

        if (out_vcoords_high[1] < 0)
            out_vcoords_high[1] = 0;
        else if (out_vcoords_high[1] > _v.ny())
            out_vcoords_high[1] = _v.ny();
        if (out_vcoords_low[1] < 0)
            out_vcoords_low[1] = 0;
        else if (out_vcoords_low[1] > _v.ny())
            out_vcoords_low[1] = _v.ny();

        if (out_vcoords_high[2] < 0)
            out_vcoords_high[2] = 0;
        else if (out_vcoords_high[2] > _v.nx())
            out_vcoords_high[2] = _v.nx();
        if (out_vcoords_low[2] < 0)
            out_vcoords_low[2] = 0;
        else if (out_vcoords_low[2] > _v.nx())
            out_vcoords_low[2] = _v.nx();

        // TODO: Reimplement

        // Transform to map coordinates
        //        out_st.s.left = _v.left() + out_vcoords_low[2] * _v.dx();
        //        out_st.s.right= _v.left() + out_vcoords_high[2] * _v.dx();
        //
        //        out_st.s.bottom = _v.bottom() + out_vcoords_low[1] * _v.dy();
        //        out_st.s.top= _v.bottom() + out_vcoords_high[1] * _v.dy();
        //
        //
        //        if (_v.dt().dt_unit == datetime_unit::DAY) {
        //            out_st.t0 = _v.t0() + boost::gregorian::days(out_vcoords_low[0] * _v.dt().dt_interval);
        //            out_st.t1 = _v.t0() + boost::gregorian::days(out_vcoords_high[0] * _v.dt().dt_interval);
        //        }
        //        else if (_v.dt().dt_unit == datetime_unit::WEEK) {
        //            out_st.t0 = _v.t0() + boost::gregorian::weeks(out_vcoords_low[0] * _v.dt().dt_interval);
        //            out_st.t1 = _v.t0() + boost::gregorian::weeks(out_vcoords_high[0] * _v.dt().dt_interval);
        //        }
        //        else if (_v.dt().dt_unit ==datetime_unit::MONTH) {
        //            out_st.t0 = _v.t0() + boost::gregorian::months(out_vcoords_low[0] * _v.dt().dt_interval);
        //            out_st.t1 = _v.t0() + boost::gregorian::months(out_vcoords_high[0] * _v.dt_interval());
        //        }
        //        else if (_v.dt().dt_unit == datetime_unit::YEAR) {
        //            out_st.t0 = _v.t0() + boost::gregorian::years(out_vcoords_low[0] * _v.dt_interval());
        //            out_st.t1 = _v.t0() + boost::gregorian::years(out_vcoords_high[0] * _v.dt_interval());
        //        }
        //        else throw std::string("ERROR in default_chunking::bounds_from_chunk(): time-based resolution (hours, minutes, seconds) is not supported in gdalcubes");

        return out_st;
    }

   protected:
    cube_view _v;
    //image_collection _collection;
    uint32_t _size_x;
    uint32_t _size_y;
    uint32_t _size_t;
};

#endif  //CHUNKING_H
