# Kitchen CAD Designer - System Interfaces

This directory contains the main system interfaces for the Kitchen CAD Designer application. These interfaces define the contracts for the core functionality of the system.

## Implemented Interfaces

### IGeometryEngine
- **Purpose**: Defines geometric operations including primitive creation, boolean operations, and geometric analysis
- **Key Methods**: 
  - `createBox()`, `createCylinder()`, `createSphere()`, `createCone()`
  - `performBoolean()` for union, difference, and intersection operations
  - `getBoundingBox()`, `getVolume()`, `getSurfaceArea()`
  - `intersects()`, `distanceBetween()`, `transform()`

### ISceneManager  
- **Purpose**: Manages objects in the 3D scene including addition, removal, selection, and spatial queries
- **Key Methods**:
  - `addObject()`, `removeObject()`, `getObject()`
  - `getObjectsInRegion()`, `findIntersectingObjects()`, `findNearbyObjects()`
  - `moveObject()`, `translateObject()`, `rotateObject()`, `scaleObject()`
  - Selection management: `setSelection()`, `addToSelection()`, `clearSelection()`

### IProjectRepository
- **Purpose**: Handles project persistence operations using SQLite for local storage
- **Key Methods**:
  - CRUD operations: `createProject()`, `loadProject()`, `saveProject()`, `deleteProject()`
  - Project queries: `listProjects()`, `listRecentProjects()`, `searchProjects()`
  - Backup/restore: `exportProject()`, `importProject()`, `backup()`, `restore()`
  - Auto-save support: `enableAutoSave()`, `disableAutoSave()`

### IRenderEngine
- **Purpose**: Defines 3D rendering operations for visualization and output
- **Key Methods**:
  - Rendering: `renderScene()`, `renderToBuffer()`, `renderToFile()`
  - Camera management: `setCamera()`, `getCamera()`
  - Lighting: `addLight()`, `removeLight()`, `clearLights()`
  - Material management: `registerMaterial()`, `unregisterMaterial()`
  - Selection/picking: `pickObject()`, `pickObjects()`

### ICADExporter
- **Purpose**: Exports 3D models and scenes to various CAD formats
- **Key Methods**:
  - Format support: `getSupportedFormats()`, `isFormatSupported()`
  - Export operations: `exportObject()`, `exportObjects()`, `exportProject()`
  - Selective export: `exportByCategory()`, `exportSelection()`
  - Batch operations: `exportMultipleFormats()`
  - Validation: `validateExportPath()`, `canExportObject()`

## Supporting Types

### Core Types (src/core/)
- **Shape3D**: Abstract base class for 3D geometric shapes
- **Face**: Represents a face of a 3D shape
- **Camera3D**: 3D camera with position, orientation, and projection parameters
- **Light**: Base class for scene lights (Ambient, Directional, Point, Spot)
- **Material**: Material properties for physically-based rendering

### Enumerations
- **BooleanOperation**: Union, Difference, Intersection
- **ProjectionType**: Perspective, Orthographic  
- **RenderQuality**: Draft, Standard, High, Production
- **ViewMode**: Wireframe, Shaded, ShadedWithEdges, Realistic, Raytraced
- **CADFormat**: STEP, IGES, STL, OBJ, PLY, COLLADA, GLTF, X3D
- **LightType**: Ambient, Directional, Point, Spot

## Integration with Existing System

The interfaces are designed to work seamlessly with the existing geometry system:
- Uses existing `Point3D`, `Vector3D`, `Transform3D`, `BoundingBox` types
- Compatible with the `Matrix4x4` and `GeometryUtils` classes
- Follows the established `KitchenCAD` namespace structure
- Integrates with the CMake build system

## Requirements Mapping

This implementation addresses the following requirements from the specification:

- **Requirement 1.1**: Project creation and management (IProjectRepository)
- **Requirement 4.1**: Object placement and manipulation (ISceneManager)  
- **Requirement 6.1**: 2D/3D visualization (IRenderEngine)
- **Requirement 9.1**: CAD format export (ICADExporter)

## Next Steps

These interfaces provide the foundation for implementing the concrete classes that will handle:
1. OpenCascade integration for geometry operations
2. Qt-based scene management and rendering
3. SQLite database operations for project persistence
4. CAD file format export functionality

The interfaces use dependency injection patterns to allow for easy testing and different backend implementations.