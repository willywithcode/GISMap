#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mapwidget.h"
#include "aircraftdialog.h"
#include "polygoneditor.h"
#include "../models/aircraft.h"
#include <QTimer>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_menuBar(nullptr)
    // , m_toolBar(nullptr)  // Removed - only keep menu bar
    , m_tileServerGroup(nullptr)
    , m_openStreetMapAction(nullptr)
    , m_satelliteAction(nullptr)
{
    ui->setupUi(this);
    
    // Setup menu bar and toolbar first
    setupMenuBar();
    // setupToolBar();  // Commented out - only keep menu bar with dropdowns
    
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
        // Connect to position updates for real-time coordinate display
        connect(aircraft, &Aircraft::positionChanged, this, [this, aircraft](const QPointF& position) {
            if (aircraft == m_mapWidget->aircraftLayer()->selectedAircraft()) {
                QString info = QString("Selected Aircraft - Lon: %1, Lat: %2, Heading: %3°, State: %4")
                              .arg(position.x(), 0, 'f', 6)
                              .arg(position.y(), 0, 'f', 6)
                              .arg(aircraft->heading(), 0, 'f', 1)
                              .arg(aircraft->state() == Aircraft::Normal ? "Normal" : 
                                   aircraft->state() == Aircraft::InRegion ? "In Region" : "Selected");
                m_aircraftLabel->setText(info);
            }
        });
        
        // Initial display
        QString info = QString("Selected Aircraft - Lon: %1, Lat: %2, Heading: %3°, State: %4")
                      .arg(aircraft->position().x(), 0, 'f', 6)
                      .arg(aircraft->position().y(), 0, 'f', 6)
                      .arg(aircraft->heading(), 0, 'f', 1)
                      .arg(aircraft->state() == Aircraft::Normal ? "Normal" : 
                           aircraft->state() == Aircraft::InRegion ? "In Region" : "Selected");
        m_aircraftLabel->setText(info);
        statusBar()->showMessage("Aircraft selected - coordinates updating in real-time", 3000);
    } else {
        // Aircraft deselected
        m_aircraftLabel->setText("No aircraft selected");
        statusBar()->showMessage("Aircraft deselected", 2000);
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
    
    // Create Aircraft menu
    QMenu* aircraftMenu = m_menuBar->addMenu("&Aircraft");
    
    // Add aircraft action
    m_addAircraftAction = new QAction("&Add Aircraft", this);
    m_addAircraftAction->setShortcut(QKeySequence("Ctrl+A"));
    m_addAircraftAction->setStatusTip("Add a new aircraft to the map");
    connect(m_addAircraftAction, &QAction::triggered, this, &MainWindow::onAddAircraft);
    aircraftMenu->addAction(m_addAircraftAction);
    
    // Edit aircraft action
    m_editAircraftAction = new QAction("&Edit Aircraft", this);
    m_editAircraftAction->setShortcut(QKeySequence("Ctrl+E"));
    m_editAircraftAction->setStatusTip("Edit the selected aircraft");
    connect(m_editAircraftAction, &QAction::triggered, this, &MainWindow::onEditAircraft);
    aircraftMenu->addAction(m_editAircraftAction);
    
    // Delete aircraft action
    m_deleteAircraftAction = new QAction("&Delete Aircraft", this);
    m_deleteAircraftAction->setShortcut(QKeySequence::Delete);
    m_deleteAircraftAction->setStatusTip("Delete the selected aircraft");
    connect(m_deleteAircraftAction, &QAction::triggered, this, &MainWindow::onDeleteAircraft);
    aircraftMenu->addAction(m_deleteAircraftAction);
    
    // Create Polygon menu
    QMenu* polygonMenu = m_menuBar->addMenu("&Polygons");
    
    // Edit polygons action
    m_editPolygonsAction = new QAction("&Edit Regions", this);
    m_editPolygonsAction->setShortcut(QKeySequence("Ctrl+P"));
    m_editPolygonsAction->setStatusTip("Edit polygon regions (green areas)");
    connect(m_editPolygonsAction, &QAction::triggered, this, &MainWindow::onEditPolygons);
    polygonMenu->addAction(m_editPolygonsAction);
    
    // Create View menu
    QMenu* viewMenu = m_menuBar->addMenu("&View");
    
    // Toggle trails action
    m_toggleTrailsAction = new QAction("&Show Flight Trails", this);
    m_toggleTrailsAction->setCheckable(true);
    m_toggleTrailsAction->setChecked(true);
    m_toggleTrailsAction->setShortcut(QKeySequence("Ctrl+T"));
    m_toggleTrailsAction->setStatusTip("Toggle aircraft flight trail display");
    connect(m_toggleTrailsAction, &QAction::triggered, this, &MainWindow::onToggleTrails);
    viewMenu->addAction(m_toggleTrailsAction);
    
    // Clear trails action
    m_clearTrailsAction = new QAction("&Clear All Trails", this);
    m_clearTrailsAction->setShortcut(QKeySequence("Ctrl+Shift+T"));
    m_clearTrailsAction->setStatusTip("Clear all aircraft flight trails");
    connect(m_clearTrailsAction, &QAction::triggered, this, &MainWindow::onClearTrails);
    viewMenu->addAction(m_clearTrailsAction);
}

/*
// Toolbar removed - only keep menu bar with dropdowns
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
    
    m_toolBar->addSeparator();
    
    // Add aircraft management actions to toolbar
    m_toolBar->addAction(m_addAircraftAction);
    m_toolBar->addAction(m_editAircraftAction);
    m_toolBar->addAction(m_deleteAircraftAction);
    
    m_toolBar->addSeparator();
    
    // Add polygon and trail actions to toolbar
    m_toolBar->addAction(m_editPolygonsAction);
    m_toolBar->addAction(m_toggleTrailsAction);
    m_toolBar->addAction(m_clearTrailsAction);
}
*/

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

