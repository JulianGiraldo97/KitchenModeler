#include "MainWindow.h"
#include "DesignCanvas.h"
#include "CatalogPanel.h"
#include "PropertiesPanel.h"
#include "ProjectExplorer.h"

#include "../controllers/ProjectController.h"
#include "../controllers/CatalogController.h"
#include "../controllers/DesignController.h"
#include "../utils/Logger.h"

#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QSplitter>
#include <QProgressBar>

using namespace KitchenCAD::UI;
using namespace KitchenCAD::Controllers;
using namespace KitchenCAD::Utils;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_designCanvas(nullptr)
    , m_catalogPanel(nullptr)
    , m_propertiesPanel(nullptr)
    , m_projectExplorer(nullptr)
    , m_catalogDock(nullptr)
    , m_propertiesDock(nullptr)
    , m_projectDock(nullptr)
    , m_projectModified(false)
{
    LOG_INFO("Initializing MainWindow");
    
    // Initialize controllers
    m_projectController = std::make_unique<ProjectController>(this);
    m_catalogController = std::make_unique<CatalogController>(this);
    m_designController = std::make_unique<DesignController>(this);
    
    setupUI();
    setupMenus();
    setupToolbars();
    setupStatusBar();
    setupDockWidgets();
    connectSignals();
    
    // Load settings
    QSettings settings;
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    m_recentFiles = settings.value("recentFiles").toStringList();
    updateRecentFiles();
    
    updateWindowTitle();
    
    LOG_INFO("MainWindow initialized successfully");
}

MainWindow::~MainWindow()
{
    LOG_INFO("Destroying MainWindow");
    
    // Save settings
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("recentFiles", m_recentFiles);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_projectModified) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("Unsaved Changes"),
            tr("The current project has unsaved changes. Do you want to save before closing?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
        );
        
        if (reply == QMessageBox::Save) {
            saveProject();
            event->accept();
        } else if (reply == QMessageBox::Discard) {
            event->accept();
        } else {
            event->ignore();
            return;
        }
    }
    
    event->accept();
}

void MainWindow::setupUI()
{
    setWindowTitle("Kitchen CAD Designer");
    setMinimumSize(800, 600);
    resize(1200, 800);
    
    // Create central widget (design canvas)
    m_designCanvas = new DesignCanvas(this);
    setCentralWidget(m_designCanvas);
    
    // Create panels
    m_catalogPanel = new CatalogPanel(this);
    m_propertiesPanel = new PropertiesPanel(this);
    m_projectExplorer = new ProjectExplorer(this);
}

