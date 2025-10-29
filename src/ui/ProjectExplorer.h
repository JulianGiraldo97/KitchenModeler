#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QMenu>
#include <QAction>
#include <QFileSystemWatcher>
#include <memory>

// Forward declarations
namespace KitchenCAD::Services {
    class ProjectManager;
}

namespace KitchenCAD::UI {

class ProjectTreeWidget;

class ProjectExplorer : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectExplorer(QWidget *parent = nullptr);
    ~ProjectExplorer() override;

    // Project management
    void setCurrentProject(const QString& projectPath);
    void refreshProject();
    void clearProject();
    
    // Selection
    QString getSelectedItemPath() const;
    QStringList getSelectedItemPaths() const;

Q_SIGNALS:
    void fileSelected(const QString& filePath);
    void fileDoubleClicked(const QString& filePath);
    void projectStructureChanged();

private Q_SLOTS:
    void onItemSelectionChanged();
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onContextMenuRequested(const QPoint& pos);
    void onRefreshRequested();
    void onNewFolder();
    void onNewFile();
    void onRename();
    void onDelete();
    void onShowInExplorer();
    void onFileSystemChanged(const QString& path);

private:
    void setupUI();
    void setupContextMenu();
    void connectSignals();
    
    void populateProjectTree();
    void addProjectItem(QTreeWidgetItem* parent, const QString& path, const QString& name, bool isDirectory);
    void updateProjectInfo();
    
    QIcon getFileIcon(const QString& filePath) const;
    QString getFileSize(const QString& filePath) const;
    QString getFileModified(const QString& filePath) const;

private:
    // UI Components
    QVBoxLayout* m_mainLayout;
    
    // Header
    QLabel* m_titleLabel;
    QLabel* m_projectPathLabel;
    QPushButton* m_refreshButton;
    
    // Filter controls
    QHBoxLayout* m_filterLayout;
    QLineEdit* m_filterEdit;
    QComboBox* m_filterTypeCombo;
    
    // Project tree
    ProjectTreeWidget* m_projectTree;
    
    // Status
    QLabel* m_statusLabel;
    
    // Context menu
    QMenu* m_contextMenu;
    QAction* m_newFolderAction;
    QAction* m_newFileAction;
    QAction* m_renameAction;
    QAction* m_deleteAction;
    QAction* m_showInExplorerAction;
    QAction* m_refreshAction;
    
    // Services
    std::unique_ptr<KitchenCAD::Services::ProjectManager> m_projectManager;
    
    // File system monitoring
    QFileSystemWatcher* m_fileWatcher;
    
    // State
    QString m_currentProjectPath;
    QString m_filterText;
    QString m_filterType;
};

class ProjectTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    explicit ProjectTreeWidget(QWidget *parent = nullptr);

    void setFilter(const QString& text, const QString& type);
    void clearFilter();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;

Q_SIGNALS:
    void filesDropped(const QStringList& filePaths, const QString& targetPath);
    void itemMoved(const QString& sourcePath, const QString& targetPath);

private:
    bool isItemVisible(QTreeWidgetItem* item) const;
    void updateItemVisibility();

private:
    QString m_filterText;
    QString m_filterType;
};

} // namespace KitchenCAD::UI