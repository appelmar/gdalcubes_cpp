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

#ifndef CUBE_H
#define CUBE_H

#include <mutex>
#include "view.h"

typedef std::array<uint32_t, 4> cube_coordinate_btyx;
typedef std::array<uint32_t, 3> cube_coordinate_tyx;
typedef std::array<uint32_t, 4> cube_size_btyx;
typedef std::array<uint32_t, 3> cube_size_tyx;
typedef std::array<uint32_t, 4> chunk_coordinate_btyx;
typedef std::array<uint32_t, 3> chunk_coordinate_tyx;
typedef std::array<uint32_t, 4> chunk_size_btyx;
typedef std::array<uint32_t, 3> chunk_size_tyx;

typedef uint32_t chunkid_t;

struct band {
    band(std::string name) : name(name), no_data_value(NAN), offset(0), scale(1), unit(""), type("float64") {}
    std::string name;
    double no_data_value;
    double offset;
    double scale;
    std::string unit;
    std::string type;
};

class band_collection {
   public:
    void add(band b) {
        if (!has(b.name)) {
            _bands.push_back(b);
            _band_idx[b.name] = _bands.size() - 1;
        }
    }

    inline bool has(std::string name) {
        return _band_idx.find(name) != _band_idx.end();
    }

    inline band get(std::string name) {
        return _bands[_band_idx[name]];
    }

    inline uint32_t get_index(std::string name) {
        return _band_idx[name];
    }

    inline band get(uint32_t i) {
        return _bands[i];
    }

    inline uint32_t count() {
        return _bands.size();
    }

   protected:
    std::map<std::string, uint32_t> _band_idx;
    std::vector<band> _bands;
};

class chunk_data {
   public:
    chunk_data() : _buf(nullptr), _size() {}

    ~chunk_data() {
        if (_buf) free(_buf);
    }

    inline uint16_t count_bands() { return _size[0]; }
    inline uint32_t count_values() { return _size[1] * _size[2] * _size[3]; }
    uint64_t total_size_bytes() {
        return empty() ? 0 : sizeof(double) * _size[0] * _size[1] * _size[2] * _size[3];
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
    chunk_size_btyx _size;
};

class cube {
   public:
    cube(std::shared_ptr<cube_st_reference> st_ref) : _st_ref(st_ref), _chunk_size(), _nthreads(1), _size() {
        _size[0] = 0;
        _size[1] = st_ref->nt();
        _size[2] = st_ref->ny();
        _size[3] = st_ref->nx();

        _chunk_size = {16, 256, 256};

        // TODO: add bands
    }

    virtual ~cube() {}

    chunkid_t find_chunk_that_contains(coords_st p) const {
        uint32_t cumprod = 1;
        chunkid_t id = 0;

        // 1. Convert map coordinates to view-based coordinates
        coords_nd<uint32_t, 3> s = _st_ref->cube_coords(p);

        // 2. Find out which chunk contains the integer view coordinates
        id += cumprod * (s[2] / _chunk_size[2]);
        cumprod *= (uint32_t)std::ceil(((double)_st_ref->nx()) / ((double)_chunk_size[2]));
        id += cumprod * (s[1] / _chunk_size[1]);
        cumprod *= (uint32_t)std::ceil(((double)(_st_ref->ny()) / ((double)_chunk_size[1])));
        id += cumprod * (s[0] / _chunk_size[0]);
        //cumprod *= (uint32_t)std::ceil(((double)(_c->view().nt()) / ((double)_size_t);

        return id;
    }

