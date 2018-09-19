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

#include "view.h"



class chunking; // forward declaration

class cube {

public:
    cube(std::shared_ptr<image_collection> ic);
    cube(std::string icfile);
    cube(std::shared_ptr<image_collection> ic, cube_view v);
    cube(std::string icfile, cube_view v);
    cube(std::shared_ptr<image_collection> ic, std::string vfile) ;
    cube(std::string icfile,std::string vfile);
    ~cube();


    inline std::shared_ptr<image_collection> collection() {return _collection;}
    inline cube_view& view() {return _view;}


    void set_chunking(chunking* c);
    const chunking* get_chunking();

   protected:
    cube_view _view;
    const std::shared_ptr<image_collection> _collection;
    chunking* _chunking; // owned by this class

};

#endif  //CUBE_H