void MainWindow::setupMenus()
{
    // File Menu
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    
    m_newAction = m_fileMenu->addAction(tr("&New Project"), this, &MainWindow::newProject);
    m_newAction->setShortcut(QKeySequence::New);
    m_newAction->setStatusTip(tr("Create a new project"));
    
    m_openAction = m_fileMenu->addAction(tr("&Open Project..."), this, &MainWindow::openProject);
    m_openAction->setShortcut(QKeySequence::Open);
    m_openAction->setStatusTip(tr("Open an existing project"));
    
    m_recentMenu = m_fileMenu->addMenu(tr("Recent Projects"));
    
    m_fileMenu->addSeparator();
    
    m_saveAction = m_fileMenu->addAction(tr("&Save Project"), this, &MainWindow::saveProject);
    m_saveAction->setShortcut(QKeySequence::Save);
    m_saveAction->setStatusTip(tr("Save the current project"));
    
    m_saveAsAction = m_fileMenu->addAction(tr("Save Project &As..."), this, &MainWindow::saveProjectAs);
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    m_saveAsAction->setStatusTip(tr("Save the project with a new name"));
    
    m_fileMenu->addSeparator();
    
    m_exportAction = m_fileMenu->addAction(tr("&Export..."), this, &MainWindow::exportProject);
    m_exportAction->setShortcut(QKeySequence("Ctrl+E"));
    m_exportAction->setStatusTip(tr("Export project to various formats"));
    
    m_fileMenu->addSeparator();
    
    m_exitAction = m_fileMenu->addAction(tr("E&xit"), this, &MainWindow::exitApplication);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip(tr("Exit the application"));
    
    // Edit Menu
    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    
    m_undoAction = m_editMenu->addAction(tr("&Undo"), this, &MainWindow::undo);
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setStatusTip(tr("Undo the last action"));
    
    m_redoAction = m_editMenu->addAction(tr("&Redo"), this, &MainWindow::redo);
    m_redoAction->setShortcut(QKeySequence::Redo);
    m_redoAction->setStatusTip(tr("Redo the last undone action"));
    
    m_editMenu->addSeparator();
    
    m_cutAction = m_editMenu->addAction(tr("Cu&t"), this, &MainWindow::cut);
    m_cutAction->setShortcut(QKeySequence::Cut);
    m_cutAction->setStatusTip(tr("Cut selected objects"));
    
    m_copyAction = m_editMenu->addAction(tr("&Copy"), this, &MainWindow::copy);
    m_copyAction->setShortcut(QKeySequence::Copy);
    m_copyAction->setStatusTip(tr("Copy selected objects"));
    
    m_pasteAction = m_editMenu->addAction(tr("&Paste"), this, &MainWindow::paste);
    m_pasteAction->setShortcut(QKeySequence::Paste);
    m_pasteAction->setStatusTip(tr("Paste objects from clipboard"));
    
    m_editMenu->addSeparator();
    
    m_deleteAction = m_editMenu->addAction(tr("&Delete"), this, &MainWindow::deleteSelected);
    m_deleteAction->setShortcut(QKeySequence::Delete);
    m_deleteAction->setStatusTip(tr("Delete selected objects"));
    
    m_selectAllAction = m_editMenu->addAction(tr("Select &All"), this, &MainWindow::selectAll);
    m_selectAllAction->setShortcut(QKeySequence::SelectAll);
    m_selectAllAction->setStatusTip(tr("Select all objects"));
    
    // View Menu
    m_viewMenu = menuBar()->addMenu(tr("&View"));
    
    m_catalogAction = m_viewMenu->addAction(tr("&Catalog Panel"), this, &MainWindow::toggleCatalogPanel);
    m_catalogAction->setCheckable(true);
    m_catalogAction->setChecked(true);
    m_catalogAction->setStatusTip(tr("Show/hide the catalog panel"));
    
    m_propertiesAction = m_viewMenu->addAction(tr("&Properties Panel"), this, &MainWindow::togglePropertiesPanel);
    m_propertiesAction->setCheckable(true);
    m_propertiesAction->setChecked(true);
    m_propertiesAction->setStatusTip(tr("Show/hide the properties panel"));
    
    m_projectAction = m_viewMenu->addAction(tr("Project &Explorer"), this, &MainWindow::toggleProjectExplorer);
    m_projectAction->setCheckable(true);
    m_projectAction->setChecked(true);
    m_projectAction->setStatusTip(tr("Show/hide the project explorer"));
    
    m_viewMenu->addSeparator();
    
    m_viewMenu->addAction(tr("&Reset Layout"), this, &MainWindow::resetLayout);
    
    m_fullscreenAction = m_viewMenu->addAction(tr("&Fullscreen"), this, &MainWindow::toggleFullscreen);
    m_fullscreenAction->setShortcut(QKeySequence::FullScreen);
    m_fullscreenAction->setCheckable(true);
    m_fullscreenAction->setStatusTip(tr("Toggle fullscreen mode"));
    
    // Tools Menu
    m_toolsMenu = menuBar()->addMenu(tr("&Tools"));
    
    m_validateAction = m_toolsMenu->addAction(tr("&Validate Design"), this, &MainWindow::validateDesign);
    m_validateAction->setShortcut(QKeySequence("F7"));
    m_validateAction->setStatusTip(tr("Validate the current design"));
    
    m_bomAction = m_toolsMenu->addAction(tr("Generate &BOM"), this, &MainWindow::generateBOM);
    m_bomAction->setShortcut(QKeySequence("Ctrl+B"));
    m_bomAction->setStatusTip(tr("Generate Bill of Materials"));
    
    m_toolsMenu->addSeparator();
    
    m_preferencesAction = m_toolsMenu->addAction(tr("&Preferences..."), this, &MainWindow::showPreferences);
    m_preferencesAction->setShortcut(QKeySequence::Preferences);
    m_preferencesAction->setStatusTip(tr("Open application preferences"));
    
    // Help Menu
    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    
    m_helpAction = m_helpMenu->addAction(tr("&Help"), this, &MainWindow::showHelp);
    m_helpAction->setShortcut(QKeySequence::HelpContents);
    m_helpAction->setStatusTip(tr("Show help documentation"));
    
    m_helpMenu->addSeparator();
    
    m_aboutAction = m_helpMenu->addAction(tr("&About"), this, &MainWindow::showAbout);
    m_aboutAction->setStatusTip(tr("Show information about the application"));
}

