#include "PropertiesPanel.h"
#include "../scene/SceneManager.h"
#include "../models/CatalogItem.h"
#include "../utils/Logger.h"

#include <QApplication>
#include <QPainter>
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>

using namespace KitchenCAD::UI;
using namespace KitchenCAD::Scene;
using namespace KitchenCAD::Models;
using namespace KitchenCAD::Utils;

PropertiesPanel::PropertiesPanel(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_scrollArea(nullptr)
    , m_scrollWidget(nullptr)
    , m_tabWidget(nullptr)
    , m_updating(false)
    , m_hasChanges(false)
{
    LOG_INFO("Initializing PropertiesPanel");
    
    // Initialize scene manager
    m_sceneManager = std::make_unique<SceneManager>();
    
    setupUI();
    connectSignals();
    
    // Initially disable all controls
    enableControls(false);
    
    LOG_INFO("PropertiesPanel initialized");
}

PropertiesPanel::~PropertiesPanel()
{
    LOG_INFO("Destroying PropertiesPanel");
}

void PropertiesPanel::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    m_mainLayout->setSpacing(5);
    
    // Title
    m_titleLabel = new QLabel(tr("Properties"), this);
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; padding: 5px;");
    m_mainLayout->addWidget(m_titleLabel);
    
    // Object info header
    QGroupBox* headerGroup = new QGroupBox(tr("Object Information"), this);
    QGridLayout* headerLayout = new QGridLayout(headerGroup);
    
    headerLayout->addWidget(new QLabel(tr("Type:"), headerGroup), 0, 0);
    m_objectTypeLabel = new QLabel(tr("None"), headerGroup);
    m_objectTypeLabel->setStyleSheet("font-weight: bold;");
    headerLayout->addWidget(m_objectTypeLabel, 0, 1);
    
    headerLayout->addWidget(new QLabel(tr("ID:"), headerGroup), 1, 0);
    m_objectIdLabel = new QLabel(tr("None"), headerGroup);
    m_objectIdLabel->setStyleSheet("font-family: monospace; font-size: 10px;");
    headerLayout->addWidget(m_objectIdLabel, 1, 1);
    
    m_mainLayout->addWidget(headerGroup);
    
    // Create scroll area for tabs
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    m_scrollWidget = new QWidget();
    QVBoxLayout* scrollLayout = new QVBoxLayout(m_scrollWidget);
    
    // Tab widget
    m_tabWidget = new QTabWidget(m_scrollWidget);
    scrollLayout->addWidget(m_tabWidget);
    
    setupGeneralTab();
    setupTransformTab();
    setupMaterialTab();
    setupAdvancedTab();
    
    m_scrollArea->setWidget(m_scrollWidget);
    m_mainLayout->addWidget(m_scrollArea, 1);
    
    // Control buttons
    m_buttonLayout = new QHBoxLayout();
    
    m_resetButton = new QPushButton(tr("Reset"), this);
    m_resetButton->setToolTip(tr("Reset all properties to default values"));
    m_buttonLayout->addWidget(m_resetButton);
    
    m_revertButton = new QPushButton(tr("Revert"), this);
    m_revertButton->setToolTip(tr("Revert changes to original values"));
    m_buttonLayout->addWidget(m_revertButton);
    
    m_buttonLayout->addStretch();
    
    m_applyButton = new QPushButton(tr("Apply"), this);
    m_applyButton->setToolTip(tr("Apply changes to selected object"));
    m_applyButton->setDefault(true);
    m_buttonLayout->addWidget(m_applyButton);
    
    m_mainLayout->addLayout(m_buttonLayout);
}v
oid PropertiesPanel::setupGeneralTab()
{
    m_generalTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_generalTab);
    
    // Basic properties
    QGroupBox* basicGroup = new QGroupBox(tr("Basic Properties"), m_generalTab);
    QGridLayout* basicLayout = new QGridLayout(basicGroup);
    
    basicLayout->addWidget(new QLabel(tr("Name:"), basicGroup), 0, 0);
    m_nameEdit = new QLineEdit(basicGroup);
    basicLayout->addWidget(m_nameEdit, 0, 1);
    
    basicLayout->addWidget(new QLabel(tr("Description:"), basicGroup), 1, 0);
    m_descriptionEdit = new QLineEdit(basicGroup);
    basicLayout->addWidget(m_descriptionEdit, 1, 1);
    
    basicLayout->addWidget(new QLabel(tr("Category:"), basicGroup), 2, 0);
    m_categoryCombo = new QComboBox(basicGroup);
    basicLayout->addWidget(m_categoryCombo, 2, 1);
    
    layout->addWidget(basicGroup);
    
    // Visibility and state
    QGroupBox* stateGroup = new QGroupBox(tr("State"), m_generalTab);
    QVBoxLayout* stateLayout = new QVBoxLayout(stateGroup);
    
    m_visibleCheck = new QCheckBox(tr("Visible"), stateGroup);
    m_visibleCheck->setChecked(true);
    stateLayout->addWidget(m_visibleCheck);
    
    m_lockedCheck = new QCheckBox(tr("Locked"), stateGroup);
    stateLayout->addWidget(m_lockedCheck);
    
    QHBoxLayout* layerLayout = new QHBoxLayout();
    layerLayout->addWidget(new QLabel(tr("Layer:"), stateGroup));
    m_layerCombo = new QComboBox(stateGroup);
    m_layerCombo->addItems({"Default", "Structure", "Furniture", "Appliances"});
    layerLayout->addWidget(m_layerCombo);
    stateLayout->addLayout(layerLayout);
    
    layout->addWidget(stateGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_generalTab, tr("General"));
}

