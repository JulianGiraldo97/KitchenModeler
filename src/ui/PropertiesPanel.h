#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QSlider>
#include <QColorDialog>
#include <QTabWidget>
#include <memory>

// Forward declarations
namespace KitchenCAD::Scene {
    class SceneManager;
}

namespace KitchenCAD::Models {
    class CatalogItem;
}

namespace KitchenCAD::UI {

class ColorButton;
class MaterialEditor;
class TransformEditor;

class PropertiesPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PropertiesPanel(QWidget *parent = nullptr);
    ~PropertiesPanel() override;

    // Object selection
    void setSelectedObject(const QString& objectId);
    void clearSelection();
    QString getSelectedObjectId() const { return m_selectedObjectId; }

Q_SIGNALS:
    void propertyChanged(const QString& objectId, const QString& property, const QVariant& value);
    void transformChanged(const QString& objectId);
    void materialChanged(const QString& objectId);

private Q_SLOTS:
    void onPropertyValueChanged();
    void onTransformChanged();
    void onMaterialChanged();
    void onResetToDefaults();
    void onApplyToSelected();

private:
    void setupUI();
    void setupGeneralTab();
    void setupTransformTab();
    void setupMaterialTab();
    void setupAdvancedTab();
    void connectSignals();
    
    void updateProperties();
    void updateGeneralProperties();
    void updateTransformProperties();
    void updateMaterialProperties();
    void updateAdvancedProperties();
    
    void loadObjectProperties(const QString& objectId);
    void saveObjectProperties();
    
    void enableControls(bool enabled);

private:
    // UI Components
    QVBoxLayout* m_mainLayout;
    QScrollArea* m_scrollArea;
    QWidget* m_scrollWidget;
    QTabWidget* m_tabWidget;
    
    // Header
    QLabel* m_titleLabel;
    QLabel* m_objectTypeLabel;
    QLabel* m_objectIdLabel;
    
    // General tab
    QWidget* m_generalTab;
    QLineEdit* m_nameEdit;
    QLineEdit* m_descriptionEdit;
    QComboBox* m_categoryCombo;
    QCheckBox* m_visibleCheck;
    QCheckBox* m_lockedCheck;
    QComboBox* m_layerCombo;
    
    // Transform tab
    QWidget* m_transformTab;
    TransformEditor* m_transformEditor;
    
    // Position controls
    QGroupBox* m_positionGroup;
    QDoubleSpinBox* m_posXSpin;
    QDoubleSpinBox* m_posYSpin;
    QDoubleSpinBox* m_posZSpin;
    QPushButton* m_centerButton;
    QPushButton* m_snapToGridButton;
    
    // Rotation controls
    QGroupBox* m_rotationGroup;
    QDoubleSpinBox* m_rotXSpin;
    QDoubleSpinBox* m_rotYSpin;
    QDoubleSpinBox* m_rotZSpin;
    QPushButton* m_rotate90Button;
    QPushButton* m_resetRotationButton;
    
    // Scale controls
    QGroupBox* m_scaleGroup;
    QDoubleSpinBox* m_scaleXSpin;
    QDoubleSpinBox* m_scaleYSpin;
    QDoubleSpinBox* m_scaleZSpin;
    QCheckBox* m_uniformScaleCheck;
    QPushButton* m_resetScaleButton;
    
    // Material tab
    QWidget* m_materialTab;
    MaterialEditor* m_materialEditor;
    
    // Material selection
    QComboBox* m_materialPresetCombo;
    QPushButton* m_loadMaterialButton;
    QPushButton* m_saveMaterialButton;
    
    // Material properties
    QGroupBox* m_colorGroup;
    ColorButton* m_diffuseColorButton;
    ColorButton* m_specularColorButton;
    ColorButton* m_emissiveColorButton;
    
    QGroupBox* m_surfaceGroup;
    QSlider* m_roughnessSlider;
    QSlider* m_metallicSlider;
    QSlider* m_reflectanceSlider;
    QDoubleSpinBox* m_roughnessSpin;
    QDoubleSpinBox* m_metallicSpin;
    QDoubleSpinBox* m_reflectanceSpin;
    
    QGroupBox* m_textureGroup;
    QLineEdit* m_diffuseTextureEdit;
    QLineEdit* m_normalTextureEdit;
    QLineEdit* m_roughnessTextureEdit;
    QPushButton* m_browseDiffuseButton;
    QPushButton* m_browseNormalButton;
    QPushButton* m_browseRoughnessButton;
    
    // Advanced tab
    QWidget* m_advancedTab;
    
    // Catalog info
    QGroupBox* m_catalogGroup;
    QLabel* m_catalogIdLabel;
    QLabel* m_catalogNameLabel;
    QLabel* m_dimensionsLabel;
    QLabel* m_basePriceLabel;
    
    // Physics properties
    QGroupBox* m_physicsGroup;
    QDoubleSpinBox* m_massSpin;
    QComboBox* m_collisionTypeCombo;
    QCheckBox* m_staticCheck;
    
    // Custom properties
    QGroupBox* m_customGroup;
    QVBoxLayout* m_customLayout;
    QPushButton* m_addPropertyButton;
    
    // Control buttons
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_resetButton;
    QPushButton* m_applyButton;
    QPushButton* m_revertButton;
    
    // Services
    std::unique_ptr<KitchenCAD::Scene::SceneManager> m_sceneManager;
    
    // State
    QString m_selectedObjectId;
    bool m_updating;
    bool m_hasChanges;
    QVariantMap m_originalProperties;
    QVariantMap m_currentProperties;
};

class ColorButton : public QPushButton
{
    Q_OBJECT

public:
    explicit ColorButton(QWidget *parent = nullptr);
    
    void setColor(const QColor& color);
    QColor getColor() const { return m_color; }

Q_SIGNALS:
    void colorChanged(const QColor& color);

private Q_SLOTS:
    void chooseColor();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QColor m_color;
};

class MaterialEditor : public QWidget
{
    Q_OBJECT

public:
    explicit MaterialEditor(QWidget *parent = nullptr);
    
    void setMaterialProperties(const QVariantMap& properties);
    QVariantMap getMaterialProperties() const;
    
    void resetToDefaults();

Q_SIGNALS:
    void materialChanged();

private Q_SLOTS:
    void onPropertyChanged();

private:
    void setupUI();
    void connectSignals();
    void updatePreview();

private:
    // Material preview
    QLabel* m_previewLabel;
    
    // Property controls
    QVariantMap m_properties;
    QMap<QString, QWidget*> m_propertyWidgets;
    
    bool m_updating;
};

class TransformEditor : public QWidget
{
    Q_OBJECT

public:
    explicit TransformEditor(QWidget *parent = nullptr);
    
    void setTransform(const QVector3D& position, const QVector3D& rotation, const QVector3D& scale);
    void getTransform(QVector3D& position, QVector3D& rotation, QVector3D& scale) const;
    
    void resetTransform();

Q_SIGNALS:
    void transformChanged();

private Q_SLOTS:
    void onValueChanged();
    void onResetPosition();
    void onResetRotation();
    void onResetScale();

private:
    void setupUI();
    void connectSignals();

private:
    // Transform components
    QDoubleSpinBox* m_positionSpins[3];
    QDoubleSpinBox* m_rotationSpins[3];
    QDoubleSpinBox* m_scaleSpins[3];
    
    // Control buttons
    QPushButton* m_resetPosButton;
    QPushButton* m_resetRotButton;
    QPushButton* m_resetScaleButton;
    
    bool m_updating;
};

} // namespace KitchenCAD::UI