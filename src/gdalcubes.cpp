
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

/**
 * This file contains the main entry for the command line client.
 */

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include "build_info.h"
#include "image_collection.h"
#include "image_collection_cube.h"
#include "reduce.h"
#include "utils.h"

std::vector<std::string> string_list_from_text_file(std::string filename) {
    std::vector<std::string> out;

    std::string line;
    std::ifstream infile(filename);
    while (std::getline(infile, line))
        out.push_back(line);
    return out;
}

void print_usage(std::string command = "") {
    if (command == "create_collection") {
        std::cout << "Usage: gdalcubes create_collection [options] IN DEST" << std::endl;
        std::cout << std::endl;
        std::cout << "Create a new GDAL image collection (an SQLite database) from a list of GDAL Datasets (files, URLs, or other descriptors for GDALOpen()) and "
                     "a collection format definition. IN can be either a directory or a simple text file where each line is interpreted as a potential GDALDataset reference. If IN "
                     "is a directory, all containing files will be considered as potential GDALDatasets first and checked if they match the collection format afterwards. "
                  << std::endl;
        std::cout << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  -f, --format                  Path of the collection format description JSON file, this option is required" << std::endl;
        std::cout << "  -R, --recursive               If IN is a directory, do a recursive file listing" << std::endl;
        std::cout << "  -s, --strict                  Cancel if a single GDALDataset cannot be added to the collection. If not given, ignore failing datasets in the output collection" << std::endl;
        std::cout << std::endl;
    } else if (command == "info") {
        std::cout << "Usage: gdalcubes info SOURCE" << std::endl;
        std::cout << std::endl;
        std::cout << "Print information about a specified GDAL image collection (SOURCE)." << std::endl;
        std::cout << std::endl;
    } else if (command == "reduce") {
        std::cout << "Usage: gdalcubes reduce [options] SOURCE DEST" << std::endl;
        std::cout << std::endl;
        std::cout << "Reduce a given image collection (SOURCE) over time with the specified method and produce a single output image (DEST). The specified reducer "
                     "will be applied over all bands of the input collection. To convert the image collection to a cube, a data view "
                     "JSON file must be specified as -v or --view option. Depending on the collection's size and the location of their data "
                     "the reduction might be time-consuming."
                  << std::endl;
        std::cout << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  -v, --view               Filename of the JSON data view description, this option is required" << std::endl;
        std::cout << "  -r, --reducer            Reduction method, currently 'mean', 'median', 'min', or 'max', defaults to 'mean'" << std::endl;
        std::cout << "      --gdal-of            GDAL output format, defaults to GTiff" << std::endl;
        std::cout << "      --gdal-co            GDAL create options as 'KEY=VALUE' strings, can be passed multiple times" << std::endl;
        std::cout << std::endl;
        std::cout << "Please use 'gdalcubes command --help' for further information about command-specific arguments." << std::endl;
    } else if (command == "stream") {
    } else {
        std::cout << "Usage: gdalcubes command [arguments]" << std::endl;
        std::cout << "   or: gdalcubes [--help | --version]" << std::endl;
        std::cout << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  info                     Print metadata of a GDAL image collection file " << std::endl;
        std::cout << "  create_collection        Create a new image collection from GDAL datasets" << std::endl;
        std::cout << "  reduce                   Reduce a GDAL cube over time to a single GDAL image" << std::endl;
        std::cout << "  stream                   Stream chunks of a GDAL cube to stdin of other programs" << std::endl;
        std::cout << std::endl;
        std::cout << "Please use 'gdalcubes command --help' for further information about command-specific arguments." << std::endl;
    }
}

