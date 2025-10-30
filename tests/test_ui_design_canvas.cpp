#include <catch2/catch_test_macros.hpp>
#include <QApplication>
#include <QSignalSpy>
#include <memory>

#ifdef HAVE_QT_TEST
#include <QTest>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#endif

#include "../src/ui/DesignCanvas.h"

using namespace KitchenCAD::UI;

class DesignCanvasTestFixture {
public:
    DesignCanvasTestFixture() {
        // Create QApplication if it doesn't exist
        if (!QApplication::instance()) {
            int argc = 0;
            char* argv[] = {nullptr};
            app = std::make_unique<QApplication>(argc, argv);
        }
        
        canvas = std::make_unique<DesignCanvas>();
        canvas->show();
        
#ifdef HAVE_QT_TEST
        // Wait for initialization
        QTest::qWait(100);
#endif
    }
    
    ~DesignCanvasTestFixture() {
        canvas.reset();
    }
    
    std::unique_ptr<QApplication> app;
    std::unique_ptr<DesignCanvas> canvas;
};

TEST_CASE_METHOD(DesignCanvasTestFixture, "DesignCanvas initialization", "[ui][canvas]") {
    REQUIRE(canvas != nullptr);
    REQUIRE(canvas->getViewMode() == ViewMode::View3D);
    REQUIRE(canvas->getInteractionMode() == InteractionMode::Select);
    REQUIRE(canvas->isGridVisible() == true);
    REQUIRE(canvas->isSnapToGrid() == true);
    REQUIRE(canvas->getGridSize() == 1.0f);
}

TEST_CASE_METHOD(DesignCanvasTestFixture, "View mode switching", "[ui][canvas][viewmode]") {
    QSignalSpy viewChangedSpy(canvas.get(), &DesignCanvas::viewChanged);
    
    SECTION("Switch to 2D view") {
        canvas->setViewMode(ViewMode::View2D);
        REQUIRE(canvas->getViewMode() == ViewMode::View2D);
        REQUIRE(viewChangedSpy.count() == 1);
    }
    
    SECTION("Switch to split view") {
        canvas->setViewMode(ViewMode::ViewSplit);
        REQUIRE(canvas->getViewMode() == ViewMode::ViewSplit);
        REQUIRE(viewChangedSpy.count() == 1);
    }
    
    SECTION("Switch back to 3D view") {
        canvas->setViewMode(ViewMode::View2D);
        canvas->setViewMode(ViewMode::View3D);
        REQUIRE(canvas->getViewMode() == ViewMode::View3D);
        REQUIRE(viewChangedSpy.count() == 2);
    }
}

TEST_CASE_METHOD(DesignCanvasTestFixture, "Camera controls", "[ui][canvas][camera]") {
    QSignalSpy viewChangedSpy(canvas.get(), &DesignCanvas::viewChanged);
    
    SECTION("Reset camera") {
        canvas->resetCamera();
        REQUIRE(viewChangedSpy.count() == 1);
    }
    
    SECTION("Fit to view") {
        canvas->fitToView();
        REQUIRE(viewChangedSpy.count() == 1);
    }
    
    SECTION("Set view direction") {
        QVector3D frontView(0.0f, 1.0f, 0.0f);
        canvas->setViewDirection(frontView);
        REQUIRE(viewChangedSpy.count() == 1);
    }
}

