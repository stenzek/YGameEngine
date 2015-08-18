/****************************************************************************
** Meta object code from reading C++ file 'EditorMapEditMode.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "Editor/PrecompiledHeader.h"
#include "EditorMapEditMode.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EditorMapEditMode.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_EditorMapEditMode_t {
    QByteArrayData data[5];
    char stringdata[65];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EditorMapEditMode_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EditorMapEditMode_t qt_meta_stringdata_EditorMapEditMode = {
    {
QT_MOC_LITERAL(0, 0, 17), // "EditorMapEditMode"
QT_MOC_LITERAL(1, 18, 18), // "OnUIShowSkyChanged"
QT_MOC_LITERAL(2, 37, 0), // ""
QT_MOC_LITERAL(3, 38, 7), // "checked"
QT_MOC_LITERAL(4, 46, 18) // "OnUIShowSunChanged"

    },
    "EditorMapEditMode\0OnUIShowSkyChanged\0"
    "\0checked\0OnUIShowSunChanged"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EditorMapEditMode[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   24,    2, 0x08 /* Private */,
       4,    1,   27,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    3,
    QMetaType::Void, QMetaType::Bool,    3,

       0        // eod
};

void EditorMapEditMode::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        EditorMapEditMode *_t = static_cast<EditorMapEditMode *>(_o);
        switch (_id) {
        case 0: _t->OnUIShowSkyChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->OnUIShowSunChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject EditorMapEditMode::staticMetaObject = {
    { &EditorEditMode::staticMetaObject, qt_meta_stringdata_EditorMapEditMode.data,
      qt_meta_data_EditorMapEditMode,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *EditorMapEditMode::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EditorMapEditMode::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_EditorMapEditMode.stringdata))
        return static_cast<void*>(const_cast< EditorMapEditMode*>(this));
    return EditorEditMode::qt_metacast(_clname);
}

int EditorMapEditMode::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = EditorEditMode::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 2;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
