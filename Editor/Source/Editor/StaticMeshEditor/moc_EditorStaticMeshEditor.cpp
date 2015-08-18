/****************************************************************************
** Meta object code from reading C++ file 'EditorStaticMeshEditor.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "Editor/PrecompiledHeader.h"
#include "EditorStaticMeshEditor.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EditorStaticMeshEditor.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_EditorStaticMeshEditor_t {
    QByteArrayData data[39];
    char stringdata[1029];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EditorStaticMeshEditor_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EditorStaticMeshEditor_t qt_meta_stringdata_EditorStaticMeshEditor = {
    {
QT_MOC_LITERAL(0, 0, 22), // "EditorStaticMeshEditor"
QT_MOC_LITERAL(1, 23, 23), // "OnActionOpenMeshClicked"
QT_MOC_LITERAL(2, 47, 0), // ""
QT_MOC_LITERAL(3, 48, 23), // "OnActionSaveMeshClicked"
QT_MOC_LITERAL(4, 72, 25), // "OnActionSaveMeshAsClicked"
QT_MOC_LITERAL(5, 98, 20), // "OnActionCloseClicked"
QT_MOC_LITERAL(6, 119, 34), // "OnActionCameraPerspectiveTrig..."
QT_MOC_LITERAL(7, 154, 7), // "checked"
QT_MOC_LITERAL(8, 162, 30), // "OnActionCameraArcballTriggered"
QT_MOC_LITERAL(9, 193, 32), // "OnActionCameraIsometricTriggered"
QT_MOC_LITERAL(10, 226, 30), // "OnActionViewWireframeTriggered"
QT_MOC_LITERAL(11, 257, 26), // "OnActionViewUnlitTriggered"
QT_MOC_LITERAL(12, 284, 24), // "OnActionViewLitTriggered"
QT_MOC_LITERAL(13, 309, 32), // "OnActionViewFlagShadowsTriggered"
QT_MOC_LITERAL(14, 342, 41), // "OnActionViewFlagWireframeOver..."
QT_MOC_LITERAL(15, 384, 32), // "OnActionToolInformationTriggered"
QT_MOC_LITERAL(16, 417, 31), // "OnActionToolOperationsTriggered"
QT_MOC_LITERAL(17, 449, 30), // "OnActionToolMaterialsTriggered"
QT_MOC_LITERAL(18, 480, 24), // "OnActionToolLODTriggered"
QT_MOC_LITERAL(19, 505, 30), // "OnActionToolCollisionTriggered"
QT_MOC_LITERAL(20, 536, 37), // "OnActionToolLightManipulatorT..."
QT_MOC_LITERAL(21, 574, 24), // "OnSwapChainWidgetResized"
QT_MOC_LITERAL(22, 599, 22), // "OnSwapChainWidgetPaint"
QT_MOC_LITERAL(23, 622, 30), // "OnSwapChainWidgetKeyboardEvent"
QT_MOC_LITERAL(24, 653, 16), // "const QKeyEvent*"
QT_MOC_LITERAL(25, 670, 14), // "pKeyboardEvent"
QT_MOC_LITERAL(26, 685, 27), // "OnSwapChainWidgetMouseEvent"
QT_MOC_LITERAL(27, 713, 18), // "const QMouseEvent*"
QT_MOC_LITERAL(28, 732, 11), // "pMouseEvent"
QT_MOC_LITERAL(29, 744, 27), // "OnSwapChainWidgetWheelEvent"
QT_MOC_LITERAL(30, 772, 18), // "const QWheelEvent*"
QT_MOC_LITERAL(31, 791, 11), // "pWheelEvent"
QT_MOC_LITERAL(32, 803, 33), // "OnSwapChainWidgetGainedFocusE..."
QT_MOC_LITERAL(33, 837, 25), // "OnFrameExecutionTriggered"
QT_MOC_LITERAL(34, 863, 18), // "timeSinceLastFrame"
QT_MOC_LITERAL(35, 882, 35), // "OnOperationsGenerateTangentsC..."
QT_MOC_LITERAL(36, 918, 29), // "OnOperationsCenterMeshClicked"
QT_MOC_LITERAL(37, 948, 39), // "OnOperationsRemoveUnusedVerti..."
QT_MOC_LITERAL(38, 988, 40) // "OnOperationsRemoveUnusedTrian..."

    },
    "EditorStaticMeshEditor\0OnActionOpenMeshClicked\0"
    "\0OnActionSaveMeshClicked\0"
    "OnActionSaveMeshAsClicked\0"
    "OnActionCloseClicked\0"
    "OnActionCameraPerspectiveTriggered\0"
    "checked\0OnActionCameraArcballTriggered\0"
    "OnActionCameraIsometricTriggered\0"
    "OnActionViewWireframeTriggered\0"
    "OnActionViewUnlitTriggered\0"
    "OnActionViewLitTriggered\0"
    "OnActionViewFlagShadowsTriggered\0"
    "OnActionViewFlagWireframeOverlayTriggered\0"
    "OnActionToolInformationTriggered\0"
    "OnActionToolOperationsTriggered\0"
    "OnActionToolMaterialsTriggered\0"
    "OnActionToolLODTriggered\0"
    "OnActionToolCollisionTriggered\0"
    "OnActionToolLightManipulatorTriggered\0"
    "OnSwapChainWidgetResized\0"
    "OnSwapChainWidgetPaint\0"
    "OnSwapChainWidgetKeyboardEvent\0"
    "const QKeyEvent*\0pKeyboardEvent\0"
    "OnSwapChainWidgetMouseEvent\0"
    "const QMouseEvent*\0pMouseEvent\0"
    "OnSwapChainWidgetWheelEvent\0"
    "const QWheelEvent*\0pWheelEvent\0"
    "OnSwapChainWidgetGainedFocusEvent\0"
    "OnFrameExecutionTriggered\0timeSinceLastFrame\0"
    "OnOperationsGenerateTangentsClicked\0"
    "OnOperationsCenterMeshClicked\0"
    "OnOperationsRemoveUnusedVerticesClicked\0"
    "OnOperationsRemoveUnusedTrianglesClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EditorStaticMeshEditor[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      29,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,  159,    2, 0x08 /* Private */,
       3,    0,  160,    2, 0x08 /* Private */,
       4,    0,  161,    2, 0x08 /* Private */,
       5,    0,  162,    2, 0x08 /* Private */,
       6,    1,  163,    2, 0x08 /* Private */,
       8,    1,  166,    2, 0x08 /* Private */,
       9,    1,  169,    2, 0x08 /* Private */,
      10,    1,  172,    2, 0x08 /* Private */,
      11,    1,  175,    2, 0x08 /* Private */,
      12,    1,  178,    2, 0x08 /* Private */,
      13,    1,  181,    2, 0x08 /* Private */,
      14,    1,  184,    2, 0x08 /* Private */,
      15,    1,  187,    2, 0x08 /* Private */,
      16,    1,  190,    2, 0x08 /* Private */,
      17,    1,  193,    2, 0x08 /* Private */,
      18,    1,  196,    2, 0x08 /* Private */,
      19,    1,  199,    2, 0x08 /* Private */,
      20,    1,  202,    2, 0x08 /* Private */,
      21,    0,  205,    2, 0x08 /* Private */,
      22,    0,  206,    2, 0x08 /* Private */,
      23,    1,  207,    2, 0x08 /* Private */,
      26,    1,  210,    2, 0x08 /* Private */,
      29,    1,  213,    2, 0x08 /* Private */,
      32,    0,  216,    2, 0x08 /* Private */,
      33,    1,  217,    2, 0x08 /* Private */,
      35,    0,  220,    2, 0x08 /* Private */,
      36,    0,  221,    2, 0x08 /* Private */,
      37,    0,  222,    2, 0x08 /* Private */,
      38,    0,  223,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 24,   25,
    QMetaType::Void, 0x80000000 | 27,   28,
    QMetaType::Void, 0x80000000 | 30,   31,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Float,   34,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void EditorStaticMeshEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        EditorStaticMeshEditor *_t = static_cast<EditorStaticMeshEditor *>(_o);
        switch (_id) {
        case 0: _t->OnActionOpenMeshClicked(); break;
        case 1: _t->OnActionSaveMeshClicked(); break;
        case 2: _t->OnActionSaveMeshAsClicked(); break;
        case 3: _t->OnActionCloseClicked(); break;
        case 4: _t->OnActionCameraPerspectiveTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 5: _t->OnActionCameraArcballTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->OnActionCameraIsometricTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->OnActionViewWireframeTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: _t->OnActionViewUnlitTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 9: _t->OnActionViewLitTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 10: _t->OnActionViewFlagShadowsTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 11: _t->OnActionViewFlagWireframeOverlayTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 12: _t->OnActionToolInformationTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 13: _t->OnActionToolOperationsTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 14: _t->OnActionToolMaterialsTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 15: _t->OnActionToolLODTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 16: _t->OnActionToolCollisionTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 17: _t->OnActionToolLightManipulatorTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 18: _t->OnSwapChainWidgetResized(); break;
        case 19: _t->OnSwapChainWidgetPaint(); break;
        case 20: _t->OnSwapChainWidgetKeyboardEvent((*reinterpret_cast< const QKeyEvent*(*)>(_a[1]))); break;
        case 21: _t->OnSwapChainWidgetMouseEvent((*reinterpret_cast< const QMouseEvent*(*)>(_a[1]))); break;
        case 22: _t->OnSwapChainWidgetWheelEvent((*reinterpret_cast< const QWheelEvent*(*)>(_a[1]))); break;
        case 23: _t->OnSwapChainWidgetGainedFocusEvent(); break;
        case 24: _t->OnFrameExecutionTriggered((*reinterpret_cast< float(*)>(_a[1]))); break;
        case 25: _t->OnOperationsGenerateTangentsClicked(); break;
        case 26: _t->OnOperationsCenterMeshClicked(); break;
        case 27: _t->OnOperationsRemoveUnusedVerticesClicked(); break;
        case 28: _t->OnOperationsRemoveUnusedTrianglesClicked(); break;
        default: ;
        }
    }
}

const QMetaObject EditorStaticMeshEditor::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_EditorStaticMeshEditor.data,
      qt_meta_data_EditorStaticMeshEditor,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *EditorStaticMeshEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EditorStaticMeshEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_EditorStaticMeshEditor.stringdata))
        return static_cast<void*>(const_cast< EditorStaticMeshEditor*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int EditorStaticMeshEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 29)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 29;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 29)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 29;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
