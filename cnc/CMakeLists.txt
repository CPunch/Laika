cmake_minimum_required(VERSION 3.16)

set(CNC_INCLUDEDIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

project(LaikaCNC VERSION 1.0)

# Put CMake targets (ALL_BUILD/ZERO_CHECK) into a folder
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# compile LaikaCNC
file(GLOB_RECURSE CNCSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/**.c)
file(GLOB_RECURSE CNCHEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/**.h)
add_executable(LaikaCNC ${CNCSOURCE} ${CNCHEADERS})
target_link_libraries(LaikaCNC PUBLIC LaikaLib)

# add include directory
target_include_directories(LaikaCNC PUBLIC ${CNC_INCLUDEDIR})
