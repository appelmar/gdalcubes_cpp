/*
    MIT License

    Copyright (c) 2019 Marius Appel <marius.appel@uni-muenster.de>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#ifndef DUMMY_H
#define DUMMY_H

#include "cube.h"

namespace gdalcubes {

/**
     * @brief A dummy data cube with n bands and a simple fill value
     */
class dummy_cube : public cube {
   public:
    /**
         * @brief Create a dummy data cube with a simple fill value
         * @note This static creation method should preferably be used instead of the constructors as
         * the constructors will not set connections between cubes properly.
         * @param v shape of the cube
         * @param nbands number of bands
         * @param fill fill value
         * @return a shared pointer to the created data cube instance
         */
    static std::shared_ptr<dummy_cube> create(cube_view v, uint16_t nbands = 1, double fill = 1.0) {
        std::shared_ptr<dummy_cube> out = std::make_shared<dummy_cube>(v, nbands, fill);
        return out;
    }

   public:
    dummy_cube(cube_view v, uint16_t nbands = 1, double fill = 1.0) : cube(std::make_shared<cube_view>(v)),
                                                                      _fill(fill) {
        for (uint16_t ib = 0; ib < nbands; ++ib) {
            band b("band" + std::to_string(ib + 1));
            b.scale = 1.0;
            b.offset = 0.0;
            _bands.add(b);
        }
    }

    void set_chunk_size(uint32_t t, uint32_t y, uint32_t x) {
        _chunk_size = {t, y, x};
    }

   public:
    ~dummy_cube() {}

    std::shared_ptr<chunk_data> read_chunk(chunkid_t id) override;

    nlohmann::json make_constructible_json() override {
        nlohmann::json out;
        out["cube_type"] = "dummy";
        out["view"] = nlohmann::json::parse(std::dynamic_pointer_cast<cube_view>(_st_ref)->write_json_string());
        out["fill"] = _fill;
        out["chunk_size"] = _chunk_size;
        return out;
    }

   private:
    double _fill;

    virtual void set_st_reference(std::shared_ptr<cube_st_reference> stref) override {
        // copy fields from st_reference type
        _st_ref->win() = stref->win();
        _st_ref->srs() = stref->srs();
        _st_ref->ny() = stref->ny();
        _st_ref->nx() = stref->nx();
        _st_ref->t0() = stref->t0();
        _st_ref->t1() = stref->t1();
        _st_ref->dt(stref->dt());
    }
};
}  // namespace gdalcubes

#endif  //DUMMY_H
