#include "aircraft.h"
#include "../core/viewtransform.h"
#include "../core/configmanager.h"
#include <QPainter>
#include <QTransform>
#include <QtMath>
#include <QDebug>
#include <QUuid>
#include <QRandomGenerator>
#include <pqxx/pqxx>

Aircraft::Aircraft(const QPointF& position, QObject* parent)
    : GeometryObject(parent)
    , m_position(position)
    , m_velocity(0.0, 0.0)
    , m_heading(0.0)
    , m_altitude(10000.0)
    , m_speed(250.0)
    , m_state(Normal)
    , m_isMoving(false)
    , m_updateInterval(1000)
    , m_updateTimer(new QTimer(this))
    , m_trailEnabled(true)  // Enable trail by default
    , m_maxTrailPoints(50)  // Keep last 50 position points
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
{
    generateAircraftId();
    setCallSign(QString("AC%1").arg(QRandomGenerator::global()->bounded(1000, 9999)));
    
    connect(m_updateTimer, &QTimer::timeout, this, &Aircraft::updatePosition);
    m_updateTimer->setInterval(m_updateInterval);
}

Aircraft::Aircraft(QObject* parent)
    : Aircraft(QPointF(106.0, 20.5), parent)
{
    // Default constructor delegates to the position constructor
}

Aircraft::Aircraft(const QString& aircraftId, QObject* parent)
    : GeometryObject(parent)
    , m_aircraftId(aircraftId)
    , m_updateTimer(new QTimer(this))
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
{
    connect(m_updateTimer, &QTimer::timeout, this, &Aircraft::updatePosition);
    m_updateTimer->setInterval(m_updateInterval);
    
    // Try to load from database
    loadFromDatabase(aircraftId);
}

void Aircraft::render(QPainter& painter, const ViewTransform& transform)
{
    QPointF pixelPos = transform.geoToScreen(m_position);
    
    // Draw flight trail if enabled
    if (m_trailEnabled && m_flightTrail.size() > 1) {
        QPen trailPen(getStateColor(), 2);
        trailPen.setStyle(Qt::DashLine);
        painter.setPen(trailPen);
        
        // Draw trail with fading effect
        for (int i = 1; i < m_flightTrail.size(); ++i) {
            QPointF startPixel = transform.geoToScreen(m_flightTrail[i-1]);
            QPointF endPixel = transform.geoToScreen(m_flightTrail[i]);
            
            // Calculate fade alpha (newer points more opaque)
            double fadeRatio = static_cast<double>(i) / m_flightTrail.size();
            QColor trailColor = getStateColor();
            trailColor.setAlphaF(fadeRatio * 0.8); // Max 80% opacity
            
            QPen fadedPen(trailColor, 2);
            fadedPen.setStyle(Qt::DashLine);
            painter.setPen(fadedPen);
            
            painter.drawLine(startPixel, endPixel);
        }
    }
    
    // Create aircraft icon based on current state
    QColor color = getStateColor();
    bool highlighted = isSelected() || (m_state == Selected);
    QPixmap icon = createAircraftIcon(color, highlighted);
    
    // Apply rotation based on heading
    QTransform iconTransform;
    iconTransform.translate(pixelPos.x(), pixelPos.y());
    iconTransform.rotate(m_heading);
    iconTransform.translate(-AIRCRAFT_SIZE/2, -AIRCRAFT_SIZE/2);
    
    painter.save();
    painter.setTransform(iconTransform, true);
    painter.drawPixmap(0, 0, icon);
    painter.restore();
    
    // Draw aircraft info if selected
    if (highlighted) {
        painter.setPen(QPen(Qt::white, 1));
        painter.setFont(QFont("Arial", 10));
        QRectF textRect(pixelPos.x() + 15, pixelPos.y() - 15, 120, 60);
        painter.fillRect(textRect, QBrush(QColor(0, 0, 0, 180)));
        painter.setPen(Qt::white);
        
        QString infoText = QString("%1\nAlt: %2m\nSpd: %3 m/s")
            .arg(m_callSign)
            .arg(static_cast<int>(m_altitude))
            .arg(static_cast<int>(m_speed));
        
        if (m_trailEnabled) {
            infoText += QString("\nTrail: %1 pts").arg(m_flightTrail.size());
        }
        
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignTop, infoText);
    }
}

