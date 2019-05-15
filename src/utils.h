/*
    MIT License

    Copyright (c) 2019 Marius Appel <marius.appel@uni-muenster.de>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
#ifndef UTILS_H
#define UTILS_H

#include <gdal_priv.h>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <string>

/**
 * @brief A utility class for commonly used functions
 */
class utils {
   public:
    /**
    * @brief Generate a unique random filename
    * @param n number of characters of the random part
    * @param prefix string to append before the random part
    * @param suffix string to append after the random part
    * @return filename string
    */
    static std::string generate_unique_filename(uint16_t n = 8, std::string prefix = "", std::string suffix = "") {
        static std::mt19937 gen(time(NULL));  //Standard mersenne_twister_engine seeded with rd()
        static const std::string LETTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        static std::uniform_int_distribution<> dis(0, LETTERS.length() - 1);
        static std::mutex mtx;
        mtx.lock();
        std::stringstream ss;
        for (uint16_t i = 0; i < n; ++i) {
            ss << LETTERS[dis(gen)];
        }
        std::string out = prefix + ss.str() + suffix;
        mtx.unlock();
        return out;
    }

    /**
     * @brief Get the current datetime
     * @see  http://stackoverflow.com/questions/997946/how-to-get-current-time-and-date-in-c
     * @return datetime string
     */
    static std::string get_curdatetime() {
        // Current date/time based on current system
        time_t now = time(0);

        // Convert now to tm struct for local timezone
        tm *localtm = localtime(&now);

        std::stringstream out;
        out << (localtm->tm_year + 1900) << "-" << std::setfill('0') << std::setw(2)
            << (localtm->tm_mon + 1) << "-" << std::setfill('0') << std::setw(2)
            << localtm->tm_mday << " " << std::setfill('0') << std::setw(2)
            << localtm->tm_hour << ":" << std::setfill('0') << std::setw(2)
            << localtm->tm_min << ":" << std::setfill('0') << std::setw(2)
            << localtm->tm_sec;
        return out.str();
    }

    /**
     * @brief Get the current date
     * @return date string
     */
    static std::string get_curdate() {
        // Current date/time based on current system
        time_t now = time(0);

        // Convert now to tm struct for local timezone
        tm *localtm = localtime(&now);

        std::stringstream out;
        out << (localtm->tm_year + 1900) << "-" << std::setfill('0') << std::setw(2)
            << (localtm->tm_mon + 1) << "-" << std::setfill('0') << std::setw(2)
            << localtm->tm_mday;
        return out.str();
    }

    /**
     * Convert a type name to the corresponding GDAL data type
     * @param s type name
     * @return GDAL data type
     */
    static GDALDataType gdal_type_from_string(std::string s) {
        if (s == "int16") return GDALDataType::GDT_Int16;
        if (s == "int32") return GDALDataType::GDT_Int32;
        if (s == "uint8") return GDALDataType::GDT_Byte;
        if (s == "uint16") return GDALDataType::GDT_UInt16;
        if (s == "uint32") return GDALDataType::GDT_UInt32;
        if (s == "float64") return GDALDataType::GDT_Float64;
        if (s == "float32") return GDALDataType::GDT_Float32;
        return GDALDataType::GDT_Unknown;
    }

    /**
     * @brief Convert a GDAL data type to a string typename used in gdalcubes
     * @param t GDAL data type
     * @return type name string
     */
    static std::string string_from_gdal_type(GDALDataType t) {
        switch (t) {
            case GDT_Float64:
                return "float64";
            case GDT_Float32:
                return "float32";
            case GDT_Int16:
                return "int16";
            case GDT_Int32:
                return "int32";
            case GDT_UInt32:
                return "uint32";
            case GDT_UInt16:
                return "uint16";
            case GDT_Byte:
                return "uint8";
            default:
                return "null";
        }
    }

    static std::string dbl_to_string(double x, uint8_t precision = std::numeric_limits<double>::max_digits10) {
        std::ostringstream ss;
        ss << std::fixed;
        ss << std::setprecision(precision);
        ss << x;
        return ss.str();
    }
};

#endif  //UTILS_H
