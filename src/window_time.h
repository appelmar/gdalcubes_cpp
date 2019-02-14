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

#ifndef WINDOW_TIME_H
#define WINDOW_TIME_H

#include "cube.h"

// TODO: add custom reducer function
// TODO: add kernel reducer function
// TODO: add boundary fill options (e.g. NA, wrap, repeat, mirror, ...)
/**
 * @brief A data cube that applies reducer functions over selected bands of a data cube over time
 * @note This is a reimplementation of reduce_cube. The new implementation allows to apply different reducers to different bands instead of just one reducer to all bands of the input data cube
 */
class window_time_cube : public cube {
   public:
    /**
     * @brief Create a data cube that applies a reducer function on a given input data cube over time
     * @note This static creation method should preferably be used instead of the constructors as
     * the constructors will not set connections between cubes properly.
     * @param in input data cube
     * @param reducer reducer function
     * @return a shared pointer to the created data cube instance
     */
    static std::shared_ptr<window_time_cube> create(std::shared_ptr<cube> in, std::vector<std::pair<std::string, std::string>> reducer_bands, uint16_t win_size_l, uint16_t win_size_r) {
        std::shared_ptr<window_time_cube> out = std::make_shared<window_time_cube>(in, reducer_bands, win_size_l, win_size_r);
        in->add_child_cube(out);
        out->add_parent_cube(in);
        return out;
    }

    /**
    * @brief Create a data cube that applies a convolution kernel on a given input data cube over time
    * @note This static creation method should preferably be used instead of the constructors as
    * the constructors will not set connections between cubes properly.
    * @param in input data cube
    * @param reducer reducer function
    * @return a shared pointer to the created data cube instance
    */
    static std::shared_ptr<window_time_cube> create(std::shared_ptr<cube> in, std::vector<double> kernel, uint16_t win_size_l, uint16_t win_size_r) {
        std::shared_ptr<window_time_cube> out = std::make_shared<window_time_cube>(in, kernel, win_size_l, win_size_r);
        in->add_child_cube(out);
        out->add_parent_cube(in);
        return out;
    }

   public:
    window_time_cube(std::shared_ptr<cube> in, std::vector<std::pair<std::string, std::string>> reducer_bands, uint16_t win_size_l, uint16_t win_size_r) : cube(std::make_shared<cube_st_reference>(*(in->st_reference()))), _in_cube(in), _reducer_bands(reducer_bands), _win_size_l(win_size_l), _win_size_r(win_size_r), _f(), _band_idx_in(), _kernel() {  // it is important to duplicate st reference here, otherwise changes will affect input cube as well
        _chunk_size[0] = _in_cube->chunk_size()[0];
        _chunk_size[1] = _in_cube->chunk_size()[1];
        _chunk_size[2] = _in_cube->chunk_size()[2];

        for (uint16_t i = 0; i < reducer_bands.size(); ++i) {
            std::string reducerstr = reducer_bands[i].first;
            std::string bandstr = reducer_bands[i].second;
            _f.push_back(get_default_reducer_by_name(reducerstr));

            band b = in->bands().get(bandstr);
            b.name = b.name + "_" + reducerstr;
            _bands.add(b);

            // TODO: test whether index returned here is really equal to the index in the chunk storage
            _band_idx_in.push_back(in->bands().get_index(bandstr));
        }
    }

    window_time_cube(std::shared_ptr<cube> in, std::vector<double> kernel, uint16_t win_size_l, uint16_t win_size_r) : cube(std::make_shared<cube_st_reference>(*(in->st_reference()))), _in_cube(in), _reducer_bands(), _win_size_l(win_size_l), _win_size_r(win_size_r), _f(), _band_idx_in(), _kernel(kernel) {  // it is important to duplicate st reference here, otherwise changes will affect input cube as well
        _chunk_size[0] = _in_cube->chunk_size()[0];
        _chunk_size[1] = _in_cube->chunk_size()[1];
        _chunk_size[2] = _in_cube->chunk_size()[2];

        if (!win_size_l + (uint32_t)1 + win_size_r == kernel.size()) {
            GCBS_ERROR("kernel size does not match window size");
            throw std::string("ERROR in window_time_cube::window_time_cube(): Kernel size does not match window size");
        }

        for (uint16_t i = 0; i < in->bands().count(); ++i) {
            _f.push_back(get_kernel_reducer(kernel));

            band b = in->bands().get(i);
            ;
            _bands.add(b);
        }
    }

   public:
    ~window_time_cube() {}

    std::shared_ptr<chunk_data> read_chunk(chunkid_t id) override;

    std::shared_ptr<chunk_data> read_chunk_reducerfunc(chunkid_t id);
    std::shared_ptr<chunk_data> read_chunk_kernelfunc(chunkid_t id);

    nlohmann::json make_constructible_json() override {
        nlohmann::json out;
        out["cube_type"] = "window_time";
        if (!_kernel.empty()) {
            out["kernel"] = _kernel;
        } else {
            out["reducer_bands"] = _reducer_bands;
        }
        out["win_size_l"] = _win_size_l;
        out["win_size_r"] = _win_size_r;
        out["in_cube"] = _in_cube->make_constructible_json();
        return out;
    }

   private:
    std::shared_ptr<cube> _in_cube;
    std::vector<std::pair<std::string, std::string>> _reducer_bands;
    uint16_t _win_size_l;
    uint16_t _win_size_r;
    std::vector<std::function<double(double* buf, uint16_t n)>> _f;
    std::vector<uint16_t> _band_idx_in;
    std::vector<double> _kernel;

    virtual void set_st_reference(std::shared_ptr<cube_st_reference> stref) override {
        // copy fields from st_reference type
        _st_ref->win() = stref->win();
        _st_ref->srs() = stref->srs();
        _st_ref->ny() = stref->ny();
        _st_ref->nx() = stref->nx();
        _st_ref->t0() = stref->t0();
        _st_ref->t1() = stref->t1();
        _st_ref->dt() = stref->dt();
    }

    std::function<double(double* buf, uint16_t n)> get_default_reducer_by_name(std::string name);
    std::function<double(double* buf, uint16_t n)> get_kernel_reducer(std::vector<double> kernel);
};

#endif  // WINDOW_TIME_H
