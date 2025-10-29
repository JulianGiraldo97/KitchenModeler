#include "DesignCanvas.h"
#include "../scene/SceneManager.h"
#include "../core/Camera3D.h"
#include "../utils/Logger.h"

#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QTime>
#include <QApplication>
#include <cmath>

using namespace KitchenCAD::UI;
using namespace KitchenCAD::Scene;
using namespace KitchenCAD::Core;
using namespace KitchenCAD::Utils;

DesignCanvas::DesignCanvas(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_viewMode(ViewMode::View3D)
    , m_interactionMode(InteractionMode::Select)
    , m_viewportWidth(800)
    , m_viewportHeight(600)
    , m_aspectRatio(4.0f/3.0f)
    , m_mousePressed(false)
    , m_pressedButton(Qt::NoButton)
    , m_ctrlPressed(false)
    , m_shiftPressed(false)
    , m_altPressed(false)
    , m_gridVisible(true)
    , m_snapToGrid(true)
    , m_gridSize(1.0f)
    , m_gridVBO(0)
    , m_gridVAO(0)
    , m_axesVisible(true)
    , m_axesVBO(0)
    , m_axesVAO(0)
    , m_selectionBoxActive(false)
    , m_animationEnabled(true)
    , m_initialized(false)
    , m_needsUpdate(true)
    , m_frameCount(0)
    , m_fps(0.0f)
    , m_zoom2D(1.0f)
    , m_pan2D(0.0f, 0.0f)
    , m_backgroundColor(64, 64, 64)
    , m_gridColor(128, 128, 128)
    , m_selectionColor(255, 165, 0) // Orange
{
    LOG_INFO("Initializing DesignCanvas");
    
    // Set OpenGL format
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(4); // Anti-aliasing
    setFormat(format);
    
    // Initialize camera
    m_camera = std::make_unique<Camera3D>();
    m_camera->setPosition(QVector3D(5.0f, 5.0f, 5.0f));
    m_camera->setTarget(QVector3D(0.0f, 0.0f, 0.0f));
    m_camera->setUp(QVector3D(0.0f, 0.0f, 1.0f));
    
    // Initialize scene manager
    m_sceneManager = std::make_unique<SceneManager>();
    
    // Setup animation timer
    m_animationTimer = new QTimer(this);
    connect(m_animationTimer, &QTimer::timeout, this, &DesignCanvas::updateAnimation);
    m_animationTimer->start(16); // ~60 FPS
    
    // Setup axes colors
    m_axesColors[0] = QColor(255, 0, 0);   // X - Red
    m_axesColors[1] = QColor(0, 255, 0);   // Y - Green
    m_axesColors[2] = QColor(0, 0, 255);   // Z - Blue
    
    // Enable mouse tracking and keyboard focus
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    
    LOG_INFO("DesignCanvas initialized");
}

DesignCanvas::~DesignCanvas()
{
    LOG_INFO("Destroying DesignCanvas");
    
    // Cleanup OpenGL resources
    makeCurrent();
    
    if (m_gridVBO) {
        glDeleteBuffers(1, &m_gridVBO);
    }
    if (m_gridVAO) {
        glDeleteVertexArrays(1, &m_gridVAO);
    }
    if (m_axesVBO) {
        glDeleteBuffers(1, &m_axesVBO);
    }
    if (m_axesVAO) {
        glDeleteVertexArrays(1, &m_axesVAO);
    }
    
    doneCurrent();
}

void DesignCanvas::initializeGL()
{
    LOG_INFO("Initializing OpenGL context");
    
    initializeOpenGLFunctions();
    
    // Set clear color
    glClearColor(m_backgroundColor.redF(), m_backgroundColor.greenF(), 
                 m_backgroundColor.blueF(), 1.0f);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Enable multisampling for anti-aliasing
    glEnable(GL_MULTISAMPLE);
    
    // Setup lighting
    setupLighting();
    
    // Setup materials
    setupMaterials();
    
    // Create grid and axes VBOs
    createGridVBO();
    createAxesVBO();
    
    m_initialized = true;
    
    LOG_INFO("OpenGL context initialized successfully");
}

void DesignCanvas::paintGL()
{
    if (!m_initialized) {
        return;
    }
    
    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Update matrices
    updateProjectionMatrix();
    updateViewMatrix();
    
    // Render based on view mode
    switch (m_viewMode) {
        case ViewMode::View2D:
            render2D();
            break;
        case ViewMode::View3D:
            render3D();
            break;
        case ViewMode::ViewSplit:
            renderSplit();
            break;
    }
    
    // Update frame counter
    m_frameCount++;
    if (m_frameTimer.elapsed() >= 1000) {
        m_fps = m_frameCount * 1000.0f / m_frameTimer.elapsed();
        m_frameCount = 0;
        m_frameTimer.restart();
    }
    
    m_needsUpdate = false;
}

void DesignCanvas::resizeGL(int width, int height)
{
    m_viewportWidth = width;
    m_viewportHeight = height;
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    
    glViewport(0, 0, width, height);
    
    updateProjectionMatrix();
    m_needsUpdate = true;
    
    LOG_DEBUGF("Viewport resized to {}x{}", width, height);
}

void DesignCanvas::render3D()
{
    // Set 3D viewport
    glViewport(0, 0, m_viewportWidth, m_viewportHeight);
    
    // Render grid
    if (m_gridVisible) {
        renderGrid();
    }
    
    // Render axes
    if (m_axesVisible) {
        renderAxes();
    }
    
    // Render scene objects
    renderObjects();
    
    // Render selection
    renderSelection();
    
    // Render UI overlays
    renderUI();
}

void DesignCanvas::render2D()
{
    // Set 2D viewport
    glViewport(0, 0, m_viewportWidth, m_viewportHeight);
    
    // Setup 2D projection
    QMatrix4x4 projection;
    float halfWidth = m_viewportWidth * 0.5f / m_zoom2D;
    float halfHeight = m_viewportHeight * 0.5f / m_zoom2D;
    projection.ortho(-halfWidth + m_pan2D.x(), halfWidth + m_pan2D.x(),
                     -halfHeight + m_pan2D.y(), halfHeight + m_pan2D.y(),
                     -1000.0f, 1000.0f);
    
    // Render 2D view (top-down)
    if (m_gridVisible) {
        renderGrid();
    }
    
    renderObjects();
    renderSelection();
    renderUI();
}

void DesignCanvas::renderSplit()
{
    // Left half - 2D view
    glViewport(0, 0, m_viewportWidth / 2, m_viewportHeight);
    render2D();
    
    // Right half - 3D view
    glViewport(m_viewportWidth / 2, 0, m_viewportWidth / 2, m_viewportHeight);
    render3D();
    
    // Draw separator line
    glViewport(0, 0, m_viewportWidth, m_viewportHeight);
    // TODO: Draw vertical line at center
}

void DesignCanvas::renderGrid()
{
    if (!m_gridVAO) {
        return;
    }
    
    // TODO: Implement grid rendering with shaders
    // For now, just a placeholder
    glBindVertexArray(m_gridVAO);
    // Render grid lines
    glBindVertexArray(0);
}

void DesignCanvas::renderAxes()
{
    if (!m_axesVAO) {
        return;
    }
    
    // TODO: Implement axes rendering with shaders
    // For now, just a placeholder
    glBindVertexArray(m_axesVAO);
    // Render coordinate axes
    glBindVertexArray(0);
}

void DesignCanvas::renderObjects()
{
    // TODO: Render scene objects using scene manager
    // This will be implemented when scene objects are available
}

void DesignCanvas::renderSelection()
{
    if (m_selectionBoxActive) {
        // TODO: Render selection box
    }
    
    // TODO: Render selection highlights on selected objects
}

void DesignCanvas::renderUI()
{
    // TODO: Render UI overlays (coordinates, FPS, etc.)
}

void DesignCanvas::mousePressEvent(QMouseEvent *event)
{
    m_mousePressed = true;
    m_pressedButton = event->button();
    m_lastMousePos = event->pos();
    m_mousePressPos = event->pos();
    
    if (event->button() == Qt::LeftButton) {
        if (m_interactionMode == InteractionMode::Select) {
            handleSelection(event->pos());
        }
    }
    
    setFocus(); // Ensure we receive keyboard events
}

void DesignCanvas::mouseMoveEvent(QMouseEvent *event)
{
    QPoint delta = event->pos() - m_lastMousePos;
    m_lastMousePos = event->pos();
    
    if (m_mousePressed) {
        if (m_pressedButton == Qt::MiddleButton || 
            (m_pressedButton == Qt::LeftButton && m_altPressed)) {
            // Pan
            handlePanning(delta);
        } else if (m_pressedButton == Qt::RightButton || 
                   (m_pressedButton == Qt::LeftButton && m_ctrlPressed)) {
            // Rotate
            handleRotation(delta);
        }
        
        m_needsUpdate = true;
        update();
    }
    
    // Update coordinates in status bar
    QVector3D worldPos = screenToWorld(event->pos());
    emit coordinatesChanged(worldPos);
}

void DesignCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    
    m_mousePressed = false;
    m_pressedButton = Qt::NoButton;
    
    if (m_selectionBoxActive) {
        m_selectionBoxActive = false;
        m_needsUpdate = true;
        update();
    }
}

