# Source files will be added here as development progresses
# This is a placeholder CMakeLists.txt for the src directory

# For now, create a simple main.cpp placeholder
set(SOURCES
    main.cpp
)

# Create the main executable
add_executable(KitchenCADDesigner ${SOURCES})

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