void PropertiesPanel::setupTransformTab()
{
    m_transformTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_transformTab);
    
    // Position group
    m_positionGroup = new QGroupBox(tr("Position"), m_transformTab);
    QGridLayout* posLayout = new QGridLayout(m_positionGroup);
    
    posLayout->addWidget(new QLabel(tr("X:"), m_positionGroup), 0, 0);
    m_posXSpin = new QDoubleSpinBox(m_positionGroup);
    m_posXSpin->setRange(-1000.0, 1000.0);
    m_posXSpin->setDecimals(3);
    m_posXSpin->setSuffix(" m");
    posLayout->addWidget(m_posXSpin, 0, 1);
    
    posLayout->addWidget(new QLabel(tr("Y:"), m_positionGroup), 1, 0);
    m_posYSpin = new QDoubleSpinBox(m_positionGroup);
    m_posYSpin->setRange(-1000.0, 1000.0);
    m_posYSpin->setDecimals(3);
    m_posYSpin->setSuffix(" m");
    posLayout->addWidget(m_posYSpin, 1, 1);
    
    posLayout->addWidget(new QLabel(tr("Z:"), m_positionGroup), 2, 0);
    m_posZSpin = new QDoubleSpinBox(m_positionGroup);
    m_posZSpin->setRange(-1000.0, 1000.0);
    m_posZSpin->setDecimals(3);
    m_posZSpin->setSuffix(" m");
    posLayout->addWidget(m_posZSpin, 2, 1);
    
    QHBoxLayout* posButtonLayout = new QHBoxLayout();
    m_centerButton = new QPushButton(tr("Center"), m_positionGroup);
    m_snapToGridButton = new QPushButton(tr("Snap to Grid"), m_positionGroup);
    posButtonLayout->addWidget(m_centerButton);
    posButtonLayout->addWidget(m_snapToGridButton);
    posLayout->addLayout(posButtonLayout, 3, 0, 1, 2);
    
    layout->addWidget(m_positionGroup);
    
    // Rotation group
    m_rotationGroup = new QGroupBox(tr("Rotation"), m_transformTab);
    QGridLayout* rotLayout = new QGridLayout(m_rotationGroup);
    
    rotLayout->addWidget(new QLabel(tr("X:"), m_rotationGroup), 0, 0);
    m_rotXSpin = new QDoubleSpinBox(m_rotationGroup);
    m_rotXSpin->setRange(-360.0, 360.0);
    m_rotXSpin->setDecimals(1);
    m_rotXSpin->setSuffix("°");
    rotLayout->addWidget(m_rotXSpin, 0, 1);
    
    rotLayout->addWidget(new QLabel(tr("Y:"), m_rotationGroup), 1, 0);
    m_rotYSpin = new QDoubleSpinBox(m_rotationGroup);
    m_rotYSpin->setRange(-360.0, 360.0);
    m_rotYSpin->setDecimals(1);
    m_rotYSpin->setSuffix("°");
    rotLayout->addWidget(m_rotYSpin, 1, 1);
    
    rotLayout->addWidget(new QLabel(tr("Z:"), m_rotationGroup), 2, 0);
    m_rotZSpin = new QDoubleSpinBox(m_rotationGroup);
    m_rotZSpin->setRange(-360.0, 360.0);
    m_rotZSpin->setDecimals(1);
    m_rotZSpin->setSuffix("°");
    rotLayout->addWidget(m_rotZSpin, 2, 1);
    
    QHBoxLayout* rotButtonLayout = new QHBoxLayout();
    m_rotate90Button = new QPushButton(tr("Rotate 90°"), m_rotationGroup);
    m_resetRotationButton = new QPushButton(tr("Reset"), m_rotationGroup);
    rotButtonLayout->addWidget(m_rotate90Button);
    rotButtonLayout->addWidget(m_resetRotationButton);
    rotLayout->addLayout(rotButtonLayout, 3, 0, 1, 2);
    
    layout->addWidget(m_rotationGroup);
    
    // Scale group
    m_scaleGroup = new QGroupBox(tr("Scale"), m_transformTab);
    QGridLayout* scaleLayout = new QGridLayout(m_scaleGroup);
    
    m_uniformScaleCheck = new QCheckBox(tr("Uniform Scale"), m_scaleGroup);
    m_uniformScaleCheck->setChecked(true);
    scaleLayout->addWidget(m_uniformScaleCheck, 0, 0, 1, 2);
    
    scaleLayout->addWidget(new QLabel(tr("X:"), m_scaleGroup), 1, 0);
    m_scaleXSpin = new QDoubleSpinBox(m_scaleGroup);
    m_scaleXSpin->setRange(0.1, 10.0);
    m_scaleXSpin->setDecimals(2);
    m_scaleXSpin->setValue(1.0);
    scaleLayout->addWidget(m_scaleXSpin, 1, 1);
    
    scaleLayout->addWidget(new QLabel(tr("Y:"), m_scaleGroup), 2, 0);
    m_scaleYSpin = new QDoubleSpinBox(m_scaleGroup);
    m_scaleYSpin->setRange(0.1, 10.0);
    m_scaleYSpin->setDecimals(2);
    m_scaleYSpin->setValue(1.0);
    scaleLayout->addWidget(m_scaleYSpin, 2, 1);
    
    scaleLayout->addWidget(new QLabel(tr("Z:"), m_scaleGroup), 3, 0);
    m_scaleZSpin = new QDoubleSpinBox(m_scaleGroup);
    m_scaleZSpin->setRange(0.1, 10.0);
    m_scaleZSpin->setDecimals(2);
    m_scaleZSpin->setValue(1.0);
    scaleLayout->addWidget(m_scaleZSpin, 3, 1);
    
    m_resetScaleButton = new QPushButton(tr("Reset Scale"), m_scaleGroup);
    scaleLayout->addWidget(m_resetScaleButton, 4, 0, 1, 2);
    
    layout->addWidget(m_scaleGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_transformTab, tr("Transform"));
}

