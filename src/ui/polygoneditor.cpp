#include "polygoneditor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QSplitter>
#include <QDoubleSpinBox>
#include <QDebug>
#include <QUuid>

PolygonEditor::PolygonEditor(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Polygon Region Editor");
    setModal(true);
    resize(800, 600);
    
    setupUI();
    setupConnections();
    loadRegions();
}

void PolygonEditor::setupUI()
{
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    
    // Create splitter
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    mainLayout->addWidget(splitter);
    
    // Left panel - Regions list
    QWidget* leftPanel = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    
    QGroupBox* regionsGroup = new QGroupBox("Regions");
    QVBoxLayout* regionsLayout = new QVBoxLayout(regionsGroup);
    
    m_regionsListWidget = new QListWidget();
    regionsLayout->addWidget(m_regionsListWidget);
    
    // Regions buttons
    QHBoxLayout* regionButtonsLayout = new QHBoxLayout();
    m_addRegionButton = new QPushButton("Add Region");
    m_deleteRegionButton = new QPushButton("Delete Region");
    m_loadButton = new QPushButton("Reload from DB");
    
    regionButtonsLayout->addWidget(m_addRegionButton);
    regionButtonsLayout->addWidget(m_deleteRegionButton);
    regionButtonsLayout->addWidget(m_loadButton);
    regionsLayout->addLayout(regionButtonsLayout);
    
    leftLayout->addWidget(regionsGroup);
    
    splitter->addWidget(leftPanel);
    
    // Right panel - Region details
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    
    // Region info group
    QGroupBox* infoGroup = new QGroupBox("Region Information");
    QFormLayout* infoLayout = new QFormLayout(infoGroup);
    
    m_nameEdit = new QLineEdit();
    m_nameEdit->setMaxLength(100);
    infoLayout->addRow("Name:", m_nameEdit);
    
    m_descriptionEdit = new QTextEdit();
    m_descriptionEdit->setMaximumHeight(80);
    infoLayout->addRow("Description:", m_descriptionEdit);
    
    rightLayout->addWidget(infoGroup);
    
    // Points group
    QGroupBox* pointsGroup = new QGroupBox("Polygon Points (Longitude, Latitude)");
    QVBoxLayout* pointsLayout = new QVBoxLayout(pointsGroup);
    
    m_pointsTable = new QTableWidget();
    m_pointsTable->setColumnCount(2);
    QStringList headers;
    headers << "Longitude" << "Latitude";
    m_pointsTable->setHorizontalHeaderLabels(headers);
    m_pointsTable->horizontalHeader()->setStretchLastSection(true);
    
    pointsLayout->addWidget(m_pointsTable);
    
    // Points buttons
    QHBoxLayout* pointButtonsLayout = new QHBoxLayout();
    m_addPointButton = new QPushButton("Add Point");
    m_deletePointButton = new QPushButton("Delete Point");
    
    pointButtonsLayout->addWidget(m_addPointButton);
    pointButtonsLayout->addWidget(m_deletePointButton);
    pointButtonsLayout->addStretch();
    pointsLayout->addLayout(pointButtonsLayout);
    
    rightLayout->addWidget(pointsGroup);
    
    // Save/Close buttons
    QHBoxLayout* bottomButtonsLayout = new QHBoxLayout();
    m_saveRegionButton = new QPushButton("Save Region");
    m_closeButton = new QPushButton("Close");
    
    bottomButtonsLayout->addStretch();
    bottomButtonsLayout->addWidget(m_saveRegionButton);
    bottomButtonsLayout->addWidget(m_closeButton);
    
    rightLayout->addLayout(bottomButtonsLayout);
    
    splitter->addWidget(rightPanel);
    
    // Set splitter proportions
    splitter->setSizes({300, 500});
    
    // Initially disable editing controls
    m_deleteRegionButton->setEnabled(false);
    m_saveRegionButton->setEnabled(false);
    m_addPointButton->setEnabled(false);
    m_deletePointButton->setEnabled(false);
    m_nameEdit->setEnabled(false);
    m_descriptionEdit->setEnabled(false);
    m_pointsTable->setEnabled(false);
}

