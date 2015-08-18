/****************************************************************************
** Meta object code from reading C++ file 'EditorPropertyEditorWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "Editor/PrecompiledHeader.h"
#include "EditorPropertyEditorWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'EditorPropertyEditorWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_EditorPropertyEditorWidget_t {
    QByteArrayData data[25];
    char stringdata[464];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EditorPropertyEditorWidget_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EditorPropertyEditorWidget_t qt_meta_stringdata_EditorPropertyEditorWidget = {
    {
QT_MOC_LITERAL(0, 0, 26), // "EditorPropertyEditorWidget"
QT_MOC_LITERAL(1, 27, 21), // "OnPropertyValueChange"
QT_MOC_LITERAL(2, 49, 0), // ""
QT_MOC_LITERAL(3, 50, 11), // "const char*"
QT_MOC_LITERAL(4, 62, 12), // "propertyName"
QT_MOC_LITERAL(5, 75, 13), // "propertyValue"
QT_MOC_LITERAL(6, 89, 11), // "AddProperty"
QT_MOC_LITERAL(7, 101, 31), // "const PropertyTemplateProperty*"
QT_MOC_LITERAL(8, 133, 19), // "pPropertyDefinition"
QT_MOC_LITERAL(9, 153, 12), // "currentValue"
QT_MOC_LITERAL(10, 166, 25), // "AddPropertiesFromTemplate"
QT_MOC_LITERAL(11, 192, 23), // "const PropertyTemplate*"
QT_MOC_LITERAL(12, 216, 9), // "pTemplate"
QT_MOC_LITERAL(13, 226, 15), // "ClearProperties"
QT_MOC_LITERAL(14, 242, 27), // "SetDestinationPropertyTable"
QT_MOC_LITERAL(15, 270, 14), // "PropertyTable*"
QT_MOC_LITERAL(16, 285, 14), // "pPropertyTable"
QT_MOC_LITERAL(17, 300, 19), // "UpdatePropertyValue"
QT_MOC_LITERAL(18, 320, 28), // "UpdatePropertyValueFromTable"
QT_MOC_LITERAL(19, 349, 31), // "OnTreeBrowserCurrentItemChanged"
QT_MOC_LITERAL(20, 381, 14), // "QtBrowserItem*"
QT_MOC_LITERAL(21, 396, 12), // "pBrowserItem"
QT_MOC_LITERAL(22, 409, 32), // "OnPropertyManagerPropertyChanged"
QT_MOC_LITERAL(23, 442, 11), // "QtProperty*"
QT_MOC_LITERAL(24, 454, 9) // "pProperty"

    },
    "EditorPropertyEditorWidget\0"
    "OnPropertyValueChange\0\0const char*\0"
    "propertyName\0propertyValue\0AddProperty\0"
    "const PropertyTemplateProperty*\0"
    "pPropertyDefinition\0currentValue\0"
    "AddPropertiesFromTemplate\0"
    "const PropertyTemplate*\0pTemplate\0"
    "ClearProperties\0SetDestinationPropertyTable\0"
    "PropertyTable*\0pPropertyTable\0"
    "UpdatePropertyValue\0UpdatePropertyValueFromTable\0"
    "OnTreeBrowserCurrentItemChanged\0"
    "QtBrowserItem*\0pBrowserItem\0"
    "OnPropertyManagerPropertyChanged\0"
    "QtProperty*\0pProperty"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EditorPropertyEditorWidget[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   59,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    2,   64,    2, 0x0a /* Public */,
      10,    1,   69,    2, 0x0a /* Public */,
      13,    0,   72,    2, 0x0a /* Public */,
      14,    1,   73,    2, 0x0a /* Public */,
      17,    2,   76,    2, 0x0a /* Public */,
      18,    1,   81,    2, 0x0a /* Public */,
      19,    1,   84,    2, 0x08 /* Private */,
      22,    1,   87,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 3,    4,    5,

 // slots: parameters
    QMetaType::Bool, 0x80000000 | 7, 0x80000000 | 3,    8,    9,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 15,   16,
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 3,    4,    5,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 20,   21,
    QMetaType::Void, 0x80000000 | 23,   24,

       0        // eod
};

void EditorPropertyEditorWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        EditorPropertyEditorWidget *_t = static_cast<EditorPropertyEditorWidget *>(_o);
        switch (_id) {
        case 0: _t->OnPropertyValueChange((*reinterpret_cast< const char*(*)>(_a[1])),(*reinterpret_cast< const char*(*)>(_a[2]))); break;
        case 1: { bool _r = _t->AddProperty((*reinterpret_cast< const PropertyTemplateProperty*(*)>(_a[1])),(*reinterpret_cast< const char*(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 2: _t->AddPropertiesFromTemplate((*reinterpret_cast< const PropertyTemplate*(*)>(_a[1]))); break;
        case 3: _t->ClearProperties(); break;
        case 4: _t->SetDestinationPropertyTable((*reinterpret_cast< PropertyTable*(*)>(_a[1]))); break;
        case 5: _t->UpdatePropertyValue((*reinterpret_cast< const char*(*)>(_a[1])),(*reinterpret_cast< const char*(*)>(_a[2]))); break;
        case 6: _t->UpdatePropertyValueFromTable((*reinterpret_cast< const char*(*)>(_a[1]))); break;
        case 7: _t->OnTreeBrowserCurrentItemChanged((*reinterpret_cast< QtBrowserItem*(*)>(_a[1]))); break;
        case 8: _t->OnPropertyManagerPropertyChanged((*reinterpret_cast< QtProperty*(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (EditorPropertyEditorWidget::*_t)(const char * , const char * );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EditorPropertyEditorWidget::OnPropertyValueChange)) {
                *result = 0;
            }
        }
    }
}

const QMetaObject EditorPropertyEditorWidget::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_EditorPropertyEditorWidget.data,
      qt_meta_data_EditorPropertyEditorWidget,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *EditorPropertyEditorWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EditorPropertyEditorWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_EditorPropertyEditorWidget.stringdata))
        return static_cast<void*>(const_cast< EditorPropertyEditorWidget*>(this));
    return QWidget::qt_metacast(_clname);
}

int EditorPropertyEditorWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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

