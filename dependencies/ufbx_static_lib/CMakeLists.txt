cmake_minimum_required(VERSION 3.12)
project(ufbx_project LANGUAGES C CXX)

add_library(ufbx STATIC ../ufbx/ufbx.c ../ufbx/ufbx.h)

target_include_directories(ufbx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../ufbx/)

target_compile_features(ufbx PUBLIC c_std_99)