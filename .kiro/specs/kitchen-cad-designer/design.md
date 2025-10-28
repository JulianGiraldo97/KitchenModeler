# Documento de Diseño - Kitchen CAD Designer

## Visión General

Kitchen CAD Designer es una aplicación de escritorio nativa desarrollada en C++20 que utiliza Qt 6 para la interfaz de usuario, OpenCascade (OCCT) como motor geométrico 3D, SQLite para persistencia local y OpenGL para renderizado. La arquitectura sigue un patrón de capas bien definido que separa la presentación, lógica de negocio, motor CAD y persistencia de datos.

## Arquitectura del Sistema

### Arquitectura en Capas

```
┌─────────────────────────────────────────────────────────────┐
│                    CAPA DE PRESENTACIÓN                     │
│  Qt 6 Widgets + QML │ OpenGL Viewport │ Event Handlers     │
├─────────────────────────────────────────────────────────────┤
│                    CAPA DE APLICACIÓN                       │
│  Controllers │ View Models │ Command Pattern │ Observers    │
├─────────────────────────────────────────────────────────────┤
│                    CAPA DE NEGOCIO                          │
│  Project Manager │ Catalog Service │ BOM Generator │ etc.   │
├─────────────────────────────────────────────────────────────┤
│                    CAPA DE MOTOR CAD                        │
│  OpenCascade │ Geometry Engine │ Render Engine │ Exporters │
├─────────────────────────────────────────────────────────────┤
│                    CAPA DE PERSISTENCIA                     │
│  SQLite │ Repository Pattern │ Data Access Objects          │
└─────────────────────────────────────────────────────────────┘
```

### Componentes Principales

#### 1. Capa de Presentación (UI Layer)
- **MainWindow**: Ventana principal con menús, toolbars y paneles
- **DesignCanvas**: Widget personalizado Qt/OpenGL para visualización 2D/3D
- **CatalogPanel**: Panel lateral para navegación del catálogo
- **PropertiesPanel**: Panel de propiedades de objetos seleccionados
- **ProjectExplorer**: Explorador de proyectos y archivos

#### 2. Capa de Aplicación (Application Layer)
- **DesignController**: Controlador principal de la lógica de diseño
- **CatalogController**: Gestión de catálogos y módulos
- **ProjectController**: Operaciones de proyecto (nuevo, abrir, guardar)
- **RenderController**: Control del motor de renderizado
- **ExportController**: Gestión de exportaciones

#### 3. Capa de Negocio (Business Layer)
- **ProjectManager**: Gestión del ciclo de vida de proyectos
- **CatalogService**: Lógica de negocio del catálogo
- **BOMGenerator**: Generación de listas de materiales
- **ValidationService**: Validación de restricciones de diseño
- **PricingService**: Cálculo de precios y costos

#### 4. Capa de Motor CAD (CAD Engine Layer)
- **GeometryEngine**: Wrapper de OpenCascade para operaciones geométricas
- **SceneManager**: Gestión de la escena 3D y objetos
- **RenderEngine**: Motor de renderizado con OpenGL/OpenCascade
- **CADExporter**: Exportación a formatos CAD (STEP, IGES, STL)

#### 5. Capa de Persistencia (Data Layer)
- **DatabaseManager**: Gestión de conexiones SQLite
- **ProjectRepository**: Persistencia de proyectos
- **CatalogRepository**: Persistencia de catálogos
- **ConfigRepository**: Configuración de usuario## Co
mponentes y Interfaces

### Interfaces Principales

#### IGeometryEngine
```cpp
class IGeometryEngine {
public:
    virtual ~IGeometryEngine() = default;
    virtual std::unique_ptr<Shape3D> createBox(const Point3D& origin, double width, double height, double depth) = 0;
    virtual std::unique_ptr<Shape3D> createCylinder(const Point3D& center, double radius, double height) = 0;
    virtual bool performBoolean(Shape3D& target, const Shape3D& tool, BooleanOperation op) = 0;
    virtual std::vector<Face> getFaces(const Shape3D& shape) = 0;
    virtual BoundingBox getBoundingBox(const Shape3D& shape) = 0;
};
```

#### ISceneManager
```cpp
class ISceneManager {
public:
    virtual ~ISceneManager() = default;
    virtual ObjectId addObject(std::unique_ptr<SceneObject> object) = 0;
    virtual bool removeObject(ObjectId id) = 0;
    virtual SceneObject* getObject(ObjectId id) = 0;
    virtual std::vector<ObjectId> getObjectsInRegion(const BoundingBox& region) = 0;
    virtual bool moveObject(ObjectId id, const Transform3D& transform) = 0;
    virtual void setSelection(const std::vector<ObjectId>& selection) = 0;
};
```

