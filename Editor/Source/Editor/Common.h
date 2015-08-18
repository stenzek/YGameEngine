#pragma once

#include <QtCore/QVariant>

// engine includes
#include "Engine/Common.h"
#include "Engine/Engine.h"
#include "Renderer/RendererTypes.h"

// disable warnings that qt files inflict
#if Y_COMPILER_MSVC
    #pragma warning(push)
    #pragma warning(disable: 4127)      // warning C4127: conditional expression is constant
    #pragma warning(disable: 4512)      // warning C4512: 'QtPrivate::QSlotObjectBase' : assignment operator could not be generated
    #pragma warning(disable: 4389)      // warning C4389: '==' : signed/unsigned mismatch
#endif

// qt includes
#define QT_NO_SIGNALS_SLOTS_KEYWORDS 1
#define QT_NO_EMIT 1
#include <QtCore/QEvent>
#include <QtCore/QStack>
#include <QtCore/QTimer>
#include <QtCore/QVariant>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QStandardItemModel>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QColumnView>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListView>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSlider>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStackedLayout>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTimeEdit>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QWidgetAction>

// property browser
#include <QtPropertyBrowser/qtpropertymanager.h>
#include <QtPropertyBrowser/qtpropertybrowser.h>
#include <QtPropertyBrowser/qttreepropertybrowser.h>
#include <QtPropertyBrowser/qteditorfactory.h>
#include <QtPropertyBrowser/qtvariantproperty.h>

#if Y_COMPILER_MSVC
    #pragma warning(pop)
#endif

//#if !defined(Q_NO_TEMPLATE_FRIENDS) && !defined(Q_CC_MSVC)
//#error moo
//#endif

// signal blocker helper
#include "SignalBlocker.h"

// editor includes
#include "Editor/Defines.h"

// flow layout
#include "Editor/FlowLayout.h"