void MainWindow::setupToolbars()
{
    // Main toolbar
    m_mainToolbar = addToolBar(tr("Main"));
    m_mainToolbar->addAction(m_newAction);
    m_mainToolbar->addAction(m_openAction);
    m_mainToolbar->addAction(m_saveAction);
    m_mainToolbar->addSeparator();
    m_mainToolbar->addAction(m_exportAction);
    
    // Edit toolbar
    m_editToolbar = addToolBar(tr("Edit"));
    m_editToolbar->addAction(m_undoAction);
    m_editToolbar->addAction(m_redoAction);
    m_editToolbar->addSeparator();
    m_editToolbar->addAction(m_cutAction);
    m_editToolbar->addAction(m_copyAction);
    m_editToolbar->addAction(m_pasteAction);
    m_editToolbar->addAction(m_deleteAction);
    
    // View toolbar
    m_viewToolbar = addToolBar(tr("View"));
    m_viewToolbar->addAction(m_catalogAction);
    m_viewToolbar->addAction(m_propertiesAction);
    m_viewToolbar->addAction(m_projectAction);
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel(tr("Ready"));
    statusBar()->addWidget(m_statusLabel);
    
    statusBar()->addPermanentWidget(new QLabel(tr("Coordinates:")));
    m_coordinatesLabel = new QLabel("(0, 0, 0)");
    statusBar()->addPermanentWidget(m_coordinatesLabel);
    
    statusBar()->addPermanentWidget(new QLabel(tr("Zoom:")));
    m_zoomLabel = new QLabel("100%");
    statusBar()->addPermanentWidget(m_zoomLabel);
}

void MainWindow::setupDockWidgets()
{
    // Catalog dock
    m_catalogDock = new QDockWidget(tr("Catalog"), this);
    m_catalogDock->setWidget(m_catalogPanel);
    m_catalogDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, m_catalogDock);
    
    // Properties dock
    m_propertiesDock = new QDockWidget(tr("Properties"), this);
    m_propertiesDock->setWidget(m_propertiesPanel);
    m_propertiesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, m_propertiesDock);
    
    // Project explorer dock
    m_projectDock = new QDockWidget(tr("Project Explorer"), this);
    m_projectDock->setWidget(m_projectExplorer);
    m_projectDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, m_projectDock);
    
    // Tabify some docks
    tabifyDockWidget(m_catalogDock, m_projectDock);
    m_catalogDock->raise(); // Make catalog the active tab
}

