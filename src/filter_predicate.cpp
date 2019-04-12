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

#include "filter_predicate.h"
#include "external/tinyexpr/tinyexpr.h"

std::shared_ptr<chunk_data> filter_predicate_cube::read_chunk(chunkid_t id) {
    GCBS_TRACE("filter_predicate_cube::read_chunk(" + std::to_string(id) + ")");

    if (id >= count_chunks())
        return std::shared_ptr<chunk_data>();  // chunk is outside of the view, we don't need to read anything.

    std::shared_ptr<chunk_data> out = std::make_shared<chunk_data>();

    // Parse expressions and create symbol table
    std::vector<double> values;
    values.resize(_in_cube->bands().count(), NAN);
    // WARNING: never change size of values from now

    // TODO: add further variables like x, y, t to symbol table

    std::vector<te_variable> vars;
    for (uint16_t i = 0; i < _in_cube->bands().count(); ++i) {
        char* varname = new char[_in_cube->bands().get(i).name.length() + 1];
        std::string temp_name = _in_cube->bands().get(i).name;
        std::transform(temp_name.begin(), temp_name.end(), temp_name.begin(), ::tolower);
        std::strncpy(varname, temp_name.c_str(), temp_name.length() + 1);
        vars.push_back({varname, &values[i]});
    }

    int err;
    te_expr* expr = te_compile(_pred.c_str(), vars.data(), vars.size(), &err);
    if (!expr) {
        std::string msg = "Cannot parse predicate '" + _pred + "': error at token " + std::to_string(err);
        GCBS_ERROR(msg);
        te_free(expr);
        return out;
    }

    std::shared_ptr<chunk_data> in = _in_cube->read_chunk(id);
    out->size({_bands.count(), in->size()[1], in->size()[2], in->size()[3]});
    out->buf(std::calloc(_bands.count() * in->size()[1] * in->size()[2] * in->size()[3], sizeof(double)));

    // We do not need to fill with NAN because we can be sure that it is completeley filled from the input cube
    //double *begin = (double *)out->buf();
    //double *end = ((double *)out->buf()) + size_btyx[0] * size_btyx[1] * size_btyx[2] * size_btyx[3];
    // std::fill(begin, end, NAN);

    for (uint32_t i = 0; i < in->size()[1] * in->size()[2] * in->size()[3]; ++i) {
        for (uint16_t inb = 0; inb < in->size()[0]; ++inb) {
            values[inb] = ((double*)in->buf())[inb * in->size()[1] * in->size()[2] * in->size()[3] + i];
        }
        if (te_eval(expr) != 0) {
            for (uint16_t ib = 0; ib < _bands.count(); ++ib) {
                ((double*)out->buf())[ib * in->size()[1] * in->size()[2] * in->size()[3] + i] = ((double*)in->buf())[ib * in->size()[1] * in->size()[2] * in->size()[3] + i];
            }
        } else {
            for (uint16_t ib = 0; ib < _bands.count(); ++ib) {
                ((double*)out->buf())[ib * in->size()[1] * in->size()[2] * in->size()[3] + i] = NAN;
            }
        }
    }

    te_free(expr);

    for (uint16_t i = 0; i < _in_cube->bands().count(); ++i) {  // only free name of actual variables (not functions)
        delete[] vars[i].name;
    }

    return out;
}

bool filter_predicate_cube::parse_predicate() {
    bool res = true;
    std::vector<double> dummy_values;
    std::vector<te_variable> vars;
    for (uint16_t i = 0; i < _in_cube->bands().count(); ++i) {
        dummy_values.push_back(1.0);
        char* varname = new char[_in_cube->bands().get(i).name.length() + 1];
        std::string temp_name = _in_cube->bands().get(i).name;
        std::transform(temp_name.begin(), temp_name.end(), temp_name.begin(), ::tolower);
        std::strncpy(varname, temp_name.c_str(), temp_name.length() + 1);
        vars.push_back({varname, &dummy_values[i]});
    }

    int err = 0;

    te_expr* expr = te_compile(_pred.c_str(), vars.data(), vars.size(), &err);
    if (expr) {
        te_free(expr);
    } else {
        res = false;
        std::string msg = "Cannot parse predicate'" + _pred + "': error at token " + std::to_string(err);
        GCBS_ERROR(msg);
        // Continue anyway to process all expressions
    }
    for (uint16_t i = 0; i < _in_cube->bands().count(); ++i) {  // only free name of actual variables (not functions)
        delete[] vars[i].name;
    }
    return res;
}
