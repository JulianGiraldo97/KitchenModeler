#include "CatalogPanel.h"
#include "../services/CatalogService.h"
#include "../models/CatalogItem.h"
#include "../utils/Logger.h"

#include <QGridLayout>
#include <QHeaderView>
#include <QScrollArea>
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QPainter>
#include <QMouseEvent>

using namespace KitchenCAD::UI;
using namespace KitchenCAD::Services;
using namespace KitchenCAD::Models;
using namespace KitchenCAD::Utils;

CatalogPanel::CatalogPanel(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_filtersGroup(nullptr)
    , m_searchEdit(nullptr)
    , m_categoryCombo(nullptr)
    , m_sortCombo(nullptr)
    , m_viewModeCombo(nullptr)
    , m_dimensionGroup(nullptr)
    , m_priceGroup(nullptr)
    , m_itemList(nullptr)
    , m_statusLabel(nullptr)
    , m_refreshButton(nullptr)
    , m_clearFiltersButton(nullptr)
    , m_currentCategory("All")
    , m_viewMode(ViewMode::List)
    , m_sortOrder(SortOrder::Name)
{
    LOG_INFO("Initializing CatalogPanel");
    
    // Initialize catalog service
    m_catalogService = std::make_unique<CatalogService>();
    
    setupUI();
    setupFilters();
    setupItemList();
    connectSignals();
    
    // Load initial data
    refreshCatalog();
    
    LOG_INFO("CatalogPanel initialized");
}

CatalogPanel::~CatalogPanel()
{
    LOG_INFO("Destroying CatalogPanel");
}

void CatalogPanel::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    m_mainLayout->setSpacing(5);
    
    // Title
    QLabel* titleLabel = new QLabel(tr("Catalog"), this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; padding: 5px;");
    m_mainLayout->addWidget(titleLabel);
    
    // Search box
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search items..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_mainLayout->addWidget(m_searchEdit);
    
    // Quick controls
    QHBoxLayout* quickLayout = new QHBoxLayout();
    
    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->setMinimumWidth(100);
    quickLayout->addWidget(new QLabel(tr("Category:"), this));
    quickLayout->addWidget(m_categoryCombo);
    
    m_viewModeCombo = new QComboBox(this);
    m_viewModeCombo->addItem(tr("List"), static_cast<int>(ViewMode::List));
    m_viewModeCombo->addItem(tr("Grid"), static_cast<int>(ViewMode::Grid));
    m_viewModeCombo->addItem(tr("Details"), static_cast<int>(ViewMode::Details));
    quickLayout->addWidget(new QLabel(tr("View:"), this));
    quickLayout->addWidget(m_viewModeCombo);
    
    m_mainLayout->addLayout(quickLayout);
    
    // Sort controls
    QHBoxLayout* sortLayout = new QHBoxLayout();
    m_sortCombo = new QComboBox(this);
    m_sortCombo->addItem(tr("Name"), static_cast<int>(SortOrder::Name));
    m_sortCombo->addItem(tr("Category"), static_cast<int>(SortOrder::Category));
    m_sortCombo->addItem(tr("Price"), static_cast<int>(SortOrder::Price));
    m_sortCombo->addItem(tr("Width"), static_cast<int>(SortOrder::Width));
    m_sortCombo->addItem(tr("Height"), static_cast<int>(SortOrder::Height));
    m_sortCombo->addItem(tr("Depth"), static_cast<int>(SortOrder::Depth));
    
    sortLayout->addWidget(new QLabel(tr("Sort by:"), this));
    sortLayout->addWidget(m_sortCombo);
    sortLayout->addStretch();
    
    m_mainLayout->addLayout(sortLayout);
}

void CatalogPanel::setupFilters()
{
    // Filters group
    m_filtersGroup = new QGroupBox(tr("Filters"), this);
    m_filtersGroup->setCheckable(true);
    m_filtersGroup->setChecked(false);
    QVBoxLayout* filtersLayout = new QVBoxLayout(m_filtersGroup);
    
    // Dimension filters
    m_dimensionGroup = new QGroupBox(tr("Dimensions (cm)"), m_filtersGroup);
    QGridLayout* dimLayout = new QGridLayout(m_dimensionGroup);
    
    // Width filter
    m_widthLabel = new QLabel(tr("Width:"), m_dimensionGroup);
    m_widthMinSlider = new QSlider(Qt::Horizontal, m_dimensionGroup);
    m_widthMaxSlider = new QSlider(Qt::Horizontal, m_dimensionGroup);
    m_widthMinSpin = new QSpinBox(m_dimensionGroup);
    m_widthMaxSpin = new QSpinBox(m_dimensionGroup);
    
    m_widthMinSlider->setRange(m_filterRanges.widthMin, m_filterRanges.widthMax);
    m_widthMaxSlider->setRange(m_filterRanges.widthMin, m_filterRanges.widthMax);
    m_widthMinSpin->setRange(m_filterRanges.widthMin, m_filterRanges.widthMax);
    m_widthMaxSpin->setRange(m_filterRanges.widthMin, m_filterRanges.widthMax);
    m_widthMinSlider->setValue(m_filterRanges.widthMin);
    m_widthMaxSlider->setValue(m_filterRanges.widthMax);
    m_widthMinSpin->setValue(m_filterRanges.widthMin);
    m_widthMaxSpin->setValue(m_filterRanges.widthMax);
    
    dimLayout->addWidget(m_widthLabel, 0, 0);
    dimLayout->addWidget(m_widthMinSpin, 0, 1);
    dimLayout->addWidget(m_widthMinSlider, 0, 2);
    dimLayout->addWidget(m_widthMaxSlider, 0, 3);
    dimLayout->addWidget(m_widthMaxSpin, 0, 4);
    
    // Height filter
    m_heightLabel = new QLabel(tr("Height:"), m_dimensionGroup);
    m_heightMinSlider = new QSlider(Qt::Horizontal, m_dimensionGroup);
    m_heightMaxSlider = new QSlider(Qt::Horizontal, m_dimensionGroup);
    m_heightMinSpin = new QSpinBox(m_dimensionGroup);
    m_heightMaxSpin = new QSpinBox(m_dimensionGroup);
    
    m_heightMinSlider->setRange(m_filterRanges.heightMin, m_filterRanges.heightMax);
    m_heightMaxSlider->setRange(m_filterRanges.heightMin, m_filterRanges.heightMax);
    m_heightMinSpin->setRange(m_filterRanges.heightMin, m_filterRanges.heightMax);
    m_heightMaxSpin->setRange(m_filterRanges.heightMin, m_filterRanges.heightMax);
    m_heightMinSlider->setValue(m_filterRanges.heightMin);
    m_heightMaxSlider->setValue(m_filterRanges.heightMax);
    m_heightMinSpin->setValue(m_filterRanges.heightMin);
    m_heightMaxSpin->setValue(m_filterRanges.heightMax);
    
    dimLayout->addWidget(m_heightLabel, 1, 0);
    dimLayout->addWidget(m_heightMinSpin, 1, 1);
    dimLayout->addWidget(m_heightMinSlider, 1, 2);
    dimLayout->addWidget(m_heightMaxSlider, 1, 3);
    dimLayout->addWidget(m_heightMaxSpin, 1, 4);
    
    // Depth filter
    m_depthLabel = new QLabel(tr("Depth:"), m_dimensionGroup);
    m_depthMinSlider = new QSlider(Qt::Horizontal, m_dimensionGroup);
    m_depthMaxSlider = new QSlider(Qt::Horizontal, m_dimensionGroup);
    m_depthMinSpin = new QSpinBox(m_dimensionGroup);
    m_depthMaxSpin = new QSpinBox(m_dimensionGroup);
    
    m_depthMinSlider->setRange(m_filterRanges.depthMin, m_filterRanges.depthMax);
    m_depthMaxSlider->setRange(m_filterRanges.depthMin, m_filterRanges.depthMax);
    m_depthMinSpin->setRange(m_filterRanges.depthMin, m_filterRanges.depthMax);
    m_depthMaxSpin->setRange(m_filterRanges.depthMin, m_filterRanges.depthMax);
    m_depthMinSlider->setValue(m_filterRanges.depthMin);
    m_depthMaxSlider->setValue(m_filterRanges.depthMax);
    m_depthMinSpin->setValue(m_filterRanges.depthMin);
    m_depthMaxSpin->setValue(m_filterRanges.depthMax);
    
    dimLayout->addWidget(m_depthLabel, 2, 0);
    dimLayout->addWidget(m_depthMinSpin, 2, 1);
    dimLayout->addWidget(m_depthMinSlider, 2, 2);
    dimLayout->addWidget(m_depthMaxSlider, 2, 3);
    dimLayout->addWidget(m_depthMaxSpin, 2, 4);
    
    filtersLayout->addWidget(m_dimensionGroup);
    
    // Price filters
    m_priceGroup = new QGroupBox(tr("Price Range"), m_filtersGroup);
    QHBoxLayout* priceLayout = new QHBoxLayout(m_priceGroup);
    
    m_priceMinSlider = new QSlider(Qt::Horizontal, m_priceGroup);
    m_priceMaxSlider = new QSlider(Qt::Horizontal, m_priceGroup);
    m_priceMinSpin = new QSpinBox(m_priceGroup);
    m_priceMaxSpin = new QSpinBox(m_priceGroup);
    
    m_priceMinSlider->setRange(m_filterRanges.priceMin, m_filterRanges.priceMax);
    m_priceMaxSlider->setRange(m_filterRanges.priceMin, m_filterRanges.priceMax);
    m_priceMinSpin->setRange(m_filterRanges.priceMin, m_filterRanges.priceMax);
    m_priceMaxSpin->setRange(m_filterRanges.priceMin, m_filterRanges.priceMax);
    m_priceMinSlider->setValue(m_filterRanges.priceMin);
    m_priceMaxSlider->setValue(m_filterRanges.priceMax);
    m_priceMinSpin->setValue(m_filterRanges.priceMin);
    m_priceMaxSpin->setValue(m_filterRanges.priceMax);
    
    priceLayout->addWidget(new QLabel(tr("Min:"), m_priceGroup));
    priceLayout->addWidget(m_priceMinSpin);
    priceLayout->addWidget(m_priceMinSlider);
    priceLayout->addWidget(new QLabel(tr("Max:"), m_priceGroup));
    priceLayout->addWidget(m_priceMaxSpin);
    priceLayout->addWidget(m_priceMaxSlider);
    
    filtersLayout->addWidget(m_priceGroup);
    
    // Clear filters button
    m_clearFiltersButton = new QPushButton(tr("Clear Filters"), m_filtersGroup);
    filtersLayout->addWidget(m_clearFiltersButton);
    
    m_mainLayout->addWidget(m_filtersGroup);
}

void CatalogPanel::setupItemList()
{
    // Item list
    m_itemList = new CatalogListWidget(this);
    m_itemList->setDragDropMode(QAbstractItemView::DragOnly);
    m_itemList->setDefaultDropAction(Qt::CopyAction);
    m_itemList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_itemList->setResizeMode(QListView::Adjust);
    m_itemList->setViewMode(QListView::ListMode);
    m_itemList->setIconSize(QSize(64, 64));
    m_itemList->setGridSize(QSize(80, 80));
    
    m_mainLayout->addWidget(m_itemList, 1); // Give it stretch factor
    
    // Status and controls
    QHBoxLayout* statusLayout = new QHBoxLayout();
    
    m_statusLabel = new QLabel(tr("Ready"), this);
    m_statusLabel->setStyleSheet("color: #666; font-size: 11px;");
    statusLayout->addWidget(m_statusLabel);
    
    statusLayout->addStretch();
    
    m_refreshButton = new QPushButton(tr("Refresh"), this);
    m_refreshButton->setMaximumWidth(80);
    statusLayout->addWidget(m_refreshButton);
    
    m_mainLayout->addLayout(statusLayout);
}

void CatalogPanel::connectSignals()
{
    // Search and filters
    connect(m_searchEdit, &QLineEdit::textChanged, this, &CatalogPanel::onSearchTextChanged);
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &CatalogPanel::onCategoryChanged);
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &CatalogPanel::onSortOrderChanged);
    connect(m_viewModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &CatalogPanel::onViewModeChanged);
    
    // Filter controls
    connect(m_filtersGroup, &QGroupBox::toggled, this, &CatalogPanel::onFilterChanged);
    connect(m_clearFiltersButton, &QPushButton::clicked, [this]() {
        // Reset all filters to default values
        m_widthMinSlider->setValue(m_filterRanges.widthMin);
        m_widthMaxSlider->setValue(m_filterRanges.widthMax);
        m_heightMinSlider->setValue(m_filterRanges.heightMin);
        m_heightMaxSlider->setValue(m_filterRanges.heightMax);
        m_depthMinSlider->setValue(m_filterRanges.depthMin);
        m_depthMaxSlider->setValue(m_filterRanges.depthMax);
        m_priceMinSlider->setValue(m_filterRanges.priceMin);
        m_priceMaxSlider->setValue(m_filterRanges.priceMax);
        applyFilters();
    });
    
    // Dimension filter connections
    connect(m_widthMinSlider, &QSlider::valueChanged, m_widthMinSpin, &QSpinBox::setValue);
    connect(m_widthMaxSlider, &QSlider::valueChanged, m_widthMaxSpin, &QSpinBox::setValue);
    connect(m_widthMinSpin, QOverload<int>::of(&QSpinBox::valueChanged), m_widthMinSlider, &QSlider::setValue);
    connect(m_widthMaxSpin, QOverload<int>::of(&QSpinBox::valueChanged), m_widthMaxSlider, &QSlider::setValue);
    connect(m_widthMinSlider, &QSlider::valueChanged, this, &CatalogPanel::onFilterChanged);
    connect(m_widthMaxSlider, &QSlider::valueChanged, this, &CatalogPanel::onFilterChanged);
    
    connect(m_heightMinSlider, &QSlider::valueChanged, m_heightMinSpin, &QSpinBox::setValue);
    connect(m_heightMaxSlider, &QSlider::valueChanged, m_heightMaxSpin, &QSpinBox::setValue);
    connect(m_heightMinSpin, QOverload<int>::of(&QSpinBox::valueChanged), m_heightMinSlider, &QSlider::setValue);
    connect(m_heightMaxSpin, QOverload<int>::of(&QSpinBox::valueChanged), m_heightMaxSlider, &QSlider::setValue);
    connect(m_heightMinSlider, &QSlider::valueChanged, this, &CatalogPanel::onFilterChanged);
    connect(m_heightMaxSlider, &QSlider::valueChanged, this, &CatalogPanel::onFilterChanged);
    
    connect(m_depthMinSlider, &QSlider::valueChanged, m_depthMinSpin, &QSpinBox::setValue);
    connect(m_depthMaxSlider, &QSlider::valueChanged, m_depthMaxSpin, &QSpinBox::setValue);
    connect(m_depthMinSpin, QOverload<int>::of(&QSpinBox::valueChanged), m_depthMinSlider, &QSlider::setValue);
    connect(m_depthMaxSpin, QOverload<int>::of(&QSpinBox::valueChanged), m_depthMaxSlider, &QSlider::setValue);
    connect(m_depthMinSlider, &QSlider::valueChanged, this, &CatalogPanel::onFilterChanged);
    connect(m_depthMaxSlider, &QSlider::valueChanged, this, &CatalogPanel::onFilterChanged);
    
    // Price filter connections
    connect(m_priceMinSlider, &QSlider::valueChanged, m_priceMinSpin, &QSpinBox::setValue);
    connect(m_priceMaxSlider, &QSlider::valueChanged, m_priceMaxSpin, &QSpinBox::setValue);
    connect(m_priceMinSpin, QOverload<int>::of(&QSpinBox::valueChanged), m_priceMinSlider, &QSlider::setValue);
    connect(m_priceMaxSpin, QOverload<int>::of(&QSpinBox::valueChanged), m_priceMaxSlider, &QSlider::setValue);
    connect(m_priceMinSlider, &QSlider::valueChanged, this, &CatalogPanel::onFilterChanged);
    connect(m_priceMaxSlider, &QSlider::valueChanged, this, &CatalogPanel::onFilterChanged);
    
    // Item list
    connect(m_itemList, &QListWidget::itemSelectionChanged, this, &CatalogPanel::onItemSelectionChanged);
    connect(m_itemList, &QListWidget::itemDoubleClicked, this, &CatalogPanel::onItemDoubleClicked);
    connect(m_itemList, &CatalogListWidget::itemDragStarted, this, &CatalogPanel::itemDragStarted);
    
    // Enable drag and drop
    m_itemList->setDragEnabled(true);
    m_itemList->setDragDropMode(QAbstractItemView::DragOnly);
    
    // Controls
    connect(m_refreshButton, &QPushButton::clicked, this, &CatalogPanel::refreshCatalog);
}