void PropertiesPanel::setupMaterialTab()
{
    m_materialTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_materialTab);
    
    // Material selection
    QGroupBox* selectionGroup = new QGroupBox(tr("Material Selection"), m_materialTab);
    QVBoxLayout* selectionLayout = new QVBoxLayout(selectionGroup);
    
    m_materialPresetCombo = new QComboBox(selectionGroup);
    m_materialPresetCombo->addItems({
        tr("Wood - Oak"), tr("Wood - Pine"), tr("Wood - Walnut"),
        tr("Metal - Steel"), tr("Metal - Aluminum"), tr("Metal - Brass"),
        tr("Plastic - White"), tr("Plastic - Black"), tr("Glass - Clear"),
        tr("Stone - Marble"), tr("Stone - Granite"), tr("Custom...")
    });
    selectionLayout->addWidget(m_materialPresetCombo);
    
    QHBoxLayout* materialButtonLayout = new QHBoxLayout();
    m_loadMaterialButton = new QPushButton(tr("Load..."), selectionGroup);
    m_saveMaterialButton = new QPushButton(tr("Save..."), selectionGroup);
    materialButtonLayout->addWidget(m_loadMaterialButton);
    materialButtonLayout->addWidget(m_saveMaterialButton);
    selectionLayout->addLayout(materialButtonLayout);
    
    layout->addWidget(selectionGroup);
    
    // Color properties
    m_colorGroup = new QGroupBox(tr("Colors"), m_materialTab);
    QGridLayout* colorLayout = new QGridLayout(m_colorGroup);
    
    colorLayout->addWidget(new QLabel(tr("Diffuse:"), m_colorGroup), 0, 0);
    m_diffuseColorButton = new ColorButton(m_colorGroup);
    m_diffuseColorButton->setColor(QColor(200, 200, 200));
    colorLayout->addWidget(m_diffuseColorButton, 0, 1);
    
    colorLayout->addWidget(new QLabel(tr("Specular:"), m_colorGroup), 1, 0);
    m_specularColorButton = new ColorButton(m_colorGroup);
    m_specularColorButton->setColor(QColor(255, 255, 255));
    colorLayout->addWidget(m_specularColorButton, 1, 1);
    
    colorLayout->addWidget(new QLabel(tr("Emissive:"), m_colorGroup), 2, 0);
    m_emissiveColorButton = new ColorButton(m_colorGroup);
    m_emissiveColorButton->setColor(QColor(0, 0, 0));
    colorLayout->addWidget(m_emissiveColorButton, 2, 1);
    
    layout->addWidget(m_colorGroup);
    
    // Surface properties
    m_surfaceGroup = new QGroupBox(tr("Surface Properties"), m_materialTab);
    QGridLayout* surfaceLayout = new QGridLayout(m_surfaceGroup);
    
    // Roughness
    surfaceLayout->addWidget(new QLabel(tr("Roughness:"), m_surfaceGroup), 0, 0);
    m_roughnessSlider = new QSlider(Qt::Horizontal, m_surfaceGroup);
    m_roughnessSlider->setRange(0, 100);
    m_roughnessSlider->setValue(50);
    surfaceLayout->addWidget(m_roughnessSlider, 0, 1);
    m_roughnessSpin = new QDoubleSpinBox(m_surfaceGroup);
    m_roughnessSpin->setRange(0.0, 1.0);
    m_roughnessSpin->setDecimals(2);
    m_roughnessSpin->setValue(0.5);
    surfaceLayout->addWidget(m_roughnessSpin, 0, 2);
    
    // Metallic
    surfaceLayout->addWidget(new QLabel(tr("Metallic:"), m_surfaceGroup), 1, 0);
    m_metallicSlider = new QSlider(Qt::Horizontal, m_surfaceGroup);
    m_metallicSlider->setRange(0, 100);
    m_metallicSlider->setValue(0);
    surfaceLayout->addWidget(m_metallicSlider, 1, 1);
    m_metallicSpin = new QDoubleSpinBox(m_surfaceGroup);
    m_metallicSpin->setRange(0.0, 1.0);
    m_metallicSpin->setDecimals(2);
    m_metallicSpin->setValue(0.0);
    surfaceLayout->addWidget(m_metallicSpin, 1, 2);
    
    // Reflectance
    surfaceLayout->addWidget(new QLabel(tr("Reflectance:"), m_surfaceGroup), 2, 0);
    m_reflectanceSlider = new QSlider(Qt::Horizontal, m_surfaceGroup);
    m_reflectanceSlider->setRange(0, 100);
    m_reflectanceSlider->setValue(4);
    surfaceLayout->addWidget(m_reflectanceSlider, 2, 1);
    m_reflectanceSpin = new QDoubleSpinBox(m_surfaceGroup);
    m_reflectanceSpin->setRange(0.0, 1.0);
    m_reflectanceSpin->setDecimals(2);
    m_reflectanceSpin->setValue(0.04);
    surfaceLayout->addWidget(m_reflectanceSpin, 2, 2);
    
    layout->addWidget(m_surfaceGroup);
    
    // Texture properties
    m_textureGroup = new QGroupBox(tr("Textures"), m_materialTab);
    QGridLayout* textureLayout = new QGridLayout(m_textureGroup);
    
    textureLayout->addWidget(new QLabel(tr("Diffuse:"), m_textureGroup), 0, 0);
    m_diffuseTextureEdit = new QLineEdit(m_textureGroup);
    textureLayout->addWidget(m_diffuseTextureEdit, 0, 1);
    m_browseDiffuseButton = new QPushButton(tr("..."), m_textureGroup);
    m_browseDiffuseButton->setMaximumWidth(30);
    textureLayout->addWidget(m_browseDiffuseButton, 0, 2);
    
    textureLayout->addWidget(new QLabel(tr("Normal:"), m_textureGroup), 1, 0);
    m_normalTextureEdit = new QLineEdit(m_textureGroup);
    textureLayout->addWidget(m_normalTextureEdit, 1, 1);
    m_browseNormalButton = new QPushButton(tr("..."), m_textureGroup);
    m_browseNormalButton->setMaximumWidth(30);
    textureLayout->addWidget(m_browseNormalButton, 1, 2);
    
    textureLayout->addWidget(new QLabel(tr("Roughness:"), m_textureGroup), 2, 0);
    m_roughnessTextureEdit = new QLineEdit(m_textureGroup);
    textureLayout->addWidget(m_roughnessTextureEdit, 2, 1);
    m_browseRoughnessButton = new QPushButton(tr("..."), m_textureGroup);
    m_browseRoughnessButton->setMaximumWidth(30);
    textureLayout->addWidget(m_browseRoughnessButton, 2, 2);
    
    layout->addWidget(m_textureGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_materialTab, tr("Material"));
}