bool Aircraft::containsPoint(const QPointF& geoPoint)
{
    // Simple distance-based hit testing
    double distance = qSqrt(qPow(geoPoint.x() - m_position.x(), 2) + 
                           qPow(geoPoint.y() - m_position.y(), 2));
    
    // Convert pixel radius to geographic units (rough approximation)
    const double pixelToGeoRatio = 0.001; // This should be calculated properly
    return distance < (SELECTION_RADIUS * pixelToGeoRatio);
}

QRectF Aircraft::boundingBox() const
{
    const double margin = 0.01; // Geographic margin
    return QRectF(m_position.x() - margin, m_position.y() - margin,
                  2 * margin, 2 * margin);
}

QString Aircraft::getInfo() const
{
    return QString("Aircraft ID: %1\nCall Sign: %2\nType: %3\nPosition: %4, %5\nAltitude: %6m\nSpeed: %7 m/s\nHeading: %8°")
        .arg(m_aircraftId)
        .arg(m_callSign)
        .arg(m_aircraftType)
        .arg(m_position.x(), 0, 'f', 4)
        .arg(m_position.y(), 0, 'f', 4)
        .arg(static_cast<int>(m_altitude))
        .arg(static_cast<int>(m_speed))
        .arg(static_cast<int>(m_heading));
}

void Aircraft::setPosition(const QPointF& position)
{
    if (m_position != position) {
        m_position = position;
        updateTimestamp();
        emit positionChanged(position);
        
        // Auto-save to database if moving
        if (m_isMoving) {
            updateInDatabase();
        }
    }
}

void Aircraft::setVelocity(const QPointF& velocity)
{
    m_velocity = velocity;
    updateHeadingFromVelocity();
    updateTimestamp();
}

void Aircraft::setHeading(double heading)
{
    if (qAbs(m_heading - heading) > 0.1) {
        m_heading = heading;
        updateTimestamp();
        emit headingChanged(heading);
    }
}

void Aircraft::setAltitude(double altitude)
{
    if (qAbs(m_altitude - altitude) > 1.0) {
        m_altitude = altitude;
        updateTimestamp();
        emit altitudeChanged(altitude);
    }
}

void Aircraft::setSpeed(double speed)
{
    if (qAbs(m_speed - speed) > 0.1) {
        m_speed = speed;
        updateTimestamp();
        emit speedChanged(speed);
    }
}

void Aircraft::setState(State state)
{
    if (m_state != state) {
        m_state = state;
        updateTimestamp();
        emit stateChanged(state);
    }
}

void Aircraft::startMovement()
{
    if (!m_isMoving) {
        m_isMoving = true;
        m_updateTimer->start();
        qDebug() << "Aircraft" << m_callSign << "started movement";
    }
}

void Aircraft::stopMovement()
{
    if (m_isMoving) {
        m_isMoving = false;
        m_updateTimer->stop();
        updateInDatabase(); // Save final position
        qDebug() << "Aircraft" << m_callSign << "stopped movement";
    }
}

void Aircraft::setUpdateInterval(int milliseconds)
{
    m_updateInterval = milliseconds;
    m_updateTimer->setInterval(milliseconds);
}

void Aircraft::setSelected(bool selected)
{
    GeometryObject::setSelected(selected);
    if (selected) {
        setState(Selected);
    } else {
        setState(Normal); // Will be updated by position checking
    }
}

