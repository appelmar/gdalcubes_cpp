// Generated by using Rcpp::compileAttributes() -> do not edit by hand
// Generator token: 10BE3573-1514-4C36-9D1C-5A225CD40393

#include <Rcpp.h>

using namespace Rcpp;

// libgdalcubes_is_null
Rcpp::LogicalVector libgdalcubes_is_null(SEXP pointer);
RcppExport SEXP _gdalcubes_libgdalcubes_is_null(SEXP pointerSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< SEXP >::type pointer(pointerSEXP);
    rcpp_result_gen = Rcpp::wrap(libgdalcubes_is_null(pointer));
    return rcpp_result_gen;
END_RCPP
}
// libgdalcubes_version
Rcpp::List libgdalcubes_version();
RcppExport SEXP _gdalcubes_libgdalcubes_version() {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    rcpp_result_gen = Rcpp::wrap(libgdalcubes_version());
    return rcpp_result_gen;
END_RCPP
}
// libgdalcubes_init
void libgdalcubes_init();
RcppExport SEXP _gdalcubes_libgdalcubes_init() {
BEGIN_RCPP
    Rcpp::RNGScope rcpp_rngScope_gen;
    libgdalcubes_init();
    return R_NilValue;
END_RCPP
}
// libgdalcubes_cleanup
void libgdalcubes_cleanup();
RcppExport SEXP _gdalcubes_libgdalcubes_cleanup() {
BEGIN_RCPP
    Rcpp::RNGScope rcpp_rngScope_gen;
    libgdalcubes_cleanup();
    return R_NilValue;
END_RCPP
}
// libgdalcubes_datetime_values
Rcpp::StringVector libgdalcubes_datetime_values(SEXP pin);
RcppExport SEXP _gdalcubes_libgdalcubes_datetime_values(SEXP pinSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< SEXP >::type pin(pinSEXP);
    rcpp_result_gen = Rcpp::wrap(libgdalcubes_datetime_values(pin));
    return rcpp_result_gen;
END_RCPP
}
// libgdalcubes_cube_info
Rcpp::List libgdalcubes_cube_info(SEXP pin);
RcppExport SEXP _gdalcubes_libgdalcubes_cube_info(SEXP pinSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< SEXP >::type pin(pinSEXP);
    rcpp_result_gen = Rcpp::wrap(libgdalcubes_cube_info(pin));
    return rcpp_result_gen;
END_RCPP
}
// libgdalcubes_get_cube_view
Rcpp::List libgdalcubes_get_cube_view(SEXP pin);
RcppExport SEXP _gdalcubes_libgdalcubes_get_cube_view(SEXP pinSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< SEXP >::type pin(pinSEXP);
    rcpp_result_gen = Rcpp::wrap(libgdalcubes_get_cube_view(pin));
    return rcpp_result_gen;
END_RCPP
}
// libgdalcubes_update_cube_view
void libgdalcubes_update_cube_view(SEXP pin, SEXP v);
RcppExport SEXP _gdalcubes_libgdalcubes_update_cube_view(SEXP pinSEXP, SEXP vSEXP) {
BEGIN_RCPP
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< SEXP >::type pin(pinSEXP);
    Rcpp::traits::input_parameter< SEXP >::type v(vSEXP);
    libgdalcubes_update_cube_view(pin, v);
    return R_NilValue;
END_RCPP
}
// libgdalcubes_open_image_collection
SEXP libgdalcubes_open_image_collection(std::string filename);
RcppExport SEXP _gdalcubes_libgdalcubes_open_image_collection(SEXP filenameSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< std::string >::type filename(filenameSEXP);
    rcpp_result_gen = Rcpp::wrap(libgdalcubes_open_image_collection(filename));
    return rcpp_result_gen;
END_RCPP
}
// libgdalcubes_image_collection_info
Rcpp::List libgdalcubes_image_collection_info(SEXP pin);
RcppExport SEXP _gdalcubes_libgdalcubes_image_collection_info(SEXP pinSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< SEXP >::type pin(pinSEXP);
    rcpp_result_gen = Rcpp::wrap(libgdalcubes_image_collection_info(pin));
    return rcpp_result_gen;
END_RCPP
}
// libgdalcubes_create_image_collection
SEXP libgdalcubes_create_image_collection(std::vector<std::string> files, std::string format_file, std::string outfile, bool unroll_archives);
RcppExport SEXP _gdalcubes_libgdalcubes_create_image_collection(SEXP filesSEXP, SEXP format_fileSEXP, SEXP outfileSEXP, SEXP unroll_archivesSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< std::vector<std::string> >::type files(filesSEXP);
    Rcpp::traits::input_parameter< std::string >::type format_file(format_fileSEXP);
    Rcpp::traits::input_parameter< std::string >::type outfile(outfileSEXP);
    Rcpp::traits::input_parameter< bool >::type unroll_archives(unroll_archivesSEXP);
    rcpp_result_gen = Rcpp::wrap(libgdalcubes_create_image_collection(files, format_file, outfile, unroll_archives));
    return rcpp_result_gen;
END_RCPP
}
// libgdalcubes_list_collection_formats
SEXP libgdalcubes_list_collection_formats();
RcppExport SEXP _gdalcubes_libgdalcubes_list_collection_formats() {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    rcpp_result_gen = Rcpp::wrap(libgdalcubes_list_collection_formats());
    return rcpp_result_gen;
END_RCPP
}
// libgdalcubes_create_image_collection_cube
SEXP libgdalcubes_create_image_collection_cube(SEXP pin, Rcpp::IntegerVector chunk_sizes, SEXP v);
RcppExport SEXP _gdalcubes_libgdalcubes_create_image_collection_cube(SEXP pinSEXP, SEXP chunk_sizesSEXP, SEXP vSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< SEXP >::type pin(pinSEXP);
    Rcpp::traits::input_parameter< Rcpp::IntegerVector >::type chunk_sizes(chunk_sizesSEXP);
    Rcpp::traits::input_parameter< SEXP >::type v(vSEXP);
    rcpp_result_gen = Rcpp::wrap(libgdalcubes_create_image_collection_cube(pin, chunk_sizes, v));
    return rcpp_result_gen;
END_RCPP
}
// libgdalcubes_create_reduce_cube
SEXP libgdalcubes_create_reduce_cube(SEXP pin, std::string reducer);
RcppExport SEXP _gdalcubes_libgdalcubes_create_reduce_cube(SEXP pinSEXP, SEXP reducerSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< SEXP >::type pin(pinSEXP);
    Rcpp::traits::input_parameter< std::string >::type reducer(reducerSEXP);
    rcpp_result_gen = Rcpp::wrap(libgdalcubes_create_reduce_cube(pin, reducer));
    return rcpp_result_gen;
END_RCPP
}
// libgdalcubes_create_join_bands_cube
SEXP libgdalcubes_create_join_bands_cube(SEXP pinA, SEXP pinB);
RcppExport SEXP _gdalcubes_libgdalcubes_create_join_bands_cube(SEXP pinASEXP, SEXP pinBSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< SEXP >::type pinA(pinASEXP);
    Rcpp::traits::input_parameter< SEXP >::type pinB(pinBSEXP);
    rcpp_result_gen = Rcpp::wrap(libgdalcubes_create_join_bands_cube(pinA, pinB));
    return rcpp_result_gen;
END_RCPP
}
// libgdalcubes_create_select_bands_cube
SEXP libgdalcubes_create_select_bands_cube(SEXP pin, std::vector<std::string> bands);
RcppExport SEXP _gdalcubes_libgdalcubes_create_select_bands_cube(SEXP pinSEXP, SEXP bandsSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< SEXP >::type pin(pinSEXP);
    Rcpp::traits::input_parameter< std::vector<std::string> >::type bands(bandsSEXP);
    rcpp_result_gen = Rcpp::wrap(libgdalcubes_create_select_bands_cube(pin, bands));
    return rcpp_result_gen;
END_RCPP
}
// libgdalcubes_create_apply_pixel_cube
SEXP libgdalcubes_create_apply_pixel_cube(SEXP pin, std::vector<std::string> expr, std::vector<std::string> names);
RcppExport SEXP _gdalcubes_libgdalcubes_create_apply_pixel_cube(SEXP pinSEXP, SEXP exprSEXP, SEXP namesSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< SEXP >::type pin(pinSEXP);
    Rcpp::traits::input_parameter< std::vector<std::string> >::type expr(exprSEXP);
    Rcpp::traits::input_parameter< std::vector<std::string> >::type names(namesSEXP);
    rcpp_result_gen = Rcpp::wrap(libgdalcubes_create_apply_pixel_cube(pin, expr, names));
    return rcpp_result_gen;
END_RCPP
}
// libgdalcubes_debug_output
void libgdalcubes_debug_output(bool debug);
RcppExport SEXP _gdalcubes_libgdalcubes_debug_output(SEXP debugSEXP) {
BEGIN_RCPP
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< bool >::type debug(debugSEXP);
    libgdalcubes_debug_output(debug);
    return R_NilValue;
END_RCPP
}
// libgdalcubes_eval_cube
void libgdalcubes_eval_cube(SEXP pin, std::string outfile);
RcppExport SEXP _gdalcubes_libgdalcubes_eval_cube(SEXP pinSEXP, SEXP outfileSEXP) {
BEGIN_RCPP
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< SEXP >::type pin(pinSEXP);
    Rcpp::traits::input_parameter< std::string >::type outfile(outfileSEXP);
    libgdalcubes_eval_cube(pin, outfile);
    return R_NilValue;
END_RCPP
}
// libgdalcubes_create_stream_cube
SEXP libgdalcubes_create_stream_cube(SEXP pin, std::string cmd);
RcppExport SEXP _gdalcubes_libgdalcubes_create_stream_cube(SEXP pinSEXP, SEXP cmdSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< SEXP >::type pin(pinSEXP);
    Rcpp::traits::input_parameter< std::string >::type cmd(cmdSEXP);
    rcpp_result_gen = Rcpp::wrap(libgdalcubes_create_stream_cube(pin, cmd));
    return rcpp_result_gen;
END_RCPP
}
// libgdalcubes_set_threads
void libgdalcubes_set_threads(IntegerVector n);
RcppExport SEXP _gdalcubes_libgdalcubes_set_threads(SEXP nSEXP) {
BEGIN_RCPP
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< IntegerVector >::type n(nSEXP);
    libgdalcubes_set_threads(n);
    return R_NilValue;
END_RCPP
}

