/****************************************************************************
** Meta object code from reading C++ file 'EditorResourceSaveDialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "Editor/PrecompiledHeader.h"
#include "EditorResourceSaveDialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EditorResourceSaveDialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_EditorResourceSaveDialog_t {
    QByteArrayData data[14];
    char stringdata[269];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EditorResourceSaveDialog_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EditorResourceSaveDialog_t qt_meta_stringdata_EditorResourceSaveDialog = {
    {
QT_MOC_LITERAL(0, 0, 24), // "EditorResourceSaveDialog"
QT_MOC_LITERAL(1, 25, 28), // "OnDirectoryComboBoxActivated"
QT_MOC_LITERAL(2, 54, 0), // ""
QT_MOC_LITERAL(3, 55, 4), // "text"
QT_MOC_LITERAL(4, 60, 19), // "OnBackButtonPressed"
QT_MOC_LITERAL(5, 80, 26), // "OnUpDirectoryButtonPressed"
QT_MOC_LITERAL(6, 107, 28), // "OnMakeDirectoryButtonPressed"
QT_MOC_LITERAL(7, 136, 23), // "OnTreeViewItemActivated"
QT_MOC_LITERAL(8, 160, 5), // "index"
QT_MOC_LITERAL(9, 166, 21), // "OnTreeViewItemClicked"
QT_MOC_LITERAL(10, 188, 29), // "OnResourceNameEditTextChanged"
QT_MOC_LITERAL(11, 218, 8), // "contents"
QT_MOC_LITERAL(12, 227, 19), // "OnSaveButtonClicked"
QT_MOC_LITERAL(13, 247, 21) // "OnCancelButtonClicked"

    },
    "EditorResourceSaveDialog\0"
    "OnDirectoryComboBoxActivated\0\0text\0"
    "OnBackButtonPressed\0OnUpDirectoryButtonPressed\0"
    "OnMakeDirectoryButtonPressed\0"
    "OnTreeViewItemActivated\0index\0"
    "OnTreeViewItemClicked\0"
    "OnResourceNameEditTextChanged\0contents\0"
    "OnSaveButtonClicked\0OnCancelButtonClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EditorResourceSaveDialog[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   59,    2, 0x08 /* Private */,
       4,    0,   62,    2, 0x08 /* Private */,
       5,    0,   63,    2, 0x08 /* Private */,
       6,    0,   64,    2, 0x08 /* Private */,
       7,    1,   65,    2, 0x08 /* Private */,
       9,    1,   68,    2, 0x08 /* Private */,
      10,    1,   71,    2, 0x08 /* Private */,
      12,    0,   74,    2, 0x08 /* Private */,
      13,    0,   75,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QModelIndex,    8,
    QMetaType::Void, QMetaType::QModelIndex,    8,
    QMetaType::Void, QMetaType::QString,   11,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void EditorResourceSaveDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        EditorResourceSaveDialog *_t = static_cast<EditorResourceSaveDialog *>(_o);
        switch (_id) {
        case 0: _t->OnDirectoryComboBoxActivated((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->OnBackButtonPressed(); break;
        case 2: _t->OnUpDirectoryButtonPressed(); break;
        case 3: _t->OnMakeDirectoryButtonPressed(); break;
        case 4: _t->OnTreeViewItemActivated((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 5: _t->OnTreeViewItemClicked((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 6: _t->OnResourceNameEditTextChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->OnSaveButtonClicked(); break;
        case 8: _t->OnCancelButtonClicked(); break;
        default: ;
        }
    }
}

const QMetaObject EditorResourceSaveDialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_EditorResourceSaveDialog.data,
      qt_meta_data_EditorResourceSaveDialog,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *EditorResourceSaveDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EditorResourceSaveDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_EditorResourceSaveDialog.stringdata))
        return static_cast<void*>(const_cast< EditorResourceSaveDialog*>(this));
    return QDialog::qt_metacast(_clname);
}

int EditorResourceSaveDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 9;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