void Aircraft::saveToDatabase()
{
    try {
        ConfigManager& config = ConfigManager::instance();
        
        QString connString = QString("host=%1 port=%2 dbname=%3 user=%4 password=%5 connect_timeout=%6")
            .arg(config.getDatabaseHost())
            .arg(config.getDatabasePort())
            .arg(config.getDatabaseName())
            .arg(config.getDatabaseUsername())
            .arg(config.getDatabasePassword())
            .arg(config.getDatabaseConnectionTimeout());
        
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        // Insert or update aircraft (upsert)
        QString upsertQuery = R"(
            INSERT INTO aircraft 
            (aircraft_id, call_sign, aircraft_type, longitude, latitude, altitude, speed, heading, 
             velocity_x, velocity_y, state, flight_route_id, is_moving, created_at, updated_at)
            VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15)
            ON CONFLICT (aircraft_id)
            DO UPDATE SET
                call_sign = EXCLUDED.call_sign,
                aircraft_type = EXCLUDED.aircraft_type,
                longitude = EXCLUDED.longitude,
                latitude = EXCLUDED.latitude,
                altitude = EXCLUDED.altitude,
                speed = EXCLUDED.speed,
                heading = EXCLUDED.heading,
                velocity_x = EXCLUDED.velocity_x,
                velocity_y = EXCLUDED.velocity_y,
                state = EXCLUDED.state,
                flight_route_id = EXCLUDED.flight_route_id,
                is_moving = EXCLUDED.is_moving,
                updated_at = CURRENT_TIMESTAMP
        )";
        
        txn.exec_params(upsertQuery.toStdString(),
            m_aircraftId.toStdString(),
            m_callSign.toStdString(),
            m_aircraftType.toStdString(),
            m_position.x(),
            m_position.y(),
            m_altitude,
            m_speed,
            m_heading,
            m_velocity.x(),
            m_velocity.y(),
            static_cast<int>(m_state),
            m_flightRouteId.toStdString(),
            m_isMoving,
            m_createdAt.toString(Qt::ISODate).toStdString(),
            m_updatedAt.toString(Qt::ISODate).toStdString()
        );
        
        txn.commit();
        
        qDebug() << "Successfully saved aircraft to database:" << m_aircraftId;
        emit databaseOperationCompleted(true, "Aircraft saved successfully");
        
    } catch (const std::exception &e) {
        qDebug() << "Error saving aircraft to database:" << e.what();
        emit databaseOperationCompleted(false, QString("Error saving aircraft: %1").arg(e.what()));
    }
}

void Aircraft::loadFromDatabase(const QString& aircraftId)
{
    try {
        ConfigManager& config = ConfigManager::instance();
        
        QString connString = QString("host=%1 port=%2 dbname=%3 user=%4 password=%5 connect_timeout=%6")
            .arg(config.getDatabaseHost())
            .arg(config.getDatabasePort())
            .arg(config.getDatabaseName())
            .arg(config.getDatabaseUsername())
            .arg(config.getDatabasePassword())
            .arg(config.getDatabaseConnectionTimeout());
        
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        QString query = R"(
            SELECT call_sign, aircraft_type, longitude, latitude, altitude, speed, heading,
                   velocity_x, velocity_y, state, flight_route_id, is_moving, created_at, updated_at
            FROM aircraft 
            WHERE aircraft_id = $1
        )";
        
        pqxx::result result = txn.exec_params(query.toStdString(), aircraftId.toStdString());
        
        if (result.empty()) {
            qDebug() << "Aircraft not found in database:" << aircraftId;
            emit databaseOperationCompleted(false, "Aircraft not found");
            return;
        }
        
        auto row = result[0];
        m_aircraftId = aircraftId;
        m_callSign = QString::fromStdString(row["call_sign"].as<std::string>());
        m_aircraftType = QString::fromStdString(row["aircraft_type"].as<std::string>());
        m_position = QPointF(row["longitude"].as<double>(), row["latitude"].as<double>());
        m_altitude = row["altitude"].as<double>();
        m_speed = row["speed"].as<double>();
        m_heading = row["heading"].as<double>();
        m_velocity = QPointF(row["velocity_x"].as<double>(), row["velocity_y"].as<double>());
        m_state = static_cast<State>(row["state"].as<int>());
        m_flightRouteId = QString::fromStdString(row["flight_route_id"].as<std::string>());
        m_isMoving = row["is_moving"].as<bool>();
        m_createdAt = QDateTime::fromString(
            QString::fromStdString(row["created_at"].as<std::string>()), Qt::ISODate);
        m_updatedAt = QDateTime::fromString(
            QString::fromStdString(row["updated_at"].as<std::string>()), Qt::ISODate);
        
        qDebug() << "Successfully loaded aircraft from database:" << aircraftId;
        emit databaseOperationCompleted(true, "Aircraft loaded successfully");
        
    } catch (const std::exception &e) {
        qDebug() << "Error loading aircraft from database:" << e.what();
        emit databaseOperationCompleted(false, QString("Error loading aircraft: %1").arg(e.what()));
    }
}

