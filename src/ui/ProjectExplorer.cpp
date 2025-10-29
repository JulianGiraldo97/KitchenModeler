#include "ProjectExplorer.h"
#include "../services/ProjectManager.h"
#include "../utils/Logger.h"

#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDrag>

using namespace KitchenCAD::UI;
using namespace KitchenCAD::Services;
using namespace KitchenCAD::Utils;

ProjectExplorer::ProjectExplorer(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_titleLabel(nullptr)
    , m_projectPathLabel(nullptr)
    , m_refreshButton(nullptr)
    , m_filterLayout(nullptr)
    , m_filterEdit(nullptr)
    , m_filterTypeCombo(nullptr)
    , m_projectTree(nullptr)
    , m_statusLabel(nullptr)
    , m_contextMenu(nullptr)
    , m_fileWatcher(nullptr)
    , m_filterType("All")
{
    LOG_INFO("Initializing ProjectExplorer");
    
    // Initialize project manager
    m_projectManager = std::make_unique<ProjectManager>();
    
    // Initialize file system watcher
    m_fileWatcher = new QFileSystemWatcher(this);
    
    setupUI();
    setupContextMenu();
    connectSignals();
    
    LOG_INFO("ProjectExplorer initialized");
}

ProjectExplorer::~ProjectExplorer()
{
    LOG_INFO("Destroying ProjectExplorer");
}

void ProjectExplorer::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    m_mainLayout->setSpacing(5);
    
    // Title and refresh button
    QHBoxLayout* headerLayout = new QHBoxLayout();
    
    m_titleLabel = new QLabel(tr("Project Explorer"), this);
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    headerLayout->addWidget(m_titleLabel);
    
    headerLayout->addStretch();
    
    m_refreshButton = new QPushButton(tr("↻"), this);
    m_refreshButton->setToolTip(tr("Refresh project"));
    m_refreshButton->setMaximumSize(25, 25);
    headerLayout->addWidget(m_refreshButton);
    
    m_mainLayout->addLayout(headerLayout);
    
    // Project path
    m_projectPathLabel = new QLabel(tr("No project loaded"), this);
    m_projectPathLabel->setStyleSheet("color: #666; font-size: 11px; padding: 2px;");
    m_projectPathLabel->setWordWrap(true);
    m_mainLayout->addWidget(m_projectPathLabel);
    
    // Filter controls
    m_filterLayout = new QHBoxLayout();
    
    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText(tr("Filter files..."));
    m_filterEdit->setClearButtonEnabled(true);
    m_filterLayout->addWidget(m_filterEdit);
    
    m_filterTypeCombo = new QComboBox(this);
    m_filterTypeCombo->addItems({
        tr("All"),
        tr("Projects (.kcd)"),
        tr("Images"),
        tr("Documents"),
        tr("3D Models"),
        tr("Textures")
    });
    m_filterTypeCombo->setMaximumWidth(100);
    m_filterLayout->addWidget(m_filterTypeCombo);
    
    m_mainLayout->addLayout(m_filterLayout);
    
    // Project tree
    m_projectTree = new ProjectTreeWidget(this);
    m_projectTree->setHeaderLabels({tr("Name"), tr("Size"), tr("Modified")});
    m_projectTree->setRootIsDecorated(true);
    m_projectTree->setAlternatingRowColors(true);
    m_projectTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_projectTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_projectTree->setDragDropMode(QAbstractItemView::DragDrop);
    m_projectTree->setDefaultDropAction(Qt::MoveAction);
    
    // Configure header
    QHeaderView* header = m_projectTree->header();
    header->setStretchLastSection(false);
    header->resizeSection(0, 200); // Name column
    header->resizeSection(1, 80);  // Size column
    header->resizeSection(2, 120); // Modified column
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    
    m_mainLayout->addWidget(m_projectTree, 1);
    
    // Status label
    m_statusLabel = new QLabel(tr("Ready"), this);
    m_statusLabel->setStyleSheet("color: #666; font-size: 11px; padding: 2px;");
    m_mainLayout->addWidget(m_statusLabel);
}

