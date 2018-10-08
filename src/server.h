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

#ifndef SERVER_H
#define SERVER_H

#include <cpprest/http_listener.h>
#include <cpprest/uri_builder.h>
#include <boost/filesystem.hpp>
#include <mutex>

class gdalcubes_server {
   public:
    gdalcubes_server(std::string host, uint16_t port = 1111, std::string basepath = "gdalcubes/api/", bool ssl = false) : _host(host), _port(port), _ssl(ssl), _basepath(basepath), _workdir(boost::filesystem::temp_directory_path() / "gdalcubes_server"), _listener() {
        if (boost::filesystem::exists(_workdir) && boost::filesystem::is_directory(_workdir)) {
            boost::filesystem::remove_all(_workdir);
        } else if (boost::filesystem::exists(_workdir) && !boost::filesystem::is_directory(_workdir)) {
            throw std::string("ERROR in gdalcubes_server::gdalcubes_server(): working directory for gdalcubes_server is an existing file.");
        }
        boost::filesystem::create_directory(_workdir);
        boost::filesystem::current_path(_workdir);

        std::string url = ssl ? "https://" : "http://";
        url += host + ":" + std::to_string(port);
        web::uri_builder uri(url);
        uri.append_path(basepath);

        _listener = web::http::experimental::listener::http_listener(uri.to_string());

        _listener.support(web::http::methods::GET, std::bind(&gdalcubes_server::handle_get, this, std::placeholders::_1));
        _listener.support(web::http::methods::POST, std::bind(&gdalcubes_server::handle_post, this, std::placeholders::_1));
        _listener.support(web::http::methods::PUT, std::bind(&gdalcubes_server::handle_put, this, std::placeholders::_1));
    }

   public:
    inline pplx::task<void> open() { return _listener.open(); }
    inline pplx::task<void> close() { return _listener.close(); }

   protected:
    void handle_get(web::http::http_request req);
    void handle_post(web::http::http_request req);
    void handle_put(web::http::http_request req);

    web::http::experimental::listener::http_listener _listener;

    const uint16_t _port;
    const std::string _host;
    const std::string _basepath;
    const bool _ssl;
    const boost::filesystem::path _workdir;
};

#endif  //SERVER_H