void CatalogPanel::refreshCatalog()
{
    LOG_INFO("Refreshing catalog");
    
    m_statusLabel->setText(tr("Loading catalog..."));
    QApplication::processEvents();
    
    try {
        // Load catalog data through service
        m_catalogService->refreshCatalog();
        
        populateCategories();
        populateItems();
        
        m_statusLabel->setText(tr("Catalog loaded successfully"));
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to refresh catalog: " + std::string(e.what()));
        m_statusLabel->setText(tr("Failed to load catalog"));
    }
}

void CatalogPanel::populateCategories()
{
    m_categoryCombo->clear();
    m_categoryCombo->addItem(tr("All Categories"), "All");
    
    // Get categories from catalog service
    m_availableCategories = m_catalogService->getAvailableCategories();
    
    for (const QString& category : m_availableCategories) {
        m_categoryCombo->addItem(category, category);
    }
}

void CatalogPanel::populateItems()
{
    m_itemList->clear();
    
    // Get all items from catalog service
    auto items = m_catalogService->getAllItems();
    
    for (const auto& item : items) {
        if (matchesFilters(*item)) {
            QListWidgetItem* listItem = new QListWidgetItem(m_itemList);
            listItem->setText(item->getName());
            listItem->setData(Qt::UserRole, item->getId());
            
            // Set tooltip with item details
            QString tooltip = QString("%1\nDimensions: %2 x %3 x %4 cm\nPrice: $%5")
                .arg(item->getName())
                .arg(item->getDimensions().width * 100) // Convert to cm
                .arg(item->getDimensions().height * 100)
                .arg(item->getDimensions().depth * 100)
                .arg(item->getPrice(), 0, 'f', 2);
            listItem->setToolTip(tooltip);
            
            // TODO: Load thumbnail icon
            // listItem->setIcon(loadThumbnail(item->getThumbnailPath()));
        }
    }
    
    updateItemDisplay();
}

