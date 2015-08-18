/****************************************************************************
** Meta object code from reading C++ file 'EditorStaticMeshImportDialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "Editor/PrecompiledHeader.h"
#include "EditorStaticMeshImportDialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EditorStaticMeshImportDialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_EditorStaticMeshImportDialog_t {
    QByteArrayData data[6];
    char stringdata[131];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EditorStaticMeshImportDialog_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EditorStaticMeshImportDialog_t qt_meta_stringdata_EditorStaticMeshImportDialog = {
    {
QT_MOC_LITERAL(0, 0, 28), // "EditorStaticMeshImportDialog"
QT_MOC_LITERAL(1, 29, 33), // "OnFileNameLineEditEditingFini..."
QT_MOC_LITERAL(2, 63, 0), // ""
QT_MOC_LITERAL(3, 64, 23), // "OnFileNameBrowseClicked"
QT_MOC_LITERAL(4, 88, 21), // "OnImportButtonClicked"
QT_MOC_LITERAL(5, 110, 20) // "OnCloseButtonClicked"

    },
    "EditorStaticMeshImportDialog\0"
    "OnFileNameLineEditEditingFinished\0\0"
    "OnFileNameBrowseClicked\0OnImportButtonClicked\0"
    "OnCloseButtonClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EditorStaticMeshImportDialog[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   34,    2, 0x08 /* Private */,
       3,    0,   35,    2, 0x08 /* Private */,
       4,    0,   36,    2, 0x08 /* Private */,
       5,    0,   37,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void EditorStaticMeshImportDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        EditorStaticMeshImportDialog *_t = static_cast<EditorStaticMeshImportDialog *>(_o);
        switch (_id) {
        case 0: _t->OnFileNameLineEditEditingFinished(); break;
        case 1: _t->OnFileNameBrowseClicked(); break;
        case 2: _t->OnImportButtonClicked(); break;
        case 3: _t->OnCloseButtonClicked(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject EditorStaticMeshImportDialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_EditorStaticMeshImportDialog.data,
      qt_meta_data_EditorStaticMeshImportDialog,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *EditorStaticMeshImportDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EditorStaticMeshImportDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_EditorStaticMeshImportDialog.stringdata))
        return static_cast<void*>(const_cast< EditorStaticMeshImportDialog*>(this));
    return QDialog::qt_metacast(_clname);
}

int EditorStaticMeshImportDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
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
QT_END_MOC_NAMESPACE
