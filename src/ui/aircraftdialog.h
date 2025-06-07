#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPointF>

class Aircraft;

/**
 * @brief Dialog for adding new aircraft or editing existing aircraft
 */
class AircraftDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AircraftDialog(QWidget *parent = nullptr);
    explicit AircraftDialog(Aircraft* aircraft, QWidget *parent = nullptr); // Edit mode
    
    // Get values from dialog
    QString getCallSign() const;
    QString getAircraftType() const;
    QPointF getPosition() const;
    QPointF getVelocity() const;
    double getHeading() const;
    double getAltitude() const;
    double getSpeed() const;
    bool isMovingEnabled() const;
    
    // Set values for editing
    void setCallSign(const QString& callSign);
    void setAircraftType(const QString& type);
    void setPosition(const QPointF& position);
    void setVelocity(const QPointF& velocity);
    void setHeading(double heading);
    void setAltitude(double altitude);
    void setSpeed(double speed);
    void setMovingEnabled(bool enabled);

private slots:
    void onOkClicked();
    void onCancelClicked();
    void updateHeadingFromVelocity();

private:
    void setupUI();
    void setupConnections();
    void loadAircraftData(Aircraft* aircraft);
    
    // UI components
    QLineEdit* m_callSignEdit;
    QLineEdit* m_aircraftTypeEdit;
    QDoubleSpinBox* m_longitudeSpinBox;
    QDoubleSpinBox* m_latitudeSpinBox;
    QDoubleSpinBox* m_velocityXSpinBox;
    QDoubleSpinBox* m_velocityYSpinBox;
    QDoubleSpinBox* m_headingSpinBox;
    QDoubleSpinBox* m_altitudeSpinBox;
    QDoubleSpinBox* m_speedSpinBox;
    QCheckBox* m_movingCheckBox;
    
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    
    // Mode
    bool m_editMode;
    Aircraft* m_aircraft; // For edit mode
}; 