void CatalogPanel::applyFilters()
{
    populateItems();
}

void CatalogPanel::updateItemDisplay()
{
    int itemCount = m_itemList->count();
    m_statusLabel->setText(tr("%1 items").arg(itemCount));
    
    // Update view mode
    switch (m_viewMode) {
        case ViewMode::List:
            m_itemList->setViewMode(QListView::ListMode);
            m_itemList->setIconSize(QSize(32, 32));
            break;
        case ViewMode::Grid:
            m_itemList->setViewMode(QListView::IconMode);
            m_itemList->setIconSize(QSize(64, 64));
            break;
        case ViewMode::Details:
            m_itemList->setViewMode(QListView::ListMode);
            m_itemList->setIconSize(QSize(48, 48));
            break;
    }
}

bool CatalogPanel::matchesSearch(const CatalogItem& item) const
{
    if (m_searchText.isEmpty()) {
        return true;
    }
    
    return item.getName().contains(m_searchText, Qt::CaseInsensitive) ||
           item.getCategory().contains(m_searchText, Qt::CaseInsensitive);
}

bool CatalogPanel::matchesCategory(const CatalogItem& item) const
{
    if (m_currentCategory == "All") {
        return true;
    }
    
    return item.getCategory() == m_currentCategory;
}

bool CatalogPanel::matchesFilters(const CatalogItem& item) const
{
    if (!matchesSearch(item) || !matchesCategory(item)) {
        return false;
    }
    
    if (!m_filtersGroup->isChecked()) {
        return true;
    }
    
    // Check dimension filters
    const auto& dims = item.getDimensions();
    int width = static_cast<int>(dims.width * 100); // Convert to cm
    int height = static_cast<int>(dims.height * 100);
    int depth = static_cast<int>(dims.depth * 100);
    
    if (width < m_widthMinSlider->value() || width > m_widthMaxSlider->value()) {
        return false;
    }
    if (height < m_heightMinSlider->value() || height > m_heightMaxSlider->value()) {
        return false;
    }
    if (depth < m_depthMinSlider->value() || depth > m_depthMaxSlider->value()) {
        return false;
    }
    
    // Check price filter
    double price = item.getPrice();
    if (price < m_priceMinSlider->value() || price > m_priceMaxSlider->value()) {
        return false;
    }
    
    return true;
}