#### IProjectRepository
```cpp
class IProjectRepository {
public:
    virtual ~IProjectRepository() = default;
    virtual std::optional<Project> loadProject(const std::string& projectId) = 0;
    virtual bool saveProject(const Project& project) = 0;
    virtual std::vector<ProjectInfo> listProjects() = 0;
    virtual bool deleteProject(const std::string& projectId) = 0;
    virtual std::string createProject(const ProjectMetadata& metadata) = 0;
};
```

### Clases de Dominio Principales

#### Project
```cpp
class Project {
private:
    std::string id_;
    std::string name_;
    ProjectMetadata metadata_;
    RoomDimensions dimensions_;
    std::vector<std::unique_ptr<SceneObject>> objects_;
    std::vector<Wall> walls_;
    std::vector<Opening> openings_;
    
public:
    Project(const std::string& name, const RoomDimensions& dimensions);
    
    // Gestión de objetos
    ObjectId addObject(std::unique_ptr<SceneObject> object);
    bool removeObject(ObjectId id);
    SceneObject* getObject(ObjectId id);
    
    // Gestión de estructura
    void addWall(const Wall& wall);
    void addOpening(const Opening& opening);
    
    // Cálculos
    BillOfMaterials generateBOM() const;
    double calculateTotalPrice() const;
    
    // Serialización
    nlohmann::json toJson() const;
    static Project fromJson(const nlohmann::json& json);
};
```

#### SceneObject
```cpp
class SceneObject {
protected:
    ObjectId id_;
    std::string catalogItemId_;
    Transform3D transform_;
    MaterialProperties material_;
    std::unique_ptr<Shape3D> geometry_;
    
public:
    SceneObject(const std::string& catalogItemId);
    virtual ~SceneObject() = default;
    
    // Propiedades básicas
    ObjectId getId() const { return id_; }
    const Transform3D& getTransform() const { return transform_; }
    void setTransform(const Transform3D& transform);
    
    // Geometría
    const Shape3D& getGeometry() const { return *geometry_; }
    BoundingBox getBoundingBox() const;
    
    // Material
    const MaterialProperties& getMaterial() const { return material_; }
    void setMaterial(const MaterialProperties& material);
    
    // Validación
    virtual bool isValidPlacement(const SceneManager& scene) const = 0;
    virtual std::vector<ValidationError> validate(const SceneManager& scene) const = 0;
    
    // Serialización
    virtual nlohmann::json toJson() const;
    static std::unique_ptr<SceneObject> fromJson(const nlohmann::json& json);
};
```

#### CatalogItem
```cpp
class CatalogItem {
private:
    std::string id_;
    std::string name_;
    std::string category_;
    Dimensions3D dimensions_;
    double basePrice_;
    std::vector<MaterialOption> materialOptions_;
    std::string modelPath_;
    std::string thumbnailPath_;
    
public:
    CatalogItem(const std::string& id, const std::string& name, const std::string& category);
    
    // Propiedades
    const std::string& getId() const { return id_; }
    const std::string& getName() const { return name_; }
    const std::string& getCategory() const { return category_; }
    const Dimensions3D& getDimensions() const { return dimensions_; }
    
    // Precio
    double getPrice(const std::string& materialId = "") const;
    
    // Materiales
    const std::vector<MaterialOption>& getMaterialOptions() const { return materialOptions_; }
    void addMaterialOption(const MaterialOption& option);
    
    // Geometría
    std::unique_ptr<Shape3D> createGeometry(IGeometryEngine& engine) const;
    
    // Serialización
    nlohmann::json toJson() const;
    static CatalogItem fromJson(const nlohmann::json& json);
};
```## Model
os de Datos

### Esquema de Base de Datos SQLite