static const R_CallMethodDef CallEntries[] = {
    {"_gdalcubes_libgdalcubes_is_null", (DL_FUNC) &_gdalcubes_libgdalcubes_is_null, 1},
    {"_gdalcubes_libgdalcubes_version", (DL_FUNC) &_gdalcubes_libgdalcubes_version, 0},
    {"_gdalcubes_libgdalcubes_init", (DL_FUNC) &_gdalcubes_libgdalcubes_init, 0},
    {"_gdalcubes_libgdalcubes_cleanup", (DL_FUNC) &_gdalcubes_libgdalcubes_cleanup, 0},
    {"_gdalcubes_libgdalcubes_datetime_values", (DL_FUNC) &_gdalcubes_libgdalcubes_datetime_values, 1},
    {"_gdalcubes_libgdalcubes_cube_info", (DL_FUNC) &_gdalcubes_libgdalcubes_cube_info, 1},
    {"_gdalcubes_libgdalcubes_get_cube_view", (DL_FUNC) &_gdalcubes_libgdalcubes_get_cube_view, 1},
    {"_gdalcubes_libgdalcubes_update_cube_view", (DL_FUNC) &_gdalcubes_libgdalcubes_update_cube_view, 2},
    {"_gdalcubes_libgdalcubes_open_image_collection", (DL_FUNC) &_gdalcubes_libgdalcubes_open_image_collection, 1},
    {"_gdalcubes_libgdalcubes_image_collection_info", (DL_FUNC) &_gdalcubes_libgdalcubes_image_collection_info, 1},
    {"_gdalcubes_libgdalcubes_create_image_collection", (DL_FUNC) &_gdalcubes_libgdalcubes_create_image_collection, 4},
    {"_gdalcubes_libgdalcubes_list_collection_formats", (DL_FUNC) &_gdalcubes_libgdalcubes_list_collection_formats, 0},
    {"_gdalcubes_libgdalcubes_create_image_collection_cube", (DL_FUNC) &_gdalcubes_libgdalcubes_create_image_collection_cube, 3},
    {"_gdalcubes_libgdalcubes_create_reduce_cube", (DL_FUNC) &_gdalcubes_libgdalcubes_create_reduce_cube, 2},
    {"_gdalcubes_libgdalcubes_create_join_bands_cube", (DL_FUNC) &_gdalcubes_libgdalcubes_create_join_bands_cube, 2},
    {"_gdalcubes_libgdalcubes_create_select_bands_cube", (DL_FUNC) &_gdalcubes_libgdalcubes_create_select_bands_cube, 2},
    {"_gdalcubes_libgdalcubes_create_apply_pixel_cube", (DL_FUNC) &_gdalcubes_libgdalcubes_create_apply_pixel_cube, 3},
    {"_gdalcubes_libgdalcubes_debug_output", (DL_FUNC) &_gdalcubes_libgdalcubes_debug_output, 1},
    {"_gdalcubes_libgdalcubes_eval_cube", (DL_FUNC) &_gdalcubes_libgdalcubes_eval_cube, 2},
    {"_gdalcubes_libgdalcubes_create_stream_cube", (DL_FUNC) &_gdalcubes_libgdalcubes_create_stream_cube, 2},
    {"_gdalcubes_libgdalcubes_set_threads", (DL_FUNC) &_gdalcubes_libgdalcubes_set_threads, 1},
    {NULL, NULL, 0}
};

RcppExport void R_init_gdalcubes(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
