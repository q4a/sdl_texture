cmake_minimum_required(VERSION 3.18)
project(d3dx9_terrain)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/utils.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(conan)
conan_add_remote(NAME storm
    URL https://storm.jfrog.io/artifactory/api/conan/conan-remote
    VERIFY_SSL True
)

### Set up output paths
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/bin)

### Set up third-party dependencies
set(ENV{CONAN_REVISIONS_ENABLED} 1)
conan_add_remote(NAME storm
    URL https://storm.jfrog.io/artifactory/api/conan/conan-remote
    VERIFY_SSL True
)
conan_cmake_run(CONANFILE conanfile.py
    BASIC_SETUP CMAKE_TARGETS
    BUILD missing
)

list(APPEND PROJECT_DIRS "src")
add_dir("${PROJECT_DIRS}" "${PROJECT_NAME}")

add_executable(${PROJECT_NAME} WIN32 ${${PROJECT_NAME}_SOURCE} ${${PROJECT_NAME}_HEADER})

target_link_libraries(${PROJECT_NAME} PRIVATE
    winmm
    CONAN_PKG::directx
)

configure_file(${PROJECT_SOURCE_DIR}/textures/coastMountain64.raw ${PROJECT_BINARY_DIR}/bin/coastMountain64.raw COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/textures/desert.bmp ${PROJECT_BINARY_DIR}/bin/desert.bmp COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/textures/grass.bmp ${PROJECT_BINARY_DIR}/bin/grass.bmp COPYONLY)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
