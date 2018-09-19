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


#include "cube.h"



typedef uint32_t chunkid;


/**
 * A chunk stores a spatial and temporal contiguous part of an image collection
 * as vector of three-dimensional arrays: (band) -> (datetime, y, x).
 * @todo add bitmap for correctly handling nodata values
 */
class chunk_data {

public:

    chunk_data() : _v(), _type(), _n(0) {}

    ~chunk_data() {
        for (uint16_t i=0; i<_v.size(); ++i) {
            if (_v[i]) free(_v[i]);
        }
    }


    chunk_data(const chunk_data&) = delete;
    void operator=(const chunk_data&) = delete;


    // move constructor
    chunk_data(chunk_data&& A) {

        // Taking over the resources
        A._type = _type;
        A._n = _n;
        A._v = _v;

        // Resetting *this to prevent freeing the data buffers in the destructor.
        _v.clear();
        _n=0;
        _type.clear();
    }


    void* read(uint16_t band);
    GDALDataType type(uint16_t bands);

    uint16_t count_bands() {return _type.size();}
    uint16_t count_values() {return _n;}
    uint64_t size_bytes(uint16_t band) {
        return GDALGetDataTypeSizeBytes(_type[band])*_n;
    }
    uint64_t size_bytes() {
        uint64_t size=0;
        for (uint16_t i=0; i<count_bands(); ++i) {
            size += GDALGetDataTypeSizeBytes(_type[i])*_n;
        }
        return size;
    }


    inline bool empty() {
        return(_v.empty());
    }

protected:
    std::vector<void*> _v;
    uint32_t _n; // number of values per element in _v;
    std::vector<GDALDataType> _type; // implies the size of values
};


//class chunking {
//   public:
//    class chunk_iterator;
//};

class default_chunking  {
   public:
    default_chunking(cube* c) : _c(c), _size_x(256), _size_y(256), _size_t(16) {
    }

    inline uint32_t& size_x() { return _size_x; }
    inline uint32_t& size_y() { return _size_y; }
    inline uint32_t& size_t() { return _size_t; }



    chunk_data read(chunkid id);


    inline uint32_t count_chunks() {
        return std::ceil((double)_c->view().nx() / (double)_size_x) * std::ceil((double)_c->view().ny() / (double)_size_y) * std::ceil((double)_c->view().nt() / (double)_size_t);
    }

    chunkid find_chunk_that_contains(coords_st p) {
        uint32_t cumprod = 1;
        chunkid id = 0;

        // 1. Convert map coordinates to view-based coordinates
        coords_nd<uint32_t, 3> s = _c->view().view_coords(p);

        // 2. Find out which chunk contains the integer view coordinates
        id += cumprod * (s[2] / _size_x);
        cumprod *= (uint32_t)std::ceil(((double)_c->view().nx()) / ((double)_size_x));
        id += cumprod * (s[1] / _size_y);
        cumprod *= (uint32_t)std::ceil(((double)(_c->view().ny()) / ((double)_size_y)));
        id += cumprod * (s[0] / _size_t);
        //cumprod *= (uint32_t)std::ceil(((double)(_c->view().nt()) / ((double)_size_t);

        return id;
    }

    bounds_st bounds_from_chunk(chunkid id) {
        coords_nd<uint32_t, 3> out_vcoords_low;
        coords_nd<uint32_t, 3> out_vcoords_high;
        bounds_st out_st;

        // Transform to global coordinates based on chunk id
        uint32_t cumprod = 1;
        int32_t n;

        n = (uint32_t)std::ceil(((double)(_c->view().nx())) / ((double)_size_x));
        out_vcoords_low[2] = (id / cumprod) % n;
        out_vcoords_low[2] *= _size_x;
        out_vcoords_high[2] = out_vcoords_low[2] + _size_x;
        cumprod *= n;

        n = (uint32_t)std::ceil(((double)(_c->view().ny())) / ((double)_size_y));
        out_vcoords_low[1] = (id / cumprod) % n;
        out_vcoords_low[1] *= _size_y;
        out_vcoords_high[1] = out_vcoords_low[1] + _size_y;
        cumprod *= n;

        n = (uint32_t)std::ceil(((double)(_c->view().nt())) / ((double)_size_t));
        out_vcoords_low[0] = (id / cumprod) % n;
        out_vcoords_low[0] *= _size_t;
        out_vcoords_high[0] = out_vcoords_low[0] + _size_t;
        cumprod *= n;

        // Shrink to view
        if (out_vcoords_high[0] < 0)
            out_vcoords_high[0] = 0;
        else if (out_vcoords_high[0] > _c->view().nt())
            out_vcoords_high[0] = _c->view().nt();
        if (out_vcoords_low[0] < 0)
            out_vcoords_low[0] = 0;
        else if (out_vcoords_low[0] > _c->view().nt())
            out_vcoords_low[0] = _c->view().nt();

        if (out_vcoords_high[1] < 0)
            out_vcoords_high[1] = 0;
        else if (out_vcoords_high[1] > _c->view().ny())
            out_vcoords_high[1] = _c->view().ny();
        if (out_vcoords_low[1] < 0)
            out_vcoords_low[1] = 0;
        else if (out_vcoords_low[1] > _c->view().ny())
            out_vcoords_low[1] = _c->view().ny();

        if (out_vcoords_high[2] < 0)
            out_vcoords_high[2] = 0;
        else if (out_vcoords_high[2] > _c->view().nx())
            out_vcoords_high[2] = _c->view().nx();
        if (out_vcoords_low[2] < 0)
            out_vcoords_low[2] = 0;
        else if (out_vcoords_low[2] > _c->view().nx())
            out_vcoords_low[2] = _c->view().nx();


        coords_st low = _c->view().map_coords(out_vcoords_low);
        coords_st high = _c->view().map_coords(out_vcoords_high);

        out_st.s.left = low.s.x;
        out_st.s.right = high.s.x;
        out_st.s.bottom = low.s.y;
        out_st.s.top = high.s.y;
        out_st.t0 = low.t;
        out_st.t1 = high.t;

        return out_st;
    }

   protected:
     cube* const _c;
    uint32_t _size_x;
    uint32_t _size_y;
    uint32_t _size_t;
};

#endif  //CHUNKING_H
