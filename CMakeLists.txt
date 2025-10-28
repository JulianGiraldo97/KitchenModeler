cmake_minimum_required(VERSION 3.20)
project(KitchenCADDesigner VERSION 1.0.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Widgets 
    OpenGL 
    OpenGLWidgets
    Sql
)

# Qt6 setup
qt6_standard_project_setup()

# Find OpenCascade (optional for now)
find_package(OpenCASCADE QUIET)
if(OpenCASCADE_FOUND)
    message(STATUS "Found OpenCASCADE version: ${OpenCASCADE_VERSION}")
    include_directories(${OpenCASCADE_INCLUDE_DIR})
    set(HAVE_OPENCASCADE TRUE)
else()
    message(WARNING "OpenCASCADE not found. Some features will be disabled.")
    set(HAVE_OPENCASCADE FALSE)
endif()

# Find SQLite3
find_package(SQLite3 REQUIRED)
if(SQLite3_FOUND)
    message(STATUS "Found SQLite3 version: ${SQLite3_VERSION}")
else()
    message(FATAL_ERROR "SQLite3 not found. Please install SQLite3 development libraries.")
endif()

# Find nlohmann/json
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann/json not found via find_package, trying pkg-config")
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(nlohmann_json QUIET nlohmann_json)
    endif()
    
    if(NOT nlohmann_json_FOUND)
        message(WARNING "nlohmann/json not found. Trying to use system headers.")
        # Try to find the header directly
        find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp
            PATHS /usr/include /usr/local/include /opt/homebrew/include
        )
        if(NLOHMANN_JSON_INCLUDE_DIR)
            message(STATUS "Found nlohmann/json headers at: ${NLOHMANN_JSON_INCLUDE_DIR}")
            set(nlohmann_json_FOUND TRUE)
        else()
            message(FATAL_ERROR "nlohmann/json is required but not found. Please install it.")
        endif()
    endif()
endif()

# Find Catch2 for testing (optional)
find_package(Catch2 3 QUIET)
if(Catch2_FOUND)
    message(STATUS "Found Catch2 for testing")
    enable_testing()
else()
    message(WARNING "Catch2 not found. Unit tests will not be available.")
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Add subdirectories
add_subdirectory(src)

# Add tests if Catch2 is available
if(Catch2_FOUND)
    add_subdirectory(tests)
endif()

# Installation configuration
install(TARGETS KitchenCADDesigner
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib/static
)

# Install resources
install(DIRECTORY resources/
    DESTINATION share/KitchenCADDesigner/resources
    FILES_MATCHING PATTERN "*"
)

# CPack configuration for packaging
include(CPack)
set(CPACK_PACKAGE_NAME "Kitchen CAD Designer")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Professional Kitchen Design Software")
set(CPACK_PACKAGE_DESCRIPTION "A comprehensive CAD application for designing kitchens, bathrooms, and modular furniture with 2D/3D visualization, material management, and export capabilities.")
set(CPACK_PACKAGE_VENDOR "Kitchen CAD Designer Team")
set(CPACK_PACKAGE_CONTACT "support@kitchencaddesigner.com")

# Platform-specific packaging
if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "Kitchen CAD Designer")
    set(CPACK_NSIS_PACKAGE_NAME "Kitchen CAD Designer")
    set(CPACK_NSIS_CONTACT "support@kitchencaddesigner.com")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_FORMAT "UDBZ")
    set(CPACK_DMG_VOLUME_NAME "Kitchen CAD Designer")
else()
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Kitchen CAD Designer Team")
    set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6core6, libqt6widgets6, libqt6opengl6, libsqlite3-0")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Graphics")
    set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase, qt6-qtbase-gui, sqlite")
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "Kitchen CAD Designer Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Qt6 version: ${Qt6_VERSION}")
message(STATUS "  OpenCASCADE: ${OpenCASCADE_FOUND}")
message(STATUS "  SQLite3: ${SQLite3_FOUND}")
message(STATUS "  nlohmann/json: ${nlohmann_json_FOUND}")
message(STATUS "  Catch2 testing: ${Catch2_FOUND}")
message(STATUS "")