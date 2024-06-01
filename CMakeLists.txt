cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(Jarvis)

file(GLOB jarvisHeaderFiles CONFIGURE_DEPENDS
     "Jarvis/Headers/jarvis/*.h"
)

file(GLOB jarvisSourceFiles CONFIGURE_DEPENDS
    "Jarvis/Source/*.c"
)

file(GLOB_RECURSE jarvisAssets CONFIGURE_DEPENDS
    "assets/*.*"
)

add_executable(Jarvis ${jarvisSourceFiles} ${jarvisHeaderFiles})

set_property(TARGET Jarvis PROPERTY COMPILE_WARNING_AS_ERROR ON)
set_target_properties(Jarvis PROPERTIES COMPILE_FLAGS "/std:c11 /ZI /Od")

include(FetchContent)

FetchContent_Populate(
    ferret
    GIT_REPOSITORY https://github.com/artyredd/SharpFerret.git
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/ferret
)

# Add the Ferret project as a subdirectory
add_subdirectory(external/Ferret)

# Ensure dependencies are built before the Jarvis target
add_dependencies(Jarvis Core Engine)

target_include_directories(Jarvis PRIVATE
    "external/Ferret/Core/Headers"
    "external/Ferret/Engine/Headers"
)

# Copy assets to the build directory
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR}/bin/assets)

target_link_libraries(Jarvis PRIVATE
    Core
    Engine
    Synchronization
)

set_target_properties(Jarvis PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)