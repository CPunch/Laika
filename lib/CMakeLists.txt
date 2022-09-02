cmake_minimum_required(VERSION 3.16)

set(LIB_INCLUDEDIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

project(LaikaLib VERSION ${LAIKA_VERSION_MAJOR}.${LAIKA_VERSION_MINOR})

# Put CMake targets (ALL_BUILD/ZERO_CHECK) into a folder
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# compile LaikaLib library
file(GLOB_RECURSE LIBSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/**.c)
file(GLOB_RECURSE LIBHEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/**.h)

# include platform specific backends
if(WIN32)
    file(GLOB_RECURSE LIBPLATFORMSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/win/**.c)
elseif(UNIX AND NOT APPLE)
    file(GLOB_RECURSE LIBPLATFORMSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/lin/**.c)
endif ()

add_library(LaikaLib STATIC ${LIBSOURCE} ${LIBHEADERS} ${LIBPLATFORMSOURCE})
target_link_libraries(LaikaLib PUBLIC sodium)

# make sure we're compiled *AFTER* lboxconfig.h has been generated
add_dependencies(LaikaLib VMBoxGen)

# add the version definitions and the 'DEBUG' preprocessor definition if we're compiling as Debug
target_compile_definitions(LaikaLib PUBLIC "$<$<CONFIG:Debug>:DEBUG>")

# add include directory
target_include_directories(LaikaLib PUBLIC ${LIB_INCLUDEDIR} ${CMAKE_CURRENT_SOURCE_DIR}/libsodium/libsodium/src/libsodium/include)

# set library name
set_target_properties(LaikaLib PROPERTIES OUTPUT_NAME laika-${LAIKA_VERSION_MAJOR}.${LAIKA_VERSION_MINOR})
