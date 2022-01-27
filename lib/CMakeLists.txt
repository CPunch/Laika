cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(LIB_INCLUDEDIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# version details
set(LAIKA_VERSION_MAJOR 0)
set(LAIKA_VERSION_MINOR 0)

project(LaikaLib VERSION ${LAIKA_VERSION_MAJOR}.${LAIKA_VERSION_MINOR})

# Put CMake targets (ALL_BUILD/ZERO_CHECK) into a folder
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(sodium_USE_STATIC_LIBS)
find_package(Sodium)

# compile LaikaLib library
file(GLOB_RECURSE LIBSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/**.c)
add_library(LaikaLib STATIC ${LIBSOURCE})
target_link_libraries(LaikaLib PRIVATE ${sodium_LIBRARY_RELEASE})

# add the version definitions and the 'DEBUG' preprocessor definition if we're compiling as Debug
target_compile_definitions(LaikaLib PUBLIC LAIKA_VERSION_MAJOR=${LAIKA_VERSION_MAJOR} LAIKA_VERSION_MINOR=${LAIKA_VERSION_MINOR} "$<$<CONFIG:Debug>:DEBUG>")

# add include directory
target_include_directories(LaikaLib PUBLIC ${LIB_INCLUDEDIR} ${sodium_INCLUDE_DIR})

# set library name
set_target_properties(LaikaLib PROPERTIES OUTPUT_NAME laika-${LAIKA_VERSION_MAJOR}.${LAIKA_VERSION_MINOR})