void Aircraft::updateInDatabase()
{
    try {
        ConfigManager& config = ConfigManager::instance();
        
        QString connString = QString("host=%1 port=%2 dbname=%3 user=%4 password=%5 connect_timeout=%6")
            .arg(config.getDatabaseHost())
            .arg(config.getDatabasePort())
            .arg(config.getDatabaseName())
            .arg(config.getDatabaseUsername())
            .arg(config.getDatabasePassword())
            .arg(config.getDatabaseConnectionTimeout());
        
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        QString updateQuery = R"(
            UPDATE aircraft SET 
                call_sign = $2, aircraft_type = $3, longitude = $4, latitude = $5,
                altitude = $6, speed = $7, heading = $8, velocity_x = $9, velocity_y = $10,
                state = $11, flight_route_id = $12, is_moving = $13, updated_at = $14
            WHERE aircraft_id = $1
        )";
        
        txn.exec_params(updateQuery.toStdString(),
            m_aircraftId.toStdString(),
            m_callSign.toStdString(),
            m_aircraftType.toStdString(),
            m_position.x(),
            m_position.y(),
            m_altitude,
            m_speed,
            m_heading,
            m_velocity.x(),
            m_velocity.y(),
            static_cast<int>(m_state),
            m_flightRouteId.toStdString(),
            m_isMoving,
            QDateTime::currentDateTime().toString(Qt::ISODate).toStdString()
        );
        
        txn.commit();
        
    } catch (const std::exception &e) {
        qDebug() << "Error updating aircraft in database:" << e.what();
    }
}

void Aircraft::deleteFromDatabase()
{
    try {
        ConfigManager& config = ConfigManager::instance();
        
        QString connString = QString("host=%1 port=%2 dbname=%3 user=%4 password=%5 connect_timeout=%6")
            .arg(config.getDatabaseHost())
            .arg(config.getDatabasePort())
            .arg(config.getDatabaseName())
            .arg(config.getDatabaseUsername())
            .arg(config.getDatabasePassword())
            .arg(config.getDatabaseConnectionTimeout());
        
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        QString deleteQuery = "DELETE FROM aircraft WHERE aircraft_id = $1";
        txn.exec_params(deleteQuery.toStdString(), m_aircraftId.toStdString());
        
        txn.commit();
        
        qDebug() << "Successfully deleted aircraft from database:" << m_aircraftId;
        emit databaseOperationCompleted(true, "Aircraft deleted successfully");
        
    } catch (const std::exception &e) {
        qDebug() << "Error deleting aircraft from database:" << e.what();
        emit databaseOperationCompleted(false, QString("Error deleting aircraft: %1").arg(e.what()));
    }
}

QVector<Aircraft*> Aircraft::loadAllFromDatabase(QObject* parent)
{
    QVector<Aircraft*> aircraft;
    
    try {
        ConfigManager& config = ConfigManager::instance();
        
        QString connString = QString("host=%1 port=%2 dbname=%3 user=%4 password=%5 connect_timeout=%6")
            .arg(config.getDatabaseHost())
            .arg(config.getDatabasePort())
            .arg(config.getDatabaseName())
            .arg(config.getDatabaseUsername())
            .arg(config.getDatabasePassword())
            .arg(config.getDatabaseConnectionTimeout());
        
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        QString query = "SELECT aircraft_id FROM aircraft ORDER BY created_at";
        pqxx::result result = txn.exec(query.toStdString());
        
        for (const auto& row : result) {
            QString aircraftId = QString::fromStdString(row["aircraft_id"].as<std::string>());
            Aircraft* ac = new Aircraft(aircraftId, parent);
            aircraft.append(ac);
        }
        
        qDebug() << "Loaded" << aircraft.size() << "aircraft from database";
        
    } catch (const std::exception &e) {
        qDebug() << "Error loading aircraft from database:" << e.what();
    }
    
    return aircraft;
}

