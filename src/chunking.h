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

typedef double value_type;  // for now, chunk data is ALWAYS double! This reduces complexity for the first version.

/**
 * A chunk stores a spatial and temporal contiguous part of an image collection
 * as four-dimensional arrays: (band, datetime, y, x).
 * The format might change in case we decide that gdalcubes should work on the original (varying) band data types.
 */
class chunk_data {
   public:
    chunk_data() : _buf(nullptr), _size() {}

    ~chunk_data() {
        if (_buf) free(_buf);
    }

    // chunk_data(const chunk_data&) = delete;
    //  void operator=(const chunk_data&) = delete;
    //
    //    // move constructor
    //    chunk_data(chunk_data&& A) {
    //        // Taking over the resources
    //        A._size = _size;
    //        A._buf = _buf;
    //
    //        // Resetting *this to prevent freeing the data buffers in the destructor.
    //        _buf = nullptr;
    //        _size[0] = 0;
    //        _size[1] = 0;
    //        _size[2] = 0;
    //        _size[3] = 0;
    //    }
    //
    //    // move assignment
    //    chunk_data& operator=(chunk_data&& A) {
    //        _size = A._size;
    //        _buf = A._buf;
    //        A._buf = nullptr;
    //        A._size[0] = 0;
    //        A._size[1] = 0;
    //        A._size[2] = 0;
    //        A._size[3] = 0;
    //        return *this;
    //    }

    inline uint16_t count_bands() { return _size[0]; }
    inline uint32_t count_values() { return _size[1] * _size[2] * _size[3]; }
    uint64_t total_size_bytes() {
        return empty() ? 0 : sizeof(value_type) * _size[0] * _size[1] * _size[2] * _size[3];
    }

    inline bool empty() {
        if (_buf) return false;
        return true;
    }

    /** These methods are dangerous and provide direct access to the data buffers, use with caution and never free any memory /
     * remove / add vector elements if you don't know exactly what you do.
     * @return
     */
    inline void* buf() { return _buf; }

    inline void buf(void* b) {
        if (_buf) free(_buf);
        _buf = b;
    }

    inline coords_nd<uint32_t, 4> size() { return _size; }
    inline void size(coords_nd<uint32_t, 4> s) { _size = s; }

   protected:
    void* _buf;
    coords_nd<uint32_t, 4> _size;
};

class chunking {
    friend class cube;

   public:
    chunking(cube* c) : _c(c) {}

    class chunk_iterator {
        friend class chunking;
        chunk_iterator& operator++() {
            ++_cur_id;
            return *this;
        }

        friend bool operator==(const chunk_iterator& lhs, const chunk_iterator& rhs) {
            return lhs._c == rhs._c && lhs._cur_id == rhs._cur_id;
        }
        friend bool operator!=(const chunk_iterator& lhs, const chunk_iterator& rhs) { return !(lhs == rhs); }

        chunk_data operator*(chunk_iterator l) {
            //return _c->read(_cur_id);
        }

       protected:
        chunk_iterator() : _cur_id(0) {}
        chunkid _cur_id;
        chunking* _c;
    };

    chunk_iterator begin() {
        chunk_iterator it;
        it._cur_id = 0;
        it._c = this;
        return it;
    }

    chunk_iterator end() {
        chunk_iterator it;
        it._cur_id = count_chunks();
        it._c = this;
        return it;
    }

    virtual std::shared_ptr<chunk_data> read(chunkid id) const = 0;
    virtual uint32_t count_chunks() const = 0;

   protected:
    cube* _c;
};

class default_chunking : public chunking {
   public:
    default_chunking(cube* c) : chunking(c), _size_x(256), _size_y(256), _size_t(16) {}

    inline uint32_t& size_x() { return _size_x; }
    inline uint32_t& size_y() { return _size_y; }
    inline uint32_t& size_t() { return _size_t; }

    std::shared_ptr<chunk_data> read(chunkid id) const override;

    uint32_t count_chunks() const override {
        return std::ceil((double)_c->view().nx() / (double)_size_x) * std::ceil((double)_c->view().ny() / (double)_size_y) * std::ceil((double)_c->view().nt() / (double)_size_t);
    }