void PropertiesPanel::setupAdvancedTab()
{
    m_advancedTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_advancedTab);
    
    // Catalog information
    m_catalogGroup = new QGroupBox(tr("Catalog Information"), m_advancedTab);
    QGridLayout* catalogLayout = new QGridLayout(m_catalogGroup);
    
    catalogLayout->addWidget(new QLabel(tr("Catalog ID:"), m_catalogGroup), 0, 0);
    m_catalogIdLabel = new QLabel(tr("N/A"), m_catalogGroup);
    m_catalogIdLabel->setStyleSheet("font-family: monospace;");
    catalogLayout->addWidget(m_catalogIdLabel, 0, 1);
    
    catalogLayout->addWidget(new QLabel(tr("Name:"), m_catalogGroup), 1, 0);
    m_catalogNameLabel = new QLabel(tr("N/A"), m_catalogGroup);
    catalogLayout->addWidget(m_catalogNameLabel, 1, 1);
    
    catalogLayout->addWidget(new QLabel(tr("Dimensions:"), m_catalogGroup), 2, 0);
    m_dimensionsLabel = new QLabel(tr("N/A"), m_catalogGroup);
    catalogLayout->addWidget(m_dimensionsLabel, 2, 1);
    
    catalogLayout->addWidget(new QLabel(tr("Base Price:"), m_catalogGroup), 3, 0);
    m_basePriceLabel = new QLabel(tr("N/A"), m_catalogGroup);
    catalogLayout->addWidget(m_basePriceLabel, 3, 1);
    
    layout->addWidget(m_catalogGroup);
    
    // Physics properties
    m_physicsGroup = new QGroupBox(tr("Physics Properties"), m_advancedTab);
    QGridLayout* physicsLayout = new QGridLayout(m_physicsGroup);
    
    physicsLayout->addWidget(new QLabel(tr("Mass:"), m_physicsGroup), 0, 0);
    m_massSpin = new QDoubleSpinBox(m_physicsGroup);
    m_massSpin->setRange(0.1, 10000.0);
    m_massSpin->setDecimals(2);
    m_massSpin->setSuffix(" kg");
    m_massSpin->setValue(10.0);
    physicsLayout->addWidget(m_massSpin, 0, 1);
    
    physicsLayout->addWidget(new QLabel(tr("Collision:"), m_physicsGroup), 1, 0);
    m_collisionTypeCombo = new QComboBox(m_physicsGroup);
    m_collisionTypeCombo->addItems({tr("Box"), tr("Mesh"), tr("None")});
    physicsLayout->addWidget(m_collisionTypeCombo, 1, 1);
    
    m_staticCheck = new QCheckBox(tr("Static Object"), m_physicsGroup);
    m_staticCheck->setChecked(true);
    physicsLayout->addWidget(m_staticCheck, 2, 0, 1, 2);
    
    layout->addWidget(m_physicsGroup);
    
    // Custom properties
    m_customGroup = new QGroupBox(tr("Custom Properties"), m_advancedTab);
    m_customLayout = new QVBoxLayout(m_customGroup);
    
    m_addPropertyButton = new QPushButton(tr("Add Property"), m_customGroup);
    m_customLayout->addWidget(m_addPropertyButton);
    m_customLayout->addStretch();
    
    layout->addWidget(m_customGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_advancedTab, tr("Advanced"));
}void
 PropertiesPanel::connectSignals()
{
    // General tab signals
    connect(m_nameEdit, &QLineEdit::textChanged, this, &PropertiesPanel::onPropertyValueChanged);
    connect(m_descriptionEdit, &QLineEdit::textChanged, this, &PropertiesPanel::onPropertyValueChanged);
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &PropertiesPanel::onPropertyValueChanged);
    connect(m_visibleCheck, &QCheckBox::toggled, this, &PropertiesPanel::onPropertyValueChanged);
    connect(m_lockedCheck, &QCheckBox::toggled, this, &PropertiesPanel::onPropertyValueChanged);
    connect(m_layerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &PropertiesPanel::onPropertyValueChanged);
    
    // Transform tab signals
    connect(m_posXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &PropertiesPanel::onTransformChanged);
    connect(m_posYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &PropertiesPanel::onTransformChanged);
    connect(m_posZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &PropertiesPanel::onTransformChanged);
    
    connect(m_rotXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &PropertiesPanel::onTransformChanged);
    connect(m_rotYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &PropertiesPanel::onTransformChanged);
    connect(m_rotZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &PropertiesPanel::onTransformChanged);
    
    connect(m_scaleXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &PropertiesPanel::onTransformChanged);
    connect(m_scaleYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &PropertiesPanel::onTransformChanged);
    connect(m_scaleZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &PropertiesPanel::onTransformChanged);
    
    // Transform buttons
    connect(m_centerButton, &QPushButton::clicked, [this]() {
        m_posXSpin->setValue(0.0);
        m_posYSpin->setValue(0.0);
        m_posZSpin->setValue(0.0);
    });
    
    connect(m_rotate90Button, &QPushButton::clicked, [this]() {
        m_rotZSpin->setValue(m_rotZSpin->value() + 90.0);
    });
    
    connect(m_resetRotationButton, &QPushButton::clicked, [this]() {
        m_rotXSpin->setValue(0.0);
        m_rotYSpin->setValue(0.0);
        m_rotZSpin->setValue(0.0);
    });
    
    connect(m_resetScaleButton, &QPushButton::clicked, [this]() {
        m_scaleXSpin->setValue(1.0);
        m_scaleYSpin->setValue(1.0);
        m_scaleZSpin->setValue(1.0);
    });
    
    // Uniform scale handling
    connect(m_uniformScaleCheck, &QCheckBox::toggled, [this](bool uniform) {
        if (uniform) {
            double value = m_scaleXSpin->value();
            m_scaleYSpin->setValue(value);
            m_scaleZSpin->setValue(value);
        }
    });
    
    connect(m_scaleXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            [this](double value) {
        if (m_uniformScaleCheck->isChecked() && !m_updating) {
            m_updating = true;
            m_scaleYSpin->setValue(value);
            m_scaleZSpin->setValue(value);
            m_updating = false;
        }
    });
    
    // Material tab signals
    connect(m_materialPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &PropertiesPanel::onMaterialChanged);
    
    connect(m_diffuseColorButton, &ColorButton::colorChanged, this, &PropertiesPanel::onMaterialChanged);
    connect(m_specularColorButton, &ColorButton::colorChanged, this, &PropertiesPanel::onMaterialChanged);
    connect(m_emissiveColorButton, &ColorButton::colorChanged, this, &PropertiesPanel::onMaterialChanged);
    
    // Surface property sliders
    connect(m_roughnessSlider, &QSlider::valueChanged, [this](int value) {
        m_roughnessSpin->setValue(value / 100.0);
    });
    connect(m_roughnessSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            [this](double value) {
        m_roughnessSlider->setValue(static_cast<int>(value * 100));
    });
    connect(m_roughnessSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &PropertiesPanel::onMaterialChanged);
    
    connect(m_metallicSlider, &QSlider::valueChanged, [this](int value) {
        m_metallicSpin->setValue(value / 100.0);
    });
    connect(m_metallicSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            [this](double value) {
        m_metallicSlider->setValue(static_cast<int>(value * 100));
    });
    connect(m_metallicSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &PropertiesPanel::onMaterialChanged);
    
    connect(m_reflectanceSlider, &QSlider::valueChanged, [this](int value) {
        m_reflectanceSpin->setValue(value / 100.0);
    });
    connect(m_reflectanceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            [this](double value) {
        m_reflectanceSlider->setValue(static_cast<int>(value * 100));
    });
    connect(m_reflectanceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &PropertiesPanel::onMaterialChanged);
    
    // Texture browse buttons
    connect(m_browseDiffuseButton, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Select Diffuse Texture"),
            QString(), tr("Image Files (*.png *.jpg *.jpeg *.bmp *.tga)"));
        if (!fileName.isEmpty()) {
            m_diffuseTextureEdit->setText(fileName);
        }
    });
    
    connect(m_browseNormalButton, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Select Normal Map"),
            QString(), tr("Image Files (*.png *.jpg *.jpeg *.bmp *.tga)"));
        if (!fileName.isEmpty()) {
            m_normalTextureEdit->setText(fileName);
        }
    });
    
    connect(m_browseRoughnessButton, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Select Roughness Map"),
            QString(), tr("Image Files (*.png *.jpg *.jpeg *.bmp *.tga)"));
        if (!fileName.isEmpty()) {
            m_roughnessTextureEdit->setText(fileName);
        }
    });
    
    // Advanced tab signals
    connect(m_massSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &PropertiesPanel::onPropertyValueChanged);
    connect(m_collisionTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &PropertiesPanel::onPropertyValueChanged);
    connect(m_staticCheck, &QCheckBox::toggled, this, &PropertiesPanel::onPropertyValueChanged);
    
    // Control buttons
    connect(m_resetButton, &QPushButton::clicked, this, &PropertiesPanel::onResetToDefaults);
    connect(m_applyButton, &QPushButton::clicked, this, &PropertiesPanel::onApplyToSelected);
    connect(m_revertButton, &QPushButton::clicked, [this]() {
        loadObjectProperties(m_selectedObjectId);
    });
}

