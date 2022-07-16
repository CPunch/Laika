cmake_minimum_required(VERSION 3.16)
project(Laika)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

execute_process(COMMAND git rev-parse --short HEAD OUTPUT_VARIABLE GIT_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)

# Set the project as the default startup project for VS
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT LaikaLib)

# Output binaries to the bin folder in the source directory
if(WIN32)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/winbin)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/winbin)
else()
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
endif()

#find_package(Sanitizers)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif ()

string(TOLOWER ${CMAKE_BUILD_TYPE} RAWCMAKEBUILDTYPE)
message(STATUS "CMAKE_BUILD_TYPE: " ${RAWCMAKEBUILDTYPE})
if(RAWCMAKEBUILDTYPE STREQUAL "debug")
  set(LAIKA_DEBUG_BUILD on)
  if(WIN32)
    # statically link debug libs for windows
    message(STATUS "Adding MSVCRT Debug libs...")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
  else()
    message(STATUS "Adding sanitizer...")

    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
  endif ()
else()
  set(LAIKA_DEBUG_BUILD off)
  # statically link non-debug libs
  if(WIN32)
    message(STATUS "Adding MSVCRT libs...")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
  endif ()
endif ()

# include libsodium
set(SODIUM_DISABLE_TESTS ON)
set(SODIUM_MINIMAL ON)
set(SODIUM_STATIC ON)
add_subdirectory(libsodium)

# ===================================== [[ CONFIG DEFAULTS ]] =====================================

set(LAIKA_VMBOXCONFIG ${CMAKE_SOURCE_DIR}/lib/include/lboxconfig.h)

# DO NOT USE THESE KEYS, TESTING ONLY (TODO: make vmboxgen auto gen these)
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
set(LAIKA_VERSION_MINOR 4)

message(STATUS "Building config file...")
configure_file(${CMAKE_SOURCE_DIR}/lib/include/lconfig.h.in ${CMAKE_SOURCE_DIR}/lib/include/lconfig.h)

# config vm boxes
add_subdirectory(tools/vmboxgen)

# ====================================== [[ BUILD TOOLING ]] ======================================

# compile laikalib, tools, cnc & bot
add_subdirectory(lib)
add_subdirectory(tools)

# these subprojects don't support windows (sorry)
add_subdirectory(bot) # windows support Soon:tm:
if(NOT WIN32 AND (UNIX AND NOT APPLE))
  add_subdirectory(cnc)
  add_subdirectory(shell)
endif ()