```sql
-- Tabla de proyectos
CREATE TABLE projects (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    room_width REAL NOT NULL,
    room_height REAL NOT NULL,
    room_depth REAL NOT NULL,
    scene_data TEXT, -- JSON serializado de la escena
    metadata TEXT   -- JSON con metadatos adicionales
);

-- Tabla de elementos del catálogo
CREATE TABLE catalog_items (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    category TEXT NOT NULL,
    width REAL NOT NULL,
    height REAL NOT NULL,
    depth REAL NOT NULL,
    base_price REAL NOT NULL DEFAULT 0.0,
    model_path TEXT,
    thumbnail_path TEXT,
    specifications TEXT, -- JSON con especificaciones técnicas
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Tabla de opciones de materiales
CREATE TABLE material_options (
    id TEXT PRIMARY KEY,
    catalog_item_id TEXT NOT NULL,
    name TEXT NOT NULL,
    texture_path TEXT,
    price_modifier REAL DEFAULT 0.0,
    properties TEXT, -- JSON con propiedades físicas
    FOREIGN KEY (catalog_item_id) REFERENCES catalog_items(id) ON DELETE CASCADE
);

-- Tabla de objetos en escena (para consultas rápidas)
CREATE TABLE scene_objects (
    id TEXT PRIMARY KEY,
    project_id TEXT NOT NULL,
    catalog_item_id TEXT NOT NULL,
    position_x REAL NOT NULL,
    position_y REAL NOT NULL,
    position_z REAL NOT NULL,
    rotation_x REAL DEFAULT 0.0,
    rotation_y REAL DEFAULT 0.0,
    rotation_z REAL DEFAULT 0.0,
    scale_x REAL DEFAULT 1.0,
    scale_y REAL DEFAULT 1.0,
    scale_z REAL DEFAULT 1.0,
    material_id TEXT,
    custom_properties TEXT, -- JSON
    FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE CASCADE,
    FOREIGN KEY (catalog_item_id) REFERENCES catalog_items(id)
);

-- Tabla de configuración de usuario
CREATE TABLE user_config (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Índices para optimización
CREATE INDEX idx_catalog_category ON catalog_items(category);
CREATE INDEX idx_scene_objects_project ON scene_objects(project_id);
CREATE INDEX idx_projects_updated ON projects(updated_at DESC);
```

### Estructuras de Datos en Memoria

#### Tipos Geométricos Básicos
```cpp
struct Point3D {
    double x, y, z;
    Point3D(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
};

struct Vector3D {
    double x, y, z;
    Vector3D(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
    Vector3D normalized() const;
    double length() const;
};

struct Transform3D {
    Point3D translation;
    Vector3D rotation; // Euler angles in radians
    Vector3D scale;
    
    Transform3D() : scale(1.0, 1.0, 1.0) {}
    Matrix4x4 toMatrix() const;
    Transform3D inverse() const;
};

struct BoundingBox {
    Point3D min, max;
    
    BoundingBox() = default;
    BoundingBox(const Point3D& min, const Point3D& max) : min(min), max(max) {}
    
    bool contains(const Point3D& point) const;
    bool intersects(const BoundingBox& other) const;
    Point3D center() const;
    Vector3D size() const;
};
```

#### Propiedades de Materiales
```cpp
struct MaterialProperties {
    std::string id;
    std::string name;
    std::string texturePath;
    
    // Propiedades físicas para renderizado
    Color diffuseColor{1.0f, 1.0f, 1.0f};
    Color specularColor{1.0f, 1.0f, 1.0f};
    float roughness = 0.5f;
    float metallic = 0.0f;
    float reflectance = 0.04f;
    
    // Propiedades para cálculos
    double pricePerSquareMeter = 0.0;
    std::string supplier;
    std::string code;
};
```

#### Lista de Materiales (BOM)
```cpp
struct BOMItem {
    std::string itemId;
    std::string name;
    std::string category;
    int quantity;
    Dimensions3D dimensions;
    std::string material;
    double unitPrice;
    double totalPrice;
    std::string supplier;
    std::string notes;
};

class BillOfMaterials {
private:
    std::vector<BOMItem> items_;
    double totalCost_;
    
public:
    void addItem(const BOMItem& item);
    void removeItem(const std::string& itemId);
    const std::vector<BOMItem>& getItems() const { return items_; }
    double getTotalCost() const { return totalCost_; }
    
    // Exportación
    void exportToCSV(const std::string& filename) const;
    void exportToJSON(const std::string& filename) const;
    nlohmann::json toJson() const;
};
```#
# Manejo de Errores

### Estrategia de Manejo de Errores

