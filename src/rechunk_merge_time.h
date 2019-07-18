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
#ifndef RECHUNK_MERGE_TIME_H
#define RECHUNK_MERGE_TIME_H

#include "cube.h"

namespace gdalcubes {

/**
* @brief A data cube that merges chunks over time, i.e. the
 * result cube will contain the same data but with larger chunks that contain complete time series
*/

class rechunk_merge_time_cube : public cube {
   public:
    /**
        * @brief Create a data cube merges chunks of an input cube over time
        * @note This static creation method should preferably be used instead of the constructors as
        * the constructors will not set connections between cubes properly.
        * @param in input data cube
        * @param reducer reducer function
        * @return a shared pointer to the created data cube instance
        */
    static std::shared_ptr<rechunk_merge_time_cube> create(std::shared_ptr<cube> in) {
        std::shared_ptr<rechunk_merge_time_cube> out = std::make_shared<rechunk_merge_time_cube>(in);
        in->add_child_cube(out);
        out->add_parent_cube(in);
        return out;
    }

   public:
    rechunk_merge_time_cube(std::shared_ptr<cube> in) : cube(std::make_shared<cube_st_reference>(*(in->st_reference()))), _in_cube(in) {  // it is important to duplicate st reference here, otherwise changes will affect input cube as well
        _st_ref->dt((_st_ref->t1() - _st_ref->t0()) + 1);
        _st_ref->t1() = _st_ref->t0();  // set nt=1
        _chunk_size[0] = _in_cube->size_t();
        _chunk_size[1] = _in_cube->chunk_size()[1];
        _chunk_size[2] = _in_cube->chunk_size()[2];

        for (uint16_t i = 0; i < _in_cube->size_bands(); ++i) {
            _bands.add(_in_cube->bands().get(i));
        }
    }

   public:
    ~rechunk_merge_time_cube() {}

    std::shared_ptr<chunk_data> read_chunk(chunkid_t id) override;

    nlohmann::json make_constructible_json() override {
        nlohmann::json out;
        out["cube_type"] = "rechunk_merge_time";
        out["in_cube"] = _in_cube->make_constructible_json();
        return out;
    }

   private:
    std::shared_ptr<cube> _in_cube;

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

#endif  //RECHUNK_MERGE_TIME_H
