
configure_file( ${CMAKE_CURRENT_LIST_DIR}/build_info.h.in ${CMAKE_CURRENT_LIST_DIR}/build_info.h )


## find GDAL
find_path(gdal_INCLUDEDIR "gdal.h" HINTS $ENV{GDAL_INCLUDEDIR} PATH_SUFFIXES gdal)
if (gdal_INCLUDEDIR-NOTFOUND)
    message(FATAL_ERROR "GDAL headers not found")
endif (gdal_INCLUDEDIR-NOTFOUND)

find_library(gdal_LIBRARY NAMES gdal gdal_i HINTS $ENV{GDAL_LIBDIR})
if (NOT gdal_LIBRARY)
    message(FATAL_ERROR "GDAL library not found!")
else (NOT gdal_LIBRARY)
    message(STATUS "GDAL library found at ${gdal_LIBRARY}")
endif (NOT gdal_LIBRARY)


##  find PROJ
find_path(proj_INCLUDEDIR "proj_api.h" HINTS $ENV{PROJ_INCLUDEDIR})
if (proj_INCLUDEDIR-NOTFOUND)
    message(FATAL_ERROR "PROJ headers not found")
endif (proj_INCLUDEDIR-NOTFOUND)

find_library(proj_LIBRARY NAMES proj proj_i HINTS $ENV{PROJ_LIBDIR} )
if (NOT proj_LIBRARY)
    message(FATAL_ERROR "PROJ library not found!")
else (NOT proj_LIBRARY)
    message(STATUS "PROJ library found at ${proj_LIBRARY}")
endif (NOT proj_LIBRARY)



## find boost
find_package(Boost COMPONENTS program_options filesystem system regex date_time REQUIRED)
if (Boost_FOUND)
    message(STATUS "Boost headers found at ${Boost_INCLUDE_DIRS}")
else (Boost_FOUND)
    message(FATAL_ERROR "Boost headers not found")
endif (Boost_FOUND)



## find SQLite
find_path(sqlite_INCLUDEDIR "sqlite3.h")
if (sqlite_INCLUDEDIR-NOTFOUND)
    message(FATAL_ERROR "SQLite headers not found")
endif (sqlite_INCLUDEDIR-NOTFOUND)

find_library(sqlite_LIBRARY NAMES sqlite sqlite3)
if (NOT sqlite_LIBRARY)
    message(FATAL_ERROR "SQLite library not found!")
else (NOT sqlite_LIBRARY)
    message(STATUS "SQLite library found at ${sqlite_LIBRARY}")
endif (NOT sqlite_LIBRARY)


find_package(Threads)



## find NETCDF_CXX4 library
find_path(NETCDF_CXX_INCLUDEDIR "netcdf")
if (NETCDF_CXX_INCLUDEDIR-NOTFOUND)
message(FATAL_ERROR "netcdf-cxx4 headers not found")
endif (NETCDF_CXX_INCLUDEDIR-NOTFOUND)

find_library(NETCDF_CXX_LIBRARY netcdf_c++4)
if (NOT NETCDF_CXX_LIBRARY)
message(FATAL_ERROR "netcdf-cxx4 library not found!")
else (NOT NETCDF_CXX_LIBRARY)
message(STATUS "netcdf-cxx4 library found at ${NETCDF_CXX_LIBRARY}")
endif (NOT NETCDF_CXX_LIBRARY)



include_directories (${gdal_INCLUDEDIR} ${proj_INCLUDEDIR} ${Boost_INCLUDE_DIRS} ${geos_INCLUDEDIR} ${sqlite_INCLUDEDIR} ${NETCDF_CXX_INCLUDEDIR})



file(GLOB SOURCE_FILES
    "*.cpp" "*.h"
)

list(REMOVE_ITEM SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/example.cpp)
list(REMOVE_ITEM SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/gdalcubes.cpp)
list(REMOVE_ITEM SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp)



add_library(libgdalcubes  ${SOURCE_FILES})
set_target_properties(libgdalcubes PROPERTIES PREFIX "")
target_link_libraries(libgdalcubes ${gdal_LIBRARY} ${Boost_LIBRARIES} ${sqlite_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${NETCDF_CXX_LIBRARY})

add_executable(gdalcubes ${CMAKE_CURRENT_SOURCE_DIR}/gdalcubes.cpp)
target_link_libraries (gdalcubes libgdalcubes)


file(GLOB TEST_FILES
        "test/*.cpp" "*.h")

add_executable(gdalcubes_test ${TEST_FILES})
target_link_libraries (gdalcubes_test libgdalcubes)


find_library(CPPRESTSDK_LIB cpprest)
find_library(OPENSSL_LIB ssl)
find_library(CRYPTO_LIB crypto)
if (NOT CPPRESTSDK_LIB OR NOT OPENSSL_LIB OR NOT CRYPTO_LIB)
    message(WARNING "Skipping build of gdalcubes_worker due to missing libraries.")
else (NOT CPPRESTSDK_LIB OR NOT OPENSSL_LIB OR NOT CRYPTO_LIB)
    message(STATUS "cpprestsdk library found at ${CPPRESTSDK_LIB}")
    message(STATUS "openssl library found at ${OPENSSL_LIB}")
    message(STATUS "crypto library found at ${CRYPTO_LIB}")
    add_executable(gdalcubes_server ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp)
    target_link_libraries (gdalcubes_server libgdalcubes ${CPPRESTSDK_LIB} ${OPENSSL_LIB} ${CRYPTO_LIB})
endif (NOT CPPRESTSDK_LIB OR NOT OPENSSL_LIB OR NOT CRYPTO_LIB)





add_executable(gdalcubes_example ${CMAKE_CURRENT_SOURCE_DIR}/example.cpp)
target_link_libraries (gdalcubes_example libgdalcubes)
