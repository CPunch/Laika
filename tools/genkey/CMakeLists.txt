cmake_minimum_required(VERSION 3.16)

project(genKey VERSION 1.0)

# Put CMake targets (ALL_BUILD/ZERO_CHECK) into a folder
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# compile genKey
file(GLOB_RECURSE GENKEYSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/**.c)
add_executable(genKey ${GENKEYSOURCE})
target_link_libraries(genKey PUBLIC LaikaLib)
