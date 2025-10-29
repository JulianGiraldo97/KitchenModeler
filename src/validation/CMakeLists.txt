# Validation module CMakeLists.txt

set(VALIDATION_SOURCES
    ValidationService.cpp
    ValidationRules.cpp
    ValidationVisualizer.cpp
)

set(VALIDATION_HEADERS
    ValidationService.h
    ValidationRules.h
    ValidationVisualizer.h
)

# Add validation sources to the main target
target_sources(${PROJECT_NAME} PRIVATE
    ${VALIDATION_SOURCES}
    ${VALIDATION_HEADERS}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)