void PropertiesPanel::setSelectedObject(const QString& objectId)
{
    if (m_selectedObjectId == objectId) {
        return;
    }
    
    LOG_DEBUGF("Setting selected object: {}", objectId.toStdString());
    
    // Save current changes if any
    if (m_hasChanges && !m_selectedObjectId.isEmpty()) {
        saveObjectProperties();
    }
    
    m_selectedObjectId = objectId;
    
    if (objectId.isEmpty()) {
        clearSelection();
    } else {
        loadObjectProperties(objectId);
        enableControls(true);
    }
}

void PropertiesPanel::clearSelection()
{
    LOG_DEBUG("Clearing selection");
    
    m_selectedObjectId.clear();
    m_hasChanges = false;
    
    // Update header
    m_objectTypeLabel->setText(tr("None"));
    m_objectIdLabel->setText(tr("None"));
    
    // Clear all controls
    m_updating = true;
    
    // General tab
    m_nameEdit->clear();
    m_descriptionEdit->clear();
    m_categoryCombo->setCurrentIndex(0);
    m_visibleCheck->setChecked(true);
    m_lockedCheck->setChecked(false);
    m_layerCombo->setCurrentIndex(0);
    
    // Transform tab
    m_posXSpin->setValue(0.0);
    m_posYSpin->setValue(0.0);
    m_posZSpin->setValue(0.0);
    m_rotXSpin->setValue(0.0);
    m_rotYSpin->setValue(0.0);
    m_rotZSpin->setValue(0.0);
    m_scaleXSpin->setValue(1.0);
    m_scaleYSpin->setValue(1.0);
    m_scaleZSpin->setValue(1.0);
    
    // Material tab
    m_materialPresetCombo->setCurrentIndex(0);
    m_diffuseColorButton->setColor(QColor(200, 200, 200));
    m_specularColorButton->setColor(QColor(255, 255, 255));
    m_emissiveColorButton->setColor(QColor(0, 0, 0));
    m_roughnessSpin->setValue(0.5);
    m_metallicSpin->setValue(0.0);
    m_reflectanceSpin->setValue(0.04);
    m_diffuseTextureEdit->clear();
    m_normalTextureEdit->clear();
    m_roughnessTextureEdit->clear();
    
    // Advanced tab
    m_catalogIdLabel->setText(tr("N/A"));
    m_catalogNameLabel->setText(tr("N/A"));
    m_dimensionsLabel->setText(tr("N/A"));
    m_basePriceLabel->setText(tr("N/A"));
    m_massSpin->setValue(10.0);
    m_collisionTypeCombo->setCurrentIndex(0);
    m_staticCheck->setChecked(true);
    
    m_updating = false;
    
    enableControls(false);
}

