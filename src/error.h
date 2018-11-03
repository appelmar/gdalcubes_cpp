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

#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <string>

#define GCBS_FATAL(MSG)  logger::fatal(MSG, std::string(__FILE__) + ":" + std::string(__func__) + ":" + std::to_string(__LINE__) + "")
#define GCBS_ERROR(MSG) logger::error(MSG, std::string(__FILE__) + ":" + std::string(__func__) + ":" + std::to_string(__LINE__) + "")
#define GCBS_WARN(MSG)  logger::warn(MSG, std::string(__FILE__) + ":" + std::string(__func__) + ":" + std::to_string(__LINE__) + "")
#define GCBS_DEBUG(MSG)  logger::debug(MSG, std::string(__FILE__) + ":" + std::string(__func__) + ":" + std::to_string(__LINE__) + "")
#define GCBS_INFO(MSG)  logger::info(MSG, std::string(__FILE__) + ":" + std::string(__func__) + ":" + std::to_string(__LINE__) + "")

enum error_level {
    INFO,
    DEBUG,
    WARNING,
    ERROR,
    FATAL
};
typedef void (*error_action)(error_level, std::string, std::string, int);

class logger {
   public:
    static void error(std::string msg, std::string where = "", int error_code = 0);
    static void warn(std::string msg, std::string where = "", int error_code = 0);
    static void debug(std::string msg, std::string where = "", int error_code = 0);
    static void fatal(std::string msg, std::string where = "", int error_code = 0);
    static void info(std::string msg, std::string where = "", int error_code = 0);
};

class error_handler {
   public:
    static void default_error_handler(error_level type, std::string msg, std::string where, int error_code) {
        std::string code = (error_code != 0)? " (" + std::to_string(error_code) + ")" : "";
        std::string where_str = (where.empty())? "" : " [in " + where + "]";
        if (type == ERROR || type == FATAL) {
            std::cerr << "ERROR" << code << ": " << msg << where_str  <<  std::endl;
        } else if (type == WARNING) {
            std::cout << "WARNING" << code << ": " << msg  << where_str  << std::endl;
        }
    }

    static void error_handler_debug(error_level type, std::string msg, std::string where, int error_code) {
        std::string code = (error_code != 0)? " (" + std::to_string(error_code) + ")" : "";
        std::string where_str = (where.empty())? "" : " [in " + where + "]";
        if (type == ERROR || type == FATAL) {
            std::cerr << "ERROR" << code << ": " << msg << where_str  <<  std::endl;
        } else if (type == WARNING) {
            std::cout << "WARNING" << code << ": " << msg  << where_str  << std::endl;
        }
        else if (type == DEBUG) {
            std::cout << "DEBUG" << code << ": " << msg  << where_str  << std::endl;
        }
    }
};

#endif  //LOG_H
