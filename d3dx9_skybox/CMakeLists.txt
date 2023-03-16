cmake_minimum_required(VERSION 3.18)
project(d3dx9_skybox)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/utils.cmake)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include(conan)
conan_add_remote(NAME storm
    URL https://storm.jfrog.io/artifactory/api/conan/conan-remote
    VERIFY_SSL True
)

### Set up output paths
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug)

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

list(APPEND PROJECT_DIRS
    "dxut/core"
    "dxut/optional"
    "src"
    )
add_dir("${PROJECT_DIRS}" "${PROJECT_NAME}")

add_definitions(-DUNICODE) # for dxut

add_executable(${PROJECT_NAME} WIN32  ${${PROJECT_NAME}_SOURCE} ${${PROJECT_NAME}_HEADER})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/dxut/core
    ${CMAKE_SOURCE_DIR}/dxut/optional
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    comctl32
    legacy_stdio_definitions
    winmm
    CONAN_PKG::directx
)

macro(configure_files srcDir destDir)
    message(STATUS "Configuring directory ${destDir}")
    make_directory(${destDir})

    file(GLOB_RECURSE templateFiles RELATIVE ${srcDir} "${srcDir}/*")
    foreach(templateFile ${templateFiles})
        set(srcTemplatePath ${srcDir}/${templateFile})
        if(NOT IS_DIRECTORY ${srcTemplatePath})
            message(STATUS "Configuring file ${templateFile}")
            configure_file(
                    ${srcTemplatePath}
                    ${destDir}/${templateFile}
                    COPYONLY)
        endif(NOT IS_DIRECTORY ${srcTemplatePath})
    endforeach(templateFile)
endmacro(configure_files)
configure_files(${PROJECT_SOURCE_DIR}/LightProbes ${PROJECT_BINARY_DIR}/bin/LightProbes)
configure_files(${PROJECT_SOURCE_DIR}/misc ${PROJECT_BINARY_DIR}/bin/misc)

configure_file(${PROJECT_SOURCE_DIR}/src/HDRFormats.fx ${PROJECT_BINARY_DIR}/bin/HDRFormats.fx COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/src/skybox.fx ${PROJECT_BINARY_DIR}/bin/skybox.fx COPYONLY)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