void PropertiesPanel::loadObjectProperties(const QString& objectId)
{
    if (objectId.isEmpty()) {
        return;
    }
    
    LOG_DEBUGF("Loading properties for object: {}", objectId.toStdString());
    
    m_updating = true;
    
    // TODO: Load actual object properties from scene manager
    // For now, use placeholder data
    
    // Update header
    m_objectTypeLabel->setText(tr("Cabinet Module"));
    m_objectIdLabel->setText(objectId);
    
    // General properties (placeholder)
    m_nameEdit->setText("Kitchen Cabinet");
    m_descriptionEdit->setText("Base cabinet module");
    m_categoryCombo->setCurrentText("Furniture");
    m_visibleCheck->setChecked(true);
    m_lockedCheck->setChecked(false);
    m_layerCombo->setCurrentText("Furniture");
    
    // Transform properties (placeholder)
    m_posXSpin->setValue(0.0);
    m_posYSpin->setValue(0.0);
    m_posZSpin->setValue(0.0);
    m_rotXSpin->setValue(0.0);
    m_rotYSpin->setValue(0.0);
    m_rotZSpin->setValue(0.0);
    m_scaleXSpin->setValue(1.0);
    m_scaleYSpin->setValue(1.0);
    m_scaleZSpin->setValue(1.0);
    
    // Material properties (placeholder)
    m_materialPresetCombo->setCurrentText("Wood - Oak");
    m_diffuseColorButton->setColor(QColor(139, 90, 43));
    m_specularColorButton->setColor(QColor(255, 255, 255));
    m_emissiveColorButton->setColor(QColor(0, 0, 0));
    m_roughnessSpin->setValue(0.7);
    m_metallicSpin->setValue(0.0);
    m_reflectanceSpin->setValue(0.04);
    
    // Advanced properties (placeholder)
    m_catalogIdLabel->setText("CAB_BASE_60");
    m_catalogNameLabel->setText("Base Cabinet 60cm");
    m_dimensionsLabel->setText("60 × 85 × 60 cm");
    m_basePriceLabel->setText("$299.99");
    m_massSpin->setValue(25.0);
    m_collisionTypeCombo->setCurrentText("Box");
    m_staticCheck->setChecked(true);
    
    m_updating = false;
    m_hasChanges = false;
    
    // Store original properties for revert functionality
    // TODO: Implement proper property storage
}

