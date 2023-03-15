from conans import ConanFile, tools
from os import getenv
from random import getrandbits
from distutils.dir_util import copy_tree

class SdlPure(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    # dependencies used in deploy binaries
    # conan-center
    requires = ["sdl/2.0.18"]
    # optional dependencies
    def requirements(self):
        if self.settings.os == "Windows":
            # storm.jfrog.io
            self.requires("directx/9.0@storm/prebuilt")
        else:
            # conan-center
            self.requires("zlib/1.2.13")#fix for error: 'libunwind/1.6.2' requires 'zlib/1.2.12' while 'libxml2/2.9.14' requires 'zlib/1.2.13'

    generators = "cmake_multi"