void PolygonEditor::setupConnections()
{
    connect(m_regionsListWidget, &QListWidget::currentRowChanged, 
            this, &PolygonEditor::onRegionSelectionChanged);
    
    connect(m_addRegionButton, &QPushButton::clicked, 
            this, &PolygonEditor::onAddRegion);
    connect(m_deleteRegionButton, &QPushButton::clicked, 
            this, &PolygonEditor::onDeleteRegion);
    connect(m_saveRegionButton, &QPushButton::clicked, 
            this, &PolygonEditor::onSaveRegion);
    
    connect(m_addPointButton, &QPushButton::clicked, 
            this, &PolygonEditor::onAddPoint);
    connect(m_deletePointButton, &QPushButton::clicked, 
            this, &PolygonEditor::onDeletePoint);
    
    connect(m_pointsTable, &QTableWidget::cellChanged, 
            this, &PolygonEditor::onPointChanged);
    
    connect(m_loadButton, &QPushButton::clicked, 
            this, &PolygonEditor::onLoadFromDatabase);
    
    connect(m_closeButton, &QPushButton::clicked, 
            this, &QDialog::accept);
}

void PolygonEditor::loadRegions()
{
    DatabaseService& dbService = DatabaseService::instance();
    m_regions = dbService.loadAllRegions();
    
    m_regionsListWidget->clear();
    for (const auto& region : m_regions) {
        m_regionsListWidget->addItem(region.name);
    }
    
    qDebug() << "Loaded" << m_regions.size() << "regions from database";
}

void PolygonEditor::onRegionSelectionChanged()
{
    int currentRow = m_regionsListWidget->currentRow();
    
    if (currentRow >= 0 && currentRow < m_regions.size()) {
        m_currentRegionIndex = currentRow;
        loadRegionData(m_regions[currentRow]);
        
        // Enable editing controls
        m_deleteRegionButton->setEnabled(true);
        m_saveRegionButton->setEnabled(true);
        m_addPointButton->setEnabled(true);
        m_deletePointButton->setEnabled(true);
        m_nameEdit->setEnabled(true);
        m_descriptionEdit->setEnabled(true);
        m_pointsTable->setEnabled(true);
    } else {
        m_currentRegionIndex = -1;
        clearRegionData();
        
        // Disable editing controls
        m_deleteRegionButton->setEnabled(false);
        m_saveRegionButton->setEnabled(false);
        m_addPointButton->setEnabled(false);
        m_deletePointButton->setEnabled(false);
        m_nameEdit->setEnabled(false);
        m_descriptionEdit->setEnabled(false);
        m_pointsTable->setEnabled(false);
    }
}

void PolygonEditor::loadRegionData(const DatabaseService::PolygonRegion& region)
{
    m_nameEdit->setText(region.name);
    m_descriptionEdit->setText(region.description);
    updatePointsTable(region.polygon);
}

void PolygonEditor::clearRegionData()
{
    m_nameEdit->clear();
    m_descriptionEdit->clear();
    m_pointsTable->setRowCount(0);
}

void PolygonEditor::updatePointsTable(const QPolygonF& polygon)
{
    m_pointsTable->setRowCount(polygon.size());
    
    for (int i = 0; i < polygon.size(); ++i) {
        QPointF point = polygon[i];
        
        QTableWidgetItem* lonItem = new QTableWidgetItem(QString::number(point.x(), 'f', 6));
        QTableWidgetItem* latItem = new QTableWidgetItem(QString::number(point.y(), 'f', 6));
        
        m_pointsTable->setItem(i, 0, lonItem);
        m_pointsTable->setItem(i, 1, latItem);
    }
}

QPolygonF PolygonEditor::getPolygonFromTable()
{
    QPolygonF polygon;
    
    for (int i = 0; i < m_pointsTable->rowCount(); ++i) {
        QTableWidgetItem* lonItem = m_pointsTable->item(i, 0);
        QTableWidgetItem* latItem = m_pointsTable->item(i, 1);
        
        if (lonItem && latItem) {
            double lon = lonItem->text().toDouble();
            double lat = latItem->text().toDouble();
            polygon << QPointF(lon, lat);
        }
    }
    
    return polygon;
}

