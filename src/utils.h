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

#ifndef UTILS_H
#define UTILS_H



class utils {

public:


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

    static std::string string_from_gdal_type(GDALDataType t) {
        switch (t) {
            case GDT_Float64: return "float64";
            case GDT_Float32: return "float32";
            case GDT_Int16: return "int16";
            case GDT_Int32: return "int32";
            case GDT_UInt32: return "uint32";
            case GDT_UInt16: return "uint16";
            case GDT_Byte: return "uint8";
            default: return "null";
        }
    }
};


#endif //UTILS_H
