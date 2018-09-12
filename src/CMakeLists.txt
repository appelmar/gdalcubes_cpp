


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
find_package(Boost COMPONENTS program_options filesystem system regex REQUIRED ) # boost property tree
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





## find libgpkg
#find_path(libgpkg_INCLUDEDIR "gpkg.h")
#if (libgpkg_INCLUDEDIR-NOTFOUND)
#    message(FATAL_ERROR "libgpkg headers not found")
#endif (libgpkg_INCLUDEDIR-NOTFOUND)
#
#find_library(libgpkg_LIBRARY NAMES gpkg)
#if (NOT libgpkg_LIBRARY)
#    message(FATAL_ERROR "libgpkg not found!")
#else (NOT libgpkg_LIBRARY)
#    message(STATUS "libgpkg found at ${libgpkg_LIBRARY}")
#    configure_file( ${CMAKE_CURRENT_LIST_DIR}/libgpkg.inc.in ${CMAKE_CURRENT_LIST_DIR}/libgpkg.inc)
#endif (NOT libgpkg_LIBRARY)





include_directories (${gdal_INCLUDEDIR} ${proj_INCLUDEDIR} ${Boost_INCLUDE_DIRS} ${geos_INCLUDEDIR} ${sqlite_INCLUDEDIR} ${libgpkg_INCLUDEDIR} ../include)



file(GLOB_RECURSE SOURCE_FILES
    "*.cpp" "*.h"
)

add_executable(gdalcubes ${SOURCE_FILES})

target_link_libraries (gdalcubes ${proj_LIBRARY} ${gdal_LIBRARY} ${Boost_LIBRARIES} ${sqlite_LIBRARY} ${libgpkg_LIBRARY})


