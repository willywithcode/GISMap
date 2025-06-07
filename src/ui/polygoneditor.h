#pragma once
#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPolygonF>
#include <QPointF>
#include "../services/databaseservice.h"

/**
 * @brief Dialog for editing polygon regions (green areas)
 */
class PolygonEditor : public QDialog
{
    Q_OBJECT

public:
    explicit PolygonEditor(QWidget *parent = nullptr);
    
signals:
    void polygonUpdated(); // Signal when polygon is modified

private slots:
    void onRegionSelectionChanged();
    void onAddRegion();
    void onDeleteRegion();
    void onSaveRegion();
    void onAddPoint();
    void onDeletePoint();
    void onPointChanged(int row, int column);
    void onLoadFromDatabase();

private:
    void setupUI();
    void setupConnections();
    void loadRegions();
    void loadRegionData(const DatabaseService::PolygonRegion& region);
    void clearRegionData();
    void updatePointsTable(const QPolygonF& polygon);
    QPolygonF getPolygonFromTable();
    
    // UI components
    QListWidget* m_regionsListWidget;
    QLineEdit* m_nameEdit;
    QTextEdit* m_descriptionEdit;
    QTableWidget* m_pointsTable;
    
    QPushButton* m_addRegionButton;
    QPushButton* m_deleteRegionButton;
    QPushButton* m_saveRegionButton;
    QPushButton* m_addPointButton;
    QPushButton* m_deletePointButton;
    QPushButton* m_loadButton;
    QPushButton* m_closeButton;
    
    // Data
    QVector<DatabaseService::PolygonRegion> m_regions;
    int m_currentRegionIndex = -1;
}; 