cmake_minimum_required(VERSION 3.16)

set(SHELL_INCLUDEDIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

project(LaikaShell VERSION 1.0)

# Put CMake targets (ALL_BUILD/ZERO_CHECK) into a folder
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# compile LaikaShell
file(GLOB_RECURSE SHELLSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/**.c)
file(GLOB_RECURSE SHELLHEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/**.h)
add_executable(LaikaShell ${SHELLSOURCE} ${SHELLHEADERS})
target_link_libraries(LaikaShell PUBLIC LaikaLib)

# add include directory
target_include_directories(LaikaShell PUBLIC ${SHELL_INCLUDEDIR})
