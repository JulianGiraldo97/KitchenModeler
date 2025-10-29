# Test configuration for Kitchen CAD Designer
# This file will be populated with actual tests as development progresses

if(NOT Catch2_FOUND)
    message(WARNING "Catch2 not found, skipping test configuration")
    return()
endif()

# Test source files
set(TEST_SOURCES
    test_main.cpp
    test_geometry.cpp
    test_persistence_basic.cpp
    test_models.cpp
    test_scene_manager.cpp
    test_validation.cpp
    ../src/utils/Logger.cpp
    ../src/persistence/DatabaseManager.cpp
    ../src/models/Project.cpp
    ../src/models/CatalogItem.cpp
    ../src/scene/SceneManager.cpp
    ../src/validation/ValidationService.cpp
    ../src/validation/ValidationRules.cpp
    ../src/validation/ValidationVisualizer.cpp
)

# Add OpenCascade tests if available
if(HAVE_OPENCASCADE)
    list(APPEND TEST_SOURCES
        test_opencascade_geometry.cpp
        ../src/geometry/OCCTShape3D.cpp
        ../src/geometry/OpenCascadeGeometryEngine.cpp
    )
endif()

# Create test executable
add_executable(KitchenCADDesigner_tests ${TEST_SOURCES})

# Link libraries
target_link_libraries(KitchenCADDesigner_tests
    Catch2::Catch2WithMain
    Qt6::Core
    Qt6::Widgets
    SQLite::SQLite3
)

# Link OpenCascade libraries if available
if(HAVE_OPENCASCADE)
    target_link_libraries(KitchenCADDesigner_tests ${OpenCASCADE_LIBRARIES})
    target_compile_definitions(KitchenCADDesigner_tests PRIVATE HAVE_OPENCASCADE)
endif()

# Link nlohmann/json if available
if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(KitchenCADDesigner_tests nlohmann_json::nlohmann_json)
endif()

# Include directories
target_include_directories(KitchenCADDesigner_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Add compile definitions
target_compile_definitions(KitchenCADDesigner_tests PRIVATE
    QT_NO_KEYWORDS
    NOMINMAX
)

# Register tests with CTest
include(Catch)
catch_discover_tests(KitchenCADDesigner_tests)