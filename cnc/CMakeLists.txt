cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CNC_INCLUDEDIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

project(LaikaCNC VERSION 1.0)

# Put CMake targets (ALL_BUILD/ZERO_CHECK) into a folder
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# compile LaikaCNC
file(GLOB_RECURSE CNCSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/**.c)
add_executable(LaikaCNC ${CNCSOURCE})
target_link_libraries(LaikaCNC PUBLIC LaikaLib)

# add the 'DEBUG' preprocessor definition if we're compiling as Debug
target_compile_definitions(LaikaCNC PUBLIC "$<$<CONFIG:Debug>:DEBUG>")

# add include directory
target_include_directories(LaikaCNC PUBLIC ${CNC_INCLUDEDIR})
