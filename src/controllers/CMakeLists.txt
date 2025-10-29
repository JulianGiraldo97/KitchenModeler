# Controllers library
set(CONTROLLERS_SOURCES
    DesignController.cpp
    ProjectController.cpp
    CatalogController.cpp
    ExportController.cpp
    RenderController.cpp
)

set(CONTROLLERS_HEADERS
    DesignController.h
    ProjectController.h
    CatalogController.h
    ExportController.h
    RenderController.h
)

add_library(controllers ${CONTROLLERS_SOURCES} ${CONTROLLERS_HEADERS})

target_link_libraries(controllers
    interfaces
    models
    services
    utils
    geometry
    core
)

target_include_directories(controllers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})