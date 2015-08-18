/****************************************************************************
** Meta object code from reading C++ file 'EditorResourceSelectionDialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "Editor/PrecompiledHeader.h"
#include "EditorResourceSelectionDialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EditorResourceSelectionDialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_EditorResourceSelectionDialog_t {
    QByteArrayData data[12];
    char stringdata[237];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EditorResourceSelectionDialog_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EditorResourceSelectionDialog_t qt_meta_stringdata_EditorResourceSelectionDialog = {
    {
QT_MOC_LITERAL(0, 0, 29), // "EditorResourceSelectionDialog"
QT_MOC_LITERAL(1, 30, 28), // "OnDirectoryComboBoxActivated"
QT_MOC_LITERAL(2, 59, 0), // ""
QT_MOC_LITERAL(3, 60, 4), // "text"
QT_MOC_LITERAL(4, 65, 19), // "OnBackButtonPressed"
QT_MOC_LITERAL(5, 85, 26), // "OnUpDirectoryButtonPressed"
QT_MOC_LITERAL(6, 112, 28), // "OnMakeDirectoryButtonPressed"
QT_MOC_LITERAL(7, 141, 23), // "OnTreeViewItemActivated"
QT_MOC_LITERAL(8, 165, 5), // "index"
QT_MOC_LITERAL(9, 171, 21), // "OnTreeViewItemClicked"
QT_MOC_LITERAL(10, 193, 21), // "OnSelectButtonClicked"
QT_MOC_LITERAL(11, 215, 21) // "OnCancelButtonClicked"

    },
    "EditorResourceSelectionDialog\0"
    "OnDirectoryComboBoxActivated\0\0text\0"
    "OnBackButtonPressed\0OnUpDirectoryButtonPressed\0"
    "OnMakeDirectoryButtonPressed\0"
    "OnTreeViewItemActivated\0index\0"
    "OnTreeViewItemClicked\0OnSelectButtonClicked\0"
    "OnCancelButtonClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EditorResourceSelectionDialog[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   54,    2, 0x08 /* Private */,
       4,    0,   57,    2, 0x08 /* Private */,
       5,    0,   58,    2, 0x08 /* Private */,
       6,    0,   59,    2, 0x08 /* Private */,
       7,    1,   60,    2, 0x08 /* Private */,
       9,    1,   63,    2, 0x08 /* Private */,
      10,    0,   66,    2, 0x08 /* Private */,
      11,    0,   67,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QModelIndex,    8,
    QMetaType::Void, QMetaType::QModelIndex,    8,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void EditorResourceSelectionDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        EditorResourceSelectionDialog *_t = static_cast<EditorResourceSelectionDialog *>(_o);
        switch (_id) {
        case 0: _t->OnDirectoryComboBoxActivated((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->OnBackButtonPressed(); break;
        case 2: _t->OnUpDirectoryButtonPressed(); break;
        case 3: _t->OnMakeDirectoryButtonPressed(); break;
        case 4: _t->OnTreeViewItemActivated((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 5: _t->OnTreeViewItemClicked((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 6: _t->OnSelectButtonClicked(); break;
        case 7: _t->OnCancelButtonClicked(); break;
        default: ;
        }
    }
}

const QMetaObject EditorResourceSelectionDialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_EditorResourceSelectionDialog.data,
      qt_meta_data_EditorResourceSelectionDialog,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *EditorResourceSelectionDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EditorResourceSelectionDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_EditorResourceSelectionDialog.stringdata))
        return static_cast<void*>(const_cast< EditorResourceSelectionDialog*>(this));
    return QDialog::qt_metacast(_clname);
}

int EditorResourceSelectionDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
