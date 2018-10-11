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

#ifndef CLIENT_H
#define CLIENT_H

#include <curl/curl.h>
#include "cube.h"

class gdalcubes_swarm {
   public:
    gdalcubes_swarm(std::vector<std::string> urls) : _cube(nullptr), _cube_ids(), _server_uris(urls), _server_handles() {
        for (uint16_t i = 0; i < _server_uris.size(); ++i)
            _server_handles.push_back(curl_easy_init());
    }

    ~gdalcubes_swarm() {
        for (uint16_t i = 0; i < _server_uris.size(); ++i)
            curl_easy_cleanup(_server_handles[i]);
    }

    inline static gdalcubes_swarm from_urls(std::vector<std::string> urls) { return gdalcubes_swarm(urls); }

    // Create from txt file where each line is a uri
    static gdalcubes_swarm from_txtfile(std::string path);

    static void init();
    static void cleanup();

    // upload execution context to all servers
    void push_execution_context(bool recursive = false);

    // create cube on all servers
    void push_cube(std::shared_ptr<cube> c);

    // Mimic cube::apply with distributed calls to cube::read_chunk()
    void apply(std::shared_ptr<cube> c, std::function<void(chunkid_t, std::shared_ptr<chunk_data>, std::mutex &)> f);

    void post_start(uint32_t chunk_id, uint16_t server_index);                           // TODO: make protected
    std::shared_ptr<chunk_data> get_download(uint32_t chunk_id, uint16_t server_index);  // TODO: make protected

   protected:
    void post_file(std::string path, uint16_t server_index);

    uint32_t post_cube(std::string json, uint16_t server_index);

    std::shared_ptr<cube> _cube;
    std::vector<uint32_t> _cube_ids;  // IDs of the cube for each server

    std::vector<CURL *> _server_handles;
    std::vector<std::string> _server_uris;
};

#endif  //CLIENT_H
