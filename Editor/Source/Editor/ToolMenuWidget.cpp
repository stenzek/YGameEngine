#include "Editor/PrecompiledHeader.h"
#include "Editor/ToolMenuWidget.h"

ToolMenuWidget::ToolMenuWidget(QWidget *pParent, Qt::Orientation orientation /* = Qt::Vertical */, bool showLabels /* = true */)
    : m_orientation(orientation), 
      m_showLabels(showLabels),
      m_iconSize(16, 16),
      m_buttonMargins(4)
{
    if (orientation == Qt::Horizontal)
    {
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        m_layout = new QHBoxLayout(this);
    }
    else
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        m_layout = new QVBoxLayout(this);
    }

    m_layout->setSizeConstraint(QLayout::SetMaximumSize);
    m_layout->setSpacing(0);
    m_layout->setMargin(0);
}

ToolMenuWidget::~ToolMenuWidget()
{

}

void ToolMenuWidget::recreateLayout()
{
    QBoxLayout *newLayout;
    if (m_orientation == Qt::Horizontal)
    {
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        newLayout = new QHBoxLayout(this);
    }
    else
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        newLayout = new QVBoxLayout(this);
    }

    newLayout->setSizeConstraint(QLayout::SetMaximumSize);
    newLayout->setSpacing(0);
    newLayout->setMargin(0);

    for (Item &item : m_items)
    {
        m_layout->removeWidget(item.pWidget);

        if (item.pAction != nullptr)
        {
            delete item.pWidget;
            item.pWidget = createToolButtonForAction(item.pAction);
        }

        newLayout->addWidget(item.pWidget);
    }

    delete m_layout;
    m_layout = newLayout;
}

void ToolMenuWidget::clear()
{
    for (Item &item : m_items)
    {
        m_layout->removeWidget(item.pWidget);
        if (item.pAction != nullptr)
        {
            delete item.pWidget;

            if (item.pAction->parent() == this)
                delete item.pAction;
        }
    }
    m_items.clear();
}

void ToolMenuWidget::addAction(QAction *pAction)
{
    Item *pExistingItem = findItemForAction(pAction);
    if (pExistingItem != nullptr)
        removeAction(pAction);

    if (pAction->isSeparator())
    {
        QFrame *line = new QFrame(this);
        line->setFrameShape(QFrame::VLine);
        line->setFrameShadow(QFrame::Raised);
        //line->setFrameShadow(QFrame::Sunken);
        Item item;
        item.pAction = pAction;
        item.pWidget = line;
        m_items.append(item);
        m_layout->addWidget(line);
    }
    else
    {
        QToolButton *pToolButton = createToolButtonForAction(pAction);
        Item item;
        item.pAction = pAction;
        item.pWidget = pToolButton;
        m_items.append(item);
        m_layout->addWidget(pToolButton);
    }
}

void ToolMenuWidget::addWidget(QWidget *pWidget)
{
    Item *pExistingItem = findItemForWidget(pWidget);
    if (pExistingItem != nullptr)
        removeWidget(pWidget);
   
    Item item;
    item.pAction = nullptr;
    item.pWidget = pWidget;
    m_items.append(item);
    m_layout->addWidget(pWidget);
}

void ToolMenuWidget::addSeperator()
{
    QAction *action = new QAction(this);
    action->setSeparator(true);
    addAction(action);
}

void ToolMenuWidget::removeAction(QAction *pAction)
{
    for (int i = 0; i < m_items.size(); i++)
    {
        if (m_items[i].pAction == pAction)
        {
            m_layout->removeWidget(m_items[i].pWidget);
            m_items.removeAt(i);
            return;
        }
    }
}

void ToolMenuWidget::removeWidget(QWidget *pWidget)
{
    for (int i = 0; i < m_items.size(); i++)
    {
        if (m_items[i].pWidget == pWidget)
        {
            m_layout->removeWidget(pWidget);
            m_items.removeAt(i);
            return;
        }
    }
}

QToolButton *ToolMenuWidget::createToolButtonForAction(QAction *pAction)
{
    QToolButton *pToolButton = new QToolButton(this);

    if (m_orientation == Qt::Horizontal)
    {
        pToolButton->setToolButtonStyle((m_showLabels) ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly);
        pToolButton->setSizePolicy((m_showLabels) ? QSizePolicy::Expanding : QSizePolicy::Fixed, (m_showLabels) ? QSizePolicy::Expanding : QSizePolicy::Fixed);
        //pToolButton->setMinimumSize(QSize(m_iconSize.width() + m_buttonMargins * 2, m_iconSize.height() + m_buttonMargins * 2));
        pToolButton->setMaximumSize(QSize(m_iconSize.width() + m_buttonMargins * 2, 16777215));
        pToolButton->setFixedSize(QSize(m_iconSize.width() + m_buttonMargins * 2, m_iconSize.height() + m_buttonMargins * 2));
    }
    else
    {
        pToolButton->setToolButtonStyle((m_showLabels) ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
        pToolButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        pToolButton->setMaximumSize(QSize(16777215, m_iconSize.height() + m_buttonMargins * 2));
        pToolButton->setFixedSize(QSize(16777215, m_iconSize.height() + m_buttonMargins * 2));
    }

    pToolButton->setAutoRaise(true);

//     pToolButton->setText(pAction->text());
//     pToolButton->setIcon(pAction->icon());
//     pToolButton->setCheckable(pAction->isCheckable());
//     pToolButton->setChecked(pAction->isChecked());
    pToolButton->setDefaultAction(pAction);
    
    //connect(pToolButton, SIGNAL(clicked(bool)), this, SLOT(OnToolButtonClicked(bool)));
    return pToolButton;
}

ToolMenuWidget::Item *ToolMenuWidget::findItemForAction(QAction *pAction)
{
    for (Item &item : m_items)
    {
        if (item.pAction == pAction)
            return &item;
    }

    return nullptr;
}

ToolMenuWidget::Item *ToolMenuWidget::findItemForWidget(QWidget *pWidget)
{
    for (Item &item : m_items)
    {
        if (item.pWidget == pWidget)
            return &item;
    }

    return nullptr;
}

void ToolMenuWidget::OnToolButtonClicked(bool checked)
{
    QObject *signalSender = sender();
    if (signalSender == nullptr)
        return;

    for (Item &item : m_items)
    {
        if (item.pWidget == signalSender)
        {
            if (item.pAction->isCheckable())
                item.pAction->setChecked(checked);
            
            item.pAction->triggered(checked);
            return;
        }
    }
}
