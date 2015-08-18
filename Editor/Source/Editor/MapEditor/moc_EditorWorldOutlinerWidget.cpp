/****************************************************************************
** Meta object code from reading C++ file 'EditorWorldOutlinerWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "Editor/PrecompiledHeader.h"
#include "EditorWorldOutlinerWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EditorWorldOutlinerWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_EditorWorldOutlinerWidget_t {
    QByteArrayData data[9];
    char stringdata[145];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EditorWorldOutlinerWidget_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EditorWorldOutlinerWidget_t qt_meta_stringdata_EditorWorldOutlinerWidget = {
    {
QT_MOC_LITERAL(0, 0, 25), // "EditorWorldOutlinerWidget"
QT_MOC_LITERAL(1, 26, 16), // "OnEntitySelected"
QT_MOC_LITERAL(2, 43, 0), // ""
QT_MOC_LITERAL(3, 44, 22), // "const EditorMapEntity*"
QT_MOC_LITERAL(4, 67, 7), // "pEntity"
QT_MOC_LITERAL(5, 75, 17), // "OnEntityActivated"
QT_MOC_LITERAL(6, 93, 21), // "OnTreeViewItemClicked"
QT_MOC_LITERAL(7, 115, 5), // "index"
QT_MOC_LITERAL(8, 121, 23) // "OnTreeViewItemActivated"

    },
    "EditorWorldOutlinerWidget\0OnEntitySelected\0"
    "\0const EditorMapEntity*\0pEntity\0"
    "OnEntityActivated\0OnTreeViewItemClicked\0"
    "index\0OnTreeViewItemActivated"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EditorWorldOutlinerWidget[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   34,    2, 0x06 /* Public */,
       5,    1,   37,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    1,   40,    2, 0x08 /* Private */,
       8,    1,   43,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,

 // slots: parameters
    QMetaType::Void, QMetaType::QModelIndex,    7,
    QMetaType::Void, QMetaType::QModelIndex,    7,

       0        // eod
};

void EditorWorldOutlinerWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        EditorWorldOutlinerWidget *_t = static_cast<EditorWorldOutlinerWidget *>(_o);
        switch (_id) {
        case 0: _t->OnEntitySelected((*reinterpret_cast< const EditorMapEntity*(*)>(_a[1]))); break;
        case 1: _t->OnEntityActivated((*reinterpret_cast< const EditorMapEntity*(*)>(_a[1]))); break;
        case 2: _t->OnTreeViewItemClicked((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 3: _t->OnTreeViewItemActivated((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (EditorWorldOutlinerWidget::*_t)(const EditorMapEntity * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EditorWorldOutlinerWidget::OnEntitySelected)) {
                *result = 0;
            }
        }
        {
            typedef void (EditorWorldOutlinerWidget::*_t)(const EditorMapEntity * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EditorWorldOutlinerWidget::OnEntityActivated)) {
                *result = 1;
            }
        }
    }
}

const QMetaObject EditorWorldOutlinerWidget::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_EditorWorldOutlinerWidget.data,
      qt_meta_data_EditorWorldOutlinerWidget,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *EditorWorldOutlinerWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EditorWorldOutlinerWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_EditorWorldOutlinerWidget.stringdata))
        return static_cast<void*>(const_cast< EditorWorldOutlinerWidget*>(this));
    return QWidget::qt_metacast(_clname);
}

int EditorWorldOutlinerWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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
void EditorWorldOutlinerWidget::OnEntitySelected(const EditorMapEntity * _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void EditorWorldOutlinerWidget::OnEntityActivated(const EditorMapEntity * _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_END_MOC_NAMESPACE