void DesignCanvas::wheelEvent(QWheelEvent *event)
{
    float delta = event->angleDelta().y() / 120.0f; // Standard wheel step
    handleZooming(delta);
    
    m_needsUpdate = true;
    update();
    
    event->accept();
}

void DesignCanvas::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Control:
            m_ctrlPressed = true;
            break;
        case Qt::Key_Shift:
            m_shiftPressed = true;
            break;
        case Qt::Key_Alt:
            m_altPressed = true;
            break;
        case Qt::Key_Delete:
            deleteSelected();
            break;
        case Qt::Key_A:
            if (m_ctrlPressed) {
                selectAll();
            }
            break;
        case Qt::Key_Escape:
            clearSelection();
            break;
        case Qt::Key_F:
            fitToView();
            break;
        case Qt::Key_R:
            resetCamera();
            break;
        case Qt::Key_G:
            setGridVisible(!m_gridVisible);
            break;
    }
    
    QOpenGLWidget::keyPressEvent(event);
}

void DesignCanvas::keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Control:
            m_ctrlPressed = false;
            break;
        case Qt::Key_Shift:
            m_shiftPressed = false;
            break;
        case Qt::Key_Alt:
            m_altPressed = false;
            break;
    }
    
    QOpenGLWidget::keyReleaseEvent(event);
}

