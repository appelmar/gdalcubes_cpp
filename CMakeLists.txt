cmake_minimum_required(VERSION 3.8)
project(gdalcubes LANGUAGES CXX VERSION 0.0.1)


#string(TIMESTAMP build_date "%Y%m%d")
#string(TIMESTAMP build_time "%H%M%S")

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type specified, using Debug")
    set(CMAKE_BUILD_TYPE Debug)
endif (NOT CMAKE_BUILD_TYPE)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug") # enable profiling and coverage reports (gcc specific, only for debug)
    message(STATUS "Coverage reports and profiling will be enabled")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg --coverage")
   # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg --coverage" )
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg --coverage")
    #set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined")
    #set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -fsanitize=address -fsanitize=undefined")
endif ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")


set(CMAKE_CXX_STANDARD 11)

add_subdirectory(src)

# cppcheck
find_program (CPPCHECK NAMES cppcheck)
if(${CPPCHECK-NOTFOUND})
    message(WARNING "could not find cppcheck, skipping static code analysis.")
else (${CPPCHECK-NOTFOUND})
    add_custom_target(cppcheck ALL
            COMMAND ${CPPCHECK} --enable=warning,performance  --language=c++ src 2> cppcheck.log
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Running Cppcheck" VERBATIM)
endif()

# code formatting
find_program (CLANG_FORMAT NAMES clang-format)
if(${CLANG_FORMAT-NOTFOUND})
    message(WARNING "could not find clang-format, skipping automated source code formatting.")
else (${CLANG_FORMAT-NOTFOUND})
    file(GLOB_RECURSE ALL_SOURCE_FILES  src/*.cpp src/*.h)
    # clang-format -i -style=file $
    add_custom_target(clangformat ALL
                      COMMAND ${CLANG_FORMAT} -i -style=file ${ALL_SOURCE_FILES}
                      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                      COMMENT "Auto-formatting source code" VERBATIM)
endif()


# export src archive
find_program (TARBIN NAMES tar)
if(${TARBIN-NOTFOUND})
    message(WARNING "could not find tar, skipping source code archive creation.")
else (${TARBIN-NOTFOUND})
    add_custom_target(srczip ALL COMMAND ${CMAKE_COMMAND} -E chdir .. tar "cfz" "${PROJECT_NAME}_src_${PROJECT_VERSION}.tar.gz" src  CMakeLists.txt LICENSE .gitignore .clang-format README.md)
endif()
