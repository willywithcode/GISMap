/****************************************************************************
** Meta object code from reading C++ file 'aircraft.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/models/aircraft.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'aircraft.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_Aircraft_t {
    QByteArrayData data[10];
    char stringdata0[117];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Aircraft_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Aircraft_t qt_meta_stringdata_Aircraft = {
    {
QT_MOC_LITERAL(0, 0, 8), // "Aircraft"
QT_MOC_LITERAL(1, 9, 15), // "positionChanged"
QT_MOC_LITERAL(2, 25, 0), // ""
QT_MOC_LITERAL(3, 26, 11), // "newPosition"
QT_MOC_LITERAL(4, 38, 12), // "stateChanged"
QT_MOC_LITERAL(5, 51, 15), // "Aircraft::State"
QT_MOC_LITERAL(6, 67, 8), // "newState"
QT_MOC_LITERAL(7, 76, 14), // "headingChanged"
QT_MOC_LITERAL(8, 91, 10), // "newHeading"
QT_MOC_LITERAL(9, 102, 14) // "updatePosition"

    },
    "Aircraft\0positionChanged\0\0newPosition\0"
    "stateChanged\0Aircraft::State\0newState\0"
    "headingChanged\0newHeading\0updatePosition"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Aircraft[] = {

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
       4,    1,   37,    2, 0x06 /* Public */,
       7,    1,   40,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       9,    0,   43,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QPointF,    3,
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void, QMetaType::Double,    8,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void Aircraft::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Aircraft *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->positionChanged((*reinterpret_cast< const QPointF(*)>(_a[1]))); break;
        case 1: _t->stateChanged((*reinterpret_cast< Aircraft::State(*)>(_a[1]))); break;
        case 2: _t->headingChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 3: _t->updatePosition(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Aircraft::*)(const QPointF & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Aircraft::positionChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Aircraft::*)(Aircraft::State );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Aircraft::stateChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (Aircraft::*)(double );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Aircraft::headingChanged)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject Aircraft::staticMetaObject = { {
    QMetaObject::SuperData::link<GeometryObject::staticMetaObject>(),
    qt_meta_stringdata_Aircraft.data,
    qt_meta_data_Aircraft,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *Aircraft::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Aircraft::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Aircraft.stringdata0))
        return static_cast<void*>(this);
    return GeometryObject::qt_metacast(_clname);
}

int Aircraft::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = GeometryObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void Aircraft::positionChanged(const QPointF & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Aircraft::stateChanged(Aircraft::State _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void Aircraft::headingChanged(double _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
