#include "DesignCanvas.h"
#include "../scene/SceneManager.h"
#include "../core/Camera3D.h"
#include "../utils/Logger.h"
#include "../geometry/Point3D.h"
#include "../geometry/Vector3D.h"

#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QTime>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <cmath>

using namespace KitchenCAD::UI;
using namespace KitchenCAD::Scene;
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
    , m_validPlacementColor(0, 255, 0, 128) // Semi-transparent green
    , m_invalidPlacementColor(255, 0, 0, 128) // Semi-transparent red
    , m_dragPreviewActive(false)
    , m_dragPreviewValid(false)
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
    m_camera = std::make_unique<KitchenCAD::Camera3D>();
    m_camera->setPosition(KitchenCAD::Geometry::Point3D(5.0, 5.0, 5.0));
    m_camera->setTarget(KitchenCAD::Geometry::Point3D(0.0, 0.0, 0.0));
    m_camera->setUp(KitchenCAD::Geometry::Vector3D(0.0, 0.0, 1.0));
    
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
    
    // Enable drag and drop
    setAcceptDrops(true);
    
    // Start frame timer
    m_frameTimer.start();
    
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
    
    // Enable line smoothing
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    
    // Setup shaders
    setupShaders();
    
    // Setup lighting
    setupLighting();
    
    // Setup materials
    setupMaterials();
    
    // Create grid and axes VBOs
    createGridVBO();
    createAxesVBO();
    
    // Setup picking framebuffer
    setupPickingFramebuffer();
    
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
    if (m_frameTimer.hasExpired(1000)) {
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
    
    // Render drag preview if active
    if (m_dragPreviewActive) {
        renderDragPreview();
        renderValidationFeedback();
    }
    
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
    if (!m_gridVAO || !m_gridShader) {
        return;
    }
    
    m_gridShader->bind();
    
    // Set uniforms
    m_gridShader->setUniformValue("u_mvpMatrix", m_projectionMatrix * m_viewMatrix);
    m_gridShader->setUniformValue("u_color", m_gridColor);
    
    // Render grid
    glBindVertexArray(m_gridVAO);
    glDrawArrays(GL_LINES, 0, m_gridLineCount * 2);
    glBindVertexArray(0);
    
    m_gridShader->release();
}

void DesignCanvas::renderAxes()
{
    if (!m_axesVAO || !m_axesShader) {
        return;
    }
    
    m_axesShader->bind();
    
    // Set uniforms
    m_axesShader->setUniformValue("u_mvpMatrix", m_projectionMatrix * m_viewMatrix);
    
    // Render axes with different colors
    glBindVertexArray(m_axesVAO);
    
    // X axis - Red
    m_axesShader->setUniformValue("u_color", m_axesColors[0]);
    glDrawArrays(GL_LINES, 0, 2);
    
    // Y axis - Green  
    m_axesShader->setUniformValue("u_color", m_axesColors[1]);
    glDrawArrays(GL_LINES, 2, 2);
    
    // Z axis - Blue
    m_axesShader->setUniformValue("u_color", m_axesColors[2]);
    glDrawArrays(GL_LINES, 4, 2);
    
    glBindVertexArray(0);
    m_axesShader->release();
}

void DesignCanvas::renderObjects()
{
    if (!m_objectShader || !m_sceneManager) {
        return;
    }
    
    m_objectShader->bind();
    
    // Set common uniforms
    m_objectShader->setUniformValue("u_viewMatrix", m_viewMatrix);
    m_objectShader->setUniformValue("u_projectionMatrix", m_projectionMatrix);
    
    // Basic lighting
    QVector3D lightDir = QVector3D(0.5f, 1.0f, 0.5f).normalized();
    m_objectShader->setUniformValue("u_lightDirection", lightDir);
    m_objectShader->setUniformValue("u_lightColor", QVector3D(1.0f, 1.0f, 1.0f));
    m_objectShader->setUniformValue("u_ambientColor", QVector3D(0.3f, 0.3f, 0.3f));
    
    // Render scene objects through scene manager
    // For now, render placeholder cubes for testing
    renderPlaceholderObjects();
    
    m_objectShader->release();
}