#### Jerarquía de Excepciones
```cpp
class KitchenCADException : public std::exception {
protected:
    std::string message_;
    std::string context_;
    
public:
    KitchenCADException(const std::string& message, const std::string& context = "")
        : message_(message), context_(context) {}
    
    const char* what() const noexcept override { return message_.c_str(); }
    const std::string& getContext() const { return context_; }
};

class GeometryException : public KitchenCADException {
public:
    GeometryException(const std::string& message) 
        : KitchenCADException(message, "Geometry") {}
};

class DatabaseException : public KitchenCADException {
public:
    DatabaseException(const std::string& message) 
        : KitchenCADException(message, "Database") {}
};

class ValidationException : public KitchenCADException {
public:
    ValidationException(const std::string& message) 
        : KitchenCADException(message, "Validation") {}
};

class ImportExportException : public KitchenCADException {
public:
    ImportExportException(const std::string& message) 
        : KitchenCADException(message, "Import/Export") {}
};
```

#### Sistema de Validación
```cpp
enum class ValidationSeverity {
    Info,
    Warning,
    Error,
    Critical
};

struct ValidationError {
    ValidationSeverity severity;
    std::string message;
    std::string objectId;
    Point3D location;
    std::string suggestion;
    
    ValidationError(ValidationSeverity sev, const std::string& msg, 
                   const std::string& objId = "", const Point3D& loc = Point3D())
        : severity(sev), message(msg), objectId(objId), location(loc) {}
};

class ValidationService {
public:
    std::vector<ValidationError> validateProject(const Project& project);
    std::vector<ValidationError> validateObject(const SceneObject& object, const SceneManager& scene);
    std::vector<ValidationError> validatePlacement(const SceneObject& object, const Transform3D& transform, const SceneManager& scene);
    
private:
    bool checkCollisions(const SceneObject& object, const SceneManager& scene);
    bool checkDimensions(const SceneObject& object);
    bool checkConstraints(const SceneObject& object, const SceneManager& scene);
};
```

#### Logging y Diagnóstico
```cpp
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

class Logger {
private:
    static std::unique_ptr<Logger> instance_;
    std::ofstream logFile_;
    LogLevel minLevel_;
    
public:
    static Logger& getInstance();
    
    void setLogLevel(LogLevel level) { minLevel_ = level; }
    void setLogFile(const std::string& filename);
    
    template<typename... Args>
    void log(LogLevel level, const std::string& format, Args&&... args);
    
    void debug(const std::string& message) { log(LogLevel::Debug, message); }
    void info(const std::string& message) { log(LogLevel::Info, message); }
    void warning(const std::string& message) { log(LogLevel::Warning, message); }
    void error(const std::string& message) { log(LogLevel::Error, message); }
    void critical(const std::string& message) { log(LogLevel::Critical, message); }
};

// Macros para logging conveniente
#define LOG_DEBUG(msg) Logger::getInstance().debug(msg)
#define LOG_INFO(msg) Logger::getInstance().info(msg)
#define LOG_WARNING(msg) Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) Logger::getInstance().error(msg)
#define LOG_CRITICAL(msg) Logger::getInstance().critical(msg)
```

## Estrategia de Pruebas

### Arquitectura de Pruebas

#### Niveles de Prueba
1. **Pruebas Unitarias**: Clases individuales y funciones
2. **Pruebas de Integración**: Interacción entre componentes
3. **Pruebas de Sistema**: Funcionalidad end-to-end
4. **Pruebas de Rendimiento**: Carga y stress testing

#### Framework de Pruebas
```cpp
// Usando Catch2 para pruebas unitarias
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Project creation and basic operations", "[project]") {
    RoomDimensions dimensions{5.0, 3.0, 2.5}; // 5m x 3m x 2.5m
    Project project("Test Kitchen", dimensions);
    
    REQUIRE(project.getName() == "Test Kitchen");
    REQUIRE(project.getDimensions().width == 5.0);
    REQUIRE(project.getObjects().empty());
}

TEST_CASE("Catalog item geometry creation", "[catalog][geometry]") {
    CatalogItem item("base_60", "Base Cabinet 60cm", "base_cabinets");
    item.setDimensions({0.6, 0.85, 0.6}); // 60cm x 85cm x 60cm
    
    MockGeometryEngine engine;
    auto geometry = item.createGeometry(engine);
    
    REQUIRE(geometry != nullptr);
    auto bbox = geometry->getBoundingBox();
    REQUIRE(bbox.size().x == Approx(0.6));
    REQUIRE(bbox.size().y == Approx(0.85));
    REQUIRE(bbox.size().z == Approx(0.6));
}

TEST_CASE("BOM generation", "[bom][calculation]") {
    Project project("Test Kitchen", {5.0, 3.0, 2.5});
    
    // Agregar algunos módulos
    auto cabinet1 = std::make_unique<CabinetObject>("base_60");
    auto cabinet2 = std::make_unique<CabinetObject>("wall_80");
    
    project.addObject(std::move(cabinet1));
    project.addObject(std::move(cabinet2));
    
    auto bom = project.generateBOM();
    
    REQUIRE(bom.getItems().size() >= 2);
    REQUIRE(bom.getTotalCost() > 0.0);
}
```

