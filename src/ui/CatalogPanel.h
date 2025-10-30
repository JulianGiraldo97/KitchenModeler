#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QSlider>
#include <QSpinBox>
#include <memory>

// Forward declarations
namespace KitchenCAD::Services {
    class CatalogService;
}

namespace KitchenCAD::Models {
    class CatalogItem;
}

namespace KitchenCAD::UI {

class CatalogItemWidget;
class CatalogListWidget;

class CatalogPanel : public QWidget
{
    Q_OBJECT

public:
    explicit CatalogPanel(QWidget *parent = nullptr);
    ~CatalogPanel() override;

    // Catalog management
    void refreshCatalog();
    void loadCatalog(const QString& catalogPath);
    
    // Selection
    QString getSelectedItemId() const;
    void clearSelection();

Q_SIGNALS:
    void itemSelected(const QString& itemId);
    void itemDoubleClicked(const QString& itemId);
    void itemDragStarted(const QString& itemId);

private Q_SLOTS:
    void onCategoryChanged();
    void onSearchTextChanged();
    void onFilterChanged();
    void onItemSelectionChanged();
    void onItemDoubleClicked(QListWidgetItem* item);
    void onSortOrderChanged();
    void onViewModeChanged();

private:
    void setupUI();
    void setupFilters();
    void setupItemList();
    void connectSignals();
    
    void populateCategories();
    void populateItems();
    void applyFilters();
    void updateItemDisplay();
    
    // Filter methods
    bool matchesSearch(const KitchenCAD::Models::CatalogItem& item) const;
    bool matchesCategory(const KitchenCAD::Models::CatalogItem& item) const;
    bool matchesFilters(const KitchenCAD::Models::CatalogItem& item) const;

private:
    // UI Components
    QVBoxLayout* m_mainLayout;
    
    // Search and filters
    QGroupBox* m_filtersGroup;
    QLineEdit* m_searchEdit;
    QComboBox* m_categoryCombo;
    QComboBox* m_sortCombo;
    QComboBox* m_viewModeCombo;
    
    // Dimension filters
    QGroupBox* m_dimensionGroup;
    QLabel* m_widthLabel;
    QSlider* m_widthMinSlider;
    QSlider* m_widthMaxSlider;
    QSpinBox* m_widthMinSpin;
    QSpinBox* m_widthMaxSpin;
    
    QLabel* m_heightLabel;
    QSlider* m_heightMinSlider;
    QSlider* m_heightMaxSlider;
    QSpinBox* m_heightMinSpin;
    QSpinBox* m_heightMaxSpin;
    
    QLabel* m_depthLabel;
    QSlider* m_depthMinSlider;
    QSlider* m_depthMaxSlider;
    QSpinBox* m_depthMinSpin;
    QSpinBox* m_depthMaxSpin;
    
    // Price filters
    QGroupBox* m_priceGroup;
    QSlider* m_priceMinSlider;
    QSlider* m_priceMaxSlider;
    QSpinBox* m_priceMinSpin;
    QSpinBox* m_priceMaxSpin;
    
    // Item display
    CatalogListWidget* m_itemList;
    
    // Status
    QLabel* m_statusLabel;
    QPushButton* m_refreshButton;
    QPushButton* m_clearFiltersButton;
    
    // Services
    std::unique_ptr<KitchenCAD::Services::CatalogService> m_catalogService;
    
    // State
    QString m_currentCategory;
    QString m_searchText;
    QStringList m_availableCategories;
    
    // Filter ranges
    struct FilterRanges {
        int widthMin = 0;
        int widthMax = 300; // cm
        int heightMin = 0;
        int heightMax = 250; // cm
        int depthMin = 0;
        int depthMax = 100; // cm
        int priceMin = 0;
        int priceMax = 10000; // currency units
    } m_filterRanges;
    
    enum class ViewMode {
        List,
        Grid,
        Details
    } m_viewMode;
    
    enum class SortOrder {
        Name,
        Category,
        Price,
        Width,
        Height,
        Depth
    } m_sortOrder;
};

/**
 * @brief Custom QListWidget that supports drag operations for catalog items
 */
class CatalogListWidget : public QListWidget
{
    Q_OBJECT

public:
    explicit CatalogListWidget(QWidget* parent = nullptr);

Q_SIGNALS:
    void itemDragStarted(const QString& itemId);

protected:
    void startDrag(Qt::DropActions supportedActions) override;
    QMimeData* mimeData(const QList<QListWidgetItem*>& items) const override;
};

class CatalogItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CatalogItemWidget(const QString& itemId, QWidget *parent = nullptr);
    
    void setItemData(const KitchenCAD::Models::CatalogItem& item);
    QString getItemId() const { return m_itemId; }

Q_SIGNALS:
    void itemClicked(const QString& itemId);
    void itemDoubleClicked(const QString& itemId);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void setupUI();
    void updateDisplay();

private:
    QString m_itemId;
    QLabel* m_thumbnailLabel;
    QLabel* m_nameLabel;
    QLabel* m_dimensionsLabel;
    QLabel* m_priceLabel;
    QLabel* m_categoryLabel;
    
    bool m_selected;
    bool m_hovered;
};

} // namespace KitchenCAD::UI