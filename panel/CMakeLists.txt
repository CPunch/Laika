cmake_minimum_required(VERSION 3.10)

set(PANEL_INCLUDEDIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(CURSES_NEED_NCURSES TRUE)

project(LaikaPanel VERSION 1.0)

# Put CMake targets (ALL_BUILD/ZERO_CHECK) into a folder
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(Curses)

# compile LaikaPanel
file(GLOB_RECURSE PANELSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/**.c)
add_executable(LaikaPanel ${PANELSOURCE})
target_link_libraries(LaikaPanel PUBLIC LaikaLib ${CURSES_LIBRARIES})

# add the 'DEBUG' preprocessor definition if we're compiling as Debug
target_compile_definitions(LaikaPanel PUBLIC "$<$<CONFIG:Debug>:DEBUG>")

# add include directory
target_include_directories(LaikaPanel PUBLIC ${PANEL_INCLUDEDIR})