#### Mocks y Stubs
```cpp
class MockGeometryEngine : public IGeometryEngine {
public:
    std::unique_ptr<Shape3D> createBox(const Point3D& origin, double width, double height, double depth) override {
        return std::make_unique<MockShape3D>(BoundingBox(origin, Point3D(origin.x + width, origin.y + height, origin.z + depth)));
    }
    
    bool performBoolean(Shape3D& target, const Shape3D& tool, BooleanOperation op) override {
        return true; // Simulación exitosa
    }
    
    // ... otras implementaciones mock
};

class MockProjectRepository : public IProjectRepository {
private:
    std::map<std::string, Project> projects_;
    
public:
    std::optional<Project> loadProject(const std::string& projectId) override {
        auto it = projects_.find(projectId);
        return (it != projects_.end()) ? std::make_optional(it->second) : std::nullopt;
    }
    
    bool saveProject(const Project& project) override {
        projects_[project.getId()] = project;
        return true;
    }
    
    // ... otras implementaciones mock
};
```#
## Integración con Tecnologías Externas

#### OpenCascade (OCCT) Integration
```cpp
class OpenCascadeGeometryEngine : public IGeometryEngine {
private:
    Handle(AIS_InteractiveContext) context_;
    
public:
    OpenCascadeGeometryEngine(Handle(AIS_InteractiveContext) context) : context_(context) {}
    
    std::unique_ptr<Shape3D> createBox(const Point3D& origin, double width, double height, double depth) override {
        gp_Pnt corner(origin.x, origin.y, origin.z);
        TopoDS_Shape box = BRepPrimAPI_MakeBox(corner, width, height, depth).Shape();
        return std::make_unique<OCCTShape3D>(box);
    }
    
    bool performBoolean(Shape3D& target, const Shape3D& tool, BooleanOperation op) override {
        auto* occtTarget = dynamic_cast<OCCTShape3D*>(&target);
        auto* occtTool = dynamic_cast<const OCCTShape3D*>(&tool);
        
        if (!occtTarget || !occtTool) return false;
        
        try {
            TopoDS_Shape result;
            switch (op) {
                case BooleanOperation::Union:
                    result = BRepAlgoAPI_Fuse(occtTarget->getShape(), occtTool->getShape()).Shape();
                    break;
                case BooleanOperation::Difference:
                    result = BRepAlgoAPI_Cut(occtTarget->getShape(), occtTool->getShape()).Shape();
                    break;
                case BooleanOperation::Intersection:
                    result = BRepAlgoAPI_Common(occtTarget->getShape(), occtTool->getShape()).Shape();
                    break;
            }
            occtTarget->setShape(result);
            return true;
        } catch (const Standard_Failure& e) {
            LOG_ERROR("Boolean operation failed: " + std::string(e.GetMessageString()));
            return false;
        }
    }
};
```

#### Qt 6 Integration
```cpp
class DesignCanvas : public QOpenGLWidget {
    Q_OBJECT
    
private:
    std::unique_ptr<ISceneManager> sceneManager_;
    std::unique_ptr<IRenderEngine> renderEngine_;
    Camera3D camera_;
    bool isDragging_ = false;
    QPoint lastMousePos_;
    
public:
    DesignCanvas(QWidget* parent = nullptr);
    
protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    
public slots:
    void addObjectToScene(const QString& catalogItemId);
    void removeSelectedObjects();
    void setViewMode(ViewMode mode);
    void resetCamera();
    
signals:
    void objectSelected(const QString& objectId);
    void objectsChanged();
    void viewChanged();
};

class MainWindow : public QMainWindow {
    Q_OBJECT
    
private:
    DesignCanvas* designCanvas_;
    CatalogPanel* catalogPanel_;
    PropertiesPanel* propertiesPanel_;
    ProjectExplorer* projectExplorer_;
    
    std::unique_ptr<ProjectController> projectController_;
    std::unique_ptr<CatalogController> catalogController_;
    
public:
    MainWindow(QWidget* parent = nullptr);
    
private slots:
    void newProject();
    void openProject();
    void saveProject();
    void exportProject();
    void showPreferences();
    
private:
    void setupUI();
    void setupMenus();
    void setupToolbars();
    void setupStatusBar();
    void connectSignals();
};
```