    bounds_nd<uint32_t, 3> chunk_limits(chunk_coordinate_tyx c) const {
        cube_coordinate_tyx out_vcoords_low;
        cube_coordinate_tyx out_vcoords_high;

        out_vcoords_low[0] = c[0] * _chunk_size[0];
        out_vcoords_low[1] = c[1] * _chunk_size[1];
        out_vcoords_low[2] = c[2] * _chunk_size[2];
        out_vcoords_high[0] = out_vcoords_low[0] + _chunk_size[0] - 1;
        out_vcoords_high[1] = out_vcoords_low[1] + _chunk_size[1] - 1;
        out_vcoords_high[2] = out_vcoords_low[2] + _chunk_size[2] - 1;

        // Shrink to view
        if (out_vcoords_high[0] < 0)
            out_vcoords_high[0] = 0;
        else if (out_vcoords_high[0] >= _st_ref->nt())
            out_vcoords_high[0] = _st_ref->nt() - 1;
        if (out_vcoords_low[0] < 0)
            out_vcoords_low[0] = 0;
        else if (out_vcoords_low[0] >= _st_ref->nt())
            out_vcoords_low[0] = _st_ref->nt() - 1;

        if (out_vcoords_high[1] < 0)
            out_vcoords_high[1] = 0;
        else if (out_vcoords_high[1] >= _st_ref->ny())
            out_vcoords_high[1] = _st_ref->ny() - 1;
        if (out_vcoords_low[1] < 0)
            out_vcoords_low[1] = 0;
        else if (out_vcoords_low[1] >= _st_ref->ny())
            out_vcoords_low[1] = _st_ref->ny() - 1;

        if (out_vcoords_high[2] < 0)
            out_vcoords_high[2] = 0;
        else if (out_vcoords_high[2] >= _st_ref->nx())
            out_vcoords_high[2] = _st_ref->nx() - 1;
        if (out_vcoords_low[2] < 0)
            out_vcoords_low[2] = 0;
        else if (out_vcoords_low[2] >= _st_ref->nx())
            out_vcoords_low[2] = _st_ref->nx() - 1;

        bounds_nd<uint32_t, 3> out;
        out.low = out_vcoords_low;
        out.high = out_vcoords_high;
        return out;
    };

    bounds_nd<uint32_t, 3> chunk_limits(chunkid_t id) const {
        coords_nd<uint32_t, 3> out_vcoords_low;
        coords_nd<uint32_t, 3> out_vcoords_high;

        // Transform to global coordinates based on chunk id
        uint32_t cumprod = 1;
        int32_t n;

        n = (uint32_t)std::ceil(((double)(_st_ref->nx())) / ((double)_chunk_size[2]));
        out_vcoords_low[2] = (id / cumprod) % n;
        out_vcoords_low[2] *= _chunk_size[2];
        out_vcoords_high[2] = out_vcoords_low[2] + _chunk_size[2] - 1;
        cumprod *= n;

        n = (uint32_t)std::ceil(((double)(_st_ref->ny())) / ((double)_chunk_size[1]));
        out_vcoords_low[1] = (id / cumprod) % n;
        out_vcoords_low[1] *= _chunk_size[1];
        out_vcoords_high[1] = out_vcoords_low[1] + _chunk_size[1] - 1;
        cumprod *= n;

        n = (uint32_t)std::ceil(((double)(_st_ref->nt())) / ((double)_chunk_size[0]));
        out_vcoords_low[0] = (id / cumprod) % n;
        out_vcoords_low[0] *= _chunk_size[0];
        out_vcoords_high[0] = out_vcoords_low[0] + _chunk_size[0] - 1;
        cumprod *= n;

        // Shrink to view
        if (out_vcoords_high[0] < 0)
            out_vcoords_high[0] = 0;
        else if (out_vcoords_high[0] >= _st_ref->nt())
            out_vcoords_high[0] = _st_ref->nt() - 1;
        if (out_vcoords_low[0] < 0)
            out_vcoords_low[0] = 0;
        else if (out_vcoords_low[0] >= _st_ref->nt())
            out_vcoords_low[0] = _st_ref->nt() - 1;

        if (out_vcoords_high[1] < 0)
            out_vcoords_high[1] = 0;
        else if (out_vcoords_high[1] >= _st_ref->ny())
            out_vcoords_high[1] = _st_ref->ny() - 1;
        if (out_vcoords_low[1] < 0)
            out_vcoords_low[1] = 0;
        else if (out_vcoords_low[1] >= _st_ref->ny())
            out_vcoords_low[1] = _st_ref->ny() - 1;

        if (out_vcoords_high[2] < 0)
            out_vcoords_high[2] = 0;
        else if (out_vcoords_high[2] >= _st_ref->nx())
            out_vcoords_high[2] = _st_ref->nx() - 1;
        if (out_vcoords_low[2] < 0)
            out_vcoords_low[2] = 0;
        else if (out_vcoords_low[2] >= _st_ref->nx())
            out_vcoords_low[2] = _st_ref->nx() - 1;

        bounds_nd<uint32_t, 3> out;
        out.low = out_vcoords_low;
        out.high = out_vcoords_high;
        return out;
    };

