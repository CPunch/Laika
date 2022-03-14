cmake_minimum_required(VERSION 3.10)

set(BOT_INCLUDEDIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

project(LaikaBot VERSION 1.0)

# Put CMake targets (ALL_BUILD/ZERO_CHECK) into a folder
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# compile LaikaBot
file(GLOB_RECURSE BOTSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/**.c)
file(GLOB_RECURSE BOTHEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/**.h)
add_executable(LaikaBot ${BOTSOURCE} ${BOTHEADERS})
target_link_libraries(LaikaBot PUBLIC LaikaLib)

# add the 'DEBUG' preprocessor definition if we're compiling as Debug
target_compile_definitions(LaikaBot PUBLIC "$<$<CONFIG:Debug>:DEBUG>")

# add include directory
target_include_directories(LaikaBot PUBLIC ${BOT_INCLUDEDIR})