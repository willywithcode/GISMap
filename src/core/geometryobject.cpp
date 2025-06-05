#include "geometryobject.h"

int GeometryObject::s_nextId = 1;

GeometryObject::GeometryObject(QObject* parent) 
    : QObject(parent), m_id(s_nextId++) {
}

void GeometryObject::setVisible(bool visible) {
    if (m_visible != visible) {
        m_visible = visible;
        emit visibilityChanged(visible);
        emit objectChanged();
    }
}

void GeometryObject::setSelected(bool selected) {
    if (m_selected != selected) {
        m_selected = selected;
        emit selectionChanged(selected);
        emit objectChanged();
    }
}