int main(int argc, char *argv[]) {
    GDALAllRegister();
    GDALSetCacheMax(1024 * 1024 * 256);            // 256 MiB
    CPLSetConfigOption("GDAL_PAM_ENABLED", "NO");  // avoid aux files for PNG tiles

    srand(time(NULL));

    namespace po = boost::program_options;
    // see https://stackoverflow.com/questions/15541498/how-to-implement-subcommands-using-boost-program-options

    po::options_description global_args("Global arguments");
    global_args.add_options()("help,h", "print usage")("version", "print version information")("debug,d", "debug output, not yet implemented")("command", po::value<std::string>(), "Command to execute")("subargs", po::value<std::vector<std::string>>(), "Arguments for command");

    po::positional_options_description pos;
    pos.add("command", 1).add("subargs", -1);

    po::variables_map vm;

    try {
        po::parsed_options parsed = po::command_line_parser(argc, argv).options(global_args).positional(pos).allow_unregistered().run();
        po::store(parsed, vm);
        if (vm.count("version")) {
            std::cout << "gdalcubes " << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH << " built on " << __DATE__ << " " << __TIME__;
            std::cout << " linked against " << GDALVersionInfo("--version") << std::endl;  // TODO add version info for other linked libraries
            return 0;
        }
        if (vm.count("help") && !vm.count("command")) {
            print_usage();
            return 0;
        }

        // if command and --help is given, print command-specific usage
        if (vm.count("help") && vm.count("command")) {
            print_usage(vm["command"].as<std::string>());
            return 0;
        }

        std::string cmd = vm["command"].as<std::string>();
        if (cmd == "create_collection") {
            po::options_description cc_desc("create_collection arguments");
            cc_desc.add_options()("recursive,R", "Scan provided directory recursively")("format,f", po::value<std::string>(), "Path of JSON collection format description")("strict,s", "stop the creation of the complete image collection if adding one of the images fails. ")("input", po::value<std::string>(), "Input, either a directory or a text file with GDALDataset references such as file paths or URLs per line.")("output", po::value<std::string>(), "Filename of the created image collection.");

            po::positional_options_description cc_pos;
            cc_pos.add("input", 1).add("output", 1);

            try {
                std::vector<std::string> opts = po::collect_unrecognized(parsed.options, po::include_positional);
                opts.erase(opts.begin());
                po::store(po::command_line_parser(opts).options(cc_desc).positional(cc_pos).run(), vm);
            } catch (...) {
                std::cout << "ERROR in gdalcubes create_collection: invalid arguments." << std::endl;
                std::cout << cc_desc << std::endl;
                return 1;
            }

            // TODO: implement debug flags

            bool recursive = false;
            bool strict = false;
            if (vm.count("recursive")) {
                recursive = true;
            }
            if (vm.count("strict")) {
                strict = true;
            }

            std::string input = vm["input"].as<std::string>();
            std::string output = vm["output"].as<std::string>();
            std::string format = vm["format"].as<std::string>();

            std::vector<std::string> in;

            namespace fs = boost::filesystem;
            fs::path p{input};
            if (fs::is_directory(p)) {
                if (recursive) {
                    fs::recursive_directory_iterator end;
                    for (fs::recursive_directory_iterator i(p); i != end; ++i) {
                        in.push_back(fs::absolute((*i).path()).string());
                    }
                } else {
                    fs::directory_iterator end;
                    for (fs::directory_iterator i(p); i != end; ++i) {
                        in.push_back(fs::absolute((*i).path()).string());
                    }
                }
            } else if (fs::is_regular_file(p)) {
                in = string_list_from_text_file(p.string());
            } else {
                throw std::string("ERROR in gdalcubes create_collection: Invalid input, provide a text file or directory.");
            }

            collection_format f(format);
            image_collection ic = image_collection::create(f, in, strict);
            ic.write(output);
            std::cout << ic.to_string() << std::endl;

        } else if (cmd == "info") {
            po::options_description info_desc("info arguments");
            info_desc.add_options()("input", po::value<std::string>(), "Filename of the image collection.");

            po::positional_options_description info_pos;
            info_pos.add("input", 1);

            try {
                std::vector<std::string> opts = po::collect_unrecognized(parsed.options, po::include_positional);
                opts.erase(opts.begin());
                po::store(po::command_line_parser(opts).options(info_desc).positional(info_pos).run(), vm);
            } catch (...) {
                std::cout << "ERROR in gdalcubes info: invalid arguments." << std::endl;
                std::cout << info_desc << std::endl;
            }

            std::string input = vm["input"].as<std::string>();

            image_collection ic = image_collection(input);

            std::vector<image_collection::band_info_row> bands = ic.get_bands();

            bounds_st e = ic.extent();
            std::cout << ic.to_string() << std::endl;
            std::cout << "date range: " << e.t0.to_string() << " to " << e.t1.to_string() << std::endl;
            std::cout << "x / lat range: " << e.s.left << " to " << e.s.right << std::endl;
            std::cout << "y / lon range: " << e.s.bottom << " to " << e.s.top << std::endl;
            std::cout << "BAND"
                      << ":("
                      << "type"
                      << ","
                      << "offset"
                      << ","
                      << "scale"
                      << ","
                      << "unit"
                      << ")" << std::endl;
            for (uint16_t i = 0; i < bands.size(); ++i) {
                std::cout << bands[i].name << ":(" << utils::string_from_gdal_type(bands[i].type) << "," << bands[i].offset << "," << bands[i].scale << "," << bands[i].unit << ")" << std::endl;
            }
        } else if (cmd == "reduce") {
            po::options_description reduce_desc("reduce arguments");
            reduce_desc.add_options()("view,v", po::value<std::string>(), "Path to the JSON data view description");
            reduce_desc.add_options()("reducer,r", po::value<std::string>()->default_value("mean"), "Reduction method, currently mean, median, min, and max are implemented.");
            reduce_desc.add_options()("gdal-of", po::value<std::string>()->default_value("GTiff"), "GDAL output format, defaults to GTiff");
            reduce_desc.add_options()("gdal-co", po::value<std::vector<std::string>>(), "GDAL create options");
            reduce_desc.add_options()("input", po::value<std::string>(), "Filename of the input image collection.");
            reduce_desc.add_options()("output", po::value<std::string>(), "Filename of the output image.");

            po::positional_options_description reduce_pos;
            reduce_pos.add("input", 1);
            reduce_pos.add("output", 1);

            try {
                std::vector<std::string> opts = po::collect_unrecognized(parsed.options, po::include_positional);
                opts.erase(opts.begin());
                po::store(po::command_line_parser(opts).options(reduce_desc).positional(reduce_pos).run(), vm);
            } catch (...) {
                std::cout << "ERROR in gdalcubes reduce: invalid arguments." << std::endl;
                std::cout << reduce_desc << std::endl;
            }

            std::string input = vm["input"].as<std::string>();
            std::string output = vm["output"].as<std::string>();

            std::vector<std::string> create_options;
            if (vm.count("gdal-co") > 0) {
                create_options = vm["gdal-co"].as<std::vector<std::string>>();
            }
            std::string reducer = vm["reducer"].as<std::string>();
            std::string outformat = vm["gdal-of"].as<std::string>();
            std::string json_view_path = vm["view"].as<std::string>();

            std::shared_ptr<image_collection> ic = std::make_shared<image_collection>(input);
            std::shared_ptr<cube> c_in = std::make_shared<image_collection_cube>(ic, json_view_path);
            std::shared_ptr<reduce_cube> c_reduce = std::make_shared<reduce_cube>(c_in, reducer);

            c_reduce->write_gdal_image(output, outformat, create_options);

        }

        else {
            std::cout << "ERROR in gdalcubes: unrecognized command." << std::endl;
            print_usage();
            return 1;
        }
    } catch (std::string s) {
        std::cout << s << std::endl;
        return 1;
    } catch (...) {
        std::cout << "ERROR in gdalcubes: invalid arguments." << std::endl;
        print_usage();
        return 1;
    }
}
