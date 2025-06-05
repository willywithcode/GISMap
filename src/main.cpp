#include "ui/mainwindow.h"
#include "core/configmanager.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // Initialize configuration manager
    ConfigManager::instance().loadConfigs();
    
    MainWindow w;
    w.show();
    return a.exec();
}