void DesignCanvas::updateAnimation()
{
    if (m_animationEnabled && m_needsUpdate) {
        update();
    }
}

void DesignCanvas::handleSelection(const QPoint& screenPos)
{
    // TODO: Implement object picking
    // For now, just emit a placeholder signal
    emit objectSelected("placeholder_object");
}

void DesignCanvas::handlePanning(const QPoint& delta)
{
    if (m_viewMode == ViewMode::View2D) {
        // 2D panning
        m_pan2D += QVector2D(delta.x() / m_zoom2D, -delta.y() / m_zoom2D);
    } else {
        // 3D panning
        QVector3D right = QVector3D::crossProduct(
            m_camera->getTarget() - m_camera->getPosition(),
            m_camera->getUp()
        ).normalized();
        QVector3D up = m_camera->getUp();
        
        float sensitivity = 0.01f;
        QVector3D panDelta = right * (-delta.x() * sensitivity) + up * (delta.y() * sensitivity);
        
        m_camera->setPosition(m_camera->getPosition() + panDelta);
        m_camera->setTarget(m_camera->getTarget() + panDelta);
    }
    
    emit viewChanged();
}

void DesignCanvas::handleRotation(const QPoint& delta)
{
    if (m_viewMode != ViewMode::View2D) {
        // 3D rotation around target
        float sensitivity = 0.5f;
        float yaw = delta.x() * sensitivity;
        float pitch = delta.y() * sensitivity;
        
        // TODO: Implement proper camera rotation
        // This is a simplified version
        QVector3D direction = m_camera->getPosition() - m_camera->getTarget();
        float distance = direction.length();
        
        // Apply rotation (simplified)
        QMatrix4x4 rotation;
        rotation.rotate(yaw, m_camera->getUp());
        rotation.rotate(pitch, QVector3D::crossProduct(direction, m_camera->getUp()));
        
        QVector3D newDirection = rotation * direction;
        m_camera->setPosition(m_camera->getTarget() + newDirection.normalized() * distance);
    }
    
    emit viewChanged();
}

void DesignCanvas::handleZooming(float delta)
{
    if (m_viewMode == ViewMode::View2D) {
        // 2D zoom
        float zoomFactor = 1.0f + delta * 0.1f;
        m_zoom2D *= zoomFactor;
        m_zoom2D = qBound(0.1f, m_zoom2D, 10.0f);
    } else {
        // 3D zoom (move camera closer/farther)
        QVector3D direction = m_camera->getTarget() - m_camera->getPosition();
        float distance = direction.length();
        float zoomFactor = 1.0f - delta * 0.1f;
        float newDistance = distance * zoomFactor;
        newDistance = qBound(0.1f, newDistance, 100.0f);
        
        m_camera->setPosition(m_camera->getTarget() - direction.normalized() * newDistance);
    }
    
    emit viewChanged();
}

QVector3D DesignCanvas::screenToWorld(const QPoint& screenPos)
{
    // TODO: Implement proper screen to world coordinate conversion
    // For now, return a placeholder
    return QVector3D(screenPos.x(), screenPos.y(), 0.0f);
}

QPoint DesignCanvas::worldToScreen(const QVector3D& worldPos)
{
    // TODO: Implement proper world to screen coordinate conversion
    // For now, return a placeholder
    return QPoint(static_cast<int>(worldPos.x()), static_cast<int>(worldPos.y()));
}

QVector3D DesignCanvas::snapToGrid(const QVector3D& position)
{
    if (!m_snapToGrid) {
        return position;
    }
    
    return QVector3D(
        std::round(position.x() / m_gridSize) * m_gridSize,
        std::round(position.y() / m_gridSize) * m_gridSize,
        std::round(position.z() / m_gridSize) * m_gridSize
    );
}