// SIGNAL 0
void EditorPropertyEditorWidget::OnPropertyValueChange(const char * _t1, const char * _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
struct qt_meta_stringdata_EditorPropertyEditorResourcePropertyManager_t {
    QByteArrayData data[10];
    char stringdata[153];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EditorPropertyEditorResourcePropertyManager_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EditorPropertyEditorResourcePropertyManager_t qt_meta_stringdata_EditorPropertyEditorResourcePropertyManager = {
    {
QT_MOC_LITERAL(0, 0, 43), // "EditorPropertyEditorResourceP..."
QT_MOC_LITERAL(1, 44, 12), // "valueChanged"
QT_MOC_LITERAL(2, 57, 0), // ""
QT_MOC_LITERAL(3, 58, 11), // "QtProperty*"
QT_MOC_LITERAL(4, 70, 9), // "pProperty"
QT_MOC_LITERAL(5, 80, 5), // "value"
QT_MOC_LITERAL(6, 86, 8), // "setValue"
QT_MOC_LITERAL(7, 95, 15), // "setResourceType"
QT_MOC_LITERAL(8, 111, 23), // "const ResourceTypeInfo*"
QT_MOC_LITERAL(9, 135, 17) // "pResourceTypeInfo"

    },
    "EditorPropertyEditorResourcePropertyManager\0"
    "valueChanged\0\0QtProperty*\0pProperty\0"
    "value\0setValue\0setResourceType\0"
    "const ResourceTypeInfo*\0pResourceTypeInfo"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EditorPropertyEditorResourcePropertyManager[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   29,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    2,   34,    2, 0x0a /* Public */,
       7,    2,   39,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, QMetaType::QString,    4,    5,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3, QMetaType::QString,    4,    5,
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 8,    4,    9,

       0        // eod
};

void EditorPropertyEditorResourcePropertyManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        EditorPropertyEditorResourcePropertyManager *_t = static_cast<EditorPropertyEditorResourcePropertyManager *>(_o);
        switch (_id) {
        case 0: _t->valueChanged((*reinterpret_cast< QtProperty*(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 1: _t->setValue((*reinterpret_cast< QtProperty*(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 2: _t->setResourceType((*reinterpret_cast< QtProperty*(*)>(_a[1])),(*reinterpret_cast< const ResourceTypeInfo*(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (EditorPropertyEditorResourcePropertyManager::*_t)(QtProperty * , const QString & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EditorPropertyEditorResourcePropertyManager::valueChanged)) {
                *result = 0;
            }
        }
    }
}

const QMetaObject EditorPropertyEditorResourcePropertyManager::staticMetaObject = {
    { &QtAbstractPropertyManager::staticMetaObject, qt_meta_stringdata_EditorPropertyEditorResourcePropertyManager.data,
      qt_meta_data_EditorPropertyEditorResourcePropertyManager,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *EditorPropertyEditorResourcePropertyManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EditorPropertyEditorResourcePropertyManager::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_EditorPropertyEditorResourcePropertyManager.stringdata))
        return static_cast<void*>(const_cast< EditorPropertyEditorResourcePropertyManager*>(this));
    return QtAbstractPropertyManager::qt_metacast(_clname);
}

int EditorPropertyEditorResourcePropertyManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QtAbstractPropertyManager::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void EditorPropertyEditorResourcePropertyManager::valueChanged(QtProperty * _t1, const QString & _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
struct qt_meta_stringdata_EditorPropertyEditorResourceEditorFactory_t {
    QByteArrayData data[9];
    char stringdata[146];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EditorPropertyEditorResourceEditorFactory_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EditorPropertyEditorResourceEditorFactory_t qt_meta_stringdata_EditorPropertyEditorResourceEditorFactory = {
    {
QT_MOC_LITERAL(0, 0, 41), // "EditorPropertyEditorResourceE..."
QT_MOC_LITERAL(1, 42, 23), // "slotManagerValueChanged"
QT_MOC_LITERAL(2, 66, 0), // ""
QT_MOC_LITERAL(3, 67, 11), // "QtProperty*"
QT_MOC_LITERAL(4, 79, 9), // "pProperty"
QT_MOC_LITERAL(5, 89, 5), // "value"
QT_MOC_LITERAL(6, 95, 22), // "slotEditorValueChanged"
QT_MOC_LITERAL(7, 118, 19), // "slotEditorDestroyed"
QT_MOC_LITERAL(8, 138, 7) // "pEditor"

    },
    "EditorPropertyEditorResourceEditorFactory\0"
    "slotManagerValueChanged\0\0QtProperty*\0"
    "pProperty\0value\0slotEditorValueChanged\0"
    "slotEditorDestroyed\0pEditor"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EditorPropertyEditorResourceEditorFactory[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    2,   29,    2, 0x08 /* Private */,
       6,    1,   34,    2, 0x08 /* Private */,
       7,    1,   37,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3, QMetaType::QString,    4,    5,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QObjectStar,    8,

       0        // eod
};

void EditorPropertyEditorResourceEditorFactory::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        EditorPropertyEditorResourceEditorFactory *_t = static_cast<EditorPropertyEditorResourceEditorFactory *>(_o);
        switch (_id) {
        case 0: _t->slotManagerValueChanged((*reinterpret_cast< QtProperty*(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 1: _t->slotEditorValueChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->slotEditorDestroyed((*reinterpret_cast< QObject*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject EditorPropertyEditorResourceEditorFactory::staticMetaObject = {
    { &QtAbstractEditorFactory<EditorPropertyEditorResourcePropertyManager>::staticMetaObject, qt_meta_stringdata_EditorPropertyEditorResourceEditorFactory.data,
      qt_meta_data_EditorPropertyEditorResourceEditorFactory,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *EditorPropertyEditorResourceEditorFactory::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EditorPropertyEditorResourceEditorFactory::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_EditorPropertyEditorResourceEditorFactory.stringdata))
        return static_cast<void*>(const_cast< EditorPropertyEditorResourceEditorFactory*>(this));
    return QtAbstractEditorFactory<EditorPropertyEditorResourcePropertyManager>::qt_metacast(_clname);
}

int EditorPropertyEditorResourceEditorFactory::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QtAbstractEditorFactory<EditorPropertyEditorResourcePropertyManager>::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}
struct qt_meta_stringdata_EditorPropertyEditorVectorPropertyManager_t {
    QByteArrayData data[13];
    char stringdata[169];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EditorPropertyEditorVectorPropertyManager_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EditorPropertyEditorVectorPropertyManager_t qt_meta_stringdata_EditorPropertyEditorVectorPropertyManager = {
    {
QT_MOC_LITERAL(0, 0, 41), // "EditorPropertyEditorVectorPro..."
QT_MOC_LITERAL(1, 42, 12), // "valueChanged"
QT_MOC_LITERAL(2, 55, 0), // ""
QT_MOC_LITERAL(3, 56, 11), // "QtProperty*"
QT_MOC_LITERAL(4, 68, 9), // "pProperty"
QT_MOC_LITERAL(5, 78, 6), // "float4"
QT_MOC_LITERAL(6, 85, 5), // "value"
QT_MOC_LITERAL(7, 91, 16), // "setNumComponents"
QT_MOC_LITERAL(8, 108, 6), // "uint32"
QT_MOC_LITERAL(9, 115, 11), // "nComponents"
QT_MOC_LITERAL(10, 127, 14), // "setValueString"
QT_MOC_LITERAL(11, 142, 11), // "const char*"
QT_MOC_LITERAL(12, 154, 14) // "setValueVector"

    },
    "EditorPropertyEditorVectorPropertyManager\0"
    "valueChanged\0\0QtProperty*\0pProperty\0"
    "float4\0value\0setNumComponents\0uint32\0"
    "nComponents\0setValueString\0const char*\0"
    "setValueVector"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EditorPropertyEditorVectorPropertyManager[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   34,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    2,   39,    2, 0x0a /* Public */,
      10,    2,   44,    2, 0x0a /* Public */,
      12,    2,   49,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 5,    4,    6,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 8,    4,    9,
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 11,    4,    6,
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 5,    4,    6,

       0        // eod
};

void EditorPropertyEditorVectorPropertyManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        EditorPropertyEditorVectorPropertyManager *_t = static_cast<EditorPropertyEditorVectorPropertyManager *>(_o);
        switch (_id) {
        case 0: _t->valueChanged((*reinterpret_cast< QtProperty*(*)>(_a[1])),(*reinterpret_cast< const float4(*)>(_a[2]))); break;
        case 1: _t->setNumComponents((*reinterpret_cast< QtProperty*(*)>(_a[1])),(*reinterpret_cast< uint32(*)>(_a[2]))); break;
        case 2: _t->setValueString((*reinterpret_cast< QtProperty*(*)>(_a[1])),(*reinterpret_cast< const char*(*)>(_a[2]))); break;
        case 3: _t->setValueVector((*reinterpret_cast< QtProperty*(*)>(_a[1])),(*reinterpret_cast< const float4(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (EditorPropertyEditorVectorPropertyManager::*_t)(QtProperty * , const float4 & );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&EditorPropertyEditorVectorPropertyManager::valueChanged)) {
                *result = 0;
            }
        }
    }
}

const QMetaObject EditorPropertyEditorVectorPropertyManager::staticMetaObject = {
    { &QtAbstractPropertyManager::staticMetaObject, qt_meta_stringdata_EditorPropertyEditorVectorPropertyManager.data,
      qt_meta_data_EditorPropertyEditorVectorPropertyManager,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *EditorPropertyEditorVectorPropertyManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EditorPropertyEditorVectorPropertyManager::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_EditorPropertyEditorVectorPropertyManager.stringdata))
        return static_cast<void*>(const_cast< EditorPropertyEditorVectorPropertyManager*>(this));
    return QtAbstractPropertyManager::qt_metacast(_clname);
}

int EditorPropertyEditorVectorPropertyManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QtAbstractPropertyManager::qt_metacall(_c, _id, _a);
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
void EditorPropertyEditorVectorPropertyManager::valueChanged(QtProperty * _t1, const float4 & _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
struct qt_meta_stringdata_EditorPropertyEditorVectorEditFactory_t {
    QByteArrayData data[10];
    char stringdata[149];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_EditorPropertyEditorVectorEditFactory_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_EditorPropertyEditorVectorEditFactory_t qt_meta_stringdata_EditorPropertyEditorVectorEditFactory = {
    {
QT_MOC_LITERAL(0, 0, 37), // "EditorPropertyEditorVectorEdi..."
QT_MOC_LITERAL(1, 38, 23), // "slotManagerValueChanged"
QT_MOC_LITERAL(2, 62, 0), // ""
QT_MOC_LITERAL(3, 63, 11), // "QtProperty*"
QT_MOC_LITERAL(4, 75, 9), // "pProperty"
QT_MOC_LITERAL(5, 85, 6), // "float4"
QT_MOC_LITERAL(6, 92, 5), // "value"
QT_MOC_LITERAL(7, 98, 22), // "slotEditorValueChanged"
QT_MOC_LITERAL(8, 121, 19), // "slotEditorDestroyed"
QT_MOC_LITERAL(9, 141, 7) // "pEditor"

    },
    "EditorPropertyEditorVectorEditFactory\0"
    "slotManagerValueChanged\0\0QtProperty*\0"
    "pProperty\0float4\0value\0slotEditorValueChanged\0"
    "slotEditorDestroyed\0pEditor"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_EditorPropertyEditorVectorEditFactory[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    2,   29,    2, 0x08 /* Private */,
       7,    1,   34,    2, 0x08 /* Private */,
       8,    1,   37,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 5,    4,    6,
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void, QMetaType::QObjectStar,    9,

       0        // eod
};

void EditorPropertyEditorVectorEditFactory::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        EditorPropertyEditorVectorEditFactory *_t = static_cast<EditorPropertyEditorVectorEditFactory *>(_o);
        switch (_id) {
        case 0: _t->slotManagerValueChanged((*reinterpret_cast< QtProperty*(*)>(_a[1])),(*reinterpret_cast< const float4(*)>(_a[2]))); break;
        case 1: _t->slotEditorValueChanged((*reinterpret_cast< const float4(*)>(_a[1]))); break;
        case 2: _t->slotEditorDestroyed((*reinterpret_cast< QObject*(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject EditorPropertyEditorVectorEditFactory::staticMetaObject = {
    { &QtAbstractEditorFactory<EditorPropertyEditorVectorPropertyManager>::staticMetaObject, qt_meta_stringdata_EditorPropertyEditorVectorEditFactory.data,
      qt_meta_data_EditorPropertyEditorVectorEditFactory,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *EditorPropertyEditorVectorEditFactory::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *EditorPropertyEditorVectorEditFactory::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_EditorPropertyEditorVectorEditFactory.stringdata))
        return static_cast<void*>(const_cast< EditorPropertyEditorVectorEditFactory*>(this));
    return QtAbstractEditorFactory<EditorPropertyEditorVectorPropertyManager>::qt_metacast(_clname);
}

int EditorPropertyEditorVectorEditFactory::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QtAbstractEditorFactory<EditorPropertyEditorVectorPropertyManager>::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