void ProjectExplorer::setupContextMenu()
{
    m_contextMenu = new QMenu(this);
    
    m_newFolderAction = m_contextMenu->addAction(tr("New Folder"), this, &ProjectExplorer::onNewFolder);
    m_newFileAction = m_contextMenu->addAction(tr("New File"), this, &ProjectExplorer::onNewFile);
    
    m_contextMenu->addSeparator();
    
    m_renameAction = m_contextMenu->addAction(tr("Rename"), this, &ProjectExplorer::onRename);
    m_deleteAction = m_contextMenu->addAction(tr("Delete"), this, &ProjectExplorer::onDelete);
    
    m_contextMenu->addSeparator();
    
    m_showInExplorerAction = m_contextMenu->addAction(tr("Show in Explorer"), this, &ProjectExplorer::onShowInExplorer);
    m_refreshAction = m_contextMenu->addAction(tr("Refresh"), this, &ProjectExplorer::onRefreshRequested);
}

void ProjectExplorer::connectSignals()
{
    // Tree widget signals
    connect(m_projectTree, &QTreeWidget::itemSelectionChanged, 
            this, &ProjectExplorer::onItemSelectionChanged);
    connect(m_projectTree, &QTreeWidget::itemDoubleClicked, 
            this, &ProjectExplorer::onItemDoubleClicked);
    connect(m_projectTree, &QTreeWidget::customContextMenuRequested, 
            this, &ProjectExplorer::onContextMenuRequested);
    
    // Filter signals
    connect(m_filterEdit, &QLineEdit::textChanged, [this](const QString& text) {
        m_filterText = text;
        m_projectTree->setFilter(m_filterText, m_filterType);
    });
    
    connect(m_filterTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            [this](int index) {
        m_filterType = m_filterTypeCombo->itemText(index);
        m_projectTree->setFilter(m_filterText, m_filterType);
    });
    
    // Control signals
    connect(m_refreshButton, &QPushButton::clicked, this, &ProjectExplorer::onRefreshRequested);
    
    // File system watcher
    connect(m_fileWatcher, &QFileSystemWatcher::directoryChanged, 
            this, &ProjectExplorer::onFileSystemChanged);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, 
            this, &ProjectExplorer::onFileSystemChanged);
}

void ProjectExplorer::setCurrentProject(const QString& projectPath)
{
    LOG_INFOF("Setting current project: {}", projectPath.toStdString());
    
    // Clear previous project
    clearProject();
    
    if (projectPath.isEmpty()) {
        return;
    }
    
    QFileInfo projectInfo(projectPath);
    if (!projectInfo.exists()) {
        LOG_ERROR("Project path does not exist: " + projectPath.toStdString());
        m_statusLabel->setText(tr("Project path not found"));
        return;
    }
    
    m_currentProjectPath = projectPath;
    
    // Update UI
    if (projectInfo.isFile()) {
        // Project file - show parent directory
        QString projectDir = projectInfo.absolutePath();
        m_projectPathLabel->setText(projectDir);
        
        // Add directory to file watcher
        m_fileWatcher->addPath(projectDir);
    } else {
        // Project directory
        m_projectPathLabel->setText(projectPath);
        
        // Add directory to file watcher
        m_fileWatcher->addPath(projectPath);
    }
    
    // Populate tree
    populateProjectTree();
    updateProjectInfo();
}

void ProjectExplorer::refreshProject()
{
    LOG_DEBUG("Refreshing project");
    
    if (m_currentProjectPath.isEmpty()) {
        return;
    }
    
    populateProjectTree();
    updateProjectInfo();
}

void ProjectExplorer::clearProject()
{
    LOG_DEBUG("Clearing project");
    
    // Clear file watcher
    if (!m_fileWatcher->directories().isEmpty()) {
        m_fileWatcher->removePaths(m_fileWatcher->directories());
    }
    if (!m_fileWatcher->files().isEmpty()) {
        m_fileWatcher->removePaths(m_fileWatcher->files());
    }
    
    // Clear tree
    m_projectTree->clear();
    
    // Reset UI
    m_currentProjectPath.clear();
    m_projectPathLabel->setText(tr("No project loaded"));
    m_statusLabel->setText(tr("Ready"));
}

