cmake_minimum_required (VERSION 3.11)
project(adb_analyzer)

add_definitions( -DLOGIC2 )

# enable generation of compile_commands.json, helpful for IDEs to locate include files.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(SOURCES
src/ADBAnalyzer.cpp
src/ADBAnalyzer.h
src/ADBAnalyzerResults.cpp
src/ADBAnalyzerResults.h
src/ADBAnalyzerSettings.cpp
src/ADBAnalyzerSettings.h
src/ADBSimulationDataGenerator.cpp
src/ADBSimulationDataGenerator.h
)

add_analyzer_plugin(adb_analyzer SOURCES ${SOURCES})
