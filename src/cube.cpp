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

#include "cube.h"

#include "chunking.h"

cube::cube(std::shared_ptr<image_collection> ic) : _collection(ic), _view(), _chunking(new default_chunking(this)) {}
cube::cube(std::string icfile) : _collection(std::make_shared<image_collection>(icfile)), _view(), _chunking(new default_chunking(this)) {}
cube::cube(std::shared_ptr<image_collection> ic, cube_view v) : _collection(ic), _view(v), _chunking(new default_chunking(this)) {}
cube::cube(std::string icfile, cube_view v) : _collection(std::make_shared<image_collection>(icfile)), _view(v), _chunking(new default_chunking(this)) {}
cube::cube(std::shared_ptr<image_collection> ic, std::string vfile) : _collection(ic), _view(cube_view::read_json(vfile)), _chunking(new default_chunking(this)) {}
cube::cube(std::string icfile, std::string vfile) : _collection(std::make_shared<image_collection>(icfile)), _view(cube_view::read_json(vfile)), _chunking(new default_chunking(this)) {}

cube::~cube() {
    if (_chunking)
        delete _chunking;
}

void cube::set_chunking(chunking* c) {
    // c will be owned (and destroyed) by this class afterwards!!
    _chunking = c;
    _chunking->_c = this;
}

std::string cube::to_string() {
    std::stringstream out;
    out << "GDAL CUBE with (x,y,t)=(" << _view.nx() << "," << _view.ny() << "," << _view.nt() << ") cells in " << _chunking->count_chunks() << " chunks." << std::endl;
    return out.str();
}