void ProjectExplorer::populateProjectTree()
{
    if (m_currentProjectPath.isEmpty()) {
        return;
    }
    
    m_projectTree->clear();
    m_statusLabel->setText(tr("Loading project structure..."));
    QApplication::processEvents();
    
    QFileInfo projectInfo(m_currentProjectPath);
    QString rootPath;
    
    if (projectInfo.isFile()) {
        rootPath = projectInfo.absolutePath();
    } else {
        rootPath = m_currentProjectPath;
    }
    
    QDir rootDir(rootPath);
    if (!rootDir.exists()) {
        m_statusLabel->setText(tr("Project directory not found"));
        return;
    }
    
    // Create root item
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_projectTree);
    rootItem->setText(0, rootDir.dirName());
    rootItem->setData(0, Qt::UserRole, rootPath);
    rootItem->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
    rootItem->setExpanded(true);
    
    // Add subdirectories and files
    QFileInfoList entries = rootDir.entryInfoList(
        QDir::AllEntries | QDir::NoDotAndDotDot, 
        QDir::DirsFirst | QDir::Name
    );
    
    for (const QFileInfo& entry : entries) {
        addProjectItem(rootItem, entry.absoluteFilePath(), entry.fileName(), entry.isDir());
    }
    
    m_statusLabel->setText(tr("%1 items").arg(entries.size()));
}

void ProjectExplorer::addProjectItem(QTreeWidgetItem* parent, const QString& path, const QString& name, bool isDirectory)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(parent);
    item->setText(0, name);
    item->setData(0, Qt::UserRole, path);
    
    QFileInfo fileInfo(path);
    
    if (isDirectory) {
        item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
        item->setText(1, ""); // No size for directories
        
        // Add subdirectories and files
        QDir dir(path);
        QFileInfoList entries = dir.entryInfoList(
            QDir::AllEntries | QDir::NoDotAndDotDot, 
            QDir::DirsFirst | QDir::Name
        );
        
        for (const QFileInfo& entry : entries) {
            addProjectItem(item, entry.absoluteFilePath(), entry.fileName(), entry.isDir());
        }
    } else {
        item->setIcon(0, getFileIcon(path));
        item->setText(1, getFileSize(path));
    }
    
    item->setText(2, getFileModified(path));
}

void ProjectExplorer::updateProjectInfo()
{
    if (m_currentProjectPath.isEmpty()) {
        return;
    }
    
    // Count items in tree
    int totalItems = 0;
    QTreeWidgetItemIterator it(m_projectTree);
    while (*it) {
        totalItems++;
        ++it;
    }
    
    m_statusLabel->setText(tr("%1 items in project").arg(totalItems));
}

QIcon ProjectExplorer::getFileIcon(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    
    // Return appropriate icon based on file type
    if (suffix == "kcd") {
        return style()->standardIcon(QStyle::SP_FileIcon); // Project file
    } else if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" || suffix == "bmp" || suffix == "gif") {
        return style()->standardIcon(QStyle::SP_FileIcon); // Image file
    } else if (suffix == "txt" || suffix == "md" || suffix == "doc" || suffix == "docx") {
        return style()->standardIcon(QStyle::SP_FileIcon); // Document file
    } else if (suffix == "step" || suffix == "stp" || suffix == "iges" || suffix == "igs" || suffix == "stl") {
        return style()->standardIcon(QStyle::SP_FileIcon); // 3D model file
    } else {
        return style()->standardIcon(QStyle::SP_FileIcon); // Generic file
    }
}

QString ProjectExplorer::getFileSize(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    qint64 size = fileInfo.size();
    
    if (size < 1024) {
        return QString("%1 B").arg(size);
    } else if (size < 1024 * 1024) {
        return QString("%1 KB").arg(size / 1024);
    } else if (size < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(size / (1024 * 1024));
    } else {
        return QString("%1 GB").arg(size / (1024 * 1024 * 1024));
    }
}

QString ProjectExplorer::getFileModified(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    QDateTime modified = fileInfo.lastModified();
    return modified.toString("yyyy-MM-dd hh:mm");
}

QString ProjectExplorer::getSelectedItemPath() const
{
    QTreeWidgetItem* currentItem = m_projectTree->currentItem();
    if (currentItem) {
        return currentItem->data(0, Qt::UserRole).toString();
    }
    return QString();
}

QStringList ProjectExplorer::getSelectedItemPaths() const
{
    QStringList paths;
    QList<QTreeWidgetItem*> selectedItems = m_projectTree->selectedItems();
    
    for (QTreeWidgetItem* item : selectedItems) {
        paths.append(item->data(0, Qt::UserRole).toString());
    }
    
    return paths;
}

// Slot implementations
void ProjectExplorer::onItemSelectionChanged()
{
    QString selectedPath = getSelectedItemPath();
    if (!selectedPath.isEmpty()) {
        LOG_DEBUGF("Item selected: {}", selectedPath.toStdString());
        emit fileSelected(selectedPath);
    }
}

