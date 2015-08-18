/****************************************************************************
** Meta object code from reading C++ file 'EditorRendererSwapChainWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "Editor/PrecompiledHeader.h"
#include "EditorRendererSwapChainWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EditorRendererSwapChainWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_EditorRendererSwapChainWidget_t {
    QByteArrayData data[18];
    char stringdata[250];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EditorRendererSwapChainWidget_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EditorRendererSwapChainWidget_t qt_meta_stringdata_EditorRendererSwapChainWidget = {
    {
QT_MOC_LITERAL(0, 0, 29), // "EditorRendererSwapChainWidget"
QT_MOC_LITERAL(1, 30, 12), // "ResizedEvent"
QT_MOC_LITERAL(2, 43, 0), // ""
QT_MOC_LITERAL(3, 44, 10), // "PaintEvent"
QT_MOC_LITERAL(4, 55, 16), // "GainedFocusEvent"
QT_MOC_LITERAL(5, 72, 14), // "LostFocusEvent"
QT_MOC_LITERAL(6, 87, 13), // "KeyboardEvent"
QT_MOC_LITERAL(7, 101, 16), // "const QKeyEvent*"
QT_MOC_LITERAL(8, 118, 14), // "pKeyboardEvent"
QT_MOC_LITERAL(9, 133, 10), // "MouseEvent"
QT_MOC_LITERAL(10, 144, 18), // "const QMouseEvent*"
QT_MOC_LITERAL(11, 163, 11), // "pMouseEvent"
QT_MOC_LITERAL(12, 175, 10), // "WheelEvent"
QT_MOC_LITERAL(13, 186, 18), // "const QWheelEvent*"
QT_MOC_LITERAL(14, 205, 11), // "pWheelEvent"
QT_MOC_LITERAL(15, 217, 9), // "DropEvent"
QT_MOC_LITERAL(16, 227, 11), // "QDropEvent*"
QT_MOC_LITERAL(17, 239, 10) // "pDropEvent"

    },
    "EditorRendererSwapChainWidget\0"
    "ResizedEvent\0\0PaintEvent\0GainedFocusEvent\0"
    "LostFocusEvent\0KeyboardEvent\0"
    "const QKeyEvent*\0pKeyboardEvent\0"
    "MouseEvent\0const QMouseEvent*\0pMouseEvent\0"
    "WheelEvent\0const QWheelEvent*\0pWheelEvent\0"
    "DropEvent\0QDropEvent*\0pDropEvent"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EditorRendererSwapChainWidget[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   54,    2, 0x06 /* Public */,
       3,    0,   55,    2, 0x06 /* Public */,
       4,    0,   56,    2, 0x06 /* Public */,
       5,    0,   57,    2, 0x06 /* Public */,
       6,    1,   58,    2, 0x06 /* Public */,
       9,    1,   61,    2, 0x06 /* Public */,
      12,    1,   64,    2, 0x06 /* Public */,
      15,    1,   67,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, 0x80000000 | 10,   11,
    QMetaType::Void, 0x80000000 | 13,   14,
    QMetaType::Void, 0x80000000 | 16,   17,

       0        // eod
};

void EditorRendererSwapChainWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        EditorRendererSwapChainWidget *_t = static_cast<EditorRendererSwapChainWidget *>(_o);
        switch (_id) {
        case 0: _t->ResizedEvent(); break;
        case 1: _t->PaintEvent(); break;
        case 2: _t->GainedFocusEvent(); break;
        case 3: _t->LostFocusEvent(); break;
        case 4: _t->KeyboardEvent((*reinterpret_cast< const QKeyEvent*(*)>(_a[1]))); break;
        case 5: _t->MouseEvent((*reinterpret_cast< const QMouseEvent*(*)>(_a[1]))); break;
        case 6: _t->WheelEvent((*reinterpret_cast< const QWheelEvent*(*)>(_a[1]))); break;
        case 7: _t->DropEvent((*reinterpret_cast< QDropEvent*(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (EditorRendererSwapChainWidget::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EditorRendererSwapChainWidget::ResizedEvent)) {
                *result = 0;
            }
        }
        {
            typedef void (EditorRendererSwapChainWidget::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EditorRendererSwapChainWidget::PaintEvent)) {
                *result = 1;
            }
        }
        {
            typedef void (EditorRendererSwapChainWidget::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EditorRendererSwapChainWidget::GainedFocusEvent)) {
                *result = 2;
            }
        }
        {
            typedef void (EditorRendererSwapChainWidget::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EditorRendererSwapChainWidget::LostFocusEvent)) {
                *result = 3;
            }
        }
        {
            typedef void (EditorRendererSwapChainWidget::*_t)(const QKeyEvent * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EditorRendererSwapChainWidget::KeyboardEvent)) {
                *result = 4;
            }
        }
        {
            typedef void (EditorRendererSwapChainWidget::*_t)(const QMouseEvent * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EditorRendererSwapChainWidget::MouseEvent)) {
                *result = 5;
            }
        }
        {
            typedef void (EditorRendererSwapChainWidget::*_t)(const QWheelEvent * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EditorRendererSwapChainWidget::WheelEvent)) {
                *result = 6;
            }
        }
        {
            typedef void (EditorRendererSwapChainWidget::*_t)(QDropEvent * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EditorRendererSwapChainWidget::DropEvent)) {
                *result = 7;
            }
        }
    }
}

const QMetaObject EditorRendererSwapChainWidget::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_EditorRendererSwapChainWidget.data,
      qt_meta_data_EditorRendererSwapChainWidget,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *EditorRendererSwapChainWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EditorRendererSwapChainWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_EditorRendererSwapChainWidget.stringdata))
        return static_cast<void*>(const_cast< EditorRendererSwapChainWidget*>(this));
    return QWidget::qt_metacast(_clname);
}

int EditorRendererSwapChainWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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

// SIGNAL 0
void EditorRendererSwapChainWidget::ResizedEvent()
{
    QMetaObject::activate(this, &staticMetaObject, 0, Q_NULLPTR);
}

// SIGNAL 1
void EditorRendererSwapChainWidget::PaintEvent()
{
    QMetaObject::activate(this, &staticMetaObject, 1, Q_NULLPTR);
}

// SIGNAL 2
void EditorRendererSwapChainWidget::GainedFocusEvent()
{
    QMetaObject::activate(this, &staticMetaObject, 2, Q_NULLPTR);
}

// SIGNAL 3
void EditorRendererSwapChainWidget::LostFocusEvent()
{
    QMetaObject::activate(this, &staticMetaObject, 3, Q_NULLPTR);
}

// SIGNAL 4
void EditorRendererSwapChainWidget::KeyboardEvent(const QKeyEvent * _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void EditorRendererSwapChainWidget::MouseEvent(const QMouseEvent * _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void EditorRendererSwapChainWidget::WheelEvent(const QWheelEvent * _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void EditorRendererSwapChainWidget::DropEvent(QDropEvent * _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}
QT_END_MOC_NAMESPACE
