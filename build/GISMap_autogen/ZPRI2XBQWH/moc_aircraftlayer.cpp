/****************************************************************************
** Meta object code from reading C++ file 'aircraftlayer.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/layers/aircraftlayer.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'aircraftlayer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_AircraftLayer_t {
    QByteArrayData data[10];
    char stringdata0[133];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_AircraftLayer_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_AircraftLayer_t qt_meta_stringdata_AircraftLayer = {
    {
QT_MOC_LITERAL(0, 0, 13), // "AircraftLayer"
QT_MOC_LITERAL(1, 14, 16), // "aircraftSelected"
QT_MOC_LITERAL(2, 31, 0), // ""
QT_MOC_LITERAL(3, 32, 9), // "Aircraft*"
QT_MOC_LITERAL(4, 42, 8), // "aircraft"
QT_MOC_LITERAL(5, 51, 18), // "aircraftDeselected"
QT_MOC_LITERAL(6, 70, 15), // "aircraftClicked"
QT_MOC_LITERAL(7, 86, 8), // "position"
QT_MOC_LITERAL(8, 95, 25), // "onAircraftPositionChanged"
QT_MOC_LITERAL(9, 121, 11) // "newPosition"

    },
    "AircraftLayer\0aircraftSelected\0\0"
    "Aircraft*\0aircraft\0aircraftDeselected\0"
    "aircraftClicked\0position\0"
    "onAircraftPositionChanged\0newPosition"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_AircraftLayer[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   34,    2, 0x06 /* Public */,
       5,    0,   37,    2, 0x06 /* Public */,
       6,    2,   38,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       8,    1,   43,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 3, QMetaType::QPointF,    4,    7,

 // slots: parameters
    QMetaType::Void, QMetaType::QPointF,    9,

       0        // eod
};

void AircraftLayer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AircraftLayer *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->aircraftSelected((*reinterpret_cast< Aircraft*(*)>(_a[1]))); break;
        case 1: _t->aircraftDeselected(); break;
        case 2: _t->aircraftClicked((*reinterpret_cast< Aircraft*(*)>(_a[1])),(*reinterpret_cast< const QPointF(*)>(_a[2]))); break;
        case 3: _t->onAircraftPositionChanged((*reinterpret_cast< const QPointF(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Aircraft* >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Aircraft* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AircraftLayer::*)(Aircraft * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AircraftLayer::aircraftSelected)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (AircraftLayer::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AircraftLayer::aircraftDeselected)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (AircraftLayer::*)(Aircraft * , const QPointF & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AircraftLayer::aircraftClicked)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject AircraftLayer::staticMetaObject = { {
    QMetaObject::SuperData::link<MapLayer::staticMetaObject>(),
    qt_meta_stringdata_AircraftLayer.data,
    qt_meta_data_AircraftLayer,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *AircraftLayer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AircraftLayer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_AircraftLayer.stringdata0))
        return static_cast<void*>(this);
    return MapLayer::qt_metacast(_clname);
}

int AircraftLayer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = MapLayer::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void AircraftLayer::aircraftSelected(Aircraft * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void AircraftLayer::aircraftDeselected()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void AircraftLayer::aircraftClicked(Aircraft * _t1, const QPointF & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