void DesignCanvas::renderSelection()
{
    if (!m_selectionShader) {
        return;
    }
    
    m_selectionShader->bind();
    m_selectionShader->setUniformValue("u_mvpMatrix", m_projectionMatrix * m_viewMatrix);
    m_selectionShader->setUniformValue("u_color", m_selectionColor);
    
    // Render selection box if active
    if (m_selectionBoxActive) {
        renderSelectionBox();
    }
    
    // Render selection highlights on selected objects
    renderSelectionHighlights();
    
    m_selectionShader->release();
}

void DesignCanvas::renderUI()
{
    // Render UI overlays in screen space
    // Switch to 2D rendering for UI elements
    
    glDisable(GL_DEPTH_TEST);
    
    // Setup 2D projection for UI
    QMatrix4x4 uiProjection;
    uiProjection.ortho(0, m_viewportWidth, m_viewportHeight, 0, -1, 1);
    
    // Render FPS counter, coordinates, etc.
    renderFPSCounter();
    renderCoordinateDisplay();
    renderViewModeIndicator();
    
    glEnable(GL_DEPTH_TEST);
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
    Q_EMIT coordinatesChanged(worldPos);
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
    // Perform object picking using color-coded rendering
    QString objectId = performObjectPicking(screenPos);
    
    if (!objectId.isEmpty()) {
        if (m_ctrlPressed) {
            // Multi-selection
            if (m_selectedObjects.contains(objectId)) {
                m_selectedObjects.removeAll(objectId);
            } else {
                m_selectedObjects.append(objectId);
            }
        } else {
            // Single selection
            m_selectedObjects.clear();
            m_selectedObjects.append(objectId);
        }
        
        Q_EMIT objectSelected(objectId);
        Q_EMIT objectsChanged();
        m_needsUpdate = true;
        update();
    } else if (!m_ctrlPressed) {
        // Clear selection if clicking on empty space
        clearSelection();
    }
}

void DesignCanvas::handlePanning(const QPoint& delta)
{
    if (m_viewMode == ViewMode::View2D) {
        // 2D panning
        m_pan2D += QVector2D(delta.x() / m_zoom2D, -delta.y() / m_zoom2D);
    } else {
        // 3D panning
        auto right = m_camera->getRight();
        auto up = m_camera->getUp();
        
        double sensitivity = 0.01;
        auto panDelta = right * (-delta.x() * sensitivity) + up * (delta.y() * sensitivity);
        
        auto currentPos = m_camera->getPosition();
        auto currentTarget = m_camera->getTarget();
        
        m_camera->setPosition(currentPos + panDelta);
        m_camera->setTarget(currentTarget + panDelta);
    }
    
    Q_EMIT viewChanged();
}

void DesignCanvas::handleRotation(const QPoint& delta)
{
    if (m_viewMode != ViewMode::View2D) {
        // 3D rotation around target using camera's orbit method
        double sensitivity = 0.01;
        double horizontalAngle = delta.x() * sensitivity;
        double verticalAngle = delta.y() * sensitivity;
        
        m_camera->orbit(horizontalAngle, verticalAngle);
    }
    
    Q_EMIT viewChanged();
}

void DesignCanvas::handleZooming(float delta)
{
    if (m_viewMode == ViewMode::View2D) {
        // 2D zoom
        float zoomFactor = 1.0f + delta * 0.1f;
        m_zoom2D *= zoomFactor;
        m_zoom2D = qBound(0.1f, m_zoom2D, 10.0f);
    } else {
        // 3D zoom using camera's zoom method
        double zoomFactor = 1.0 - delta * 0.1;
        zoomFactor = qBound(0.1, zoomFactor, 10.0);
        m_camera->zoom(zoomFactor);
    }
    
    Q_EMIT viewChanged();
}

