cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(TTSPlugin)

# Verbose logging function
macro(log_message message)
  message(STATUS "Log: ${message}")
endmacro()

# Download and include vcpkg
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  log_message("Downloading vcpkg...")
  execute_process(
    COMMAND git clone https://github.com/microsoft/vcpkg.git ${CMAKE_BINARY_DIR}/vcpkg
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    RESULT_VARIABLE result
  )
  if(result)
    message(FATAL_ERROR "Failed to clone vcpkg. Result: ${result}")
  else()
    log_message("vcpkg cloned successfully.")
  endif()
  
  execute_process(
    COMMAND ${CMAKE_BINARY_DIR}/vcpkg/bootstrap-vcpkg.bat
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/vcpkg
    RESULT_VARIABLE result
  )
  if(result)
    message(FATAL_ERROR "Failed to bootstrap vcpkg. Result: ${result}")
  else()
    log_message("vcpkg bootstrapped successfully.")
  endif()
  
  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_BINARY_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "Vcpkg toolchain file")
  log_message("vcpkg toolchain file set: ${CMAKE_TOOLCHAIN_FILE}")
endif()

# Ensure the vcpkg executable exists
set(VCPKG_EXECUTABLE ${CMAKE_BINARY_DIR}/vcpkg/vcpkg.exe)
if(NOT EXISTS ${VCPKG_EXECUTABLE})
  message(FATAL_ERROR "vcpkg executable not found at ${VCPKG_EXECUTABLE}")
else()
  log_message("vcpkg executable found at ${VCPKG_EXECUTABLE}")
endif()

# Ensure the required packages are installed
log_message("Installing packages: curl and sdl2...")
execute_process(
  COMMAND ${VCPKG_EXECUTABLE} install curl:x64-windows sdl2:x64-windows
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/vcpkg
  RESULT_VARIABLE result
)
if(result)
  message(FATAL_ERROR "Failed to install packages. Result: ${result}")
else()
  log_message("Packages installed successfully.")
endif()

# List all header and source files
file(GLOB pluginHeaderFiles CONFIGURE_DEPENDS
     "Plugin/Headers/plugin/*.h"
)
log_message("Plugin header files: ${pluginHeaderFiles}")

file(GLOB pluginSourceFiles CONFIGURE_DEPENDS
    "Plugin/Source/*.c"
)
log_message("Plugin source files: ${pluginSourceFiles}")

file(GLOB_RECURSE pluginAssets CONFIGURE_DEPENDS
    "assets/*.*"
)
log_message("Plugin assets: ${pluginAssets}")

add_library(TTSPlugin SHARED ${pluginSourceFiles} ${pluginHeaderFiles})

set_property(TARGET TTSPlugin PROPERTY COMPILE_WARNING_AS_ERROR ON)
set_target_properties(TTSPlugin PROPERTIES COMPILE_FLAGS "/std:c11 /ZI /Od /experimental:c11atomics")
target_compile_definitions(TTSPlugin PRIVATE BUILD_STATIC_LIB)

include(FetchContent)

FetchContent_Populate(
    ferret
    GIT_REPOSITORY https://github.com/artyredd/SharpFerret.git
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/ferret
)
log_message("Fetched content for Ferret repository.")

# Add the Ferret project as a subdirectory
add_subdirectory(external/ferret)
log_message("Added Ferret subdirectory.")

# Ensure dependencies are built before the Plugin target
add_dependencies(TTSPlugin Core Engine)
log_message("Added dependencies for Plugin: Core and Engine.")

find_package(CURL CONFIG REQUIRED)
log_message("Found CURL: ${CURL_INCLUDE_DIRS}")

find_package(SDL2 CONFIG REQUIRED)
log_message("Found SDL2: ${SDL2_INCLUDE_DIRS}")

target_include_directories(TTSPlugin PRIVATE
    "external/Ferret/Core/Headers"
    "external/Ferret/Engine/Headers"
    ${CURL_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
)
log_message("Set target include directories for Plugin.")

# Copy assets to the build directory
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR}/bin/)
log_message("Copied assets to ${CMAKE_BINARY_DIR}/bin/")

target_link_libraries(TTSPlugin PRIVATE
    Core
    Engine
    Synchronization
    CURL::libcurl SDL2::SDL2 SDL2::SDL2main
)
log_message("Linked libraries for Plugin: Core, Engine, Synchronization, CURL::libcurl, SDL2::SDL2, SDL2::SDL2main.")

set_target_properties(TTSPlugin PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
  ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
  ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin"
  ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin"
  LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
  LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
  LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin"
  LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin"
)
log_message("Set target properties for Plugin output directories.")