void MainWindow::connectSignals()
{
    // Connect dock widget visibility to menu actions
    connect(m_catalogDock, &QDockWidget::visibilityChanged, m_catalogAction, &QAction::setChecked);
    connect(m_propertiesDock, &QDockWidget::visibilityChanged, m_propertiesAction, &QAction::setChecked);
    connect(m_projectDock, &QDockWidget::visibilityChanged, m_projectAction, &QAction::setChecked);
    
    // Connect canvas signals
    connect(m_designCanvas, &DesignCanvas::objectSelected, this, &MainWindow::onObjectSelected);
    connect(m_designCanvas, &DesignCanvas::objectsChanged, this, &MainWindow::onObjectsChanged);
    connect(m_designCanvas, &DesignCanvas::viewChanged, this, &MainWindow::onViewChanged);
    
    // Connect controller signals
    connect(m_projectController.get(), &ProjectController::projectChanged, this, &MainWindow::onProjectChanged);
    connect(m_projectController.get(), &ProjectController::projectSaved, this, &MainWindow::onProjectSaved);
}

void MainWindow::updateWindowTitle()
{
    QString title = "Kitchen CAD Designer";
    if (!m_currentProjectPath.isEmpty()) {
        QFileInfo fileInfo(m_currentProjectPath);
        title += " - " + fileInfo.baseName();
        if (m_projectModified) {
            title += "*";
        }
    }
    setWindowTitle(title);
}

void MainWindow::updateRecentFiles()
{
    m_recentMenu->clear();
    
    for (int i = 0; i < m_recentFiles.size() && i < 10; ++i) {
        const QString& filePath = m_recentFiles.at(i);
        QFileInfo fileInfo(filePath);
        
        QAction* action = m_recentMenu->addAction(
            QString("%1. %2").arg(i + 1).arg(fileInfo.fileName()),
            this, &MainWindow::recentProject
        );
        action->setData(filePath);
        action->setStatusTip(filePath);
    }
    
    if (!m_recentFiles.isEmpty()) {
        m_recentMenu->addSeparator();
        m_recentMenu->addAction(tr("Clear Recent Files"), [this]() {
            m_recentFiles.clear();
            updateRecentFiles();
        });
    }
}

// Slot implementations
void MainWindow::newProject()
{
    LOG_INFO("Creating new project");
    m_projectController->newProject();
}

void MainWindow::openProject()
{
    LOG_INFO("Opening project");
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Open Project"),
        QString(),
        tr("Kitchen CAD Projects (*.kcd);;All Files (*)")
    );
    
    if (!fileName.isEmpty()) {
        m_projectController->openProject(fileName);
        m_currentProjectPath = fileName;
        
        // Add to recent files
        m_recentFiles.removeAll(fileName);
        m_recentFiles.prepend(fileName);
        if (m_recentFiles.size() > 10) {
            m_recentFiles.removeLast();
        }
        updateRecentFiles();
        updateWindowTitle();
    }
}

void MainWindow::saveProject()
{
    LOG_INFO("Saving project");
    if (m_currentProjectPath.isEmpty()) {
        saveProjectAs();
    } else {
        m_projectController->saveProject(m_currentProjectPath);
    }
}

void MainWindow::saveProjectAs()
{
    LOG_INFO("Saving project as");
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Save Project As"),
        QString(),
        tr("Kitchen CAD Projects (*.kcd);;All Files (*)")
    );
    
    if (!fileName.isEmpty()) {
        m_projectController->saveProject(fileName);
        m_currentProjectPath = fileName;
        updateWindowTitle();
    }
}

void MainWindow::exportProject()
{
    LOG_INFO("Exporting project");
    // TODO: Implement export dialog
    QMessageBox::information(this, tr("Export"), tr("Export functionality will be implemented in a future version."));
}

void MainWindow::recentProject()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        QString filePath = action->data().toString();
        m_projectController->openProject(filePath);
        m_currentProjectPath = filePath;
        updateWindowTitle();
    }
}

void MainWindow::exitApplication()
{
    close();
}

void MainWindow::undo()
{
    LOG_DEBUG("Undo action");
    // TODO: Implement undo functionality
}

void MainWindow::redo()
{
    LOG_DEBUG("Redo action");
    // TODO: Implement redo functionality
}

