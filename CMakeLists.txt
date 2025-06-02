cmake_minimum_required(VERSION 3.16)

set(PROJECT_VER "1.0")
set(PROJECT_VER_NUMBER 1)

# git clone https://github.com/UncleRus/esp-idf-lib
set(EXTRA_COMPONENT_DIRS "$ENV{HOME}/Tools/esp-idf-lib/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

idf_build_set_property(MINIMAL_BUILD ON)

project(heatpump_controller)
