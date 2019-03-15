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

#ifndef CUBE_FACTORY_H
#define CUBE_FACTORY_H

#include <map>
#include <memory>
#include "cube.h"

namespace gdalcubes {

/**
 * @brief Factory to create (nested) cubes from its JSON representation
 */
class cube_factory {
   public:
    static cube_factory* instance() {
        static CG g;
        if (!_instance) {
            _instance = new cube_factory();
        }
        return _instance;
    }

    /**
     * Create a cube from its JSON representation
     * @param j
     * @return
     */
    std::shared_ptr<cube> create_from_json(nlohmann::json j);

    /**
     * @brief Registers a cube type with a function to create objects of this type from a JSON description.
     *
     * @param type_name unique name for cube type
     * @param generator function to create an object from a json definition
     * @todo implement this function
     */
    void register_cube_type(std::string type_name, std::function<std::shared_ptr<cube>(nlohmann::json&)> generator);

    void register_default();

   private:
    static cube_factory* _instance;
    cube_factory() {
        register_default();
    }
    cube_factory(const cube_factory&);
    cube_factory& operator=(const cube_factory&);
    ~cube_factory() {}
    class CG {
       public:
        ~CG() {
            if (NULL != cube_factory::_instance) {
                delete cube_factory::_instance;
                cube_factory::_instance = NULL;
            }
        }
    };

    std::map<std::string, std::function<std::shared_ptr<cube>(nlohmann::json&)>> cube_generators;
};

}  // namespace gdalcubes

#endif  //CUBE_FACTORY_H
