/****************************************************************************
** Meta object code from reading C++ file 'aircraftmanager.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/managers/aircraftmanager.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'aircraftmanager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_AircraftManager_t {
    QByteArrayData data[9];
    char stringdata0[115];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_AircraftManager_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_AircraftManager_t qt_meta_stringdata_AircraftManager = {
    {
QT_MOC_LITERAL(0, 0, 15), // "AircraftManager"
QT_MOC_LITERAL(1, 16, 15), // "aircraftCreated"
QT_MOC_LITERAL(2, 32, 0), // ""
QT_MOC_LITERAL(3, 33, 9), // "Aircraft*"
QT_MOC_LITERAL(4, 43, 8), // "aircraft"
QT_MOC_LITERAL(5, 52, 15), // "aircraftRemoved"
QT_MOC_LITERAL(6, 68, 20), // "aircraftCountChanged"
QT_MOC_LITERAL(7, 89, 5), // "count"
QT_MOC_LITERAL(8, 95, 19) // "onAircraftDestroyed"

    },
    "AircraftManager\0aircraftCreated\0\0"
    "Aircraft*\0aircraft\0aircraftRemoved\0"
    "aircraftCountChanged\0count\0"
    "onAircraftDestroyed"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_AircraftManager[] = {

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
       5,    1,   37,    2, 0x06 /* Public */,
       6,    1,   40,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       8,    0,   43,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::Int,    7,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void AircraftManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AircraftManager *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->aircraftCreated((*reinterpret_cast< Aircraft*(*)>(_a[1]))); break;
        case 1: _t->aircraftRemoved((*reinterpret_cast< Aircraft*(*)>(_a[1]))); break;
        case 2: _t->aircraftCountChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->onAircraftDestroyed(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AircraftManager::*)(Aircraft * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AircraftManager::aircraftCreated)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (AircraftManager::*)(Aircraft * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AircraftManager::aircraftRemoved)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (AircraftManager::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AircraftManager::aircraftCountChanged)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject AircraftManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_AircraftManager.data,
    qt_meta_data_AircraftManager,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *AircraftManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AircraftManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_AircraftManager.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int AircraftManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
void AircraftManager::aircraftCreated(Aircraft * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void AircraftManager::aircraftRemoved(Aircraft * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void AircraftManager::aircraftCountChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
