#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QVector3D>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QTimer>
#include <QElapsedTimer>
#include <memory>

// Forward declarations
namespace KitchenCAD {
    class Camera3D;
    
    namespace Scene {
        class SceneManager;
    }
    
    namespace Geometry {
        struct Vector3D;
        struct Point3D;
    }
}

namespace KitchenCAD::UI {

enum class ViewMode {
    View2D,
    View3D,
    ViewSplit
};

enum class InteractionMode {
    Select,
    Pan,
    Rotate,
    Zoom
};

class DesignCanvas : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
    Q_DISABLE_COPY(DesignCanvas)

public:
    explicit DesignCanvas(QWidget *parent = nullptr);
    ~DesignCanvas() override;

    // View control
    void setViewMode(ViewMode mode);
    ViewMode getViewMode() const { return m_viewMode; }
    
    void setInteractionMode(InteractionMode mode);
    InteractionMode getInteractionMode() const { return m_interactionMode; }
    
    // Camera control
    void resetCamera();
    void fitToView();
    void setViewDirection(const QVector3D& direction);
    
    // Selection
    void selectAll();
    void clearSelection();
    void deleteSelected();
    QStringList getSelectedObjects() const;
    
    // Object manipulation
    void addObjectToScene(const QString& catalogItemId, const QVector3D& position);
    void removeObjectFromScene(const QString& objectId);
    
    // Drag and drop support
    void startDragPreview(const QString& catalogItemId);
    void updateDragPreview(const QPoint& position);
    void endDragPreview(bool place = false);
    bool isDragPreviewActive() const { return m_dragPreviewActive; }
    
    // Grid and snapping
    void setGridVisible(bool visible);
    bool isGridVisible() const { return m_gridVisible; }
    
    void setSnapToGrid(bool snap);
    bool isSnapToGrid() const { return m_snapToGrid; }
    
    void setGridSize(float size);
    float getGridSize() const { return m_gridSize; }

protected:
    // OpenGL overrides
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    
    // Event handling
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    
    // Drag and drop events
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

Q_SIGNALS:
    void objectSelected(const QString& objectId);
    void objectsChanged();
    void viewChanged();
    void coordinatesChanged(const QVector3D& coordinates);

private Q_SLOTS:
    void updateAnimation();

private:
    // Rendering methods
    void render3D();
    void render2D();
    void renderSplit();
    void renderGrid();
    void renderAxes();
    void renderObjects();
    void renderSelection();
    void renderUI();
    
    // Interaction methods
    void handleSelection(const QPoint& screenPos);
    void handlePanning(const QPoint& delta);
    void handleRotation(const QPoint& delta);
    void handleZooming(float delta);
    
    // Utility methods
    QVector3D screenToWorld(const QPoint& screenPos);
    QPoint worldToScreen(const QVector3D& worldPos);
    void updateProjectionMatrix();
    void updateViewMatrix();
    
    // Setup methods
    void setupShaders();
    void setupLighting();
    void setupMaterials();
    void createGridVBO();
    void createAxesVBO();
    void setupPickingFramebuffer();
    
    // Rendering helper methods
    void renderPlaceholderObjects();
    void renderSelectionBox();
    void renderSelectionHighlights();
    void renderDragPreview();
    void renderValidationFeedback();
    void renderFPSCounter();
    void renderCoordinateDisplay();
    void renderViewModeIndicator();
    
    // Object picking
    QString performObjectPicking(const QPoint& screenPos);
    
    // Utility conversion methods
    QVector3D toQt(const KitchenCAD::Geometry::Vector3D& v) const;
    QVector3D toQt(const KitchenCAD::Geometry::Point3D& p) const;
    KitchenCAD::Geometry::Vector3D fromQt(const QVector3D& v) const;
    KitchenCAD::Geometry::Point3D fromQtPoint(const QVector3D& v) const;
    
public:
    // Validation methods (public for testing)
    bool isValidPlacement(const QString& catalogItemId, const QVector3D& position) const;
    QVector3D getValidatedPosition(const QString& catalogItemId, const QVector3D& position) const;
    QVector3D snapToGrid(const QVector3D& position) const;

private:

private:
    // View state
    ViewMode m_viewMode;
    InteractionMode m_interactionMode;
    
    // Camera and matrices
    std::unique_ptr<KitchenCAD::Camera3D> m_camera;
    QMatrix4x4 m_projectionMatrix;
    QMatrix4x4 m_viewMatrix;
    QMatrix4x4 m_modelMatrix;
    
    // Viewport
    int m_viewportWidth;
    int m_viewportHeight;
    float m_aspectRatio;
    
    // Mouse interaction
    bool m_mousePressed;
    Qt::MouseButton m_pressedButton;
    QPoint m_lastMousePos;
    QPoint m_mousePressPos;
    
    // Keyboard state
    bool m_ctrlPressed;
    bool m_shiftPressed;
    bool m_altPressed;
    
    // Grid and snapping
    bool m_gridVisible;
    bool m_snapToGrid;
    float m_gridSize;
    unsigned int m_gridVBO;
    unsigned int m_gridVAO;
    
    // Axes
    bool m_axesVisible;
    unsigned int m_axesVBO;
    unsigned int m_axesVAO;
    
    // Selection
    QStringList m_selectedObjects;
    QVector3D m_selectionBoxStart;
    QVector3D m_selectionBoxEnd;
    bool m_selectionBoxActive;
    
    // Animation
    QTimer* m_animationTimer;
    bool m_animationEnabled;
    
    // Scene management
    std::unique_ptr<KitchenCAD::Scene::SceneManager> m_sceneManager;
    
    // Rendering state
    bool m_initialized;
    bool m_needsUpdate;
    
    // Performance
    int m_frameCount;
    QElapsedTimer m_frameTimer;
    float m_fps;
    
    // 2D view specific
    float m_zoom2D;
    QVector2D m_pan2D;
    
    // Colors and styling
    QColor m_backgroundColor;
    QColor m_gridColor;
    QColor m_axesColors[3]; // X, Y, Z
    QColor m_selectionColor;
    QColor m_validPlacementColor;
    QColor m_invalidPlacementColor;
    
    // Drag and drop state
    bool m_dragPreviewActive;
    QString m_dragPreviewItemId;
    QVector3D m_dragPreviewPosition;
    QPoint m_lastDragPosition;
    bool m_dragPreviewValid;
    
    // Shader programs
    std::unique_ptr<QOpenGLShaderProgram> m_gridShader;
    std::unique_ptr<QOpenGLShaderProgram> m_axesShader;
    std::unique_ptr<QOpenGLShaderProgram> m_objectShader;
    std::unique_ptr<QOpenGLShaderProgram> m_selectionShader;
    
    // Grid data
    std::vector<float> m_gridVertices;
    int m_gridLineCount;
    
    // Axes data
    std::vector<float> m_axesVertices;
    
    // Object picking
    unsigned int m_pickingFramebuffer;
    unsigned int m_pickingTexture;
    unsigned int m_pickingDepthBuffer;
};

} // namespace KitchenCAD::UI