void PropertiesPanel::saveObjectProperties()
{
    if (m_selectedObjectId.isEmpty() || !m_hasChanges) {
        return;
    }
    
    LOG_DEBUGF("Saving properties for object: {}", m_selectedObjectId.toStdString());
    
    // TODO: Save properties to scene manager
    // For now, just mark as saved
    m_hasChanges = false;
}

void PropertiesPanel::enableControls(bool enabled)
{
    // Enable/disable all tabs
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        m_tabWidget->widget(i)->setEnabled(enabled);
    }
    
    // Enable/disable control buttons
    m_resetButton->setEnabled(enabled);
    m_applyButton->setEnabled(enabled && m_hasChanges);
    m_revertButton->setEnabled(enabled && m_hasChanges);
}

// Slot implementations
void PropertiesPanel::onPropertyValueChanged()
{
    if (m_updating) {
        return;
    }
    
    m_hasChanges = true;
    m_applyButton->setEnabled(true);
    m_revertButton->setEnabled(true);
    
    // Emit property changed signal
    QObject* sender = this->sender();
    if (sender) {
        QString property = sender->objectName();
        QVariant value;
        
        // Get value based on sender type
        if (auto* lineEdit = qobject_cast<QLineEdit*>(sender)) {
            value = lineEdit->text();
        } else if (auto* spinBox = qobject_cast<QSpinBox*>(sender)) {
            value = spinBox->value();
        } else if (auto* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(sender)) {
            value = doubleSpinBox->value();
        } else if (auto* comboBox = qobject_cast<QComboBox*>(sender)) {
            value = comboBox->currentText();
        } else if (auto* checkBox = qobject_cast<QCheckBox*>(sender)) {
            value = checkBox->isChecked();
        }
        
        emit propertyChanged(m_selectedObjectId, property, value);
    }
}

