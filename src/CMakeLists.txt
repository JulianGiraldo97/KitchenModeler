# Source files for Kitchen CAD Designer
set(SOURCES
    main.cpp
    utils/Logger.cpp
)

# Header files for geometry system
set(GEOMETRY_HEADERS
    geometry/Point3D.h
    geometry/Vector3D.h
    geometry/Matrix4x4.h
    geometry/Transform3D.h
    geometry/BoundingBox.h
    geometry/GeometryUtils.h
    geometry/Geometry.h
)

# Header files for utilities
set(UTILS_HEADERS
    utils/Logger.h
)

# Include directories
target_include_directories(KitchenCADDesigner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Create the main executable
add_executable(KitchenCADDesigner ${SOURCES})

# Add header files to the target (for IDE support)
target_sources(KitchenCADDesigner PRIVATE ${GEOMETRY_HEADERS} ${UTILS_HEADERS})

# Link Qt6 libraries
target_link_libraries(KitchenCADDesigner
    Qt6::Core
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Sql
)

# Link OpenCascade libraries
target_link_libraries(KitchenCADDesigner ${OpenCASCADE_LIBRARIES})

# Link SQLite3
target_link_libraries(KitchenCADDesigner SQLite::SQLite3)

# Link nlohmann/json if found via find_package
if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(KitchenCADDesigner nlohmann_json::nlohmann_json)
endif()

# Set target properties
set_target_properties(KitchenCADDesigner PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# Add compile definitions
target_compile_definitions(KitchenCADDesigner PRIVATE
    QT_NO_KEYWORDS  # Avoid conflicts with OpenCascade
    NOMINMAX        # Avoid min/max macro conflicts on Windows
)