void DesignCanvas::updateProjectionMatrix()
{
    m_projectionMatrix.setToIdentity();
    
    if (m_viewMode == ViewMode::View2D) {
        // Orthographic projection for 2D
        float halfWidth = m_viewportWidth * 0.5f / m_zoom2D;
        float halfHeight = m_viewportHeight * 0.5f / m_zoom2D;
        m_projectionMatrix.ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, -1000.0f, 1000.0f);
    } else {
        // Perspective projection for 3D
        m_projectionMatrix.perspective(45.0f, m_aspectRatio, 0.1f, 1000.0f);
    }
}

void DesignCanvas::updateViewMatrix()
{
    m_viewMatrix.setToIdentity();
    
    if (m_viewMode == ViewMode::View2D) {
        // 2D view matrix (top-down)
        m_viewMatrix.translate(-m_pan2D.x(), -m_pan2D.y(), 0.0f);
    } else {
        // 3D view matrix
        m_viewMatrix.lookAt(
            m_camera->getPosition(),
            m_camera->getTarget(),
            m_camera->getUp()
        );
    }
}

void DesignCanvas::setupLighting()
{
    // TODO: Setup OpenGL lighting
    // This will be implemented with proper shaders
}

void DesignCanvas::setupMaterials()
{
    // TODO: Setup material properties
    // This will be implemented with proper shaders
}

void DesignCanvas::createGridVBO()
{
    // TODO: Create grid vertex buffer object
    // For now, just create empty VBO
    glGenVertexArrays(1, &m_gridVAO);
    glGenBuffers(1, &m_gridVBO);
}

void DesignCanvas::createAxesVBO()
{
    // TODO: Create axes vertex buffer object
    // For now, just create empty VBO
    glGenVertexArrays(1, &m_axesVAO);
    glGenBuffers(1, &m_axesVBO);
}

// Public interface methods
void DesignCanvas::setViewMode(ViewMode mode)
{
    m_viewMode = mode;
    m_needsUpdate = true;
    update();
    emit viewChanged();
}

void DesignCanvas::setInteractionMode(InteractionMode mode)
{
    m_interactionMode = mode;
}

void DesignCanvas::resetCamera()
{
    m_camera->setPosition(QVector3D(5.0f, 5.0f, 5.0f));
    m_camera->setTarget(QVector3D(0.0f, 0.0f, 0.0f));
    m_camera->setUp(QVector3D(0.0f, 0.0f, 1.0f));
    
    m_zoom2D = 1.0f;
    m_pan2D = QVector2D(0.0f, 0.0f);
    
    m_needsUpdate = true;
    update();
    emit viewChanged();
}

void DesignCanvas::fitToView()
{
    // TODO: Implement fit to view based on scene bounds
    resetCamera();
}

void DesignCanvas::setViewDirection(const QVector3D& direction)
{
    QVector3D target = m_camera->getTarget();
    float distance = (m_camera->getPosition() - target).length();
    m_camera->setPosition(target + direction.normalized() * distance);
    
    m_needsUpdate = true;
    update();
    emit viewChanged();
}

void DesignCanvas::selectAll()
{
    // TODO: Select all objects in scene
    LOG_DEBUG("Select all objects");
    emit objectsChanged();
}

void DesignCanvas::clearSelection()
{
    m_selectedObjects.clear();
    m_needsUpdate = true;
    update();
    emit objectsChanged();
}

void DesignCanvas::deleteSelected()
{
    // TODO: Delete selected objects
    LOG_DEBUG("Delete selected objects");
    m_selectedObjects.clear();
    emit objectsChanged();
}

QStringList DesignCanvas::getSelectedObjects() const
{
    return m_selectedObjects;
}

void DesignCanvas::addObjectToScene(const QString& catalogItemId, const QVector3D& position)
{
    // TODO: Add object to scene using scene manager
    LOG_DEBUGF("Adding object {} at position ({}, {}, {})", 
               catalogItemId.toStdString(), position.x(), position.y(), position.z());
    
    m_needsUpdate = true;
    update();
    emit objectsChanged();
}

void DesignCanvas::removeObjectFromScene(const QString& objectId)
{
    // TODO: Remove object from scene
    LOG_DEBUGF("Removing object {}", objectId.toStdString());
    
    m_selectedObjects.removeAll(objectId);
    m_needsUpdate = true;
    update();
    emit objectsChanged();
}

void DesignCanvas::setGridVisible(bool visible)
{
    m_gridVisible = visible;
    m_needsUpdate = true;
    update();
}

void DesignCanvas::setSnapToGrid(bool snap)
{
    m_snapToGrid = snap;
}

void DesignCanvas::setGridSize(float size)
{
    m_gridSize = qMax(0.1f, size);
    m_needsUpdate = true;
    update();
}

#include "DesignCanvas.moc"