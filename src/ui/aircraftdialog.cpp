#include "aircraftdialog.h"
#include "../models/aircraft.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QtMath>
#include <QRandomGenerator>

AircraftDialog::AircraftDialog(QWidget *parent)
    : QDialog(parent)
    , m_editMode(false)
    , m_aircraft(nullptr)
{
    setWindowTitle("Add New Aircraft");
    setupUI();
    setupConnections();
    
    // Set default values
    setCallSign(QString("AC%1").arg(QRandomGenerator::global()->bounded(1000, 9999)));
    setAircraftType("Commercial");
    setPosition(QPointF(105.85, 21.03)); // Default Hanoi position
    setVelocity(QPointF(0.0005, 0.0003)); // Default velocity
    setAltitude(10000.0);
    setSpeed(250.0);
    setMovingEnabled(true);
}

AircraftDialog::AircraftDialog(Aircraft* aircraft, QWidget *parent)
    : QDialog(parent)
    , m_editMode(true)
    , m_aircraft(aircraft)
{
    setWindowTitle("Edit Aircraft");
    setupUI();
    setupConnections();
    
    if (aircraft) {
        loadAircraftData(aircraft);
    }
}

void AircraftDialog::setupUI()
{
    setModal(true);
    setFixedSize(400, 500);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Aircraft Information Group
    QGroupBox* infoGroup = new QGroupBox("Aircraft Information");
    QFormLayout* infoLayout = new QFormLayout(infoGroup);
    
    m_callSignEdit = new QLineEdit();
    m_callSignEdit->setMaxLength(20);
    infoLayout->addRow("Call Sign:", m_callSignEdit);
    
    m_aircraftTypeEdit = new QLineEdit();
    m_aircraftTypeEdit->setMaxLength(50);
    infoLayout->addRow("Aircraft Type:", m_aircraftTypeEdit);
    
    mainLayout->addWidget(infoGroup);
    
    // Position Group
    QGroupBox* positionGroup = new QGroupBox("Position");
    QFormLayout* positionLayout = new QFormLayout(positionGroup);
    
    m_longitudeSpinBox = new QDoubleSpinBox();
    m_longitudeSpinBox->setRange(104.0, 108.0);
    m_longitudeSpinBox->setDecimals(6);
    m_longitudeSpinBox->setSingleStep(0.001);
    positionLayout->addRow("Longitude:", m_longitudeSpinBox);
    
    m_latitudeSpinBox = new QDoubleSpinBox();
    m_latitudeSpinBox->setRange(20.0, 22.0);
    m_latitudeSpinBox->setDecimals(6);
    m_latitudeSpinBox->setSingleStep(0.001);
    positionLayout->addRow("Latitude:", m_latitudeSpinBox);
    
    mainLayout->addWidget(positionGroup);
    
    // Movement Group
    QGroupBox* movementGroup = new QGroupBox("Movement");
    QFormLayout* movementLayout = new QFormLayout(movementGroup);
    
    m_velocityXSpinBox = new QDoubleSpinBox();
    m_velocityXSpinBox->setRange(-0.01, 0.01);
    m_velocityXSpinBox->setDecimals(6);
    m_velocityXSpinBox->setSingleStep(0.0001);
    movementLayout->addRow("Velocity X (deg/s):", m_velocityXSpinBox);
    
    m_velocityYSpinBox = new QDoubleSpinBox();
    m_velocityYSpinBox->setRange(-0.01, 0.01);
    m_velocityYSpinBox->setDecimals(6);
    m_velocityYSpinBox->setSingleStep(0.0001);
    movementLayout->addRow("Velocity Y (deg/s):", m_velocityYSpinBox);
    
    m_headingSpinBox = new QDoubleSpinBox();
    m_headingSpinBox->setRange(0.0, 360.0);
    m_headingSpinBox->setDecimals(1);
    m_headingSpinBox->setSingleStep(1.0);
    m_headingSpinBox->setSuffix("°");
    movementLayout->addRow("Heading:", m_headingSpinBox);
    
    m_altitudeSpinBox = new QDoubleSpinBox();
    m_altitudeSpinBox->setRange(0.0, 50000.0);
    m_altitudeSpinBox->setDecimals(0);
    m_altitudeSpinBox->setSingleStep(100.0);
    m_altitudeSpinBox->setSuffix(" m");
    movementLayout->addRow("Altitude:", m_altitudeSpinBox);
    
    m_speedSpinBox = new QDoubleSpinBox();
    m_speedSpinBox->setRange(0.0, 1000.0);
    m_speedSpinBox->setDecimals(1);
    m_speedSpinBox->setSingleStep(10.0);
    m_speedSpinBox->setSuffix(" m/s");
    movementLayout->addRow("Speed:", m_speedSpinBox);
    
    m_movingCheckBox = new QCheckBox("Enable Movement");
    movementLayout->addRow("Status:", m_movingCheckBox);
    
    mainLayout->addWidget(movementGroup);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_okButton = new QPushButton("OK");
    m_cancelButton = new QPushButton("Cancel");
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);
}

