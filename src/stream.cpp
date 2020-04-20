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

#include "stream.h"
#include <stdlib.h>
#include "external/tiny-process-library/process.hpp"

#include <fstream>

namespace gdalcubes {

std::shared_ptr<chunk_data> stream_cube::read_chunk(chunkid_t id) {
    GCBS_TRACE("stream_cube::read_chunk(" + std::to_string(id) + ")");
    std::shared_ptr<chunk_data> out = std::make_shared<chunk_data>();
    if (id >= count_chunks()) {
        // chunk is outside of the cube, we don't need to read anything.
        GCBS_WARN("Chunk id " + std::to_string(id) + " is out of range");
        return out;
    }

    if (_file_streaming) {
        out = stream_chunk_file(_in_cube->read_chunk(id), id);
    } else {
        out = stream_chunk_stdin(_in_cube->read_chunk(id), id);
    }

    if (out->empty()) {
        GCBS_DEBUG("Streaming returned empty chunk " + std::to_string(id));
    }
    return out;
}

std::shared_ptr<chunk_data> stream_cube::stream_chunk_stdin(std::shared_ptr<chunk_data> data, chunkid_t id) {
    std::shared_ptr<chunk_data> out = std::make_shared<chunk_data>();

    int size[] = {(int)data->size()[0], (int)data->size()[1], (int)data->size()[2], (int)data->size()[3]};
    if (size[0] * size[1] * size[2] * size[3] == 0) {
        return out;
    }

    // 1. new process with env "GDALCUBES_STREAMING=1"
    std::string errstr;
    uint32_t databytes_read = 0;

#ifdef _WIN32
    _putenv("GDALCUBES_STREAMING=1");
    _putenv((std::string("GDALCUBES_STREAMING_CHUNK_ID") + "=" + std::to_string(id)).c_str());
#else
    setenv("GDALCUBES_STREAMING", "1", 1);
    setenv("GDALCUBES_STREAMING_CHUNK_ID", std::to_string(id).c_str(), 1);
#endif

    TinyProcessLib::Process process(_cmd, "", [out, &databytes_read](const char *bytes, std::size_t n) {

        if (databytes_read == 0) {
            // Assumption is that at least 4 integers with chunk size area always contained in the first call of this function
            if (n >= 4 * sizeof(int)) {
                chunk_size_btyx out_size = {(uint32_t)(((int *)bytes)[0]), (uint32_t)(((int *)bytes)[1]),
                                            (uint32_t)(((int *)bytes)[2]), (uint32_t)(((int *)bytes)[3])};
                out->size(out_size);
                // Fill buffers accordingly
                out->buf(std::calloc(out_size[0] * out_size[1] * out_size[2] * out_size[3], sizeof(double)));
                memcpy(out->buf(), bytes + (4 * sizeof(int)), n -  4 * sizeof(int));
                databytes_read += n - (4 * sizeof(int));
            } else {
                GCBS_WARN("Cannot read streaming result, returning empty chunk");
                databytes_read = std::numeric_limits<uint32_t>::max(); // prevent further calls to write anything into the chunk buffer
            }
        } else {
            // buffer overflow check
            if ((char*)(out->buf()) + databytes_read + n <= (char*)(out->buf()) + out->size()[0] * out->size()[1] * out->size()[2] * out->size()[3] * sizeof(double)) {
                memcpy((char*)(out->buf()) + databytes_read, bytes, n);
                databytes_read += n;
            }
        } }, [&errstr](const char *bytes, std::size_t n) {
    errstr = std::string(bytes, n);
    GCBS_DEBUG(errstr); }, true);

    // Write to stdin
    std::string proj = _in_cube->st_reference()->srs();
    process.write((char *)(size), sizeof(int) * 4);
    for (uint16_t i = 0; i < _in_cube->bands().count(); ++i) {
        int str_size = _in_cube->bands().get(i).name.size();
        process.write((char *)(&str_size), sizeof(int));
        process.write(_in_cube->bands().get(i).name.c_str(), sizeof(char) * str_size);
    }

    if (!_in_cube->st_reference()->has_regular_space()) {
        throw std::string("ERROR: chunk streaming currently does not support irregular spatial dimensions");
    }
    // NOTE: the following will only work as long as all cube st reference types with regular spatial dimensions inherit from  cube_stref_regular class
    std::shared_ptr<cube_stref_regular> stref_in = std::dynamic_pointer_cast<cube_stref_regular>(_in_cube->st_reference());

    double *dims = (double *)std::calloc(size[1] + size[2] + size[3], sizeof(double));
    for (int i = 0; i < size[1]; ++i) {
        dims[i] = stref_in->datetime_at_index(_in_cube->chunk_limits(id).low[0] + i).to_double();
    }
    for (int i = size[1]; i < size[1] + size[2]; ++i) {
        dims[i] = stref_in->win().bottom + i * _in_cube->st_reference()->dy();
    }
    for (int i = size[1] + size[2]; i < size[1] + size[2] + size[3]; ++i) {
        dims[i] = stref_in->win().left + i * _in_cube->st_reference()->dx();
    }
    process.write((char *)(dims), sizeof(double) * (size[1] + size[2] + size[3]));
    std::free(dims);

    int str_size = proj.size();
    process.write((char *)(&str_size), sizeof(int));
    process.write(proj.c_str(), sizeof(char) * str_size);
    process.write(((char *)(data->buf())), sizeof(double) * data->size()[0] * data->size()[1] * data->size()[2] * data->size()[3]);

    process.close_stdin();  // needed?

    auto exit_status = process.get_exit_status();
    if (exit_status != 0) {
        GCBS_ERROR("Child process failed with exit code " + std::to_string(exit_status));
        GCBS_ERROR("Child process output: " + errstr);
        throw std::string("ERROR in stream_cube::read_chunk(): external program returned exit code " + std::to_string(exit_status));
    }
    return out;
}

std::shared_ptr<chunk_data> stream_cube::stream_chunk_file(std::shared_ptr<chunk_data> data, chunkid_t id) {
    std::shared_ptr<chunk_data> out = std::make_shared<chunk_data>();

    int size[] = {(int)data->size()[0], (int)data->size()[1], (int)data->size()[2], (int)data->size()[3]};
    if (size[0] * size[1] * size[2] * size[3] == 0) {
        return out;
    }

    // generate in and out filename
    std::string f_in = filesystem::join(config::instance()->get_streaming_dir(), utils::generate_unique_filename(12, ".stream_", "_in"));
    std::string f_out = filesystem::join(config::instance()->get_streaming_dir(), utils::generate_unique_filename(12, ".stream_", "_out"));

    std::string errstr;  // capture error string

    // write input data
    std::ofstream f_in_stream(f_in, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!f_in_stream.is_open()) {
        GCBS_ERROR("Cannot write streaming input data to file '" + f_in + "'");
        throw std::string("ERROR in stream_cube::stream_chunk_file(): cannot write streaming input data to file '" + f_in + "'");
    }

    std::string proj = _in_cube->st_reference()->srs();
    f_in_stream.write((char *)(size), sizeof(int) * 4);
    for (uint16_t i = 0; i < _in_cube->bands().count(); ++i) {
        int str_size = _in_cube->bands().get(i).name.size();
        f_in_stream.write((char *)(&str_size), sizeof(int));
        f_in_stream.write(_in_cube->bands().get(i).name.c_str(), sizeof(char) * str_size);
    }

    double *dims = (double *)std::calloc(size[1] + size[2] + size[3], sizeof(double));
    int i = 0;
    for (int it = 0; it < size[1]; ++it) {
        dims[i] = (_in_cube->st_reference()->datetime_at_index(it + _in_cube->chunk_size()[0] * _in_cube->chunk_limits(id).low[0])).to_double();
        ++i;
    }
    bounds_st cextent = this->bounds_from_chunk(id);  // implemented in derived classes
    for (int iy = 0; iy < size[2]; ++iy) {
        dims[i] = cextent.s.bottom + chunk_size(id)[1] * st_reference()->dy() - (iy + 0.5) * st_reference()->dy();  // cell center
        ++i;
    }
    for (int ix = 0; ix < size[3]; ++ix) {
        dims[i] = cextent.s.left + (ix + 0.5) * st_reference()->dx();
        ++i;
    }

    f_in_stream.write((char *)(dims), sizeof(double) * (size[1] + size[2] + size[3]));
    std::free(dims);

    int str_size = proj.size();
    f_in_stream.write((char *)(&str_size), sizeof(int));
    f_in_stream.write(proj.c_str(), sizeof(char) * str_size);
    f_in_stream.write(((char *)(data->buf())), sizeof(double) * data->size()[0] * data->size()[1] * data->size()[2] * data->size()[3]);
    f_in_stream.close();

    /* setenv / _putenv is not thread-safe, we need to get a mutex until the child process has been started. */
    static std::mutex mtx;
    mtx.lock();
#ifdef _WIN32
    _putenv("GDALCUBES_STREAMING=1");
    //_putenv((std::string("GDALCUBES_STREAMING_DIR") + "=" + config::instance()->get_streaming_dir().c_str()).c_str());
    _putenv((std::string("GDALCUBES_STREAMING_CHUNK_ID") + "=" + std::to_string(id)).c_str());
    _putenv((std::string("GDALCUBES_STREAMING_FILE_IN") + "=" + f_in.c_str()).c_str());
    _putenv((std::string("GDALCUBES_STREAMING_FILE_OUT") + "=" + f_out.c_str()).c_str());
#else
    setenv("GDALCUBES_STREAMING", "1", 1);
    // setenv("GDALCUBES_STREAMING_DIR", config::instance()->get_streaming_dir().c_str(), 1);
    setenv("GDALCUBES_STREAMING_CHUNK_ID", std::to_string(id).c_str(), 1);
    setenv("GDALCUBES_STREAMING_FILE_IN", f_in.c_str(), 1);
    setenv("GDALCUBES_STREAMING_FILE_OUT", f_out.c_str(), 1);
#endif

    // start process
    TinyProcessLib::Process process(_cmd, "", [](const char *bytes, std::size_t n) {}, [&errstr](const char *bytes, std::size_t n) {
        errstr = std::string(bytes, n);
        GCBS_DEBUG(errstr); }, false);
    mtx.unlock();
    auto exit_status = process.get_exit_status();
    filesystem::remove(f_in);
    if (exit_status != 0) {
        GCBS_ERROR("Child process failed with exit code " + std::to_string(exit_status));
        GCBS_ERROR("Child process output: " + errstr);
        if (filesystem::exists(f_out)) {
            filesystem::remove(f_out);
        }
        throw std::string("ERROR in stream_cube::read_chunk(): external program returned exit code " + std::to_string(exit_status));
    }

    // read output data
    std::ifstream f_out_stream(f_out, std::ios::in | std::ios::binary);
    if (!f_out_stream.is_open()) {
        GCBS_ERROR("Cannot read streaming output data from file '" + f_out + "'");
        throw std::string("ERROR in stream_cube::stream_chunk_file(): cannot read streaming output data from file '" + f_out + "'");
    }

    f_out_stream.seekg(0, f_out_stream.end);
    int length = f_out_stream.tellg();
    f_out_stream.seekg(0, f_out_stream.beg);
    char *buffer = (char *)std::calloc(length, sizeof(char));
    f_out_stream.read(buffer, length);
    f_out_stream.close();

    chunk_size_btyx out_size = {(uint32_t)(((int *)buffer)[0]), (uint32_t)(((int *)buffer)[1]),
                                (uint32_t)(((int *)buffer)[2]), (uint32_t)(((int *)buffer)[3])};
    out->size(out_size);
    out->buf(std::calloc(out_size[0] * out_size[1] * out_size[2] * out_size[3], sizeof(double)));
    std::memcpy(out->buf(), buffer + (4 * sizeof(int)), length - 4 * sizeof(int));
    std::free(buffer);

    if (filesystem::exists(f_out)) {
        filesystem::remove(f_out);
    }

    return out;
}

}  // namespace gdalcubes