// Aircraft management implementations
void MainWindow::onAddAircraft()
{
    AircraftDialog dialog(this);
    
    if (dialog.exec() == QDialog::Accepted) {
        // Create new aircraft with dialog values
        if (m_mapWidget && m_mapWidget->aircraftManager()) {
            Aircraft* aircraft = m_mapWidget->aircraftManager()->createAircraft(dialog.getPosition());
            
            if (aircraft) {
                // Set aircraft properties from dialog
                aircraft->setCallSign(dialog.getCallSign());
                aircraft->setAircraftType(dialog.getAircraftType());
                aircraft->setVelocity(dialog.getVelocity());
                aircraft->setHeading(dialog.getHeading());
                aircraft->setAltitude(dialog.getAltitude());
                aircraft->setSpeed(dialog.getSpeed());
                
                // Start or stop movement based on dialog setting
                if (dialog.isMovingEnabled()) {
                    aircraft->startMovement();
                } else {
                    aircraft->stopMovement();
                }
                
                // Save to database
                aircraft->saveToDatabase();
                
                statusBar()->showMessage("New aircraft added: " + aircraft->getCallSign(), 3000);
                qDebug() << "Added new aircraft:" << aircraft->getCallSign() << "at" << aircraft->position();
            }
        }
    }
}

void MainWindow::onEditAircraft()
{
    if (!m_mapWidget || !m_mapWidget->aircraftLayer()) {
        statusBar()->showMessage("No aircraft management available", 3000);
        return;
    }
    
    Aircraft* selectedAircraft = m_mapWidget->aircraftLayer()->selectedAircraft();
    if (!selectedAircraft) {
        statusBar()->showMessage("Please select an aircraft first", 3000);
        return;
    }
    
    AircraftDialog dialog(selectedAircraft, this);
    
    if (dialog.exec() == QDialog::Accepted) {
        // Update aircraft with dialog values
        selectedAircraft->setCallSign(dialog.getCallSign());
        selectedAircraft->setAircraftType(dialog.getAircraftType());
        selectedAircraft->setPosition(dialog.getPosition());
        selectedAircraft->setVelocity(dialog.getVelocity());
        selectedAircraft->setHeading(dialog.getHeading());
        selectedAircraft->setAltitude(dialog.getAltitude());
        selectedAircraft->setSpeed(dialog.getSpeed());
        
        // Update movement state
        if (dialog.isMovingEnabled()) {
            selectedAircraft->startMovement();
        } else {
            selectedAircraft->stopMovement();
        }
        
        // Update in database
        selectedAircraft->updateInDatabase();
        
        statusBar()->showMessage("Aircraft updated: " + selectedAircraft->getCallSign(), 3000);
        qDebug() << "Updated aircraft:" << selectedAircraft->getCallSign();
    }
}

void MainWindow::onDeleteAircraft()
{
    if (!m_mapWidget || !m_mapWidget->aircraftManager() || !m_mapWidget->aircraftLayer()) {
        statusBar()->showMessage("No aircraft management available", 3000);
        return;
    }
    
    Aircraft* selectedAircraft = m_mapWidget->aircraftLayer()->selectedAircraft();
    if (!selectedAircraft) {
        statusBar()->showMessage("Please select an aircraft first", 3000);
        return;
    }
    
    QString callSign = selectedAircraft->getCallSign();
    
    // Confirm deletion
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        "Delete Aircraft", 
        QString("Are you sure you want to delete aircraft '%1'?").arg(callSign),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // Delete from database first
        selectedAircraft->deleteFromDatabase();
        
        // Remove from manager (this will also remove from layer)
        m_mapWidget->aircraftManager()->removeAircraft(selectedAircraft);
        
        statusBar()->showMessage("Aircraft deleted: " + callSign, 3000);
        qDebug() << "Deleted aircraft:" << callSign;
    }
}

// Polygon management implementations
void MainWindow::onEditPolygons()
{
    PolygonEditor editor(this);
    
    // Connect signal to refresh polygons on map
    connect(&editor, &PolygonEditor::polygonUpdated, [this]() {
        if (m_mapWidget) {
            // Refresh polygons from database
            m_mapWidget->refreshPolygons();
        }
    });
    
    editor.exec();
}

// Trail management implementations
void MainWindow::onToggleTrails()
{
    if (!m_mapWidget || !m_mapWidget->aircraftManager()) {
        return;
    }
    
    bool showTrails = m_toggleTrailsAction->isChecked();
    
    // Toggle trails for all aircraft
    QVector<Aircraft*> allAircraft = m_mapWidget->aircraftManager()->allAircraft();
    for (Aircraft* aircraft : allAircraft) {
        if (aircraft) {
            aircraft->setTrailEnabled(showTrails);
        }
    }
    
    statusBar()->showMessage(
        showTrails ? "Flight trails enabled" : "Flight trails disabled", 
        2000
    );
    
    qDebug() << "Flight trails" << (showTrails ? "enabled" : "disabled");
}

void MainWindow::onClearTrails()
{
    if (!m_mapWidget || !m_mapWidget->aircraftManager()) {
        return;
    }
    
    // Clear trails for all aircraft
    QVector<Aircraft*> allAircraft = m_mapWidget->aircraftManager()->allAircraft();
    int clearedCount = 0;
    
    for (Aircraft* aircraft : allAircraft) {
        if (aircraft) {
            aircraft->clearTrail();
            clearedCount++;
        }
    }
    
    statusBar()->showMessage(
        QString("Cleared trails for %1 aircraft").arg(clearedCount), 
        2000
    );
    
    qDebug() << "Cleared trails for" << clearedCount << "aircraft";
}