QString CatalogPanel::getSelectedItemId() const
{
    QListWidgetItem* currentItem = m_itemList->currentItem();
    if (currentItem) {
        return currentItem->data(Qt::UserRole).toString();
    }
    return QString();
}

void CatalogPanel::clearSelection()
{
    m_itemList->clearSelection();
}

void CatalogPanel::loadCatalog(const QString& catalogPath)
{
    LOG_INFOF("Loading catalog from: {}", catalogPath.toStdString());
    
    try {
        m_catalogService->loadCatalog(catalogPath);
        refreshCatalog();
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load catalog: " + std::string(e.what()));
        m_statusLabel->setText(tr("Failed to load catalog file"));
    }
}

// Slot implementations
void CatalogPanel::onCategoryChanged()
{
    m_currentCategory = m_categoryCombo->currentData().toString();
    LOG_DEBUGF("Category changed to: {}", m_currentCategory.toStdString());
    applyFilters();
}

void CatalogPanel::onSearchTextChanged()
{
    m_searchText = m_searchEdit->text();
    LOG_DEBUGF("Search text changed to: {}", m_searchText.toStdString());
    applyFilters();
}

void CatalogPanel::onFilterChanged()
{
    LOG_DEBUG("Filters changed");
    applyFilters();
}

void CatalogPanel::onItemSelectionChanged()
{
    QString itemId = getSelectedItemId();
    if (!itemId.isEmpty()) {
        LOG_DEBUGF("Item selected: {}", itemId.toStdString());
        emit itemSelected(itemId);
    }
}

