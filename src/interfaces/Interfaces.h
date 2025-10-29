#pragma once

/**
 * @file Interfaces.h
 * @brief Main header file that includes all system interfaces
 * 
 * This header provides convenient access to all the main system interfaces
 * for the Kitchen CAD Designer application.
 */

// Core interfaces
#include "IGeometryEngine.h"
#include "ISceneManager.h"
#include "IProjectRepository.h"
#include "IRenderEngine.h"
#include "ICADExporter.h"
#include "IValidationService.h"

// Supporting types
#include "../core/Shape3D.h"
#include "../core/Camera3D.h"
#include "../core/Light.h"
#include "../core/Material.h"

namespace KitchenCAD {

/**
 * @brief Interface factory for creating default implementations
 * 
 * This class provides factory methods for creating default implementations
 * of the system interfaces. Implementations can be swapped out for testing
 * or different backend technologies.
 */
class InterfaceFactory {
public:
    virtual ~InterfaceFactory() = default;
    
    // Factory methods for creating interface implementations
    virtual std::unique_ptr<IGeometryEngine> createGeometryEngine() = 0;
    virtual std::unique_ptr<ISceneManager> createSceneManager() = 0;
    virtual std::unique_ptr<IProjectRepository> createProjectRepository(const std::string& databasePath) = 0;
    virtual std::unique_ptr<IRenderEngine> createRenderEngine() = 0;
    virtual std::unique_ptr<ICADExporter> createCADExporter() = 0;
    virtual std::unique_ptr<IValidationService> createValidationService() = 0;
};

} // namespace KitchenCAD