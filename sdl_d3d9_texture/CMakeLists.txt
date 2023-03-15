cmake_minimum_required(VERSION 3.18)
project(sdl_d3d9_texture)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

if (NOT WIN32)
option(USE_CONAN "Use Conan build system" OFF)
option(USE_NINE "Use Gallium Nine for native D3D9 API" OFF)
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
endif()

### Set up output paths
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug)

if (WIN32 OR USE_CONAN)
include(conan)
### for CONAN_PKG::directx
conan_add_remote(NAME storm
    URL https://storm.jfrog.io/artifactory/api/conan/conan-remote
    VERIFY_SSL True
)
conan_cmake_run(CONANFILE conanfile.py
    BASIC_SETUP CMAKE_TARGETS
    BUILD missing
)
set(SDL_DEPS "CONAN_PKG::sdl")
else()
#sudo apt install libsdl2-dev
set(SDL_DEPS "SDL2")
endif()

# Source files

if (USE_NINE)
    add_definitions(-DUSE_NINE=1)
endif()

if (MSVC)
    add_compile_options(/std:c++latest)
else()
    add_compile_options(-std=c++20)
endif()

set(SRC_FILES
    "src/d3d_utility.cpp"
    "src/d3d_utility.h"
    "src/sdl_d3d9_texture.cpp"
)

add_executable(${PROJECT_NAME} WIN32 ${SRC_FILES})

# Dependencies

if (WIN32)
    message("Using Windows D3D9 API")
    set(NATIVE_D3D9_LIBS "CONAN_PKG::directx")
else()

    include(ExternalProject)
    ExternalProject_Add(gli
        GIT_REPOSITORY    https://github.com/g-truc/gli
        GIT_TAG           779b99ac6656e4d30c3b24e96e0136a59649a869
        GIT_SHALLOW       ON
        BUILD_ALWAYS      OFF
        BUILD_IN_SOURCE   ON
        CONFIGURE_HANDLED_BY_BUILD ON
        BUILD_COMMAND     ""
        BUILD_BYPRODUCTS  <SOURCE_DIR>/gli/gli.hpp
        INSTALL_COMMAND   ""
    )
    ExternalProject_Get_property(gli SOURCE_DIR BINARY_DIR)
    target_include_directories(${PROJECT_NAME} PRIVATE
         "${SOURCE_DIR}"
    )
    add_dependencies(${PROJECT_NAME} gli)

    if (USE_NINE) # for Gallium Nine
        message("Using Gallium Nine for native D3D9 API")

        #sudo apt install libd3dadapter9-mesa-dev
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(D3D REQUIRED IMPORTED_TARGET d3d)

        ExternalProject_Add(nine-native
            GIT_REPOSITORY    https://github.com/q4a/nine-native
            GIT_TAG           main
            GIT_SHALLOW       ON
            BUILD_ALWAYS      OFF
            BUILD_BYPRODUCTS  <BINARY_DIR>/libnine-native.a
            INSTALL_COMMAND   ""
        )
        ExternalProject_Get_property(nine-native SOURCE_DIR BINARY_DIR)
        set(NINE_NATIVE_INCLUDE_DIRS
            "${SOURCE_DIR}/include"
            "${SOURCE_DIR}/include/D3D9"
            "${SOURCE_DIR}/include/windows"
        )
        set(NATIVE_D3D9_LIBS
            "${BINARY_DIR}/libnine-native.a"
            "PkgConfig::D3D"
            X11
            xcb
            xcb-present
            xcb-dri3
            xcb-xfixes
            X11-xcb
        )
        target_include_directories(${PROJECT_NAME} PRIVATE
            "${NINE_NATIVE_INCLUDE_DIRS}"
        )
        add_dependencies(${PROJECT_NAME} nine-native)
    else() # for DXVK Native
        message("Using DXVK Native for D3D9 API")

        ExternalProject_Add(dxvk-native
            GIT_REPOSITORY    https://github.com/q4a/dxvk-native
            GIT_TAG           master
            GIT_SHALLOW       ON
            BUILD_ALWAYS      OFF
            CONFIGURE_HANDLED_BY_BUILD ON
            CONFIGURE_COMMAND meson setup ../dxvk-native --buildtype=release -Denable_d3d11=false -Denable_d3d10=false -Denable_dxgi=false
            BUILD_COMMAND     ninja
            BUILD_BYPRODUCTS  <BINARY_DIR>/src/d3d9/libdxvk_d3d9.so
            INSTALL_COMMAND   ""
        )
        ExternalProject_Get_property(dxvk-native SOURCE_DIR BINARY_DIR)
        set(DXVK_NATIVE_INCLUDE_DIRS
            "${SOURCE_DIR}/include/native/directx"
            "${SOURCE_DIR}/include/native/windows"
        )
        set(NATIVE_D3D9_LIBS ${BINARY_DIR}/src/d3d9/libdxvk_d3d9.so)
        target_include_directories(${PROJECT_NAME} PRIVATE
            "${DXVK_NATIVE_INCLUDE_DIRS}"
        )
        add_dependencies(${PROJECT_NAME} dxvk-native)
    endif()
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${NATIVE_D3D9_LIBS}
    ${SDL_DEPS}
)

# Data files

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
configure_files(${PROJECT_SOURCE_DIR}/../textures ${PROJECT_BINARY_DIR}/bin/textures)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
