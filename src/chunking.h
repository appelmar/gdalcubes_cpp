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

#include "image_collection.h"
#include "view.h"

/**
 * A chunk stores a spatial and temporal contiguous part of an image collection
 * as four-dimensional array.
 */

class chunk {
   public:
    bounds_2d<double> bounds();
    int64_t id();
};

class chunking {
    class chunk_iterator;
};

class default_chunking : public chunking {
   public:
    default_chunking(cube_view v) : _v(v), _size_x(256), _size_y(256), _size_t(16) {
    }

   protected:
    cube_view _v;
    uint32_t _size_x;
    uint32_t _size_y;
    uint32_t _size_t;
};

#endif  //CHUNKING_H
