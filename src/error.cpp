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

#include "error.h"

#include "config.h"

void logger::error(std::string msg, std::string where, int error_code) {
    config::instance()->get_error_handler()(ERROR, msg, where, error_code);
}
void logger::warn(std::string msg, std::string where, int error_code) {
    config::instance()->get_error_handler()(WARNING, msg, where, error_code);
}
void logger::debug(std::string msg, std::string where, int error_code) {
    config::instance()->get_error_handler()(DEBUG, msg, where, error_code);
}
void logger::fatal(std::string msg, std::string where, int error_code) {
    config::instance()->get_error_handler()(FATAL, msg, where, error_code);
}
void logger::info(std::string msg, std::string where, int error_code) {
    config::instance()->get_error_handler()(INFO, msg, where, error_code);
}