void CatalogPanel::onItemDoubleClicked(QListWidgetItem* item)
{
    if (item) {
        QString itemId = item->data(Qt::UserRole).toString();
        LOG_DEBUGF("Item double-clicked: {}", itemId.toStdString());
        emit itemDoubleClicked(itemId);
    }
}

void CatalogPanel::onSortOrderChanged()
{
    m_sortOrder = static_cast<SortOrder>(m_sortCombo->currentData().toInt());
    LOG_DEBUG("Sort order changed");
    // TODO: Implement sorting
    applyFilters();
}

void CatalogPanel::onViewModeChanged()
{
    m_viewMode = static_cast<ViewMode>(m_viewModeCombo->currentData().toInt());
    LOG_DEBUG("View mode changed");
    updateItemDisplay();
}

// CatalogItemWidget implementation
CatalogItemWidget::CatalogItemWidget(const QString& itemId, QWidget *parent)
    : QWidget(parent)
    , m_itemId(itemId)
    , m_selected(false)
    , m_hovered(false)
{
    setupUI();
}

void CatalogItemWidget::setupUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(2);
    
    m_thumbnailLabel = new QLabel(this);
    m_thumbnailLabel->setFixedSize(64, 64);
    m_thumbnailLabel->setAlignment(Qt::AlignCenter);
    m_thumbnailLabel->setStyleSheet("border: 1px solid #ccc; background-color: #f0f0f0;");
    layout->addWidget(m_thumbnailLabel);
    
    m_nameLabel = new QLabel(this);
    m_nameLabel->setWordWrap(true);
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setStyleSheet("font-weight: bold; font-size: 11px;");
    layout->addWidget(m_nameLabel);
    
    m_dimensionsLabel = new QLabel(this);
    m_dimensionsLabel->setAlignment(Qt::AlignCenter);
    m_dimensionsLabel->setStyleSheet("font-size: 10px; color: #666;");
    layout->addWidget(m_dimensionsLabel);
    
    m_priceLabel = new QLabel(this);
    m_priceLabel->setAlignment(Qt::AlignCenter);
    m_priceLabel->setStyleSheet("font-size: 10px; color: #0066cc; font-weight: bold;");
    layout->addWidget(m_priceLabel);
    
    m_categoryLabel = new QLabel(this);
    m_categoryLabel->setAlignment(Qt::AlignCenter);
    m_categoryLabel->setStyleSheet("font-size: 9px; color: #999;");
    layout->addWidget(m_categoryLabel);
    
    setFixedSize(80, 120);
}

