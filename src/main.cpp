#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
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
    
    QLabel* subtitle = new QLabel("Project structure initialized successfully!", centralWidget);
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet("font-size: 14px; color: #666; margin: 20px;");
    
    layout->addWidget(label);
    layout->addWidget(subtitle);
    
    window.setCentralWidget(centralWidget);
    window.show();
    
    return app.exec();
}