void ProjectExplorer::onItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column)
    
    if (item) {
        QString filePath = item->data(0, Qt::UserRole).toString();
        LOG_DEBUGF("Item double-clicked: {}", filePath.toStdString());
        
        QFileInfo fileInfo(filePath);
        if (fileInfo.isFile()) {
            emit fileDoubleClicked(filePath);
        }
    }
}

void ProjectExplorer::onContextMenuRequested(const QPoint& pos)
{
    QTreeWidgetItem* item = m_projectTree->itemAt(pos);
    
    // Enable/disable actions based on selection
    bool hasSelection = (item != nullptr);
    bool isFile = false;
    
    if (hasSelection) {
        QString path = item->data(0, Qt::UserRole).toString();
        QFileInfo fileInfo(path);
        isFile = fileInfo.isFile();
    }
    
    m_renameAction->setEnabled(hasSelection);
    m_deleteAction->setEnabled(hasSelection);
    m_showInExplorerAction->setEnabled(hasSelection);
    
    // Show context menu
    m_contextMenu->exec(m_projectTree->mapToGlobal(pos));
}

void ProjectExplorer::onRefreshRequested()
{
    refreshProject();
}

void ProjectExplorer::onNewFolder()
{
    QString parentPath = getSelectedItemPath();
    if (parentPath.isEmpty()) {
        parentPath = m_currentProjectPath;
    }
    
    QFileInfo parentInfo(parentPath);
    if (parentInfo.isFile()) {
        parentPath = parentInfo.absolutePath();
    }
    
    bool ok;
    QString folderName = QInputDialog::getText(this, tr("New Folder"),
        tr("Folder name:"), QLineEdit::Normal, tr("New Folder"), &ok);
    
    if (ok && !folderName.isEmpty()) {
        QString folderPath = QDir(parentPath).absoluteFilePath(folderName);
        QDir dir;
        if (dir.mkpath(folderPath)) {
            LOG_INFOF("Created folder: {}", folderPath.toStdString());
            refreshProject();
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to create folder."));
        }
    }
}

void ProjectExplorer::onNewFile()
{
    QString parentPath = getSelectedItemPath();
    if (parentPath.isEmpty()) {
        parentPath = m_currentProjectPath;
    }
    
    QFileInfo parentInfo(parentPath);
    if (parentInfo.isFile()) {
        parentPath = parentInfo.absolutePath();
    }
    
    bool ok;
    QString fileName = QInputDialog::getText(this, tr("New File"),
        tr("File name:"), QLineEdit::Normal, tr("new_file.txt"), &ok);
    
    if (ok && !fileName.isEmpty()) {
        QString filePath = QDir(parentPath).absoluteFilePath(fileName);
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.close();
            LOG_INFOF("Created file: {}", filePath.toStdString());
            refreshProject();
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to create file."));
        }
    }
}

void ProjectExplorer::onRename()
{
    QString currentPath = getSelectedItemPath();
    if (currentPath.isEmpty()) {
        return;
    }
    
    QFileInfo fileInfo(currentPath);
    QString currentName = fileInfo.fileName();
    
    bool ok;
    QString newName = QInputDialog::getText(this, tr("Rename"),
        tr("New name:"), QLineEdit::Normal, currentName, &ok);
    
    if (ok && !newName.isEmpty() && newName != currentName) {
        QString newPath = QDir(fileInfo.absolutePath()).absoluteFilePath(newName);
        
        if (QFile::rename(currentPath, newPath)) {
            LOG_INFOF("Renamed {} to {}", currentPath.toStdString(), newPath.toStdString());
            refreshProject();
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to rename item."));
        }
    }
}

void ProjectExplorer::onDelete()
{
    QStringList selectedPaths = getSelectedItemPaths();
    if (selectedPaths.isEmpty()) {
        return;
    }
    
    QString message;
    if (selectedPaths.size() == 1) {
        QFileInfo fileInfo(selectedPaths.first());
        message = tr("Are you sure you want to delete '%1'?").arg(fileInfo.fileName());
    } else {
        message = tr("Are you sure you want to delete %1 items?").arg(selectedPaths.size());
    }
    
    int ret = QMessageBox::question(this, tr("Delete"), message,
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        for (const QString& path : selectedPaths) {
            QFileInfo fileInfo(path);
            bool success = false;
            
            if (fileInfo.isDir()) {
                QDir dir(path);
                success = dir.removeRecursively();
            } else {
                success = QFile::remove(path);
            }
            
            if (success) {
                LOG_INFOF("Deleted: {}", path.toStdString());
            } else {
                LOG_ERROR("Failed to delete: " + path.toStdString());
            }
        }
        
        refreshProject();
    }
}

