#pragma once
#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QVector>
#include <QPainter>

class ViewTransform;

/**
 * @brief Base class for all geometric objects in the GIS system
 */
class GeometryObject : public QObject {
    Q_OBJECT
public:
    explicit GeometryObject(QObject* parent = nullptr);
    virtual ~GeometryObject() = default;

    // Pure virtual methods that must be implemented by subclasses
    virtual void render(QPainter& painter, const ViewTransform& transform) = 0;
    virtual bool containsPoint(const QPointF& geoPoint) = 0;
    virtual QRectF boundingBox() const = 0;
    virtual QString getInfo() const = 0;

    // Common properties
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible);
    
    bool isSelected() const { return m_selected; }
    virtual void setSelected(bool selected);

    int getId() const { return m_id; }

signals:
    void visibilityChanged(bool visible);
    void selectionChanged(bool selected);
    void objectChanged();

protected:
    bool m_visible = true;
    bool m_selected = false;
    int m_id;
    
private:
    static int s_nextId;
};
