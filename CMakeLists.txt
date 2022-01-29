cmake_minimum_required(VERSION 3.10)
project(Laika)

# Set the project as the default startup project for VS
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Laika)

# include our cmake modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake-modules" ${CMAKE_MODULE_PATH})

# Output binaries to the bin folder in the source directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

#find_package(Sanitizers)

if(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE Release)
endif()

string(TOLOWER ${CMAKE_BUILD_TYPE} RAWCMAKEBUILDTYPE)
message(STATUS "CMAKE_BUILD_TYPE: " ${RAWCMAKEBUILDTYPE})
if(RAWCMAKEBUILDTYPE STREQUAL "debug")
  message(STATUS "Adding sanitizer...")
  #set(SANITIZE_ADDRESS TRUE)
  #add_sanitizers(LaikaLib LaikaBot LaikaCNC)

  # bug in FindSanitizer.cmake ??? idfk, i'll investigate l8tr
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif ()

# compile laikalib, tools, cnc & bot
add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(cnc)
add_subdirectory(bot)
