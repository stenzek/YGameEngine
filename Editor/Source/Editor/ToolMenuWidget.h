#pragma once
#include "Editor/Common.h"

class ToolMenuWidget : public QWidget
{
    Q_OBJECT

public:
    ToolMenuWidget(QWidget *pParent, Qt::Orientation orientation = Qt::Vertical, bool showLabels = true);
    virtual ~ToolMenuWidget();

    const Qt::Orientation getOrientation() const { return m_orientation; }
    void setOrientation(Qt::Orientation orientation) { m_orientation = orientation; recreateLayout(); }

    const bool showLabels() const { return m_showLabels; }
    void setShowLabels(bool showLabels) { m_showLabels = showLabels; recreateLayout(); }

    const int buttonMargins() const { return m_buttonMargins; }
    void setButtonMargins(int buttonMargins) { m_buttonMargins = buttonMargins; recreateLayout(); }

    void clear();
    void addAction(QAction *pAction);
    void addWidget(QWidget *pWidget);
    void addSeperator();
    void removeAction(QAction *pAction);
    void removeWidget(QWidget *pWidget);

private:
    struct Item
    {
        QAction *pAction;
        QWidget *pWidget;
    };

    QToolButton *createToolButtonForAction(QAction *pAction);
    Item *findItemForAction(QAction *pAction);
    Item *findItemForWidget(QWidget *pWidget);
    void recreateLayout();

    Qt::Orientation m_orientation;
    bool m_showLabels;

    QList<Item> m_items;
    QBoxLayout *m_layout;
    QSize m_iconSize;
    int m_buttonMargins;

private Q_SLOTS:
    void OnToolButtonClicked(bool checked);
};

