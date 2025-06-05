#pragma once
#include <QObject>
#include <QPainter>
#include <QMouseEvent>

class ViewTransform;

/**
 * @brief Abstract base class for map layers
 */
class MapLayer : public QObject {
    Q_OBJECT
public:
    explicit MapLayer(const QString& name, QObject* parent = nullptr);
    virtual ~MapLayer() = default;

    // Pure virtual methods
    virtual void render(QPainter& painter, const ViewTransform& transform) = 0;
    virtual bool handleMouseEvent(QMouseEvent* event, const ViewTransform& transform) = 0;

    // Layer properties
    QString name() const { return m_name; }
    void setName(const QString& name);
    
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible);
    
    double opacity() const { return m_opacity; }
    void setOpacity(double opacity);
    
    int zOrder() const { return m_zOrder; }
    void setZOrder(int order);

signals:
    void layerChanged();
    void visibilityChanged(bool visible);
    void opacityChanged(double opacity);

protected:
    QString m_name;
    bool m_visible = true;
    double m_opacity = 1.0;
    int m_zOrder = 0;
};
