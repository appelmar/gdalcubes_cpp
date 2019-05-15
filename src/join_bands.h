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

#ifndef JOIN_BANDS_H
#define JOIN_BANDS_H

#include "cube.h"

/**
 * @brief A data cube that combines the bands of two identically-shaped data cubes
 */
class join_bands_cube : public cube {
   public:
    /**
     * @brief Create a data cube that combines the bands of two identically-shaped data cubes
     * @note This static creation method should preferably be used instead of the constructors as
     * the constructors will not set connections between cubes properly.
     * @param A input data cube
     * @param B input data cube
     * @return a shared pointer to the created data cube instance
     */
    static std::shared_ptr<join_bands_cube> create(std::shared_ptr<cube> A, std::shared_ptr<cube> B, std::string prefix_A = "A", std::string prefix_B = "B") {
        std::shared_ptr<join_bands_cube> out = std::make_shared<join_bands_cube>(A, B, prefix_A, prefix_B);
        A->add_child_cube(out);
        B->add_child_cube(out);
        out->add_parent_cube(A);
        out->add_parent_cube(B);
        return out;
    }

   public:
    join_bands_cube(std::shared_ptr<cube> A, std::shared_ptr<cube> B, std::string prefix_A = "A", std::string prefix_B = "B") : cube(), _in_A(A), _in_B(B), _prefix_A(prefix_A), _prefix_B(prefix_B) {
        _st_ref = std::make_shared<cube_st_reference>();

        // Check that A and B have identical shape
        if (*(_in_A->st_reference()) != *(_in_B->st_reference())) {
            throw std::string("ERROR in join_bands_cube::join_bands_cube(): Cubes have different shape");
        }

        if (!(_in_A->chunk_size()[0] == _in_B->chunk_size()[0] &&
              _in_A->chunk_size()[1] == _in_B->chunk_size()[1] &&
              _in_A->chunk_size()[2] == _in_B->chunk_size()[2])) {
            throw std::string("ERROR in join_bands_cube::join_bands_cube(): Cubes have different chunk sizes");
        }

        if (_prefix_A.empty() && _prefix_B.empty()) {
            // check that there are no name conflicts
            for (uint16_t iba = 0; iba < _in_A->bands().count(); ++iba) {
                if (_in_B->bands().has(_in_A->bands().get(iba).name)) {
                    GCBS_ERROR("cubes have bands with identical names");
                    throw std::string("ERROR in join_bands_cube::join_bands_cube(): Cubes have bands with identical names");
                }
            }
        } else {
            if (_prefix_A == _prefix_B) {
                GCBS_ERROR("cannot join cubes with identical prefix");
                throw std::string("ERROR in join_bands_cube::join_bands_cube(): Cannot join cubes with identical prefix");
            }
        }

        _st_ref->win() = _in_A->st_reference()->win();
        _st_ref->srs() = _in_A->st_reference()->srs();
        _st_ref->ny() = _in_A->st_reference()->ny();
        _st_ref->nx() = _in_A->st_reference()->nx();
        _st_ref->t0() = _in_A->st_reference()->t0();
        _st_ref->t1() = _in_A->st_reference()->t1();
        _st_ref->dt(_in_A->st_reference()->dt());

        _chunk_size[0] = _in_A->chunk_size()[0];
        _chunk_size[1] = _in_A->chunk_size()[1];
        _chunk_size[2] = _in_A->chunk_size()[2];

        for (uint16_t ib = 0; ib < _in_A->bands().count(); ++ib) {
            band b = _in_A->bands().get(ib);
            b.name = _prefix_A.empty() ? (b.name) : (_prefix_A + "." + b.name);
            _bands.add(b);
        }
        for (uint16_t ib = 0; ib < _in_B->bands().count(); ++ib) {
            band b = _in_B->bands().get(ib);
            b.name = _prefix_B.empty() ? (b.name) : (_prefix_B + "." + b.name);
            _bands.add(b);
        }
    }

   public:
    ~join_bands_cube() {}

    std::shared_ptr<chunk_data> read_chunk(chunkid_t id) override;

    nlohmann::json make_constructible_json() override {
        nlohmann::json out;
        out["cube_type"] = "join_bands";
        out["A"] = _in_A->make_constructible_json();
        out["B"] = _in_B->make_constructible_json();
        out["prefix_A"] = _prefix_A;
        out["prefix_B"] = _prefix_B;
        return out;
    }

   private:
    std::shared_ptr<cube> _in_A;
    std::shared_ptr<cube> _in_B;
    std::string _prefix_A;
    std::string _prefix_B;

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

#endif  // JOIN_BANDS_H
