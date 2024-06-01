cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(Plugin)

file(GLOB pluginHeaderFiles CONFIGURE_DEPENDS
     "Plugin/Headers/plugin/*.h"
)

file(GLOB pluginSourceFiles CONFIGURE_DEPENDS
    "Plugin/Source/*.c"
)

file(GLOB_RECURSE pluginAssets CONFIGURE_DEPENDS
    "assets/*.*"
)

add_library(Plugin SHARED ${pluginSourceFiles} ${pluginHeaderFiles})

set_property(TARGET Plugin PROPERTY COMPILE_WARNING_AS_ERROR ON)
set_target_properties(Plugin PROPERTIES COMPILE_FLAGS "/std:c11 /ZI /Od")

include(FetchContent)

FetchContent_Populate(
    ferret
    GIT_REPOSITORY https://github.com/artyredd/SharpFerret.git
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/ferret
)

# Add the Ferret project as a subdirectory
add_subdirectory(external/Ferret)

# Ensure dependencies are built before the Plugin target
add_dependencies(Plugin Core Engine)

target_include_directories(Plugin PRIVATE
    "external/Ferret/Core/Headers"
    "external/Ferret/Engine/Headers"
)

# Copy assets to the build directory
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR}/bin/assets)

target_link_libraries(Plugin PRIVATE
    Core
    Engine
    Synchronization
)

set_target_properties(Plugin PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)