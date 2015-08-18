/****************************************************************************
** Meta object code from reading C++ file 'EditorVectorEditWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "Editor/PrecompiledHeader.h"
#include "EditorVectorEditWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EditorVectorEditWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_EditorVectorEditWidget_t {
    QByteArrayData data[24];
    char stringdata[317];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EditorVectorEditWidget_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EditorVectorEditWidget_t qt_meta_stringdata_EditorVectorEditWidget = {
    {
QT_MOC_LITERAL(0, 0, 22), // "EditorVectorEditWidget"
QT_MOC_LITERAL(1, 23, 18), // "ValueChangedFloat2"
QT_MOC_LITERAL(2, 42, 0), // ""
QT_MOC_LITERAL(3, 43, 6), // "float2"
QT_MOC_LITERAL(4, 50, 5), // "value"
QT_MOC_LITERAL(5, 56, 18), // "ValueChangedFloat3"
QT_MOC_LITERAL(6, 75, 6), // "float3"
QT_MOC_LITERAL(7, 82, 18), // "ValueChangedFloat4"
QT_MOC_LITERAL(8, 101, 6), // "float4"
QT_MOC_LITERAL(9, 108, 18), // "ValueChangedString"
QT_MOC_LITERAL(10, 127, 16), // "SetNumComponents"
QT_MOC_LITERAL(11, 144, 6), // "uint32"
QT_MOC_LITERAL(12, 151, 13), // "numComponents"
QT_MOC_LITERAL(13, 165, 8), // "SetValue"
QT_MOC_LITERAL(14, 174, 1), // "x"
QT_MOC_LITERAL(15, 176, 1), // "y"
QT_MOC_LITERAL(16, 178, 1), // "z"
QT_MOC_LITERAL(17, 180, 1), // "w"
QT_MOC_LITERAL(18, 182, 21), // "OnExpandButtonClicked"
QT_MOC_LITERAL(19, 204, 20), // "OnPopupPanelXChanged"
QT_MOC_LITERAL(20, 225, 20), // "OnPopupPanelYChanged"
QT_MOC_LITERAL(21, 246, 20), // "OnPopupPanelZChanged"
QT_MOC_LITERAL(22, 267, 20), // "OnPopupPanelWChanged"
QT_MOC_LITERAL(23, 288, 28) // "OnPopupPanelNormalizeClicked"

    },
    "EditorVectorEditWidget\0ValueChangedFloat2\0"
    "\0float2\0value\0ValueChangedFloat3\0"
    "float3\0ValueChangedFloat4\0float4\0"
    "ValueChangedString\0SetNumComponents\0"
    "uint32\0numComponents\0SetValue\0x\0y\0z\0"
    "w\0OnExpandButtonClicked\0OnPopupPanelXChanged\0"
    "OnPopupPanelYChanged\0OnPopupPanelZChanged\0"
    "OnPopupPanelWChanged\0OnPopupPanelNormalizeClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EditorVectorEditWidget[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   94,    2, 0x06 /* Public */,
       5,    1,   97,    2, 0x06 /* Public */,
       7,    1,  100,    2, 0x06 /* Public */,
       9,    1,  103,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      10,    1,  106,    2, 0x0a /* Public */,
      13,    4,  109,    2, 0x0a /* Public */,
      13,    3,  118,    2, 0x2a /* Public | MethodCloned */,
      13,    2,  125,    2, 0x2a /* Public | MethodCloned */,
      13,    1,  130,    2, 0x2a /* Public | MethodCloned */,
      13,    0,  133,    2, 0x2a /* Public | MethodCloned */,
      18,    0,  134,    2, 0x08 /* Private */,
      19,    1,  135,    2, 0x08 /* Private */,
      20,    1,  138,    2, 0x08 /* Private */,
      21,    1,  141,    2, 0x08 /* Private */,
      22,    1,  144,    2, 0x08 /* Private */,
      23,    0,  147,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 6,    4,
    QMetaType::Void, 0x80000000 | 8,    4,
    QMetaType::Void, QMetaType::QString,    4,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void, QMetaType::Float, QMetaType::Float, QMetaType::Float, QMetaType::Float,   14,   15,   16,   17,
    QMetaType::Void, QMetaType::Float, QMetaType::Float, QMetaType::Float,   14,   15,   16,
    QMetaType::Void, QMetaType::Float, QMetaType::Float,   14,   15,
    QMetaType::Void, QMetaType::Float,   14,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double,    4,
    QMetaType::Void, QMetaType::Double,    4,
    QMetaType::Void, QMetaType::Double,    4,
    QMetaType::Void, QMetaType::Double,    4,
    QMetaType::Void,

       0        // eod
};

void EditorVectorEditWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        EditorVectorEditWidget *_t = static_cast<EditorVectorEditWidget *>(_o);
        switch (_id) {
        case 0: _t->ValueChangedFloat2((*reinterpret_cast< const float2(*)>(_a[1]))); break;
        case 1: _t->ValueChangedFloat3((*reinterpret_cast< const float3(*)>(_a[1]))); break;
        case 2: _t->ValueChangedFloat4((*reinterpret_cast< const float4(*)>(_a[1]))); break;
        case 3: _t->ValueChangedString((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->SetNumComponents((*reinterpret_cast< uint32(*)>(_a[1]))); break;
        case 5: _t->SetValue((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3])),(*reinterpret_cast< float(*)>(_a[4]))); break;
        case 6: _t->SetValue((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3]))); break;
        case 7: _t->SetValue((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 8: _t->SetValue((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 9: _t->SetValue(); break;
        case 10: _t->OnExpandButtonClicked(); break;
        case 11: _t->OnPopupPanelXChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 12: _t->OnPopupPanelYChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 13: _t->OnPopupPanelZChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 14: _t->OnPopupPanelWChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 15: _t->OnPopupPanelNormalizeClicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (EditorVectorEditWidget::*_t)(const float2 & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EditorVectorEditWidget::ValueChangedFloat2)) {
                *result = 0;
            }
        }
        {
            typedef void (EditorVectorEditWidget::*_t)(const float3 & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EditorVectorEditWidget::ValueChangedFloat3)) {
                *result = 1;
            }
        }
        {
            typedef void (EditorVectorEditWidget::*_t)(const float4 & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EditorVectorEditWidget::ValueChangedFloat4)) {
                *result = 2;
            }
        }
        {
            typedef void (EditorVectorEditWidget::*_t)(const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EditorVectorEditWidget::ValueChangedString)) {
                *result = 3;
            }
        }
    }
}

const QMetaObject EditorVectorEditWidget::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_EditorVectorEditWidget.data,
      qt_meta_data_EditorVectorEditWidget,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *EditorVectorEditWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EditorVectorEditWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_EditorVectorEditWidget.stringdata))
        return static_cast<void*>(const_cast< EditorVectorEditWidget*>(this));
    return QWidget::qt_metacast(_clname);
}

int EditorVectorEditWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void EditorVectorEditWidget::ValueChangedFloat2(const float2 & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void EditorVectorEditWidget::ValueChangedFloat3(const float3 & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void EditorVectorEditWidget::ValueChangedFloat4(const float4 & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void EditorVectorEditWidget::ValueChangedString(const QString & _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_END_MOC_NAMESPACE
