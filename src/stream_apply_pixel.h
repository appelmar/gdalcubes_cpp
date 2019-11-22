

#ifndef STREAM_APPLY_PIXEL_H
#define STREAM_APPLY_PIXEL_H

#include "cube.h"

namespace gdalcubes {

/**
* @brief A data cube that applies a user-defined function over pixel time-series
*/

class stream_apply_pixel_cube : public cube {
   public:
    /**
        * @brief Create a data cube that applies a user-defined function independently on all pixels
        * @note This static creation method should preferably be used instead of the constructors as
        * the constructors will not set connections between cubes properly.
        * @param in input data cube
        * @param cmd external program call
        * @param nbands number of new bands in the output cube
        * @param names string vector of output band names, must have the size nbands or empty (the default). If empty,
        * output bands will be named "band1", "band2", ...
        * @param keep_bands if true, bands will be added to the existing bands of the input cube, otherwise (default) they are dropped
        * @return a shared pointer to the created data cube instance
        */
    static std::shared_ptr<stream_apply_pixel_cube>
    create(std::shared_ptr<cube> in, std::string cmd, uint16_t nbands,
           std::vector<std::string> names = std::vector<std::string>(), bool keep_bands = false) {
        std::shared_ptr<stream_apply_pixel_cube> out = std::make_shared<stream_apply_pixel_cube>(in, cmd, nbands,
                                                                                                 names, keep_bands);
        in->add_child_cube(out);
        out->add_parent_cube(in);
        return out;
    }

   public:
    stream_apply_pixel_cube(std::shared_ptr<cube> in, std::string cmd, uint16_t nbands,
                            std::vector<std::string> names = std::vector<std::string>(), bool keep_bands = false) : cube(std::make_shared<cube_st_reference>(*(in->st_reference()))), _in_cube(in), _cmd(cmd), _nbands(nbands), _names(names), _keep_bands(keep_bands) {  // it is important to duplicate st reference here, otherwise changes will affect input cube as well

        _chunk_size[0] = _in_cube->chunk_size()[0];
        _chunk_size[1] = _in_cube->chunk_size()[1];
        _chunk_size[2] = _in_cube->chunk_size()[2];

        if (!names.empty()) {
            if (names.size() != nbands) {
                GCBS_ERROR("size of names is different to nbands");
                throw std::string(
                    "ERROR in stream_reduce_time_cube::reduce_time_stream_cube(): size of names is different to nbands");
            }
        }

        if (_keep_bands) {
            // TODO: check for band name conflicts here
            for (uint16_t i = 0; i < _in_cube->size_bands(); ++i) {
                _bands.add(_in_cube->bands().get(i));
            }
        }

        for (uint16_t i = 0; i < nbands; ++i) {
            std::string name;
            if (!_names.empty())
                name = _names[i];
            else
                name = "x" + std::to_string(i + 1);
            _bands.add(band(name));
        }
    }

   public:
    ~stream_apply_pixel_cube() {}

    std::shared_ptr<chunk_data> read_chunk(chunkid_t id) override;

    json11::Json make_constructible_json() override {
        json11::Json::object out;
        out["cube_type"] = "stream_apply_pixel_cube";
        out["cmd"] = _cmd;
        out["nbands"] = _nbands;
        out["names"] = _names;
        out["keep_bands"] = _keep_bands;
        out["in_cube"] = _in_cube->make_constructible_json();
        return out;
    }

   private:
    std::shared_ptr<cube> _in_cube;
    std::string _cmd;
    uint16_t _nbands;
    std::vector<std::string> _names;
    bool _keep_bands;

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

#endif  //STREAM_APPLY_PIXEL_H
