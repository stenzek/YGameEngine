/****************************************************************************
** Meta object code from reading C++ file 'EditorCreateTerrainDialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "Editor/PrecompiledHeader.h"
#include "EditorCreateTerrainDialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EditorCreateTerrainDialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_EditorCreateTerrainDialog_t {
    QByteArrayData data[9];
    char stringdata[144];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EditorCreateTerrainDialog_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EditorCreateTerrainDialog_t qt_meta_stringdata_EditorCreateTerrainDialog = {
    {
QT_MOC_LITERAL(0, 0, 25), // "EditorCreateTerrainDialog"
QT_MOC_LITERAL(1, 26, 8), // "Validate"
QT_MOC_LITERAL(2, 35, 0), // ""
QT_MOC_LITERAL(3, 36, 18), // "OnLayerListChanged"
QT_MOC_LITERAL(4, 55, 5), // "value"
QT_MOC_LITERAL(5, 61, 21), // "OnHeightFormatChanged"
QT_MOC_LITERAL(6, 83, 12), // "currentIndex"
QT_MOC_LITERAL(7, 96, 23), // "OnCreateButtonTriggered"
QT_MOC_LITERAL(8, 120, 23) // "OnCancelButtonTriggered"

    },
    "EditorCreateTerrainDialog\0Validate\0\0"
    "OnLayerListChanged\0value\0OnHeightFormatChanged\0"
    "currentIndex\0OnCreateButtonTriggered\0"
    "OnCancelButtonTriggered"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EditorCreateTerrainDialog[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   39,    2, 0x08 /* Private */,
       3,    1,   40,    2, 0x08 /* Private */,
       5,    1,   43,    2, 0x08 /* Private */,
       7,    0,   46,    2, 0x08 /* Private */,
       8,    0,   47,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Bool,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::Int,    6,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void EditorCreateTerrainDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        EditorCreateTerrainDialog *_t = static_cast<EditorCreateTerrainDialog *>(_o);
        switch (_id) {
        case 0: { bool _r = _t->Validate();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 1: _t->OnLayerListChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->OnHeightFormatChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->OnCreateButtonTriggered(); break;
        case 4: _t->OnCancelButtonTriggered(); break;
        default: ;
        }
    }
}

const QMetaObject EditorCreateTerrainDialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_EditorCreateTerrainDialog.data,
      qt_meta_data_EditorCreateTerrainDialog,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *EditorCreateTerrainDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EditorCreateTerrainDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_EditorCreateTerrainDialog.stringdata))
        return static_cast<void*>(const_cast< EditorCreateTerrainDialog*>(this));
    return QDialog::qt_metacast(_clname);
}

int EditorCreateTerrainDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
