#include "componentguiclass.h"
#include <iostream>
#include "graphicsrectitem.h"
#include "graphicsconnectoritem.h"
#include <ostream>

ComponentGuiClass::ComponentGuiClass(const QString &fileName, QString componentName,QPoint position, QGraphicsView *parentView, QGraphicsItem *parent)
        : QGraphicsWidget(parent)
{
    setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);

    //widget = new QWidget;

    mParentView = parentView;

    QGraphicsSvgItem *icon = new QGraphicsSvgItem(fileName,this);
    icon->setPos(QPointF(-icon->boundingRect().width()/2, -icon->boundingRect().height()/2));
    std::cout << "x=" << this->pos().x() << "  " << "y=" << this->pos().y() << std::endl;
    std::cout << componentName.toStdString() << std::endl;

    QGraphicsTextItem *text = new QGraphicsTextItem(componentName,this);
    text->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
    text->setPos(QPointF(-text->boundingRect().width()/2, icon->boundingRect().height()/2));
    text->setTextInteractionFlags(Qt::TextEditable);

    GraphicsRectItem *rectR = new GraphicsRectItem(icon->sceneBoundingRect().width()-5,icon->sceneBoundingRect().height()/2-5,10.0,10.0,this->getParentView(),this,icon);

    GraphicsRectItem *rectL = new GraphicsRectItem(-5,icon->sceneBoundingRect().height()/2-5,10.0,10.0,this->getParentView(),this,icon);

    //icon->setPos(QPointF(-icon->boundingRect().width()/2, -icon->boundingRect().height()/2));

   // rectR->boundingRegion();
}

ComponentGuiClass::~ComponentGuiClass()
{
    //delete widget;
}


QGraphicsView *ComponentGuiClass::getParentView()
{
    return mParentView;
}

void ComponentGuiClass::addConnector(GraphicsConnectorItem *item)
{
    mConnectors.push_back(item);
    connect(this,SIGNAL(componentMoved()),mConnectors.back(),SLOT(updatePos()));
    QColor color = QColor("black");
    mConnectors.back()->setPen(QPen(color, 2));
}

//void ComponentGuiClass::moveEvent(QMoveEvent *event)
//{
//    emit componentMoved();
//    if (!mConnectors.empty())
//    {
//        QColor color = QColor("blue");
//        mConnectors.back()->setPen(QPen(color, 2));
//    }
//}


void ComponentGuiClass::moveEvent(QGraphicsItem::GraphicsItemChange *change)
{
    if (*change == QGraphicsItem::ItemPositionChange)
    {
        emit componentMoved();
        if (!mConnectors.empty())
        {
            QColor color = QColor("blue");
            mConnectors.back()->setPen(QPen(color, 2));
        }
    }
}
