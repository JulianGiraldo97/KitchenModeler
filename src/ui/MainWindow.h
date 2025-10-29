#pragma once

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QAction>
#include <QActionGroup>
#include <QLabel>
#include <QCloseEvent>
#include <memory>

// Forward declarations
class DesignCanvas;
class CatalogPanel;
class PropertiesPanel;
class ProjectExplorer;

namespace KitchenCAD::Controllers {
    class ProjectController;
    class CatalogController;
    class DesignController;
}

namespace KitchenCAD::UI {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private Q_SLOTS:
    // File menu actions
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void exportProject();
    void recentProject();
    void exitApplication();

    // Edit menu actions
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void deleteSelected();
    void selectAll();

    // View menu actions
    void toggleCatalogPanel();
    void togglePropertiesPanel();
    void toggleProjectExplorer();
    void resetLayout();
    void toggleFullscreen();

    // Tools menu actions
    void showPreferences();
    void validateDesign();
    void generateBOM();

    // Help menu actions
    void showAbout();
    void showHelp();

    // Canvas events
    void onObjectSelected(const QString& objectId);
    void onObjectsChanged();
    void onViewChanged();

    // Project events
    void onProjectChanged();
    void onProjectSaved();

private:
    void setupUI();
    void setupMenus();
    void setupToolbars();
    void setupStatusBar();
    void setupDockWidgets();
    void connectSignals();
    void updateWindowTitle();
    void updateRecentFiles();

    // UI Components
    DesignCanvas* m_designCanvas;
    CatalogPanel* m_catalogPanel;
    PropertiesPanel* m_propertiesPanel;
    ProjectExplorer* m_projectExplorer;

    // Dock widgets
    QDockWidget* m_catalogDock;
    QDockWidget* m_propertiesDock;
    QDockWidget* m_projectDock;

    // Menus
    QMenu* m_fileMenu;
    QMenu* m_editMenu;
    QMenu* m_viewMenu;
    QMenu* m_toolsMenu;
    QMenu* m_helpMenu;
    QMenu* m_recentMenu;

    // Toolbars
    QToolBar* m_mainToolbar;
    QToolBar* m_viewToolbar;
    QToolBar* m_editToolbar;

    // Actions
    QAction* m_newAction;
    QAction* m_openAction;
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QAction* m_exportAction;
    QAction* m_exitAction;

    QAction* m_undoAction;
    QAction* m_redoAction;
    QAction* m_cutAction;
    QAction* m_copyAction;
    QAction* m_pasteAction;
    QAction* m_deleteAction;
    QAction* m_selectAllAction;

    QAction* m_catalogAction;
    QAction* m_propertiesAction;
    QAction* m_projectAction;
    QAction* m_fullscreenAction;

    QAction* m_preferencesAction;
    QAction* m_validateAction;
    QAction* m_bomAction;

    QAction* m_aboutAction;
    QAction* m_helpAction;

    // Status bar widgets
    QLabel* m_statusLabel;
    QLabel* m_coordinatesLabel;
    QLabel* m_zoomLabel;

    // Controllers
    std::unique_ptr<KitchenCAD::Controllers::ProjectController> m_projectController;
    std::unique_ptr<KitchenCAD::Controllers::CatalogController> m_catalogController;
    std::unique_ptr<KitchenCAD::Controllers::DesignController> m_designController;

    // State
    QString m_currentProjectPath;
    bool m_projectModified;
    QStringList m_recentFiles;
};

} // namespace KitchenCAD::UI