QVector3D DesignCanvas::screenToWorld(const QPoint& screenPos)
{
    // Convert screen coordinates to normalized device coordinates
    float x = (2.0f * screenPos.x()) / m_viewportWidth - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y()) / m_viewportHeight;
    
    if (m_viewMode == ViewMode::View2D) {
        // For 2D view, project onto the XY plane
        float worldX = (x / m_zoom2D) * (m_viewportWidth * 0.5f) + m_pan2D.x();
        float worldY = (y / m_zoom2D) * (m_viewportHeight * 0.5f) + m_pan2D.y();
        return QVector3D(worldX, worldY, 0.0f);
    } else {
        // For 3D view, cast a ray and intersect with ground plane (Z=0)
        QMatrix4x4 invViewProj = (m_projectionMatrix * m_viewMatrix).inverted();
        
        // Near and far points in world space
        QVector4D nearPoint = invViewProj * QVector4D(x, y, -1.0f, 1.0f);
        QVector4D farPoint = invViewProj * QVector4D(x, y, 1.0f, 1.0f);
        
        nearPoint /= nearPoint.w();
        farPoint /= farPoint.w();
        
        // Ray direction
        QVector3D rayDir = (farPoint.toVector3D() - nearPoint.toVector3D()).normalized();
        
        // Intersect with Z=0 plane
        if (std::abs(rayDir.z()) > 1e-6) {
            float t = -nearPoint.z() / rayDir.z();
            return nearPoint.toVector3D() + rayDir * t;
        }
        
        return QVector3D(0.0f, 0.0f, 0.0f);
    }
}

QPoint DesignCanvas::worldToScreen(const QVector3D& worldPos)
{
    QMatrix4x4 mvp = m_projectionMatrix * m_viewMatrix;
    QVector4D clipPos = mvp * QVector4D(worldPos, 1.0f);
    
    if (clipPos.w() != 0.0f) {
        clipPos /= clipPos.w();
    }
    
    // Convert from normalized device coordinates to screen coordinates
    int screenX = static_cast<int>((clipPos.x() + 1.0f) * 0.5f * m_viewportWidth);
    int screenY = static_cast<int>((1.0f - clipPos.y()) * 0.5f * m_viewportHeight);
    
    return QPoint(screenX, screenY);
}

