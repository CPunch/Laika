cmake_minimum_required(VERSION 3.16)

project(vmTest VERSION 1.0)

# Put CMake targets (ALL_BUILD/ZERO_CHECK) into a folder
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# compile vmTest
file(GLOB_RECURSE VMTESTSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/**.c)
add_executable(vmTest ${VMTESTSOURCE})
target_link_libraries(vmTest PUBLIC LaikaLib)

# add the 'DEBUG' preprocessor definition if we're compiling as Debug
target_compile_definitions(vmTest PUBLIC "$<$<CONFIG:Debug>:DEBUG>")