    chunkid find_chunk_that_contains(coords_st p) const {
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

    bounds_nd<uint32_t, 3> chunk_limits(chunkid id) const {
        coords_nd<uint32_t, 3> out_vcoords_low;
        coords_nd<uint32_t, 3> out_vcoords_high;

        // Transform to global coordinates based on chunk id
        uint32_t cumprod = 1;
        int32_t n;

        n = (uint32_t)std::ceil(((double)(_c->view().nx())) / ((double)_size_x));
        out_vcoords_low[2] = (id / cumprod) % n;
        out_vcoords_low[2] *= _size_x;
        out_vcoords_high[2] = out_vcoords_low[2] + _size_x - 1;
        cumprod *= n;

        n = (uint32_t)std::ceil(((double)(_c->view().ny())) / ((double)_size_y));
        out_vcoords_low[1] = (id / cumprod) % n;
        out_vcoords_low[1] *= _size_y;
        out_vcoords_high[1] = out_vcoords_low[1] + _size_y - 1;
        cumprod *= n;

        n = (uint32_t)std::ceil(((double)(_c->view().nt())) / ((double)_size_t));
        out_vcoords_low[0] = (id / cumprod) % n;
        out_vcoords_low[0] *= _size_t;
        out_vcoords_high[0] = out_vcoords_low[0] + _size_t - 1;
        cumprod *= n;

        // Shrink to view
        if (out_vcoords_high[0] < 0)
            out_vcoords_high[0] = 0;
        else if (out_vcoords_high[0] >= _c->view().nt())
            out_vcoords_high[0] = _c->view().nt() - 1;
        if (out_vcoords_low[0] < 0)
            out_vcoords_low[0] = 0;
        else if (out_vcoords_low[0] >= _c->view().nt())
            out_vcoords_low[0] = _c->view().nt() - 1;

        if (out_vcoords_high[1] < 0)
            out_vcoords_high[1] = 0;
        else if (out_vcoords_high[1] >= _c->view().ny())
            out_vcoords_high[1] = _c->view().ny() - 1;
        if (out_vcoords_low[1] < 0)
            out_vcoords_low[1] = 0;
        else if (out_vcoords_low[1] >= _c->view().ny())
            out_vcoords_low[1] = _c->view().ny() - 1;

        if (out_vcoords_high[2] < 0)
            out_vcoords_high[2] = 0;
        else if (out_vcoords_high[2] >= _c->view().nx())
            out_vcoords_high[2] = _c->view().nx() - 1;
        if (out_vcoords_low[2] < 0)
            out_vcoords_low[2] = 0;
        else if (out_vcoords_low[2] >= _c->view().nx())
            out_vcoords_low[2] = _c->view().nx() - 1;

        bounds_nd<uint32_t, 3> out;
        out.low = out_vcoords_low;
        out.high = out_vcoords_high;
        return out;
    };

    /**
     * Derive the true size of a specific chunk. The size may be different
     * at the boundary regions of the view.
     * @return
     */
    coords_nd<uint32_t, 3> chunk_size(chunkid id) const {
        bounds_nd<uint32_t, 3> vb = chunk_limits(id);
        coords_nd<uint32_t, 3> out;
        out[0] = vb.high[0] - vb.low[0] + 1;
        out[1] = vb.high[1] - vb.low[1] + 1;
        out[2] = vb.high[2] - vb.low[2] + 1;
        return out;
    };

    bounds_st bounds_from_chunk(chunkid id) const {
        bounds_st out_st;

        bounds_nd<uint32_t, 3> vb = chunk_limits(id);

        coords_st low = _c->view().map_coords(vb.low);
        vb.high[0] += 1;
        vb.high[1] += 1;
        vb.high[2] += 1;
        coords_st high = _c->view().map_coords(vb.high);

        out_st.s.left = low.s.x;
        out_st.s.right = high.s.x;
        out_st.s.bottom = low.s.y;
        out_st.s.top = high.s.y;
        out_st.t0 = low.t;
        out_st.t1 = high.t;

        return out_st;
    }

   protected:
    uint32_t _size_x;
    uint32_t _size_y;
    uint32_t _size_t;
};

#endif  //CHUNKING_H