    virtual std::string to_string() {
        // TODO: implement default to_string method here

        return std::string("cube::to_string() has no default method yet.");
    }

    inline uint32_t count_chunks() const {
        return count_chunks_x() * count_chunks_y() * count_chunks_t();
    }

    inline uint32_t count_chunks_x() const {
        return std::ceil((double)_st_ref->nx() / (double)_chunk_size[2]);
    }
    inline uint32_t count_chunks_y() const {
        return std::ceil((double)_st_ref->ny() / (double)_chunk_size[1]);
    }
    inline uint32_t count_chunks_t() const {
        return std::ceil((double)_st_ref->nt() / (double)_chunk_size[0]);
    }

    chunk_coordinate_tyx chunk_coords_from_id(chunkid_t id) {
        chunk_coordinate_tyx out;

        uint32_t cumprod = 1;
        int32_t n;

        n = count_chunks_x();
        out[2] = (id / cumprod) % n;
        cumprod *= n;
        n = count_chunks_y();
        out[1] = (id / cumprod) % n;
        cumprod *= n;
        n = count_chunks_t();
        out[0] = (id / cumprod) % n;
        cumprod *= n;

        return out;
    }

    chunkid_t chunk_id_from_coords(chunk_coordinate_tyx c) {
        return c[0] * count_chunks_y() * count_chunks_x() + c[1] * count_chunks_x() + c[2];
    }

    cube_coordinate_btyx low() const {
        return {0, 0, 0, 0};
    }
    cube_coordinate_btyx high() const {
        return {_size[0] - 1, _size[1] - 1, _size[2] - 1, _size[3] - 1};
    }

    /**
     * Derive the true size of a specific chunk. The size may be different
     * at the boundary regions of the view.
     * @return
     */
    coords_nd<uint32_t, 3> chunk_size(chunkid_t id) const {
        bounds_nd<uint32_t, 3> vb = chunk_limits(id);
        coords_nd<uint32_t, 3> out;
        out[0] = vb.high[0] - vb.low[0] + 1;
        out[1] = vb.high[1] - vb.low[1] + 1;
        out[2] = vb.high[2] - vb.low[2] + 1;
        return out;
    };

    bounds_st bounds_from_chunk(chunkid_t id) const {
        bounds_st out_st;

        bounds_nd<uint32_t, 3> vb = chunk_limits(id);

        coords_st low = _st_ref->map_coords(vb.low);
        vb.high[0] += 1;
        vb.high[1] += 1;
        vb.high[2] += 1;
        coords_st high = _st_ref->map_coords(vb.high);

        out_st.s.left = low.s.x;
        out_st.s.right = high.s.x;
        out_st.s.bottom = low.s.y;
        out_st.s.top = high.s.y;
        out_st.t0 = low.t;
        out_st.t1 = high.t;

        return out_st;
    }

    inline cube_size_tyx chunk_size() { return _chunk_size; }
    inline void chunk_size(cube_size_tyx size) { _chunk_size = size; }
    inline cube_st_reference st_reference() { return *_st_ref; }
    inline void st_reference(cube_st_reference* st_ref) {
        _st_ref = std::make_shared<cube_st_reference>(*st_ref);
        _size[1] = st_ref->nt();
        _size[2] = st_ref->ny();
        _size[3] = st_ref->nx();
    }

    inline cube_size_btyx size() { return _size; }

    inline uint32_t size_bands() { return _size[0]; }
    inline uint32_t size_t() { return _size[1]; }
    inline uint32_t size_y() { return _size[2]; }
    inline uint32_t size_x() { return _size[3]; }

    virtual std::shared_ptr<chunk_data> read_chunk(chunkid_t id) = 0;

    void write_gtiff_directory(std::string dir);
    void write_netcdf_directory(std::string dir);

    inline band_collection bands() {
        return _bands;
    }

    void apply(std::function<void(chunkid_t, std::shared_ptr<chunk_data>, std::mutex&)> f, uint16_t nthreads = 1);

    inline void set_threads(uint16_t n) { _nthreads = n; }
    inline uint16_t get_threads() { return _nthreads; }

    virtual nlohmann::json make_constructible_json() = 0;

   protected:
    std::shared_ptr<cube_st_reference> _st_ref;
    cube_size_btyx _size;
    cube_size_tyx _chunk_size;

    band_collection _bands;
    uint16_t _nthreads;
};

#endif  //CUBE_H