void CatalogItemWidget::setItemData(const CatalogItem& item)
{
    m_nameLabel->setText(item.getName());
    
    const auto& dims = item.getDimensions();
    m_dimensionsLabel->setText(QString("%1×%2×%3")
        .arg(static_cast<int>(dims.width * 100))
        .arg(static_cast<int>(dims.height * 100))
        .arg(static_cast<int>(dims.depth * 100)));
    
    m_priceLabel->setText(QString("$%1").arg(item.getPrice(), 0, 'f', 2));
    m_categoryLabel->setText(item.getCategory());
    
    // TODO: Load thumbnail
    m_thumbnailLabel->setText("IMG");
    
    updateDisplay();
}

void CatalogItemWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit itemClicked(m_itemId);
    }
    QWidget::mousePressEvent(event);
}

void CatalogItemWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit itemDoubleClicked(m_itemId);
    }
    QWidget::mouseDoubleClickEvent(event);
}

void CatalogItemWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    
    if (m_selected || m_hovered) {
        QPainter painter(this);
        painter.setPen(QPen(m_selected ? Qt::blue : Qt::gray, 2));
        painter.drawRect(rect().adjusted(1, 1, -1, -1));
    }
}

void CatalogItemWidget::updateDisplay()
{
    update();
}

// CatalogListWidget implementation
CatalogListWidget::CatalogListWidget(QWidget* parent)
    : QListWidget(parent)
{
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setDefaultDropAction(Qt::CopyAction);
}

void CatalogListWidget::startDrag(Qt::DropActions supportedActions)
{
    QListWidgetItem* item = currentItem();
    if (!item) {
        return;
    }
    
    QString itemId = item->data(Qt::UserRole).toString();
    if (!itemId.isEmpty()) {
        emit itemDragStarted(itemId);
    }
    
    QListWidget::startDrag(supportedActions);
}

QMimeData* CatalogListWidget::mimeData(const QList<QListWidgetItem*>& items) const
{
    if (items.isEmpty()) {
        return nullptr;
    }
    
    QMimeData* mimeData = new QMimeData();
    
    // Set the catalog item ID as mime data
    QString itemId = items.first()->data(Qt::UserRole).toString();
    mimeData->setText(itemId);
    mimeData->setData("application/x-catalog-item", itemId.toUtf8());
    
    return mimeData;
}

#include "CatalogPanel.moc"