void ProjectExplorer::onShowInExplorer()
{
    QString selectedPath = getSelectedItemPath();
    if (!selectedPath.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(selectedPath));
    }
}

void ProjectExplorer::onFileSystemChanged(const QString& path)
{
    LOG_DEBUGF("File system changed: {}", path.toStdString());
    
    // Refresh project after a short delay to avoid multiple rapid updates
    QTimer::singleShot(500, this, &ProjectExplorer::refreshProject);
    
    emit projectStructureChanged();
}

// ProjectTreeWidget implementation
ProjectTreeWidget::ProjectTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    setAcceptDrops(true);
    setDragEnabled(true);
    setDropIndicatorShown(true);
}

void ProjectTreeWidget::setFilter(const QString& text, const QString& type)
{
    m_filterText = text.toLower();
    m_filterType = type;
    updateItemVisibility();
}

void ProjectTreeWidget::clearFilter()
{
    m_filterText.clear();
    m_filterType = "All";
    updateItemVisibility();
}

void ProjectTreeWidget::updateItemVisibility()
{
    QTreeWidgetItemIterator it(this);
    while (*it) {
        (*it)->setHidden(!isItemVisible(*it));
        ++it;
    }
}

bool ProjectTreeWidget::isItemVisible(QTreeWidgetItem* item) const
{
    if (m_filterText.isEmpty() && m_filterType == "All") {
        return true;
    }
    
    QString itemName = item->text(0).toLower();
    QString itemPath = item->data(0, Qt::UserRole).toString();
    QFileInfo fileInfo(itemPath);
    
    // Check text filter
    if (!m_filterText.isEmpty() && !itemName.contains(m_filterText)) {
        return false;
    }
    
    // Check type filter
    if (m_filterType != "All") {
        QString suffix = fileInfo.suffix().toLower();
        
        if (m_filterType == "Projects (.kcd)" && suffix != "kcd") {
            return false;
        } else if (m_filterType == "Images" && 
                   !QStringList({"png", "jpg", "jpeg", "bmp", "gif", "tiff"}).contains(suffix)) {
            return false;
        } else if (m_filterType == "Documents" && 
                   !QStringList({"txt", "md", "doc", "docx", "pdf"}).contains(suffix)) {
            return false;
        } else if (m_filterType == "3D Models" && 
                   !QStringList({"step", "stp", "iges", "igs", "stl", "obj"}).contains(suffix)) {
            return false;
        } else if (m_filterType == "Textures" && 
                   !QStringList({"png", "jpg", "jpeg", "tga", "dds"}).contains(suffix)) {
            return false;
        }
    }
    
    return true;
}

void ProjectTreeWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QTreeWidget::dragEnterEvent(event);
    }
}

void ProjectTreeWidget::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QTreeWidget::dragMoveEvent(event);
    }
}

void ProjectTreeWidget::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        QStringList filePaths;
        for (const QUrl& url : event->mimeData()->urls()) {
            if (url.isLocalFile()) {
                filePaths.append(url.toLocalFile());
            }
        }
        
        QTreeWidgetItem* targetItem = itemAt(event->pos());
        QString targetPath;
        if (targetItem) {
            targetPath = targetItem->data(0, Qt::UserRole).toString();
        }
        
        emit filesDropped(filePaths, targetPath);
        event->acceptProposedAction();
    } else {
        QTreeWidget::dropEvent(event);
    }
}

void ProjectTreeWidget::startDrag(Qt::DropActions supportedActions)
{
    QTreeWidgetItem* item = currentItem();
    if (item) {
        QString itemPath = item->data(0, Qt::UserRole).toString();
        
        QDrag* drag = new QDrag(this);
        QMimeData* mimeData = new QMimeData();
        
        QList<QUrl> urls;
        urls.append(QUrl::fromLocalFile(itemPath));
        mimeData->setUrls(urls);
        
        drag->setMimeData(mimeData);
        drag->exec(supportedActions);
    }
}

#include "ProjectExplorer.moc"