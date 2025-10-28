#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <iostream>

// Include our geometry system and logging
#include "geometry/Geometry.h"
#include "utils/Logger.h"

using namespace KitchenCAD::Geometry;
using namespace KitchenCAD::Utils;

void testGeometrySystem() {
    LOG_INFO("Testing Kitchen CAD Geometry System");
    
    // Test Point3D
    Point3D p1(1.0, 2.0, 3.0);
    Point3D p2(4.0, 5.0, 6.0);
    double distance = p1.distanceTo(p2);
    LOG_INFOF("Distance between points: {}", std::to_string(distance));
    
    // Test Vector3D
    Vector3D v1(1.0, 0.0, 0.0);
    Vector3D v2(0.0, 1.0, 0.0);
    Vector3D cross = v1.cross(v2);
    LOG_INFOF("Cross product result: ({}, {}, {})", 
              std::to_string(cross.x), std::to_string(cross.y), std::to_string(cross.z));
    
    // Test Transform3D
    Transform3D transform(Point3D(10.0, 20.0, 30.0), Vector3D(0.0, 0.0, GeometryUtils::degreesToRadians(90.0)));
    Point3D transformedPoint = transform.transformPoint(Point3D(1.0, 0.0, 0.0));
    LOG_INFOF("Transformed point: ({}, {}, {})", 
              std::to_string(transformedPoint.x), std::to_string(transformedPoint.y), std::to_string(transformedPoint.z));
    
    // Test BoundingBox
    BoundingBox bbox(Point3D(-1.0, -1.0, -1.0), Point3D(1.0, 1.0, 1.0));
    Point3D center = bbox.center();
    Vector3D size = bbox.size();
    LOG_INFOF("BoundingBox center: ({}, {}, {}), size: ({}, {}, {})",
              std::to_string(center.x), std::to_string(center.y), std::to_string(center.z),
              std::to_string(size.x), std::to_string(size.y), std::to_string(size.z));
    
    // Test Matrix4x4
    Matrix4x4 matrix = Matrix4x4::translation(Vector3D(5.0, 10.0, 15.0));
    Point3D matrixTransformed = matrix.transformPoint(Point3D(1.0, 1.0, 1.0));
    LOG_INFOF("Matrix transformed point: ({}, {}, {})",
              std::to_string(matrixTransformed.x), std::to_string(matrixTransformed.y), std::to_string(matrixTransformed.z));
    
    // Test GeometryUtils
    double angleRad = GeometryUtils::degreesToRadians(90.0);
    double angleDeg = GeometryUtils::radiansToDegrees(angleRad);
    LOG_INFOF("Angle conversion: 90° = {} rad = {}°", std::to_string(angleRad), std::to_string(angleDeg));
    
    // Test Dimensions3D
    Dimensions3D dims(2.0, 3.0, 4.0);
    double volume = dims.volume();
    LOG_INFOF("Dimensions volume: {}", std::to_string(volume));
    
    LOG_INFO("Geometry system test completed successfully!");
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Initialize logging
    Logger& logger = Logger::getInstance();
    logger.setLogLevel(LogLevel::Debug);
    logger.setConsoleLogging(true);
    
    LOG_INFO("Kitchen CAD Designer starting up...");
    
    // Test our geometry system
    testGeometrySystem();
    
    // Create a simple placeholder window
    QMainWindow window;
    window.setWindowTitle("Kitchen CAD Designer");
    window.resize(800, 600);
    
    // Create central widget with placeholder content
    QWidget* centralWidget = new QWidget(&window);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);
    
    QLabel* label = new QLabel("Kitchen CAD Designer", centralWidget);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("font-size: 24px; font-weight: bold; margin: 50px;");
    
    QLabel* subtitle = new QLabel("Geometry system initialized successfully!", centralWidget);
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet("font-size: 14px; color: #666; margin: 20px;");
    
    QLabel* details = new QLabel("Check console for geometry system test results", centralWidget);
    details->setAlignment(Qt::AlignCenter);
    details->setStyleSheet("font-size: 12px; color: #888; margin: 10px;");
    
    layout->addWidget(label);
    layout->addWidget(subtitle);
    layout->addWidget(details);
    
    window.setCentralWidget(centralWidget);
    window.show();
    
    LOG_INFO("Kitchen CAD Designer UI initialized");
    
    return app.exec();
}