#### SQLite Integration con sqlite-modern-cpp
```cpp
class SQLiteProjectRepository : public IProjectRepository {
private:
    std::unique_ptr<sqlite::database> db_;
    
public:
    SQLiteProjectRepository(const std::string& dbPath) {
        db_ = std::make_unique<sqlite::database>(dbPath);
        initializeTables();
    }
    
    std::optional<Project> loadProject(const std::string& projectId) override {
        try {
            std::string name, sceneData, metadata;
            double width, height, depth;
            
            *db_ << "SELECT name, room_width, room_height, room_depth, scene_data, metadata "
                    "FROM projects WHERE id = ?" 
                 << projectId
                 >> std::tie(name, width, height, depth, sceneData, metadata);
            
            Project project(name, RoomDimensions{width, height, depth});
            project.setId(projectId);
            
            // Deserializar escena desde JSON
            if (!sceneData.empty()) {
                auto sceneJson = nlohmann::json::parse(sceneData);
                project.loadSceneFromJson(sceneJson);
            }
            
            return project;
        } catch (const sqlite::sqlite_exception& e) {
            LOG_ERROR("Failed to load project: " + std::string(e.what()));
            return std::nullopt;
        }
    }
    
    bool saveProject(const Project& project) override {
        try {
            auto sceneJson = project.serializeSceneToJson();
            auto metadataJson = project.getMetadata().toJson();
            
            *db_ << "INSERT OR REPLACE INTO projects "
                    "(id, name, room_width, room_height, room_depth, scene_data, metadata, updated_at) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, datetime('now'))"
                 << project.getId()
                 << project.getName()
                 << project.getDimensions().width
                 << project.getDimensions().height
                 << project.getDimensions().depth
                 << sceneJson.dump()
                 << metadataJson.dump();
            
            return true;
        } catch (const sqlite::sqlite_exception& e) {
            LOG_ERROR("Failed to save project: " + std::string(e.what()));
            return false;
        }
    }
    
private:
    void initializeTables() {
        // Ejecutar scripts de creación de tablas
        *db_ << R"(
            CREATE TABLE IF NOT EXISTS projects (
                id TEXT PRIMARY KEY,
                name TEXT NOT NULL,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                room_width REAL NOT NULL,
                room_height REAL NOT NULL,
                room_depth REAL NOT NULL,
                scene_data TEXT,
                metadata TEXT
            )
        )";
        
        // ... otras tablas
    }
};
```

### Configuración del Sistema de Build (CMake)

#### CMakeLists.txt Principal
```cmake
cmake_minimum_required(VERSION 3.20)
project(KitchenCADDesigner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configuración de Qt 6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets)
qt6_standard_project_setup()

# Configuración de OpenCascade
find_package(OpenCASCADE REQUIRED)

# Configuración de SQLite
find_package(SQLite3 REQUIRED)

# Configuración de nlohmann/json
find_package(nlohmann_json REQUIRED)

# Configuración de Catch2 para pruebas
find_package(Catch2 3 REQUIRED)

# Directorios de inclusión
include_directories(${OpenCASCADE_INCLUDE_DIR})
include_directories(src)

# Subdirectorios
add_subdirectory(src)
add_subdirectory(tests)

# Configuración de instalación
install(TARGETS KitchenCADDesigner
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib/static)

# CPack para empaquetado
include(CPack)
set(CPACK_PACKAGE_NAME "Kitchen CAD Designer")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Professional Kitchen Design Software")
```

### Dockerfile para Build Reproducible
```dockerfile
FROM ubuntu:22.04

# Instalar dependencias del sistema
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    qt6-base-dev \
    qt6-tools-dev \
    libqt6opengl6-dev \
    libsqlite3-dev \
    libocct-*-dev \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

# Configurar directorio de trabajo
WORKDIR /app

# Copiar código fuente
COPY . .

# Crear directorio de build
RUN mkdir build && cd build

# Configurar y compilar
RUN cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# Ejecutar pruebas
RUN cd build && ctest --output-on-failure

# Punto de entrada
CMD ["./build/src/KitchenCADDesigner"]
```

Este diseño proporciona una arquitectura sólida y escalable que cumple con todos los requisitos especificados, utilizando las mejores prácticas de desarrollo en C++ moderno y las tecnologías solicitadas.