bool Aircraft::existsInDatabase(const QString& aircraftId)
{
    try {
        ConfigManager& config = ConfigManager::instance();
        
        QString connString = QString("host=%1 port=%2 dbname=%3 user=%4 password=%5 connect_timeout=%6")
            .arg(config.getDatabaseHost())
            .arg(config.getDatabasePort())
            .arg(config.getDatabaseName())
            .arg(config.getDatabaseUsername())
            .arg(config.getDatabasePassword())
            .arg(config.getDatabaseConnectionTimeout());
        
        pqxx::connection c(connString.toStdString());
        pqxx::work txn(c);
        
        QString query = "SELECT COUNT(*) FROM aircraft WHERE aircraft_id = $1";
        pqxx::result result = txn.exec_params(query.toStdString(), aircraftId.toStdString());
        
        return result[0][0].as<int>() > 0;
        
    } catch (const std::exception &e) {
        qDebug() << "Error checking aircraft existence:" << e.what();
        return false;
    }
}

void Aircraft::updatePosition()
{
    if (!m_isMoving) return;
    
    QPointF newPosition = m_position + m_velocity;
    
    // Add current position to trail before updating
    if (m_trailEnabled) {
        addTrailPoint(m_position);
    }
    
    setPosition(newPosition);
    
    qDebug() << "Aircraft" << m_aircraftId << "moved to" << newPosition;
}

void Aircraft::updateHeadingFromVelocity()
{
    if (m_velocity.x() != 0 || m_velocity.y() != 0) {
        // Calculate heading from velocity vector
        double heading = qRadiansToDegrees(qAtan2(m_velocity.x(), m_velocity.y()));
        if (heading < 0) heading += 360;
        setHeading(heading);
    }
}

QPixmap Aircraft::createAircraftIcon(const QColor& color, bool highlighted)
{
    QPixmap pixmap(static_cast<int>(AIRCRAFT_SIZE), static_cast<int>(AIRCRAFT_SIZE));
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Create aircraft shape (simple triangle)
    QPolygonF aircraftShape;
    aircraftShape << QPointF(AIRCRAFT_SIZE/2, 2)           // Nose
                 << QPointF(AIRCRAFT_SIZE/2 - 6, AIRCRAFT_SIZE - 2)  // Left wing
                 << QPointF(AIRCRAFT_SIZE/2, AIRCRAFT_SIZE - 6)       // Center back
                 << QPointF(AIRCRAFT_SIZE/2 + 6, AIRCRAFT_SIZE - 2);  // Right wing
    
    // Fill and outline
    painter.setBrush(QBrush(color));
    painter.setPen(QPen(highlighted ? Qt::yellow : Qt::black, highlighted ? 2 : 1));
    painter.drawPolygon(aircraftShape);
    
    return pixmap;
}

QColor Aircraft::getStateColor() const
{
    switch (m_state) {
        case Normal:
            return Qt::blue;
        case InRegion:
            return Qt::red;
        case Selected:
            return Qt::yellow;
        default:
            return Qt::blue;
    }
}

void Aircraft::generateAircraftId()
{
    m_aircraftId = QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void Aircraft::updateTimestamp()
{
    m_updatedAt = QDateTime::currentDateTime();
}

void Aircraft::addTrailPoint(const QPointF& position)
{
    m_flightTrail.append(position);
    
    // Limit trail length
    if (m_flightTrail.size() > m_maxTrailPoints) {
        m_flightTrail.removeFirst();
    }
}