void PropertiesPanel::onTransformChanged()
{
    if (m_updating) {
        return;
    }
    
    onPropertyValueChanged();
    emit transformChanged(m_selectedObjectId);
}

void PropertiesPanel::onMaterialChanged()
{
    if (m_updating) {
        return;
    }
    
    onPropertyValueChanged();
    emit materialChanged(m_selectedObjectId);
}

void PropertiesPanel::onResetToDefaults()
{
    if (m_selectedObjectId.isEmpty()) {
        return;
    }
    
    int ret = QMessageBox::question(this, tr("Reset Properties"),
        tr("Are you sure you want to reset all properties to their default values?"),
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        LOG_DEBUG("Resetting properties to defaults");
        // TODO: Reset to actual default values
        clearSelection();
        setSelectedObject(m_selectedObjectId);
    }
}

void PropertiesPanel::onApplyToSelected()
{
    if (m_selectedObjectId.isEmpty()) {
        return;
    }
    
    LOG_DEBUG("Applying properties to selected object");
    saveObjectProperties();
}

// ColorButton implementation
ColorButton::ColorButton(QWidget *parent)
    : QPushButton(parent)
    , m_color(Qt::white)
{
    setFixedSize(40, 25);
    connect(this, &QPushButton::clicked, this, &ColorButton::chooseColor);
}

void ColorButton::setColor(const QColor& color)
{
    if (m_color != color) {
        m_color = color;
        update();
        emit colorChanged(color);
    }
}

void ColorButton::chooseColor()
{
    QColor newColor = QColorDialog::getColor(m_color, this, tr("Choose Color"));
    if (newColor.isValid()) {
        setColor(newColor);
    }
}

void ColorButton::paintEvent(QPaintEvent* event)
{
    QPushButton::paintEvent(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRect colorRect = rect().adjusted(4, 4, -4, -4);
    painter.fillRect(colorRect, m_color);
    painter.setPen(QPen(Qt::black, 1));
    painter.drawRect(colorRect);
}

#include "PropertiesPanel.moc"