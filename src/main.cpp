#include <QApplication>
#include <QMainWindow>
#include "MapWidget.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("GIS Map Application");
    mainWindow.resize(1024, 768);
    
    MapWidget *mapWidget = new MapWidget(&mainWindow);
    mainWindow.setCentralWidget(mapWidget);
    
    mainWindow.show();
    
    return app.exec();
} 