QVector3D DesignCanvas::snapToGrid(const QVector3D& position) const
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
        // 3D view matrix using camera
        auto pos = toQt(m_camera->getPosition());
        auto target = toQt(m_camera->getTarget());
        auto up = toQt(m_camera->getUp());
        
        m_viewMatrix.lookAt(pos, target, up);
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
    // Generate grid vertices
    m_gridVertices.clear();
    
    int gridExtent = 50; // Grid extends from -50 to +50
    float step = m_gridSize;
    
    // Horizontal lines
    for (int i = -gridExtent; i <= gridExtent; ++i) {
        float y = i * step;
        m_gridVertices.insert(m_gridVertices.end(), {
            -gridExtent * step, y, 0.0f,  // Start point
            gridExtent * step, y, 0.0f    // End point
        });
    }
    
    // Vertical lines
    for (int i = -gridExtent; i <= gridExtent; ++i) {
        float x = i * step;
        m_gridVertices.insert(m_gridVertices.end(), {
            x, -gridExtent * step, 0.0f,  // Start point
            x, gridExtent * step, 0.0f    // End point
        });
    }
    
    m_gridLineCount = m_gridVertices.size() / 6; // 2 vertices per line, 3 components per vertex
    
    // Create VAO and VBO
    glGenVertexArrays(1, &m_gridVAO);
    glGenBuffers(1, &m_gridVBO);
    
    glBindVertexArray(m_gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_gridVBO);
    glBufferData(GL_ARRAY_BUFFER, m_gridVertices.size() * sizeof(float), 
                 m_gridVertices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void DesignCanvas::createAxesVBO()
{
    // Create axes vertices (X, Y, Z axes)
    m_axesVertices = {
        // X axis (Red) - from origin to positive X
        0.0f, 0.0f, 0.0f,
        5.0f, 0.0f, 0.0f,
        
        // Y axis (Green) - from origin to positive Y
        0.0f, 0.0f, 0.0f,
        0.0f, 5.0f, 0.0f,
        
        // Z axis (Blue) - from origin to positive Z
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 5.0f
    };
    
    // Create VAO and VBO
    glGenVertexArrays(1, &m_axesVAO);
    glGenBuffers(1, &m_axesVBO);
    
    glBindVertexArray(m_axesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_axesVBO);
    glBufferData(GL_ARRAY_BUFFER, m_axesVertices.size() * sizeof(float), 
                 m_axesVertices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

// Public interface methods
void DesignCanvas::setViewMode(ViewMode mode)
{
    m_viewMode = mode;
    m_needsUpdate = true;
    update();
    Q_EMIT viewChanged();
}

void DesignCanvas::setInteractionMode(InteractionMode mode)
{
    m_interactionMode = mode;
}

void DesignCanvas::resetCamera()
{
    m_camera->setPosition(KitchenCAD::Geometry::Point3D(5.0, 5.0, 5.0));
    m_camera->setTarget(KitchenCAD::Geometry::Point3D(0.0, 0.0, 0.0));
    m_camera->setUp(KitchenCAD::Geometry::Vector3D(0.0, 0.0, 1.0));
    
    m_zoom2D = 1.0f;
    m_pan2D = QVector2D(0.0f, 0.0f);
    
    m_needsUpdate = true;
    update();
    Q_EMIT viewChanged();
}

void DesignCanvas::fitToView()
{
    // TODO: Implement fit to view based on scene bounds
    resetCamera();
}

void DesignCanvas::setViewDirection(const QVector3D& direction)
{
    auto target = m_camera->getTarget();
    auto currentPos = m_camera->getPosition();
    auto toCamera = KitchenCAD::Geometry::Vector3D(target, currentPos);
    double distance = toCamera.length();
    
    auto newDirection = fromQt(direction.normalized());
    auto newPos = target + newDirection * distance;
    m_camera->setPosition(newPos);
    
    m_needsUpdate = true;
    update();
    Q_EMIT viewChanged();
}

void DesignCanvas::selectAll()
{
    // TODO: Select all objects in scene
    LOG_DEBUG("Select all objects");
    Q_EMIT objectsChanged();
}

void DesignCanvas::clearSelection()
{
    m_selectedObjects.clear();
    m_needsUpdate = true;
    update();
    Q_EMIT objectsChanged();
}

void DesignCanvas::deleteSelected()
{
    // TODO: Delete selected objects
    LOG_DEBUG("Delete selected objects");
    m_selectedObjects.clear();
    Q_EMIT objectsChanged();
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
    Q_EMIT objectsChanged();
}

void DesignCanvas::removeObjectFromScene(const QString& objectId)
{
    // TODO: Remove object from scene
    LOG_DEBUGF("Removing object {}", objectId.toStdString());
    
    m_selectedObjects.removeAll(objectId);
    m_needsUpdate = true;
    update();
    Q_EMIT objectsChanged();
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


void DesignCanvas::setupShaders()
{
    // Grid shader
    m_gridShader = std::make_unique<QOpenGLShaderProgram>();
    
    // Grid vertex shader
    const char* gridVertexShader = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        
        uniform mat4 u_mvpMatrix;
        
        void main()
        {
            gl_Position = u_mvpMatrix * vec4(aPos, 1.0);
        }
    )";
    
    // Grid fragment shader
    const char* gridFragmentShader = R"(
        #version 330 core
        out vec4 FragColor;
        
        uniform vec4 u_color;
        
        void main()
        {
            FragColor = u_color;
        }
    )";
    
    m_gridShader->addShaderFromSourceCode(QOpenGLShader::Vertex, gridVertexShader);
    m_gridShader->addShaderFromSourceCode(QOpenGLShader::Fragment, gridFragmentShader);
    m_gridShader->link();
    
    // Axes shader (same as grid shader)
    m_axesShader = std::make_unique<QOpenGLShaderProgram>();
    m_axesShader->addShaderFromSourceCode(QOpenGLShader::Vertex, gridVertexShader);
    m_axesShader->addShaderFromSourceCode(QOpenGLShader::Fragment, gridFragmentShader);
    m_axesShader->link();
    
    // Object shader
    m_objectShader = std::make_unique<QOpenGLShaderProgram>();
    
    const char* objectVertexShader = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        
        uniform mat4 u_modelMatrix;
        uniform mat4 u_viewMatrix;
        uniform mat4 u_projectionMatrix;
        
        out vec3 FragPos;
        out vec3 Normal;
        
        void main()
        {
            FragPos = vec3(u_modelMatrix * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(u_modelMatrix))) * aNormal;
            
            gl_Position = u_projectionMatrix * u_viewMatrix * vec4(FragPos, 1.0);
        }
    )";
    
    const char* objectFragmentShader = R"(
        #version 330 core
        out vec4 FragColor;
        
        in vec3 FragPos;
        in vec3 Normal;
        
        uniform vec3 u_lightDirection;
        uniform vec3 u_lightColor;
        uniform vec3 u_ambientColor;
        uniform vec3 u_objectColor;
        
        void main()
        {
            // Ambient lighting
            vec3 ambient = u_ambientColor;
            
            // Diffuse lighting
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(-u_lightDirection);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * u_lightColor;
            
            vec3 result = (ambient + diffuse) * u_objectColor;
            FragColor = vec4(result, 1.0);
        }
    )";
    
    m_objectShader->addShaderFromSourceCode(QOpenGLShader::Vertex, objectVertexShader);
    m_objectShader->addShaderFromSourceCode(QOpenGLShader::Fragment, objectFragmentShader);
    m_objectShader->link();
    
    // Selection shader (same as grid shader)
    m_selectionShader = std::make_unique<QOpenGLShaderProgram>();
    m_selectionShader->addShaderFromSourceCode(QOpenGLShader::Vertex, gridVertexShader);
    m_selectionShader->addShaderFromSourceCode(QOpenGLShader::Fragment, gridFragmentShader);
    m_selectionShader->link();
}

