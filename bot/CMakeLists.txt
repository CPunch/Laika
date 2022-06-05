cmake_minimum_required(VERSION 3.16)

set(BOT_INCLUDEDIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

project(LaikaBot VERSION 1.0)

# Put CMake targets (ALL_BUILD/ZERO_CHECK) into a folder
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# compile LaikaBot
file(GLOB_RECURSE BOTSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/**.c)
file(GLOB_RECURSE BOTHEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/**.h)

# include platform specific backends
if(WIN32)
    file(GLOB_RECURSE BOTPLATFORMSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/win/**.c)
    if (NOT LAIKA_DEBUG_BUILD)
        set(BOTFLAGS WIN32)
    endif ()
elseif(UNIX AND NOT APPLE)
    file(GLOB_RECURSE BOTPLATFORMSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/lin/**.c)
    set(BOTPLATFORMLIBS util)
endif ()

add_executable(LaikaBot ${BOTFLAGS} ${BOTSOURCE} ${BOTHEADERS} ${BOTPLATFORMSOURCE})
target_link_libraries(LaikaBot PUBLIC LaikaLib ${BOTPLATFORMLIBS})

# make sure lboxconfig.h is generated before building
if(LAIKA_OBFUSCATE)
    add_dependencies(LaikaBot VMBoxGen)
endif ()

# add the 'DEBUG' preprocessor definition if we're compiling as Debug
target_compile_definitions(LaikaBot PUBLIC "$<$<CONFIG:Debug>:DEBUG>")

# add include directory
target_include_directories(LaikaBot PUBLIC ${BOT_INCLUDEDIR})

# strip symbols for UNIX binaries
if((UNIX AND NOT APPLE) AND NOT LAIKA_DEBUG_BUILD)
    message(STATUS "Stripping LaikaBot symbols...")
    target_link_options(LaikaBot PRIVATE -s)
endif ()