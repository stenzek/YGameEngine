/****************************************************************************
** Meta object code from reading C++ file 'EditorMapWindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "Editor/PrecompiledHeader.h"
#include "EditorMapWindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EditorMapWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_EditorMapWindow_t {
    QByteArrayData data[43];
    char stringdata[1216];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EditorMapWindow_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EditorMapWindow_t qt_meta_stringdata_EditorMapWindow = {
    {
QT_MOC_LITERAL(0, 0, 15), // "EditorMapWindow"
QT_MOC_LITERAL(1, 16, 27), // "OnActionFileNewMapTriggered"
QT_MOC_LITERAL(2, 44, 0), // ""
QT_MOC_LITERAL(3, 45, 28), // "OnActionFileOpenMapTriggered"
QT_MOC_LITERAL(4, 74, 28), // "OnActionFileSaveMapTriggered"
QT_MOC_LITERAL(5, 103, 30), // "OnActionFileSaveMapAsTriggered"
QT_MOC_LITERAL(6, 134, 29), // "OnActionFileCloseMapTriggered"
QT_MOC_LITERAL(7, 164, 25), // "OnActionFileExitTriggered"
QT_MOC_LITERAL(8, 190, 51), // "OnActionEditReferenceCoordina..."
QT_MOC_LITERAL(9, 242, 7), // "checked"
QT_MOC_LITERAL(10, 250, 51), // "OnActionEditReferenceCoordina..."
QT_MOC_LITERAL(11, 302, 34), // "OnActionViewWorldOutlinerTrig..."
QT_MOC_LITERAL(12, 337, 36), // "OnActionViewResourceBrowserTr..."
QT_MOC_LITERAL(13, 374, 28), // "OnActionViewToolboxTriggered"
QT_MOC_LITERAL(14, 403, 35), // "OnActionViewPropertyEditorTri..."
QT_MOC_LITERAL(15, 439, 21), // "OnActionViewThemeNone"
QT_MOC_LITERAL(16, 461, 21), // "OnActionViewThemeDark"
QT_MOC_LITERAL(17, 483, 26), // "OnActionViewThemeDarkOther"
QT_MOC_LITERAL(18, 510, 33), // "OnActionToolsMapEditModeTrigg..."
QT_MOC_LITERAL(19, 544, 36), // "OnActionToolsEntityEditModeTr..."
QT_MOC_LITERAL(20, 581, 38), // "OnActionToolsGeometryEditMode..."
QT_MOC_LITERAL(21, 620, 48), // "OnActionToolsHeightfieldTerra..."
QT_MOC_LITERAL(22, 669, 35), // "OnActionToolsCreateTerrainTri..."
QT_MOC_LITERAL(23, 705, 35), // "OnActionToolsDeleteTerrainTri..."
QT_MOC_LITERAL(24, 741, 40), // "OnActionToolsCreateBlockTerra..."
QT_MOC_LITERAL(25, 782, 40), // "OnActionToolsDeleteBlockTerra..."
QT_MOC_LITERAL(26, 823, 33), // "OnStatusBarGridSnapEnabledTog..."
QT_MOC_LITERAL(27, 857, 34), // "OnStatusBarGridSnapIntervalCh..."
QT_MOC_LITERAL(28, 892, 5), // "value"
QT_MOC_LITERAL(29, 898, 34), // "OnStatusBarGridLinesVisibleTo..."
QT_MOC_LITERAL(30, 933, 35), // "OnStatusBarGridLinesIntervalC..."
QT_MOC_LITERAL(31, 969, 29), // "OnWorldOutlinerEntitySelected"
QT_MOC_LITERAL(32, 999, 22), // "const EditorMapEntity*"
QT_MOC_LITERAL(33, 1022, 7), // "pEntity"
QT_MOC_LITERAL(34, 1030, 30), // "OnWorldOutlinerEntityActivated"
QT_MOC_LITERAL(35, 1061, 32), // "OnToolboxTabWidgetCurrentChanged"
QT_MOC_LITERAL(36, 1094, 5), // "index"
QT_MOC_LITERAL(37, 1100, 31), // "OnPropertyEditorPropertyChanged"
QT_MOC_LITERAL(38, 1132, 11), // "const char*"
QT_MOC_LITERAL(39, 1144, 12), // "propertyName"
QT_MOC_LITERAL(40, 1157, 13), // "propertyValue"
QT_MOC_LITERAL(41, 1171, 25), // "OnFrameExecutionTriggered"
QT_MOC_LITERAL(42, 1197, 18) // "timeSinceLastFrame"

    },
    "EditorMapWindow\0OnActionFileNewMapTriggered\0"
    "\0OnActionFileOpenMapTriggered\0"
    "OnActionFileSaveMapTriggered\0"
    "OnActionFileSaveMapAsTriggered\0"
    "OnActionFileCloseMapTriggered\0"
    "OnActionFileExitTriggered\0"
    "OnActionEditReferenceCoordinateSystemLocalTriggered\0"
    "checked\0OnActionEditReferenceCoordinateSystemWorldTriggered\0"
    "OnActionViewWorldOutlinerTriggered\0"
    "OnActionViewResourceBrowserTriggered\0"
    "OnActionViewToolboxTriggered\0"
    "OnActionViewPropertyEditorTriggered\0"
    "OnActionViewThemeNone\0OnActionViewThemeDark\0"
    "OnActionViewThemeDarkOther\0"
    "OnActionToolsMapEditModeTriggered\0"
    "OnActionToolsEntityEditModeTriggered\0"
    "OnActionToolsGeometryEditModeTriggered\0"
    "OnActionToolsHeightfieldTerrainEditModeTriggered\0"
    "OnActionToolsCreateTerrainTriggered\0"
    "OnActionToolsDeleteTerrainTriggered\0"
    "OnActionToolsCreateBlockTerrainTriggered\0"
    "OnActionToolsDeleteBlockTerrainTriggered\0"
    "OnStatusBarGridSnapEnabledToggled\0"
    "OnStatusBarGridSnapIntervalChanged\0"
    "value\0OnStatusBarGridLinesVisibleToggled\0"
    "OnStatusBarGridLinesIntervalChanged\0"
    "OnWorldOutlinerEntitySelected\0"
    "const EditorMapEntity*\0pEntity\0"
    "OnWorldOutlinerEntityActivated\0"
    "OnToolboxTabWidgetCurrentChanged\0index\0"
    "OnPropertyEditorPropertyChanged\0"
    "const char*\0propertyName\0propertyValue\0"
    "OnFrameExecutionTriggered\0timeSinceLastFrame"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EditorMapWindow[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      32,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,  174,    2, 0x08 /* Private */,
       3,    0,  175,    2, 0x08 /* Private */,
       4,    0,  176,    2, 0x08 /* Private */,
       5,    0,  177,    2, 0x08 /* Private */,
       6,    0,  178,    2, 0x08 /* Private */,
       7,    0,  179,    2, 0x08 /* Private */,
       8,    1,  180,    2, 0x08 /* Private */,
      10,    1,  183,    2, 0x08 /* Private */,
      11,    1,  186,    2, 0x08 /* Private */,
      12,    1,  189,    2, 0x08 /* Private */,
      13,    1,  192,    2, 0x08 /* Private */,
      14,    1,  195,    2, 0x08 /* Private */,
      15,    0,  198,    2, 0x08 /* Private */,
      16,    0,  199,    2, 0x08 /* Private */,
      17,    0,  200,    2, 0x08 /* Private */,
      18,    1,  201,    2, 0x08 /* Private */,
      19,    1,  204,    2, 0x08 /* Private */,
      20,    1,  207,    2, 0x08 /* Private */,
      21,    1,  210,    2, 0x08 /* Private */,
      22,    0,  213,    2, 0x08 /* Private */,
      23,    0,  214,    2, 0x08 /* Private */,
      24,    0,  215,    2, 0x08 /* Private */,
      25,    0,  216,    2, 0x08 /* Private */,
      26,    1,  217,    2, 0x08 /* Private */,
      27,    1,  220,    2, 0x08 /* Private */,
      29,    1,  223,    2, 0x08 /* Private */,
      30,    1,  226,    2, 0x08 /* Private */,
      31,    1,  229,    2, 0x08 /* Private */,
      34,    1,  232,    2, 0x08 /* Private */,
      35,    1,  235,    2, 0x08 /* Private */,
      37,    2,  238,    2, 0x08 /* Private */,
      41,    1,  243,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Double,   28,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Double,   28,
    QMetaType::Void, 0x80000000 | 32,   33,
    QMetaType::Void, 0x80000000 | 32,   33,
    QMetaType::Void, QMetaType::Int,   36,
    QMetaType::Void, 0x80000000 | 38, 0x80000000 | 38,   39,   40,
    QMetaType::Void, QMetaType::Float,   42,

       0        // eod
};

void EditorMapWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        EditorMapWindow *_t = static_cast<EditorMapWindow *>(_o);
        switch (_id) {
        case 0: _t->OnActionFileNewMapTriggered(); break;
        case 1: _t->OnActionFileOpenMapTriggered(); break;
        case 2: _t->OnActionFileSaveMapTriggered(); break;
        case 3: _t->OnActionFileSaveMapAsTriggered(); break;
        case 4: _t->OnActionFileCloseMapTriggered(); break;
        case 5: _t->OnActionFileExitTriggered(); break;
        case 6: _t->OnActionEditReferenceCoordinateSystemLocalTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 7: _t->OnActionEditReferenceCoordinateSystemWorldTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 8: _t->OnActionViewWorldOutlinerTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 9: _t->OnActionViewResourceBrowserTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 10: _t->OnActionViewToolboxTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 11: _t->OnActionViewPropertyEditorTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 12: _t->OnActionViewThemeNone(); break;
        case 13: _t->OnActionViewThemeDark(); break;
        case 14: _t->OnActionViewThemeDarkOther(); break;
        case 15: _t->OnActionToolsMapEditModeTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 16: _t->OnActionToolsEntityEditModeTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 17: _t->OnActionToolsGeometryEditModeTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 18: _t->OnActionToolsHeightfieldTerrainEditModeTriggered((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 19: _t->OnActionToolsCreateTerrainTriggered(); break;
        case 20: _t->OnActionToolsDeleteTerrainTriggered(); break;
        case 21: _t->OnActionToolsCreateBlockTerrainTriggered(); break;
        case 22: _t->OnActionToolsDeleteBlockTerrainTriggered(); break;
        case 23: _t->OnStatusBarGridSnapEnabledToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 24: _t->OnStatusBarGridSnapIntervalChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 25: _t->OnStatusBarGridLinesVisibleToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 26: _t->OnStatusBarGridLinesIntervalChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 27: _t->OnWorldOutlinerEntitySelected((*reinterpret_cast< const EditorMapEntity*(*)>(_a[1]))); break;
        case 28: _t->OnWorldOutlinerEntityActivated((*reinterpret_cast< const EditorMapEntity*(*)>(_a[1]))); break;
        case 29: _t->OnToolboxTabWidgetCurrentChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 30: _t->OnPropertyEditorPropertyChanged((*reinterpret_cast< const char*(*)>(_a[1])),(*reinterpret_cast< const char*(*)>(_a[2]))); break;
        case 31: _t->OnFrameExecutionTriggered((*reinterpret_cast< float(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject EditorMapWindow::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_EditorMapWindow.data,
      qt_meta_data_EditorMapWindow,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *EditorMapWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EditorMapWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_EditorMapWindow.stringdata))
        return static_cast<void*>(const_cast< EditorMapWindow*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int EditorMapWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 32)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 32;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 32)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 32;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
