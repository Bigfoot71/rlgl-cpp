cmake_minimum_required(VERSION 3.0)
project(rlgl)

# Define the library version
set(${PROJECT_NAME}_VERSION_MAJOR 4)
set(${PROJECT_NAME}_VERSION_MINOR 5)

# Set the CMP0054 policy to avoid excessive variable expansion in conditionals.
# This is important to prevent unintended variable expansion.
# For example, if "PLATFORM" is "DRM," we want to ensure proper condition checks.
cmake_policy(SET CMP0054 NEW)

# Set the directory for easier includes.
# You can find included files in <root>/cmmake for reference.
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Include build options exposed to CMake.
include(CMakeOptions.txt)

# Include conguration for 'rlgl'
include(ConfigRLGL)

# Include header files.
include_directories(include)

# Retrieve source files.
include(source/CMakeLists.txt)
include(external/glad/CMakeLists.txt)

# Create the static library.
add_library(${PROJECT_NAME} STATIC ${RLGL_SOURCES} ${GLAD_SOURCES})
# You can also create a shared library using the following line:
# add_library(${PROJECT_NAME} SHARED ${RLGL_SOURCES})

# Set library properties, including version information.
set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}
    SOVERSION ${${PROJECT_NAME}_VERSION_MAJOR}
    ${PLATFORM_CPP}
    ${GRAPHICS}
)
