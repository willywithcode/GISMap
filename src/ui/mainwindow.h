#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStatusBar>
#include <QLabel>
#include <QPointF>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QActionGroup>
#include <QDebug>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MapWidget;
class Aircraft;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateStatusBar(double lon, double lat, int zoom);
    void onAircraftSelected(Aircraft* aircraft);
    void onAircraftClicked(Aircraft* aircraft, const QPointF& position);
    void onTileServerChanged();
    void updateCacheStats(); // New slot for updating cache statistics

private:
    void setupMenuBar();
    void setupToolBar();
    void updateTileServerActions();
    
    Ui::MainWindow *ui;
    QLabel *m_coordsLabel;
    QLabel *m_zoomLabel;
    QLabel *m_aircraftLabel;
    QLabel *m_cacheStatsLabel;  // New cache statistics label
    MapWidget *m_mapWidget;
    
    // Menu and toolbar components
    QMenuBar *m_menuBar;
    QToolBar *m_toolBar;
    QActionGroup *m_tileServerGroup;
    QAction *m_openStreetMapAction;
    QAction *m_satelliteAction;
};

#endif // MAINWINDOW_H
