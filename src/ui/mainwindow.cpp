#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mapwidget.h"
#include "../models/aircraft.h"
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_menuBar(nullptr)
    , m_toolBar(nullptr)
    , m_tileServerGroup(nullptr)
    , m_openStreetMapAction(nullptr)
    , m_satelliteAction(nullptr)
{
    ui->setupUi(this);
    
    // Setup menu bar and toolbar first
    setupMenuBar();
    setupToolBar();
    
    // Create map widget
    m_mapWidget = new MapWidget(this);
    setCentralWidget(m_mapWidget);
    
    // Setup status bar
    statusBar()->show();
    m_coordsLabel = new QLabel("Coordinates: 105.85, 21.03", this);
    m_zoomLabel = new QLabel("Zoom: 12", this);
    m_aircraftLabel = new QLabel("No aircraft selected", this);
    m_cacheStatsLabel = new QLabel("Cache: 0 MB", this);
    
    statusBar()->addWidget(m_coordsLabel);
    statusBar()->addWidget(m_aircraftLabel);
    statusBar()->addWidget(m_cacheStatsLabel);
    statusBar()->addPermanentWidget(m_zoomLabel);
    
    // Setup cache statistics timer
    QTimer* cacheStatsTimer = new QTimer(this);
    connect(cacheStatsTimer, &QTimer::timeout, this, &MainWindow::updateCacheStats);
    cacheStatsTimer->start(5000); // Update every 5 seconds
    
    // Connect signals
    connect(m_mapWidget, &MapWidget::coordinatesChanged, this, &MainWindow::updateStatusBar);
    connect(m_mapWidget, &MapWidget::aircraftSelected, this, &MainWindow::onAircraftSelected);
    connect(m_mapWidget, &MapWidget::aircraftClicked, this, &MainWindow::onAircraftClicked);
    
    // Update tile server actions to reflect current state
    updateTileServerActions();
    
    setWindowTitle("GIS Map Application - Hanoi, Vietnam");
    resize(1000, 700);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateStatusBar(double lon, double lat, int zoom) {
    m_coordsLabel->setText(QString("Coordinates: %1, %2").arg(lon, 0, 'f', 4).arg(lat, 0, 'f', 4));
    m_zoomLabel->setText(QString("Zoom: %1").arg(zoom));
}

void MainWindow::onAircraftSelected(Aircraft* aircraft)
{
    if (aircraft) {
        QString info = QString("Selected: %1").arg(aircraft->getInfo());
        m_aircraftLabel->setText(info);
        statusBar()->showMessage("Aircraft selected - click elsewhere to deselect", 3000);
    }
}

void MainWindow::onAircraftClicked(Aircraft* aircraft, const QPointF& position)
{
    if (aircraft) {
        QString message = QString("Aircraft clicked at (%1, %2)")
                         .arg(position.x(), 0, 'f', 6)
                         .arg(position.y(), 0, 'f', 6);
        statusBar()->showMessage(message, 2000);
    }
}

void MainWindow::setupMenuBar()
{
    m_menuBar = menuBar();
    
    // Create Map menu
    QMenu* mapMenu = m_menuBar->addMenu("&Map");
    
    // Create tile server submenu
    QMenu* tileServerMenu = mapMenu->addMenu("&Tile Server");
    
    // Create action group for tile servers (mutual exclusion)
    m_tileServerGroup = new QActionGroup(this);
    
    // OpenStreetMap action
    m_openStreetMapAction = new QAction("&OpenStreetMap", this);
    m_openStreetMapAction->setCheckable(true);
    m_openStreetMapAction->setChecked(true); // Default selected
    m_openStreetMapAction->setData("openstreetmap");
    m_openStreetMapAction->setStatusTip("Switch to OpenStreetMap tiles");
    
    // Satellite action
    m_satelliteAction = new QAction("&Satellite", this);
    m_satelliteAction->setCheckable(true);
    m_satelliteAction->setData("satellite");
    m_satelliteAction->setStatusTip("Switch to satellite imagery tiles");
    
    // Add actions to group and menu
    m_tileServerGroup->addAction(m_openStreetMapAction);
    m_tileServerGroup->addAction(m_satelliteAction);
    
    tileServerMenu->addAction(m_openStreetMapAction);
    tileServerMenu->addAction(m_satelliteAction);
    
    // Connect action group signal
    connect(m_tileServerGroup, &QActionGroup::triggered, this, &MainWindow::onTileServerChanged);
    
    // Add separator and other map options
    mapMenu->addSeparator();
    
    // Add refresh action
    QAction* refreshAction = new QAction("&Refresh Map", this);
    refreshAction->setShortcut(QKeySequence::Refresh);
    refreshAction->setStatusTip("Refresh the map tiles");
    connect(refreshAction, &QAction::triggered, [this]() {
        m_mapWidget->refreshMap();
        statusBar()->showMessage("Map refreshed", 2000);
    });
    mapMenu->addAction(refreshAction);
    
    // Add clear cache action
    QAction* clearCacheAction = new QAction("&Clear Tile Cache", this);
    clearCacheAction->setStatusTip("Clear all cached map tiles");
    connect(clearCacheAction, &QAction::triggered, [this]() {
        m_mapWidget->clearTileCache();
        statusBar()->showMessage("Tile cache cleared", 2000);
    });
    mapMenu->addAction(clearCacheAction);
}

void MainWindow::setupToolBar()
{
    m_toolBar = addToolBar("Map Tools");
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    
    // Add tile server actions to toolbar
    m_toolBar->addAction(m_openStreetMapAction);
    m_toolBar->addAction(m_satelliteAction);
    
    m_toolBar->addSeparator();
    
    // Add refresh button to toolbar
    QAction* refreshAction = new QAction("Refresh", this);
    refreshAction->setStatusTip("Refresh the map tiles");
    connect(refreshAction, &QAction::triggered, [this]() {
        m_mapWidget->refreshMap();
        statusBar()->showMessage("Map refreshed", 2000);
    });
    m_toolBar->addAction(refreshAction);
}

void MainWindow::onTileServerChanged()
{
    QAction* action = m_tileServerGroup->checkedAction();
    if (action && m_mapWidget) {
        QString serverName = action->data().toString();
        qDebug() << "Switching to tile server:" << serverName;
        
        m_mapWidget->setTileServer(serverName);
        
        QString message = QString("Switched to %1 tiles").arg(action->text().remove('&'));
        statusBar()->showMessage(message, 3000);
    }
}

void MainWindow::updateTileServerActions()
{
    // This method can be used to sync UI state with current tile server
    // For now, we'll start with OpenStreetMap as default
    if (m_openStreetMapAction) {
        m_openStreetMapAction->setChecked(true);
    }
}

void MainWindow::updateCacheStats()
{
    // Update cache statistics label
    if (m_cacheStatsLabel && m_mapWidget) {
        qint64 cacheSize = m_mapWidget->getTileCacheSize();
        m_cacheStatsLabel->setText(QString("Cache: %1 MB").arg(cacheSize));
    }
}