void AircraftDialog::setupConnections()
{
    connect(m_okButton, &QPushButton::clicked, this, &AircraftDialog::onOkClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &AircraftDialog::onCancelClicked);
    
    // Auto-update heading when velocity changes
    connect(m_velocityXSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &AircraftDialog::updateHeadingFromVelocity);
    connect(m_velocityYSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &AircraftDialog::updateHeadingFromVelocity);
}

void AircraftDialog::loadAircraftData(Aircraft* aircraft)
{
    if (!aircraft) return;
    
    setCallSign(aircraft->getCallSign());
    setAircraftType(aircraft->getAircraftType());
    setPosition(aircraft->position());
    setVelocity(aircraft->velocity());
    setHeading(aircraft->heading());
    setAltitude(aircraft->altitude());
    setSpeed(aircraft->speed());
    setMovingEnabled(aircraft->isMoving());
}

void AircraftDialog::onOkClicked()
{
    // Validate input
    if (m_callSignEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Call Sign cannot be empty.");
        return;
    }
    
    if (m_aircraftTypeEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", "Aircraft Type cannot be empty.");
        return;
    }
    
    accept();
}

void AircraftDialog::onCancelClicked()
{
    reject();
}

void AircraftDialog::updateHeadingFromVelocity()
{
    double vx = m_velocityXSpinBox->value();
    double vy = m_velocityYSpinBox->value();
    
    if (vx != 0.0 || vy != 0.0) {
        double heading = qRadiansToDegrees(qAtan2(vx, vy));
        if (heading < 0) heading += 360.0;
        
        // Block signals to prevent recursion
        m_headingSpinBox->blockSignals(true);
        m_headingSpinBox->setValue(heading);
        m_headingSpinBox->blockSignals(false);
    }
}

// Getters
QString AircraftDialog::getCallSign() const
{
    return m_callSignEdit->text().trimmed();
}

QString AircraftDialog::getAircraftType() const
{
    return m_aircraftTypeEdit->text().trimmed();
}

QPointF AircraftDialog::getPosition() const
{
    return QPointF(m_longitudeSpinBox->value(), m_latitudeSpinBox->value());
}

QPointF AircraftDialog::getVelocity() const
{
    return QPointF(m_velocityXSpinBox->value(), m_velocityYSpinBox->value());
}

double AircraftDialog::getHeading() const
{
    return m_headingSpinBox->value();
}

double AircraftDialog::getAltitude() const
{
    return m_altitudeSpinBox->value();
}

double AircraftDialog::getSpeed() const
{
    return m_speedSpinBox->value();
}

bool AircraftDialog::isMovingEnabled() const
{
    return m_movingCheckBox->isChecked();
}

// Setters
void AircraftDialog::setCallSign(const QString& callSign)
{
    m_callSignEdit->setText(callSign);
}

void AircraftDialog::setAircraftType(const QString& type)
{
    m_aircraftTypeEdit->setText(type);
}

void AircraftDialog::setPosition(const QPointF& position)
{
    m_longitudeSpinBox->setValue(position.x());
    m_latitudeSpinBox->setValue(position.y());
}

void AircraftDialog::setVelocity(const QPointF& velocity)
{
    m_velocityXSpinBox->setValue(velocity.x());
    m_velocityYSpinBox->setValue(velocity.y());
    updateHeadingFromVelocity();
}

void AircraftDialog::setHeading(double heading)
{
    m_headingSpinBox->setValue(heading);
}

void AircraftDialog::setAltitude(double altitude)
{
    m_altitudeSpinBox->setValue(altitude);
}

void AircraftDialog::setSpeed(double speed)
{
    m_speedSpinBox->setValue(speed);
}

void AircraftDialog::setMovingEnabled(bool enabled)
{
    m_movingCheckBox->setChecked(enabled);
} 