void DesignCanvas::setupPickingFramebuffer()
{
    // Create framebuffer for object picking
    glGenFramebuffers(1, &m_pickingFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_pickingFramebuffer);
    
    // Create color texture for object IDs
    glGenTextures(1, &m_pickingTexture);
    glBindTexture(GL_TEXTURE_2D, m_pickingTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, m_viewportWidth, m_viewportHeight, 
                 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pickingTexture, 0);
    
    // Create depth buffer
    glGenRenderbuffers(1, &m_pickingDepthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_pickingDepthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_viewportWidth, m_viewportHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_pickingDepthBuffer);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Picking framebuffer not complete!");
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

QString DesignCanvas::performObjectPicking(const QPoint& screenPos)
{
    if (!m_pickingFramebuffer) {
        return QString();
    }
    
    // Render to picking framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_pickingFramebuffer);
    glViewport(0, 0, m_viewportWidth, m_viewportHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Render objects with unique colors for picking
    // TODO: Implement object picking rendering
    
    // Read pixel at mouse position
    float pixelData[3];
    glReadPixels(screenPos.x(), m_viewportHeight - screenPos.y(), 1, 1, GL_RGB, GL_FLOAT, pixelData);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Convert color back to object ID
    // TODO: Implement color to ID conversion
    
    return QString(); // Placeholder
}

void DesignCanvas::renderPlaceholderObjects()
{
    // Render some placeholder cubes for testing
    // This will be replaced with actual scene object rendering
    
    QMatrix4x4 modelMatrix;
    
    // Cube 1
    modelMatrix.setToIdentity();
    modelMatrix.translate(2.0f, 0.0f, 0.5f);
    m_objectShader->setUniformValue("u_modelMatrix", modelMatrix);
    m_objectShader->setUniformValue("u_objectColor", QVector3D(0.8f, 0.2f, 0.2f));
    
    // TODO: Render actual cube geometry
    
    // Cube 2
    modelMatrix.setToIdentity();
    modelMatrix.translate(-2.0f, 0.0f, 0.5f);
    m_objectShader->setUniformValue("u_modelMatrix", modelMatrix);
    m_objectShader->setUniformValue("u_objectColor", QVector3D(0.2f, 0.8f, 0.2f));
    
    // TODO: Render actual cube geometry
}

void DesignCanvas::renderSelectionBox()
{
    // Render selection box outline
    // TODO: Implement selection box rendering
}

void DesignCanvas::renderSelectionHighlights()
{
    // Render highlights around selected objects
    // TODO: Implement selection highlight rendering
}

void DesignCanvas::renderFPSCounter()
{
    // TODO: Render FPS counter using QPainter overlay
}

void DesignCanvas::renderCoordinateDisplay()
{
    // TODO: Render coordinate display using QPainter overlay
}

void DesignCanvas::renderViewModeIndicator()
{
    // TODO: Render view mode indicator using QPainter overlay
}

// Drag and drop event handlers
void DesignCanvas::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-catalog-item")) {
        QString itemId = event->mimeData()->data("application/x-catalog-item");
        startDragPreview(itemId);
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void DesignCanvas::dragMoveEvent(QDragMoveEvent *event)
{
    if (m_dragPreviewActive) {
        updateDragPreview(event->position().toPoint());
        event->acceptProposedAction();
        update();
    } else {
        event->ignore();
    }
}

void DesignCanvas::dragLeaveEvent(QDragLeaveEvent *event)
{
    endDragPreview(false);
    event->accept();
    update();
}

void DesignCanvas::dropEvent(QDropEvent *event)
{
    if (m_dragPreviewActive && m_dragPreviewValid) {
        // Place the object at the validated position
        addObjectToScene(m_dragPreviewItemId, m_dragPreviewPosition);
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
    
    endDragPreview(false);
    update();
}

// Drag preview methods
void DesignCanvas::startDragPreview(const QString& catalogItemId)
{
    m_dragPreviewActive = true;
    m_dragPreviewItemId = catalogItemId;
    m_dragPreviewPosition = QVector3D(0, 0, 0);
    m_dragPreviewValid = false;
    
    LOG_DEBUG("Started drag preview for item: " + catalogItemId.toStdString());
}

void DesignCanvas::updateDragPreview(const QPoint& position)
{
    if (!m_dragPreviewActive) {
        return;
    }
    
    m_lastDragPosition = position;
    
    // Convert screen position to world position
    QVector3D worldPos = screenToWorld(position);
    
    // Validate and snap to grid
    QVector3D validatedPos = getValidatedPosition(m_dragPreviewItemId, worldPos);
    m_dragPreviewPosition = validatedPos;
    
    // Check if placement is valid
    m_dragPreviewValid = isValidPlacement(m_dragPreviewItemId, validatedPos);
    
    LOG_DEBUG("Updated drag preview position: (" + 
              std::to_string(validatedPos.x()) + ", " + 
              std::to_string(validatedPos.y()) + ", " + 
              std::to_string(validatedPos.z()) + ") Valid: " + 
              (m_dragPreviewValid ? "true" : "false"));
}

void DesignCanvas::endDragPreview(bool place)
{
    if (!m_dragPreviewActive) {
        return;
    }
    
    if (place && m_dragPreviewValid) {
        addObjectToScene(m_dragPreviewItemId, m_dragPreviewPosition);
    }
    
    m_dragPreviewActive = false;
    m_dragPreviewItemId.clear();
    m_dragPreviewValid = false;
    
    LOG_DEBUG("Ended drag preview");
}

// Validation methods
bool DesignCanvas::isValidPlacement(const QString& catalogItemId, const QVector3D& position) const
{
    // Basic validation - check if position is within scene bounds
    // In a real implementation, this would check for collisions with other objects
    
    // For now, just check if position is reasonable (not too far from origin)
    const float maxDistance = 50.0f; // 50 meters from origin
    float distance = position.length();
    
    if (distance > maxDistance) {
        return false;
    }
    
    // Check if position is above ground (Z >= 0)
    if (position.z() < 0.0f) {
        return false;
    }
    
    // TODO: Add collision detection with existing objects
    // TODO: Add validation against room boundaries
    // TODO: Add catalog item specific validation rules
    
    return true;
}

QVector3D DesignCanvas::getValidatedPosition(const QString& catalogItemId, const QVector3D& position) const
{
    QVector3D validatedPos = position;
    
    // Snap to grid if enabled
    if (m_snapToGrid) {
        validatedPos = snapToGrid(validatedPos);
    }
    
    // Ensure object is placed on the ground (Z = 0 for floor objects)
    // TODO: This should be configurable based on object type
    validatedPos.setZ(0.0f);
    
    return validatedPos;
}

// Rendering methods for drag preview
void DesignCanvas::renderDragPreview()
{
    if (!m_dragPreviewActive || !m_objectShader) {
        return;
    }
    
    m_objectShader->bind();
    
    // Set up transformation matrix for preview object
    QMatrix4x4 modelMatrix;
    modelMatrix.translate(m_dragPreviewPosition);
    
    // Set uniforms
    m_objectShader->setUniformValue("u_modelMatrix", modelMatrix);
    m_objectShader->setUniformValue("u_viewMatrix", m_viewMatrix);
    m_objectShader->setUniformValue("u_projectionMatrix", m_projectionMatrix);
    
    // Set preview color based on validity
    QVector3D previewColor = m_dragPreviewValid ? 
        QVector3D(0.0f, 1.0f, 0.0f) :  // Green for valid
        QVector3D(1.0f, 0.0f, 0.0f);   // Red for invalid
    
    m_objectShader->setUniformValue("u_objectColor", previewColor);
    
    // Set lighting
    QVector3D lightDir = QVector3D(0.5f, 1.0f, 0.5f).normalized();
    m_objectShader->setUniformValue("u_lightDirection", lightDir);
    m_objectShader->setUniformValue("u_lightColor", QVector3D(1.0f, 1.0f, 1.0f));
    m_objectShader->setUniformValue("u_ambientColor", QVector3D(0.3f, 0.3f, 0.3f));
    
    // Render a placeholder cube for the preview
    // TODO: Load actual geometry from catalog item
    // For now, render a simple wireframe cube
    
    m_objectShader->release();
}

void DesignCanvas::renderValidationFeedback()
{
    if (!m_dragPreviewActive) {
        return;
    }
    
    // Render visual feedback around the preview position
    // This could include:
    // - Grid highlighting
    // - Collision indicators
    // - Dimension guides
    // - Snap indicators
    
    // For now, render a simple circle on the ground plane
    if (m_gridShader) {
        m_gridShader->bind();
        
        QColor feedbackColor = m_dragPreviewValid ? m_validPlacementColor : m_invalidPlacementColor;
        m_gridShader->setUniformValue("u_color", feedbackColor);
        m_gridShader->setUniformValue("u_mvpMatrix", m_projectionMatrix * m_viewMatrix);
        
        // TODO: Render feedback geometry (circle, square, etc.)
        
        m_gridShader->release();
    }
}

// Utility conversion methods
QVector3D DesignCanvas::toQt(const KitchenCAD::Geometry::Vector3D& v) const
{
    return QVector3D(static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z));
}

QVector3D DesignCanvas::toQt(const KitchenCAD::Geometry::Point3D& p) const
{
    return QVector3D(static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z));
}

KitchenCAD::Geometry::Vector3D DesignCanvas::fromQt(const QVector3D& v) const
{
    return KitchenCAD::Geometry::Vector3D(v.x(), v.y(), v.z());
}

KitchenCAD::Geometry::Point3D DesignCanvas::fromQtPoint(const QVector3D& v) const
{
    return KitchenCAD::Geometry::Point3D(v.x(), v.y(), v.z());
}