void PolygonEditor::onAddRegion()
{
    DatabaseService::PolygonRegion newRegion;
    newRegion.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    newRegion.name = "New Region";
    newRegion.description = "New polygon region";
    newRegion.createdAt = QDateTime::currentDateTime();
    newRegion.updatedAt = QDateTime::currentDateTime();
    
    // Create default rectangle around Hanoi
    newRegion.polygon << QPointF(105.8, 21.0)
                      << QPointF(105.8, 21.1)
                      << QPointF(105.9, 21.1)
                      << QPointF(105.9, 21.0)
                      << QPointF(105.8, 21.0); // Close polygon
    
    m_regions.append(newRegion);
    m_regionsListWidget->addItem(newRegion.name);
    
    // Select the new region
    m_regionsListWidget->setCurrentRow(m_regions.size() - 1);
    
    qDebug() << "Added new region:" << newRegion.name;
}

void PolygonEditor::onDeleteRegion()
{
    if (m_currentRegionIndex < 0 || m_currentRegionIndex >= m_regions.size()) {
        return;
    }
    
    DatabaseService::PolygonRegion& region = m_regions[m_currentRegionIndex];
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Delete Region",
        QString("Are you sure you want to delete region '%1'?").arg(region.name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // Delete from database
        DatabaseService& dbService = DatabaseService::instance();
        dbService.deleteRegion(region.id);
        
        // Remove from local list
        m_regions.removeAt(m_currentRegionIndex);
        delete m_regionsListWidget->takeItem(m_currentRegionIndex);
        
        qDebug() << "Deleted region:" << region.name;
        
        emit polygonUpdated();
    }
}

void PolygonEditor::onSaveRegion()
{
    if (m_currentRegionIndex < 0 || m_currentRegionIndex >= m_regions.size()) {
        return;
    }
    
    DatabaseService::PolygonRegion& region = m_regions[m_currentRegionIndex];
    
    // Update region data from UI
    region.name = m_nameEdit->text();
    region.description = m_descriptionEdit->toPlainText();
    region.polygon = getPolygonFromTable();
    region.updatedAt = QDateTime::currentDateTime();
    
    // Validate polygon
    if (region.polygon.size() < 3) {
        QMessageBox::warning(this, "Invalid Polygon", 
                           "A polygon must have at least 3 points.");
        return;
    }
    
    // Save to database
    DatabaseService& dbService = DatabaseService::instance();
    bool success = dbService.saveRegion(region);
    
    if (success) {
        // Update list widget
        m_regionsListWidget->item(m_currentRegionIndex)->setText(region.name);
        
        QMessageBox::information(this, "Save Successful", 
                                QString("Region '%1' saved successfully.").arg(region.name));
        
        emit polygonUpdated();
        
        qDebug() << "Saved region:" << region.name;
    } else {
        QMessageBox::warning(this, "Save Failed", 
                           "Failed to save region to database.");
    }
}

void PolygonEditor::onAddPoint()
{
    int row = m_pointsTable->rowCount();
    m_pointsTable->insertRow(row);
    
    // Add default point near Hanoi
    QTableWidgetItem* lonItem = new QTableWidgetItem("105.850");
    QTableWidgetItem* latItem = new QTableWidgetItem("21.030");
    
    m_pointsTable->setItem(row, 0, lonItem);
    m_pointsTable->setItem(row, 1, latItem);
    
    // Select the new row
    m_pointsTable->selectRow(row);
}

void PolygonEditor::onDeletePoint()
{
    int currentRow = m_pointsTable->currentRow();
    if (currentRow >= 0) {
        m_pointsTable->removeRow(currentRow);
    }
}

void PolygonEditor::onPointChanged(int row, int column)
{
    Q_UNUSED(row)
    Q_UNUSED(column)
    // Point data changed - could add real-time validation here
}

void PolygonEditor::onLoadFromDatabase()
{
    loadRegions();
    clearRegionData();
    m_currentRegionIndex = -1;
    
    QMessageBox::information(this, "Reload Complete", 
                           "Regions reloaded from database.");
} 