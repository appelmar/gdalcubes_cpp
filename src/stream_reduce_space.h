

#ifndef STREAM_REDUCE_SPACE_H
#define STREAM_REDUCE_SPACE_H

#include "cube.h"

namespace gdalcubes {

/**
* @brief A data cube that applies a user-defined function over data cube time slices
*/

class stream_reduce_space_cube : public cube {
   public:
    /**
        * @brief Create a data cube that applies a user-defined function on a given input data cube over space
        * @note This static creation method should preferably be used instead of the constructors as
        * the constructors will not set connections between cubes properly.
        * @param in input data cube
        * @param cmd external program call
        * @param nbands number of bands in the output cube
        * @param names string vector of output band names, must have the size nbands or empty (the default). If empty,
        * output bands will be named "band1", "band2", ...
        * @return a shared pointer to the created data cube instance
        */
    static std::shared_ptr<stream_reduce_space_cube>
    create(std::shared_ptr<cube> in, std::string cmd, uint16_t nbands,
           std::vector<std::string> names = std::vector<std::string>()) {
        std::shared_ptr<stream_reduce_space_cube> out = std::make_shared<stream_reduce_space_cube>(in, cmd, nbands,
                                                                                                   names);
        in->add_child_cube(out);
        out->add_parent_cube(in);
        return out;
    }

   public:
    stream_reduce_space_cube(std::shared_ptr<cube> in, std::string cmd, uint16_t nbands,
                             std::vector<std::string> names = std::vector<std::string>()) : cube(in->st_reference()->copy()), _in_cube(in), _cmd(cmd), _nbands(nbands), _names(names) {  // it is important to duplicate st reference here, otherwise changes will affect input cube as well

        if (cube_stref::type_string(_st_ref) == "cube_stref_regular") {
            std::shared_ptr<cube_stref_regular> stref = std::dynamic_pointer_cast<cube_stref_regular>(_st_ref);
            stref->set_x_axis(_st_ref->left(), _st_ref->right(), (uint32_t)1);
            stref->set_y_axis(_st_ref->bottom(), _st_ref->top(), (uint32_t)1);
        } else if (cube_stref::type_string(_st_ref) == "cube_stref_labeled_time") {
            std::shared_ptr<cube_stref_labeled_time> stref = std::dynamic_pointer_cast<cube_stref_labeled_time>(_st_ref);
            stref->set_x_axis(_st_ref->left(), _st_ref->right(), (uint32_t)1);
            stref->set_y_axis(_st_ref->bottom(), _st_ref->top(), (uint32_t)1);
        }
        _chunk_size[0] = _in_cube->chunk_size()[0];
        _chunk_size[1] = 1;
        _chunk_size[2] = 1;

        if (!names.empty()) {
            if (names.size() != nbands) {
                GCBS_ERROR("size of names is different to nbands");
                throw std::string(
                    "ERROR in stream_reduce_space_cube::reduce_time_stream_cube(): size of names is different to nbands");
            }
        }

        for (uint16_t i = 0; i < nbands; ++i) {
            std::string name;
            if (!_names.empty())
                name = _names[i];
            else
                name = "band" + std::to_string(i + 1);
            _bands.add(band(name));
        }
    }

   public:
    ~stream_reduce_space_cube() {}

    std::shared_ptr<chunk_data> read_chunk(chunkid_t id) override;

    json11::Json make_constructible_json() override {
        json11::Json::object out;
        out["cube_type"] = "stream_reduce_space";
        out["cmd"] = _cmd;
        out["nbands"] = _nbands;
        out["names"] = _names;
        out["in_cube"] = _in_cube->make_constructible_json();
        return out;
    }

   private:
    std::shared_ptr<cube> _in_cube;
    std::string _cmd;
    uint16_t _nbands;
    std::vector<std::string> _names;
};

}  // namespace gdalcubes

#endif  //STREAM_REDUCE_SPACE_H
