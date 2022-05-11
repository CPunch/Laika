cmake_minimum_required(VERSION 3.10)

set(LIB_INCLUDEDIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# DO NOT USE THESE KEYS, TESTING ONLY
if(NOT LAIKA_PUBKEY)
    set(LAIKA_PUBKEY "40d5534aca77d1f5ec2bbe79dd9d0f52a78148918f95814404cefe97c34c5c27")
endif ()

if(NOT LAIKA_PRIVKEY)
    set(LAIKA_PRIVKEY "90305aa77023d1c1e03265c3b6af046eb58d6ec8ba650b0dffed01379feab8cc")
endif ()

if(NOT LAIKA_CNC_IP)
    set(LAIKA_CNC_IP "127.0.0.1")
endif ()

if(NOT LAIKA_CNC_PORT)
    set(LAIKA_CNC_PORT "13337")
endif ()

# version details
set(LAIKA_VERSION_MAJOR 0)
set(LAIKA_VERSION_MINOR 2)
set(SODIUM_DISABLE_TESTS ON)

# vmbox config location
set(LAIKA_VMBOXCONFIG ${LIB_INCLUDEDIR}/lboxconfig.h PARENT_SCOPE)

project(LaikaLib VERSION ${LAIKA_VERSION_MAJOR}.${LAIKA_VERSION_MINOR})

message(STATUS "Building config file...")
configure_file(${LIB_INCLUDEDIR}/lconfig.h.in ${LIB_INCLUDEDIR}/lconfig.h)

# Put CMake targets (ALL_BUILD/ZERO_CHECK) into a folder
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# include libsodium
add_subdirectory(libsodium)

# compile LaikaLib library
file(GLOB_RECURSE LIBSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/**.c ${CMAKE_CURRENT_SOURCE_DIR}/vendor/**.c)
file(GLOB_RECURSE LIBHEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/**.h)
add_library(LaikaLib STATIC ${LIBSOURCE} ${LIBHEADERS})
target_link_libraries(LaikaLib PUBLIC sodium)

# add the version definitions and the 'DEBUG' preprocessor definition if we're compiling as Debug
target_compile_definitions(LaikaLib PUBLIC "$<$<CONFIG:Debug>:DEBUG>")

# add include directory
target_include_directories(LaikaLib PUBLIC ${LIB_INCLUDEDIR} ${CMAKE_CURRENT_SOURCE_DIR}/libsodium/libsodium/src/libsodium/include)

# set library name
set_target_properties(LaikaLib PROPERTIES OUTPUT_NAME laika-${LAIKA_VERSION_MAJOR}.${LAIKA_VERSION_MINOR})
