cmake_minimum_required(VERSION 3.16)

project(VMBoxGen VERSION 1.0)

# Put CMake targets (ALL_BUILD/ZERO_CHECK) into a folder
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# compile vmTest
file(GLOB_RECURSE VMTESTSOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/**.c)
add_executable(VMBoxGen ${VMTESTSOURCE})
target_link_libraries(VMBoxGen PUBLIC LaikaLib)

# add the 'DEBUG' preprocessor definition if we're compiling as Debug
target_compile_definitions(VMBoxGen PUBLIC "$<$<CONFIG:Debug>:DEBUG>")

# generate the VMBOXCONFIG file
if(LAIKA_OBFUSCATE)
    add_custom_command(TARGET VMBoxGen POST_BUILD
        DEPENDS VMBoxGen
        COMMAND VMBoxGen ${LAIKA_VMBOXCONFIG}
        COMMENT "Generating VMBox data & keys..."
    )
endif()