void MainWindow::cut()
{
    LOG_DEBUG("Cut action");
    // TODO: Implement cut functionality
}

void MainWindow::copy()
{
    LOG_DEBUG("Copy action");
    // TODO: Implement copy functionality
}

void MainWindow::paste()
{
    LOG_DEBUG("Paste action");
    // TODO: Implement paste functionality
}

void MainWindow::deleteSelected()
{
    LOG_DEBUG("Delete selected action");
    m_designCanvas->deleteSelected();
}

void MainWindow::selectAll()
{
    LOG_DEBUG("Select all action");
    m_designCanvas->selectAll();
}

void MainWindow::toggleCatalogPanel()
{
    m_catalogDock->setVisible(!m_catalogDock->isVisible());
}

void MainWindow::togglePropertiesPanel()
{
    m_propertiesDock->setVisible(!m_propertiesDock->isVisible());
}

void MainWindow::toggleProjectExplorer()
{
    m_projectDock->setVisible(!m_projectDock->isVisible());
}

void MainWindow::resetLayout()
{
    // Reset dock widget positions
    removeDockWidget(m_catalogDock);
    removeDockWidget(m_propertiesDock);
    removeDockWidget(m_projectDock);
    
    addDockWidget(Qt::LeftDockWidgetArea, m_catalogDock);
    addDockWidget(Qt::RightDockWidgetArea, m_propertiesDock);
    addDockWidget(Qt::LeftDockWidgetArea, m_projectDock);
    
    tabifyDockWidget(m_catalogDock, m_projectDock);
    m_catalogDock->raise();
}

void MainWindow::toggleFullscreen()
{
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
}

void MainWindow::showPreferences()
{
    LOG_INFO("Showing preferences");
    // TODO: Implement preferences dialog
    QMessageBox::information(this, tr("Preferences"), tr("Preferences dialog will be implemented in a future version."));
}

void MainWindow::validateDesign()
{
    LOG_INFO("Validating design");
    // TODO: Implement design validation
    QMessageBox::information(this, tr("Validation"), tr("Design validation will be implemented in a future version."));
}

void MainWindow::generateBOM()
{
    LOG_INFO("Generating BOM");
    // TODO: Implement BOM generation
    QMessageBox::information(this, tr("BOM"), tr("BOM generation will be implemented in a future version."));
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, tr("About Kitchen CAD Designer"),
        tr("<h2>Kitchen CAD Designer</h2>"
           "<p>Version 1.0.0</p>"
           "<p>A professional CAD application for designing kitchens, "
           "bathrooms, and modular furniture with 2D/3D visualization, "
           "material management, and export capabilities.</p>"
           "<p>Built with Qt6, OpenCascade, and modern C++.</p>"));
}

void MainWindow::showHelp()
{
    LOG_INFO("Showing help");
    // TODO: Implement help system
    QMessageBox::information(this, tr("Help"), tr("Help system will be implemented in a future version."));
}

void MainWindow::onObjectSelected(const QString& objectId)
{
    LOG_DEBUGF("Object selected: {}", objectId.toStdString());
    m_propertiesPanel->setSelectedObject(objectId);
}

void MainWindow::onObjectsChanged()
{
    LOG_DEBUG("Objects changed");
    m_projectModified = true;
    updateWindowTitle();
}

void MainWindow::onViewChanged()
{
    // Update coordinates and zoom in status bar
    // TODO: Get actual values from canvas
    m_coordinatesLabel->setText("(0, 0, 0)");
    m_zoomLabel->setText("100%");
}

void MainWindow::onProjectChanged()
{
    LOG_DEBUG("Project changed");
    m_projectModified = true;
    updateWindowTitle();
}

void MainWindow::onProjectSaved()
{
    LOG_DEBUG("Project saved");
    m_projectModified = false;
    updateWindowTitle();
    m_statusLabel->setText(tr("Project saved"));
}

#include "MainWindow.moc"