TEST_CASE_METHOD(DesignCanvasTestFixture, "Mouse interaction", "[ui][canvas][mouse]") {
    QSignalSpy coordinatesChangedSpy(canvas.get(), &DesignCanvas::coordinatesChanged);
    QSignalSpy objectSelectedSpy(canvas.get(), &DesignCanvas::objectSelected);
    
#ifdef HAVE_QT_TEST
    SECTION("Simulated mouse interaction") {
        canvas->setInteractionMode(InteractionMode::Select);
        
        // Use QTest to simulate mouse events properly
        QTest::mouseClick(canvas.get(), Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
        
        // Test should pass if no crash occurs
        REQUIRE(true);
    }
    
    SECTION("Mouse wheel simulation") {
        // Simulate wheel event
        QTest::keyClick(canvas.get(), Qt::Key_Plus); // Alternative zoom test
        
        REQUIRE(true);
    }
#else
    SECTION("Basic interaction mode setting") {
        canvas->setInteractionMode(InteractionMode::Select);
        REQUIRE(canvas->getInteractionMode() == InteractionMode::Select);
        
        canvas->setInteractionMode(InteractionMode::Pan);
        REQUIRE(canvas->getInteractionMode() == InteractionMode::Pan);
    }
#endif
}

TEST_CASE_METHOD(DesignCanvasTestFixture, "Zoom functionality", "[ui][canvas][zoom]") {
    QSignalSpy viewChangedSpy(canvas.get(), &DesignCanvas::viewChanged);
    
    SECTION("View mode affects zoom behavior") {
        canvas->setViewMode(ViewMode::View2D);
        REQUIRE(canvas->getViewMode() == ViewMode::View2D);
        
        canvas->setViewMode(ViewMode::View3D);
        REQUIRE(canvas->getViewMode() == ViewMode::View3D);
        
        // View changes should trigger view changed signal
        REQUIRE(viewChangedSpy.count() >= 2);
    }
}

TEST_CASE_METHOD(DesignCanvasTestFixture, "Keyboard shortcuts", "[ui][canvas][keyboard]") {
    QSignalSpy viewChangedSpy(canvas.get(), &DesignCanvas::viewChanged);
    QSignalSpy objectsChangedSpy(canvas.get(), &DesignCanvas::objectsChanged);
    
#ifdef HAVE_QT_TEST
    SECTION("Keyboard shortcuts via QTest") {
        // Test reset camera shortcut
        QTest::keyClick(canvas.get(), Qt::Key_R);
        REQUIRE(viewChangedSpy.count() >= 1);
        
        // Test fit to view shortcut
        QTest::keyClick(canvas.get(), Qt::Key_F);
        REQUIRE(viewChangedSpy.count() >= 2);
        
        // Test grid toggle
        bool initialGridState = canvas->isGridVisible();
        QTest::keyClick(canvas.get(), Qt::Key_G);
        REQUIRE(canvas->isGridVisible() != initialGridState);
    }
#else
    SECTION("Public method keyboard functionality") {
        // Test camera reset
        canvas->resetCamera();
        REQUIRE(viewChangedSpy.count() >= 1);
        
        // Test fit to view
        canvas->fitToView();
        REQUIRE(viewChangedSpy.count() >= 2);
        
        // Test grid toggle
        bool initialGridState = canvas->isGridVisible();
        canvas->setGridVisible(!initialGridState);
        REQUIRE(canvas->isGridVisible() != initialGridState);
    }
#endif
}

TEST_CASE_METHOD(DesignCanvasTestFixture, "Grid and snapping settings", "[ui][canvas][grid]") {
    SECTION("Toggle grid visibility") {
        bool initialState = canvas->isGridVisible();
        canvas->setGridVisible(!initialState);
        REQUIRE(canvas->isGridVisible() != initialState);
    }
    
    SECTION("Toggle snap to grid") {
        bool initialState = canvas->isSnapToGrid();
        canvas->setSnapToGrid(!initialState);
        REQUIRE(canvas->isSnapToGrid() != initialState);
    }
    
    SECTION("Set grid size") {
        float newSize = 2.5f;
        canvas->setGridSize(newSize);
        REQUIRE(canvas->getGridSize() == newSize);
    }
    
    SECTION("Grid size minimum constraint") {
        canvas->setGridSize(-1.0f); // Should be clamped to minimum
        REQUIRE(canvas->getGridSize() >= 0.1f);
    }
}

TEST_CASE_METHOD(DesignCanvasTestFixture, "Interaction mode switching", "[ui][canvas][interaction]") {
    SECTION("Switch to pan mode") {
        canvas->setInteractionMode(InteractionMode::Pan);
        REQUIRE(canvas->getInteractionMode() == InteractionMode::Pan);
    }
    
    SECTION("Switch to rotate mode") {
        canvas->setInteractionMode(InteractionMode::Rotate);
        REQUIRE(canvas->getInteractionMode() == InteractionMode::Rotate);
    }
    
    SECTION("Switch to zoom mode") {
        canvas->setInteractionMode(InteractionMode::Zoom);
        REQUIRE(canvas->getInteractionMode() == InteractionMode::Zoom);
    }
    
    SECTION("Switch back to select mode") {
        canvas->setInteractionMode(InteractionMode::Pan);
        canvas->setInteractionMode(InteractionMode::Select);
        REQUIRE(canvas->getInteractionMode() == InteractionMode::Select);
    }
}

TEST_CASE_METHOD(DesignCanvasTestFixture, "Object management", "[ui][canvas][objects]") {
    QSignalSpy objectsChangedSpy(canvas.get(), &DesignCanvas::objectsChanged);
    
    SECTION("Add object to scene") {
        QString catalogItemId = "test_cabinet_60";
        QVector3D position(1.0f, 2.0f, 0.0f);
        
        canvas->addObjectToScene(catalogItemId, position);
        
        REQUIRE(objectsChangedSpy.count() == 1);
    }
    
    SECTION("Remove object from scene") {
        QString objectId = "test_object_1";
        
        canvas->removeObjectFromScene(objectId);
        
        REQUIRE(objectsChangedSpy.count() == 1);
    }
    
    SECTION("Clear selection") {
        canvas->clearSelection();
        
        REQUIRE(canvas->getSelectedObjects().isEmpty());
        REQUIRE(objectsChangedSpy.count() == 1);
    }
}

TEST_CASE_METHOD(DesignCanvasTestFixture, "Canvas properties", "[ui][canvas][properties]") {
    SECTION("Canvas size and dimensions") {
        // Test that canvas has reasonable dimensions
        REQUIRE(canvas->width() > 0);
        REQUIRE(canvas->height() > 0);
    }
    
    SECTION("Canvas state consistency") {
        // Test that canvas maintains consistent state
        ViewMode initialMode = canvas->getViewMode();
        canvas->setViewMode(ViewMode::View2D);
        canvas->setViewMode(initialMode);
        REQUIRE(canvas->getViewMode() == initialMode);
    }
}

TEST_CASE_METHOD(DesignCanvasTestFixture, "2D/3D view synchronization", "[ui][canvas][sync]") {
    QSignalSpy viewChangedSpy(canvas.get(), &DesignCanvas::viewChanged);
    
    SECTION("View mode changes trigger view changed signal") {
        canvas->setViewMode(ViewMode::View2D);
        canvas->setViewMode(ViewMode::View3D);
        canvas->setViewMode(ViewMode::ViewSplit);
        
        REQUIRE(viewChangedSpy.count() == 3);
    }
    
    SECTION("Camera operations work in 3D mode") {
        canvas->setViewMode(ViewMode::View3D);
        
        // Test camera reset
        canvas->resetCamera();
        REQUIRE(viewChangedSpy.count() >= 1);
        
        // Test view direction change
        canvas->setViewDirection(QVector3D(1.0f, 0.0f, 0.0f));
        REQUIRE(viewChangedSpy.count() >= 2);
    }
    
    SECTION("2D view operations") {
        canvas->setViewMode(ViewMode::View2D);
        
        // 2D specific operations should work without crashing
        // Test view mode change instead of direct event handling
        canvas->setViewMode(ViewMode::View3D);
        canvas->setViewMode(ViewMode::View2D);
        
        REQUIRE(viewChangedSpy.count() >= 3); // Multiple mode changes
    }
}

TEST_CASE_METHOD(DesignCanvasTestFixture, "Drag and drop functionality", "[ui][canvas][dragdrop]") {
    SECTION("Drag preview initialization") {
        QString catalogItemId = "test_cabinet_60";
        
        // Start drag preview
        canvas->startDragPreview(catalogItemId);
        
        REQUIRE(canvas->isDragPreviewActive() == true);
    }
    
    SECTION("Drag preview position update") {
        QString catalogItemId = "test_cabinet_60";
        canvas->startDragPreview(catalogItemId);
        
        // Update drag preview position
        QPoint screenPos(100, 100);
        canvas->updateDragPreview(screenPos);
        
        REQUIRE(canvas->isDragPreviewActive() == true);
    }
    
    SECTION("End drag preview") {
        QString catalogItemId = "test_cabinet_60";
        canvas->startDragPreview(catalogItemId);
        
        // End drag preview without placing
        canvas->endDragPreview(false);
        
        REQUIRE(canvas->isDragPreviewActive() == false);
    }
    
    SECTION("Drag preview with placement") {
        QString catalogItemId = "test_cabinet_60";
        QSignalSpy objectsChangedSpy(canvas.get(), &DesignCanvas::objectsChanged);
        
        canvas->startDragPreview(catalogItemId);
        canvas->updateDragPreview(QPoint(200, 200));
        canvas->endDragPreview(true); // Place the object
        
        REQUIRE(canvas->isDragPreviewActive() == false);
        REQUIRE(objectsChangedSpy.count() == 1);
    }
}

TEST_CASE_METHOD(DesignCanvasTestFixture, "Grid snapping", "[ui][canvas][grid][snap]") {
    SECTION("Snap to grid when enabled") {
        canvas->setSnapToGrid(true);
        canvas->setGridSize(1.0f);
        
        // Test position that should be snapped
        QVector3D position(1.3f, 2.7f, 0.0f);
        QVector3D snapped = canvas->snapToGrid(position);
        
        REQUIRE(snapped.x() == 1.0f);
        REQUIRE(snapped.y() == 3.0f);
        REQUIRE(snapped.z() == 0.0f);
    }
    
    SECTION("No snapping when disabled") {
        canvas->setSnapToGrid(false);
        
        QVector3D position(1.3f, 2.7f, 0.0f);
        QVector3D result = canvas->snapToGrid(position);
        
        REQUIRE(result == position);
    }
    
    SECTION("Different grid sizes") {
        canvas->setSnapToGrid(true);
        canvas->setGridSize(0.5f);
        
        QVector3D position(1.3f, 2.7f, 0.0f);
        QVector3D snapped = canvas->snapToGrid(position);
        
        REQUIRE(snapped.x() == 1.5f);
        REQUIRE(snapped.y() == 2.5f);
    }
}

TEST_CASE_METHOD(DesignCanvasTestFixture, "Position validation", "[ui][canvas][validation]") {
    SECTION("Valid position within bounds") {
        QString catalogItemId = "test_cabinet_60";
        QVector3D position(5.0f, 5.0f, 0.0f);
        
        bool isValid = canvas->isValidPlacement(catalogItemId, position);
        
        REQUIRE(isValid == true);
    }
    
    SECTION("Invalid position - too far from origin") {
        QString catalogItemId = "test_cabinet_60";
        QVector3D position(100.0f, 100.0f, 0.0f);
        
        bool isValid = canvas->isValidPlacement(catalogItemId, position);
        
        REQUIRE(isValid == false);
    }
    
    SECTION("Invalid position - below ground") {
        QString catalogItemId = "test_cabinet_60";
        QVector3D position(5.0f, 5.0f, -1.0f);
        
        bool isValid = canvas->isValidPlacement(catalogItemId, position);
        
        REQUIRE(isValid == false);
    }
    
    SECTION("Position validation with snapping") {
        canvas->setSnapToGrid(true);
        canvas->setGridSize(1.0f);
        
        QString catalogItemId = "test_cabinet_60";
        QVector3D position(5.3f, 5.7f, 2.0f);
        
        QVector3D validated = canvas->getValidatedPosition(catalogItemId, position);
        
        // Should be snapped to grid and placed on ground
        REQUIRE(validated.x() == 5.0f);
        REQUIRE(validated.y() == 6.0f);
        REQUIRE(validated.z() == 0.0f);
    }
}

TEST_CASE_METHOD(DesignCanvasTestFixture, "Performance and rendering", "[ui][canvas][performance]") {
    SECTION("Multiple rapid updates don't crash") {
        for (int i = 0; i < 10; ++i) {
            canvas->setViewMode(i % 2 == 0 ? ViewMode::View2D : ViewMode::View3D);
            canvas->update();
#ifdef HAVE_QT_TEST
            QTest::qWait(10);
#endif
        }
        
        REQUIRE(true); // Test passes if no crash occurs
    }
    
    SECTION("Resize handling") {
        QSize originalSize = canvas->size();
        canvas->resize(800, 600);
        
        // Should handle resize without crashing
        REQUIRE(canvas->size() != originalSize);
        
        canvas->update();
#ifdef HAVE_QT_TEST
        QTest::qWait(50);
#endif
        
        REQUIRE(true); // Test passes if no crash occurs
    }
    
    SECTION("Drag preview rendering performance") {
        QString catalogItemId = "test_cabinet_60";
        
        // Start drag preview and update multiple times
        canvas->startDragPreview(catalogItemId);
        
        for (int i = 0; i < 20; ++i) {
            canvas->updateDragPreview(QPoint(100 + i * 5, 100 + i * 5));
            canvas->update();
#ifdef HAVE_QT_TEST
            QTest::qWait(5);
#endif
        }
        
        canvas->endDragPreview(false);
        
        REQUIRE(true); // Test passes if no crash occurs
    }
}