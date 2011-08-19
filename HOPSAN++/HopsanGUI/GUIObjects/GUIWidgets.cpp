/*-----------------------------------------------------------------------------
 This source file is part of Hopsan NG

 Copyright (c) 2011 
    Mikael Axin, Robert Braun, Alessandro Dell'Amico, Björn Eriksson,
    Peter Nordin, Karl Pettersson, Petter Krus, Ingo Staack

 This file is provided "as is", with no guarantee or warranty for the
 functionality or reliability of the contents. All contents in this file is
 the original work of the copyright holders at the Division of Fluid and
 Mechatronic Systems (Flumes) at Linköping University. Modifying, using or
 redistributing any part of this file is prohibited without explicit
 permission from the copyright holders.
-----------------------------------------------------------------------------*/

//!
//! @file   GUIWidgets.cpp
//! @author Flumes <flumes@lists.iei.liu.se>
//! @date   2010-01-01
//!
//! @brief Contains the GUIWidgets classes
//!
//$Id$

#include "../common.h"

#include "GUIWidgets.h"
#include "GUISystem.h"
#include "../Widgets/ProjectTabWidget.h"
#include "../MainWindow.h"
#include "../UndoStack.h"

#include <QLabel>
#include <QDialog>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QFontDialog>
#include <QColorDialog>
#include <QGroupBox>
#include <QSpinBox>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsWidget>
#include <QComboBox>


using namespace std;


GUIWidget::GUIWidget(QPointF pos, qreal rot, selectionStatus startSelected, GUIContainerObject *pSystem, QGraphicsItem *pParent)
    : GUIObject(pos, rot, startSelected, pSystem, pParent)
{
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    pSystem->getContainedScenePtr()->addItem(this);
    this->setPos(pos);
    mIsResizing = false;        //Only used for resizable widgets
}


void GUIWidget::setOldPos()
{
    mOldPos = this->pos();
}


int GUIWidget::getWidgetIndex()
{
    return mWidgetIndex;
}

QVariant GUIWidget::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if(change == QGraphicsItem::ItemSelectedHasChanged)
    {
        if(this->isSelected())
        {
            mpParentContainerObject->rememberSelectedWidget(this);
        }
        else
        {
            mpParentContainerObject->forgetSelectedWidget(this);
        }
    }

    return GUIObject::itemChange(change, value);
}


void GUIWidget::deleteMe(undoStatus undoSettings)
{
    assert(1 == 2);
}


void GUIWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QList<GUIWidget *>::iterator it;

        //Loop through all selected widgets and register changed positions in undo stack
    bool alreadyClearedRedo = false;
    QList<GUIWidget *> selectedWidgets = mpParentContainerObject->getSelectedGUIWidgetPtrs();
    for(int i=0; i<selectedWidgets.size(); ++i)
    {
        if((selectedWidgets[i]->mOldPos != selectedWidgets[i]->pos()) && (event->button() == Qt::LeftButton) && !selectedWidgets[i]->mIsResizing)
        {
                //This check makes sure that only one undo post is created when moving several objects at once
            if(!alreadyClearedRedo)
            {
                if(mpParentContainerObject->getSelectedGUIWidgetPtrs().size() > 1)
                {
                    mpParentContainerObject->getUndoStackPtr()->newPost("movedmultiplewidgets");
                }
                else
                {
                    mpParentContainerObject->getUndoStackPtr()->newPost();
                }
                mpParentContainerObject->mpParentProjectTab->hasChanged();
                alreadyClearedRedo = true;
            }

            mpParentContainerObject->getUndoStackPtr()->registerMovedWidget(selectedWidgets[i], selectedWidgets[i]->mOldPos, selectedWidgets[i]->pos());
        }
        selectedWidgets[i]->mIsResizing = false;
    }

    GUIObject::mouseReleaseEvent(event);
}



///////////////////////////////////////////////////////////////////////////////////////
GUITextBoxWidget::GUITextBoxWidget(QString text, QPointF pos, qreal rot, selectionStatus startSelected, GUIContainerObject *pSystem, size_t widgetIndex, QGraphicsItem *pParent)
    : GUIWidget(pos, rot, startSelected, pSystem, pParent)
{
    this->mHmfTagName = HMF_TEXTBOXWIDGETTAG;

    mpRectItem = new QGraphicsRectItem(0, 0, 100, 100, this);
    QPen tempPen = mpRectItem->pen();
    tempPen.setWidth(2);
    tempPen.setStyle(Qt::SolidLine);//Qt::DotLine);
    tempPen.setCapStyle(Qt::RoundCap);
    tempPen.setJoinStyle(Qt::RoundJoin);
    mpRectItem->setPen(tempPen);
    mpRectItem->setPos(mpRectItem->pen().width()/2.0, mpRectItem->pen().width()/2.0);
    mpRectItem->show();

    mpTextItem = new QGraphicsTextItem(text, this);
    QFont tempFont = mpTextItem->font();
    tempFont.setPointSize(12);
    mpTextItem->setFont(tempFont);
    mpTextItem->setPos(this->boundingRect().center());
    mpTextItem->show();
    mpTextItem->setAcceptHoverEvents(false);

    this->setColor(QColor("darkolivegreen"));

    this->resize(mpTextItem->boundingRect().width(), mpTextItem->boundingRect().height());
    mpSelectionBox->setSize(0.0, 0.0, max(mpTextItem->boundingRect().width(), mpRectItem->boundingRect().width()),
                                      max(mpTextItem->boundingRect().height(), mpRectItem->boundingRect().height()));

    this->resize(max(mpTextItem->boundingRect().width(), mpRectItem->boundingRect().width()),
                 max(mpTextItem->boundingRect().height(), mpRectItem->boundingRect().height()));

      this->setFlag(QGraphicsItem::ItemAcceptsInputMethod, true);

    mWidthBeforeResize = mpRectItem->rect().width();
    mHeightBeforeResize = mpRectItem->rect().height();
    mPosBeforeResize = this->pos();

    mWidgetIndex = widgetIndex;
}

void GUITextBoxWidget::saveToDomElement(QDomElement &rDomElement)
{
    //! @todo Implement
}

void GUITextBoxWidget::setText(QString text)
{
    mpTextItem->setPlainText(text);
    if(mpTextItem->boundingRect().intersects(mpRectItem->boundingRect()))
    {
        mpRectItem->setRect(mpTextItem->boundingRect());
    }
    mpSelectionBox->setSize(0.0, 0.0, mpRectItem->boundingRect().width(), mpRectItem->boundingRect().height());
    mpSelectionBox->setPassive();
}


void GUITextBoxWidget::setFont(QFont font)
{
    mpTextItem->setFont(font);
    if(mpTextItem->boundingRect().intersects(mpRectItem->boundingRect()))
    {
        mpRectItem->setRect(mpTextItem->boundingRect());
    }
    mpSelectionBox->setSize(0.0, 0.0, mpRectItem->boundingRect().width(), mpRectItem->boundingRect().height());
    mpSelectionBox->setPassive();
    this->resize(mpRectItem->boundingRect().width(), mpRectItem->boundingRect().height());
}


void GUITextBoxWidget::setLineWidth(int value)
{
    QPen tempPen;
    tempPen = mpRectItem->pen();
    tempPen.setWidth(value);
    mpRectItem->setPen(tempPen);
}


void GUITextBoxWidget::setLineStyle(Qt::PenStyle style)
{
    QPen tempPen;
    tempPen = mpRectItem->pen();
    tempPen.setStyle(style);
    mpRectItem->setPen(tempPen);
}


void GUITextBoxWidget::setColor(QColor color)
{
    mpTextItem->setDefaultTextColor(color);

    QPen tempPen;
    tempPen = mpRectItem->pen();
    tempPen.setColor(color);
    mpRectItem->setPen(tempPen);
}

void GUITextBoxWidget::setSize(qreal w, qreal h)
{
    //! @todo Implement
}

void GUITextBoxWidget::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{

        //Open a dialog where line width and color can be selected
    mpEditDialog = new QDialog(gpMainWindow);
    mpEditDialog->setWindowTitle("Edit Box Widget");

    mpShowBoxCheckBoxInDialog = new QCheckBox("Show box rectangle");
    mpShowBoxCheckBoxInDialog->setChecked(mpRectItem->isVisible());

    mpWidthLabelInDialog = new QLabel("Line Width: ");
    mpWidthBoxInDialog = new QSpinBox();
    mpWidthBoxInDialog->setMinimum(1);
    mpWidthBoxInDialog->setMaximum(100);
    mpWidthBoxInDialog->setSingleStep(1);
    mpWidthBoxInDialog->setValue(mpRectItem->pen().width());
    mpColorLabelInDialog = new QLabel("Line Color: ");
    mpColorInDialogButton = new QToolButton();
    QString redString, greenString, blueString;
    redString.setNum(mpRectItem->pen().color().red());
    greenString.setNum(mpRectItem->pen().color().green());
    blueString.setNum(mpRectItem->pen().color().blue());
    mpColorInDialogButton->setStyleSheet(QString("* { background-color: rgb(" + redString + "," + greenString + "," + blueString + ") }"));
    mpColorInDialogButton->setAutoRaise(true);

    mpStyleLabelInDialog = new QLabel("Line Style: ");
    mpStyleBoxInDialog = new QComboBox();
    mpStyleBoxInDialog->insertItem(0, "Solid Line");
    mpStyleBoxInDialog->insertItem(1, "Dashes");
    mpStyleBoxInDialog->insertItem(2, "Dots");
    mpStyleBoxInDialog->insertItem(3, "Dashes and Dots");

    if(mpRectItem->pen().style() == Qt::SolidLine)
        mpStyleBoxInDialog->setCurrentIndex(0);
    if(mpRectItem->pen().style() == Qt::DashLine)
        mpStyleBoxInDialog->setCurrentIndex(1);
    if(mpRectItem->pen().style() == Qt::DotLine)
        mpStyleBoxInDialog->setCurrentIndex(2);
    if(mpRectItem->pen().style() == Qt::DashDotLine)
        mpStyleBoxInDialog->setCurrentIndex(3);

    mpTextBoxInDialog = new QTextEdit();
    mpTextBoxInDialog->setPlainText(mpTextItem->toPlainText());
    mpTextBoxInDialog->setTextColor(mSelectedColor);
    mpTextBoxInDialog->setFont(mSelectedFont);
    mpFontInDialogButton = new QPushButton("Change Font");

    QGridLayout *pEditGroupLayout = new QGridLayout();
    pEditGroupLayout->addWidget(mpTextBoxInDialog,          0,0,1,2);
    pEditGroupLayout->addWidget(mpFontInDialogButton,       1,0,1,2);
    pEditGroupLayout->addWidget(mpShowBoxCheckBoxInDialog,  2,0,1,2);
    pEditGroupLayout->addWidget(mpWidthLabelInDialog,       3,0);
    pEditGroupLayout->addWidget(mpWidthBoxInDialog,         3,1);
    pEditGroupLayout->addWidget(mpColorLabelInDialog,       4,0);
    pEditGroupLayout->addWidget(mpColorInDialogButton,      4,1);
    pEditGroupLayout->addWidget(mpStyleLabelInDialog,       5,0);
    pEditGroupLayout->addWidget(mpStyleBoxInDialog,         5,1);

    QGroupBox *pEditGroupBox = new QGroupBox();
    pEditGroupBox->setLayout(pEditGroupLayout);

    mSelectedFont = mpTextItem->font();
    mSelectedColor = mpTextItem->defaultTextColor();

    mpDoneInDialogButton = new QPushButton("Done");
    mpCancelInDialogButton = new QPushButton("Cancel");
    QDialogButtonBox *pButtonBox = new QDialogButtonBox(Qt::Horizontal);
    pButtonBox->addButton(mpDoneInDialogButton, QDialogButtonBox::ActionRole);
    pButtonBox->addButton(mpCancelInDialogButton, QDialogButtonBox::ActionRole);

    QGridLayout *pDialogLayout = new QGridLayout();
    pDialogLayout->addWidget(pEditGroupBox,0,0);
    pDialogLayout->addWidget(pButtonBox,1,0);
    mpEditDialog->setLayout(pDialogLayout);
    mpEditDialog->show();

    this->setZValue(0);
    this->setFlags(QGraphicsItem::ItemStacksBehindParent);

    mSelectedColor = mpRectItem->pen().color();

    connect(mpShowBoxCheckBoxInDialog, SIGNAL(toggled(bool)), mpWidthLabelInDialog, SLOT(setEnabled(bool)));
    connect(mpShowBoxCheckBoxInDialog, SIGNAL(toggled(bool)), mpWidthBoxInDialog, SLOT(setEnabled(bool)));
    connect(mpShowBoxCheckBoxInDialog, SIGNAL(toggled(bool)), mpStyleLabelInDialog, SLOT(setEnabled(bool)));
    connect(mpShowBoxCheckBoxInDialog, SIGNAL(toggled(bool)), mpStyleBoxInDialog, SLOT(setEnabled(bool)));
    connect(mpFontInDialogButton,SIGNAL(clicked()),this,SLOT(openFontDialog()));
    connect(mpColorInDialogButton,SIGNAL(clicked()),this,SLOT(openColorDialog()));
    connect(mpDoneInDialogButton,SIGNAL(clicked()),this,SLOT(updateWidgetFromDialog()));
    connect(mpCancelInDialogButton,SIGNAL(clicked()),mpEditDialog,SLOT(close()));
}

void GUITextBoxWidget::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    GUIObject::hoverMoveEvent(event);

    this->setCursor(Qt::ArrowCursor);
    mResizeLeft = false;
    mResizeRight = false;
    mResizeTop = false;
    mResizeBottom = false;

    int resLim = 5;

    if(event->pos().x() > boundingRect().left() && event->pos().x() < boundingRect().left()+resLim)
    {
        mResizeLeft = true;
    }
    if(event->pos().x() > boundingRect().right()-resLim && event->pos().x() < boundingRect().right())
    {
        mResizeRight = true;
    }
    if(event->pos().y() > boundingRect().top() && event->pos().y() < boundingRect().top()+resLim)
    {
        mResizeTop = true;
    }
    if(event->pos().y() > boundingRect().bottom()-resLim && event->pos().y() < boundingRect().bottom())
    {
        mResizeBottom = true;
    }

    if( (mResizeLeft && mResizeTop) || (mResizeRight && mResizeBottom) )
        this->setCursor(Qt::SizeFDiagCursor);
    else if( (mResizeTop && mResizeRight) || (mResizeBottom && mResizeLeft) )
        this->setCursor(Qt::SizeBDiagCursor);
    else if(mResizeLeft || mResizeRight)
        this->setCursor(Qt::SizeHorCursor);
    else if(mResizeTop || mResizeBottom)
        this->setCursor(Qt::SizeVerCursor);
}

void GUITextBoxWidget::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if(mResizeLeft || mResizeRight || mResizeTop || mResizeBottom)
    {
        mPosBeforeResize = this->pos();
        mWidthBeforeResize = this->mpRectItem->rect().width();
        mHeightBeforeResize = this->mpRectItem->rect().height();
        mIsResizing = true;
    }
    else
    {
        mIsResizing = false;
    }
    GUIObject::mousePressEvent(event);
}


void GUITextBoxWidget::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    GUIObject::mouseMoveEvent(event);

    if(mResizeLeft && mResizeTop)
    {
        QRectF desiredRect = QRectF(mpRectItem->rect().x(), mpRectItem->rect().y(), max(0.0, mWidthBeforeResize+mPosBeforeResize.x()-this->pos().x()), max(0.0, mHeightBeforeResize+mPosBeforeResize.y()-this->pos().y()));
        mpRectItem->setRect(desiredRect.united(mpTextItem->boundingRect()));
        if(desiredRect.width() < mpTextItem->boundingRect().width())
            this->setX(mPosBeforeResize.x()+mWidthBeforeResize-mpRectItem->boundingRect().width()+mpRectItem->pen().widthF());
        if(desiredRect.height() < mpTextItem->boundingRect().height())
            this->setY(mPosBeforeResize.y()+mHeightBeforeResize-mpRectItem->boundingRect().height()+mpRectItem->pen().widthF());
    }
    else if(mResizeTop && mResizeRight)
    {
        QRectF desiredRect = QRectF(mpRectItem->rect().x(), mpRectItem->rect().y(), max(0.0, mWidthBeforeResize-mPosBeforeResize.x()+this->pos().x()), max(0.0, mHeightBeforeResize+mPosBeforeResize.y()-this->pos().y()));
        mpRectItem->setRect(desiredRect.united(mpTextItem->boundingRect()));
        this->setX(mPosBeforeResize.x());
        if(desiredRect.height() < mpTextItem->boundingRect().height())
            this->setY(mPosBeforeResize.y()+mHeightBeforeResize-mpRectItem->boundingRect().height()+mpRectItem->pen().widthF());
    }
    else if(mResizeRight && mResizeBottom)
    {
        QRectF desiredRect = QRectF(mpRectItem->rect().x(), mpRectItem->rect().y(), max(0.0, mWidthBeforeResize-mPosBeforeResize.x()+this->pos().x()), max(0.0, mHeightBeforeResize-mPosBeforeResize.y()+this->pos().y()));
        mpRectItem->setRect(desiredRect.united(mpTextItem->boundingRect()));
        this->setX(mPosBeforeResize.x());
        this->setY(mPosBeforeResize.y());
    }
    else if(mResizeBottom && mResizeLeft)
    {
        QRectF desiredRect = QRectF(mpRectItem->rect().x(), mpRectItem->rect().y(), max(0.0, mWidthBeforeResize+mPosBeforeResize.x()-this->pos().x()), max(0.0, mHeightBeforeResize-mPosBeforeResize.y()+this->pos().y()));
        mpRectItem->setRect(desiredRect.united(mpTextItem->boundingRect()));
        this->setY(mPosBeforeResize.y());
        if(desiredRect.width() < mpTextItem->boundingRect().width())
            this->setX(mPosBeforeResize.x()+mWidthBeforeResize-mpRectItem->boundingRect().width()+mpRectItem->pen().widthF());
    }
    else if(mResizeLeft)
    {
        QRectF desiredRect = QRectF(mpRectItem->rect().x(), mpRectItem->rect().y(), max(0.0, mWidthBeforeResize+mPosBeforeResize.x()-this->pos().x()), mpRectItem->rect().height());
        mpRectItem->setRect(desiredRect.united(mpTextItem->boundingRect()));
        this->setY(mPosBeforeResize.y());
        if(desiredRect.width() < mpTextItem->boundingRect().width())
            this->setX(mPosBeforeResize.x()+mWidthBeforeResize-mpRectItem->boundingRect().width()+mpRectItem->pen().widthF());
    }
    else if(mResizeRight)
    {
        QRectF desiredRect = QRectF(mpRectItem->rect().x(), mpRectItem->rect().y(), max(0.0, mWidthBeforeResize-mPosBeforeResize.x()+this->pos().x()), mpRectItem->rect().height());
        mpRectItem->setRect(desiredRect.united(mpTextItem->boundingRect()));
        this->setPos(mPosBeforeResize);
    }
    else if(mResizeTop)
    {
        QRectF desiredRect = QRectF(mpRectItem->rect().x(), mpRectItem->rect().y(),  mpRectItem->rect().width(), max(0.0, mHeightBeforeResize+mPosBeforeResize.y()-this->pos().y()));
        mpRectItem->setRect(desiredRect.united(mpTextItem->boundingRect()));
        this->setX(mPosBeforeResize.x());
        if(desiredRect.height() < mpTextItem->boundingRect().height())
            this->setY(mPosBeforeResize.y()+mHeightBeforeResize-mpRectItem->boundingRect().height()+mpRectItem->pen().widthF());
    }
    else if(mResizeBottom)
    {
        QRectF desiredRect = QRectF(mpRectItem->rect().x(), mpRectItem->rect().y(), mpRectItem->rect().width(), max(0.0, mHeightBeforeResize-mPosBeforeResize.y()+this->pos().y()));
        mpRectItem->setRect(desiredRect.united(mpTextItem->boundingRect()));
        this->setPos(mPosBeforeResize);
    }

    mpSelectionBox->setSize(0.0, 0.0, mpRectItem->boundingRect().width(), mpRectItem->boundingRect().height());

    mpSelectionBox->setActive();
    this->resize(mpRectItem->boundingRect().width(), mpRectItem->boundingRect().height());
    mpRectItem->setPos(mpRectItem->pen().width()/2.0, mpRectItem->pen().width()/2.0);
}


void GUITextBoxWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    GUIWidget::mouseReleaseEvent(event);
    if(mWidthBeforeResize != mpRectItem->rect().width() || mHeightBeforeResize != mpRectItem->rect().height())
    {
        mpParentContainerObject->getUndoStackPtr()->newPost();
        mpParentContainerObject->getUndoStackPtr()->registerResizedBoxWidget( mWidgetIndex, mWidthBeforeResize, mHeightBeforeResize, mpRectItem->rect().width(), mpRectItem->rect().height(), mPosBeforeResize, this->pos());
        mWidthBeforeResize = mpRectItem->rect().width();
        mHeightBeforeResize = mpRectItem->rect().height();
        mPosBeforeResize = this->pos();
    }
}

void GUITextBoxWidget::deleteMe(undoStatus undoSettings)
{
    mpParentContainerObject->removeWidget(this, undoSettings);
}

void GUITextBoxWidget::updateWidgetFromDialog()
{
    //Update text
    //mpParentContainerObject->getUndoStackPtr()->newPost();
    //mpParentContainerObject->getUndoStackPtr()->registerModifiedTextWidget(mWidgetIndex, mpTextItem->toPlainText(), mpTextItem->font(), mpTextItem->defaultTextColor(), mpTextBox->toPlainText(), mSelectedFont, mSelectedColor);

    mpTextItem->setPlainText(mpTextBoxInDialog->toPlainText());
    mpTextItem->setFont(mSelectedFont);
    mpTextItem->setDefaultTextColor(mSelectedColor);

    this->resize(mpTextItem->boundingRect().width(), mpTextItem->boundingRect().height());


    //Update box
    mpRectItem->setVisible(mpShowBoxCheckBoxInDialog->isChecked());
    Qt::PenStyle selectedStyle;
    if(mpStyleBoxInDialog->currentIndex() == 0)
        selectedStyle = Qt::SolidLine;
    else if(mpStyleBoxInDialog->currentIndex() == 1)
        selectedStyle = Qt::DashLine;
    else if(mpStyleBoxInDialog->currentIndex() == 2)
        selectedStyle = Qt::DotLine;
    else if(mpStyleBoxInDialog->currentIndex() == 3)
        selectedStyle = Qt::DashDotLine;
    else
        selectedStyle = Qt::SolidLine;      //This shall never happen, but will supress a warning message

    mpParentContainerObject->getUndoStackPtr()->newPost();
    //mpParentContainerObject->getUndoStackPtr()->registerModifiedBoxWidgetStyle(mWidgetIndex, mpRectItem->pen().width(), mpRectItem->pen().style(), mpRectItem->pen().color(),
    //                                                                    mpWidthBoxInDialog->value(), selectedStyle, mSelectedColor);

    QPen tempPen = mpRectItem->pen();
    tempPen.setColor(mSelectedColor);
    tempPen.setWidth(mpWidthBoxInDialog->value());
    tempPen.setStyle(selectedStyle);
    mpRectItem->setPen(tempPen);
    mpRectItem->setRect(mpRectItem->rect().united(mpTextItem->boundingRect()));

    mpSelectionBox->setSize(0.0, 0.0, mpRectItem->boundingRect().width(), mpRectItem->boundingRect().height());

    if(this->isSelected())
    {
        mpSelectionBox->setActive();
    }
    this->resize(mpRectItem->boundingRect().width(), mpRectItem->boundingRect().height());

    mpEditDialog->close();
}

void GUITextBoxWidget::openFontDialog()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, mpTextBoxInDialog->font(), gpMainWindow);
    if (ok)
    {
        mSelectedFont = font;
        mpTextBoxInDialog->setFont(font);
    }
}

void GUITextBoxWidget::openColorDialog()
{
    QColor color;
    color = QColorDialog::getColor(mSelectedColor, gpMainWindow);

    if (color.isValid())
    {
        mSelectedColor = color;
        mpTextBoxInDialog->setTextColor(color);
        QString redString, greenString, blueString;
        redString.setNum(mSelectedColor.red());
        greenString.setNum(mSelectedColor.green());
        blueString.setNum(mSelectedColor.blue());
        mpColorInDialogButton->setStyleSheet(QString("* { background-color: rgb(" + redString + "," + greenString + "," + blueString + ") }"));
    }
}


/////////////////////////////////////////////////////////////////////////////////




















//! @brief Constructor for text widget class
//! @param text Initial text in the widget
//! @param pos Position of text widget
//! @param rot Rotation of text widget (should normally be zero)
//! @param startSelected Initial selection status of text widget
//! @param pSystem Pointer to the GUI System where text widget is located
//! @param pParent Pointer to parent object (not required)
GUITextWidget::GUITextWidget(QString text, QPointF pos, qreal rot, selectionStatus startSelected, GUIContainerObject *pSystem, size_t widgetIndex, QGraphicsItem *pParent)
    : GUIWidget(pos, rot, startSelected, pSystem, pParent)
{
    this->mHmfTagName = HMF_TEXTWIDGETTAG;

    mpTextItem = new QGraphicsTextItem(text, this);
    QFont tempFont = mpTextItem->font();
    tempFont.setPointSize(12);
    mpTextItem->setFont(tempFont);
    mpTextItem->setPos(this->boundingRect().center());
    mpTextItem->show();

    this->setTextColor(QColor("darkolivegreen"));

    this->resize(mpTextItem->boundingRect().width(), mpTextItem->boundingRect().height());
    mpSelectionBox->setSize(0.0, 0.0, mpTextItem->boundingRect().width(), mpTextItem->boundingRect().height());

    mWidgetIndex = widgetIndex;
}


//! @brief Defines double click event for text widget
void GUITextWidget::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{

        //Open a dialog where text and font can be selected
    mpEditTextDialog = new QDialog(gpMainWindow);
    mpEditTextDialog->setWindowTitle("Edit Text Widget");

    mpTextBox = new QTextEdit();
    mpTextBox->setPlainText(mpTextItem->toPlainText());
    //mpTextBox->setMaximumHeight(1000);
    mpFontInDialogButton = new QPushButton("Change Font");
    mpColorInDialogButton = new QPushButton("Change Color");
    mpExampleLabel = new QLabel("Hopsan is cool!");
    mpExampleLabel->setFont(mpTextItem->font());
    QPalette pal(mSelectedColor);
    pal.setColor( QPalette::Foreground, mSelectedColor );
    mpExampleLabel->setPalette(pal);

    QGridLayout *pTextGroupLayout = new QGridLayout();
    pTextGroupLayout->addWidget(mpTextBox,0,0,1,4);
    pTextGroupLayout->addWidget(mpExampleLabel,1,0,1,4);
    pTextGroupLayout->addWidget(mpFontInDialogButton,2,0);
    pTextGroupLayout->addWidget(mpColorInDialogButton,2,1);
    QGroupBox *pTextGroupBox = new QGroupBox();
    pTextGroupBox->setLayout(pTextGroupLayout);

    mpDoneInDialogButton = new QPushButton("Done");
    mpCancelInDialogButton = new QPushButton("Cancel");
    QDialogButtonBox *pButtonBox = new QDialogButtonBox(Qt::Horizontal);
    pButtonBox->addButton(mpDoneInDialogButton, QDialogButtonBox::ActionRole);
    pButtonBox->addButton(mpCancelInDialogButton, QDialogButtonBox::ActionRole);

    QGridLayout *pDialogLayout = new QGridLayout();
    pDialogLayout->addWidget(pTextGroupBox,0,0);
    pDialogLayout->addWidget(pButtonBox,1,0);
    mpEditTextDialog->setLayout(pDialogLayout);
    mpEditTextDialog->show();

    mSelectedFont = mpTextItem->font();
    mSelectedColor = mpTextItem->defaultTextColor();

    connect(mpColorInDialogButton,SIGNAL(clicked()),this,SLOT(openColorDialog()));
    connect(mpFontInDialogButton,SIGNAL(clicked()),this,SLOT(openFontDialog()));
    connect(mpDoneInDialogButton,SIGNAL(clicked()),this,SLOT(updateWidgetFromDialog()));
    connect(mpCancelInDialogButton,SIGNAL(clicked()),mpEditTextDialog,SLOT(close()));
}


//! @brief Slot that removes text widget from all lists and then deletes it
void GUITextWidget::deleteMe(undoStatus undoSettings)
{
    mpParentContainerObject->removeWidget(this, undoSettings);
}


//! @brief Private function that updates the text widget from the selected values in the text edit dialog
void GUITextWidget::updateWidgetFromDialog()
{
    mpParentContainerObject->getUndoStackPtr()->newPost();
    mpParentContainerObject->getUndoStackPtr()->registerModifiedTextWidget(mWidgetIndex, mpTextItem->toPlainText(), mpTextItem->font(), mpTextItem->defaultTextColor(), mpTextBox->toPlainText(), mSelectedFont, mSelectedColor);

    mpTextItem->setPlainText(mpTextBox->toPlainText());
    mpTextItem->setFont(mSelectedFont);
    mpTextItem->setDefaultTextColor(mSelectedColor);

    mpSelectionBox->setSize(0.0, 0.0, mpTextItem->boundingRect().width(), mpTextItem->boundingRect().height());
    mpSelectionBox->setActive();
    this->resize(mpTextItem->boundingRect().width(), mpTextItem->boundingRect().height());
    mpEditTextDialog->close();
}


//! @brief Sets the text in the text widget
//! @param text String containing the text
void GUITextWidget::setText(QString text)
{
    mpTextItem->setPlainText(text);
    mpSelectionBox->setSize(0.0, 0.0, mpTextItem->boundingRect().width(), mpTextItem->boundingRect().height());
    mpSelectionBox->setPassive();
}


//! @brief Changes color in text widget
//! @param color New color that shall be used
void GUITextWidget::setTextColor(QColor color)
{
    mpTextItem->setDefaultTextColor(color);
}


//! @brief Changes font in the text widget
//! @param font New font that shall be used
void GUITextWidget::setTextFont(QFont font)
{
    mpTextItem->setFont(font);
//    mpSelectionBox = new GUIObjectSelectionBox(0.0, 0.0, mpTextItem->boundingRect().width(), mpTextItem->boundingRect().height(),
//                                               QPen(QColor("red"),2*GOLDENRATIO), QPen(QColor("darkRed"),2*GOLDENRATIO), this);
    mpSelectionBox->setSize(0.0, 0.0, mpTextItem->boundingRect().width(), mpTextItem->boundingRect().height());
    mpSelectionBox->setPassive();
    this->resize(mpTextItem->boundingRect().width(), mpTextItem->boundingRect().height());
}


//! @brief Opens a font dialog, places the selected font in the member variable and updates example text in edit dialog
void GUITextWidget::openFontDialog()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, mpExampleLabel->font(), gpMainWindow);
    if (ok)
    {
        mSelectedFont = font;
        mpExampleLabel->setFont(font);
    }
}


//! @brief Opens a color dialog, places the selected color in the member variable and updates example text in edit dialog
void GUITextWidget::openColorDialog()
{
    QColor color;
    color = QColorDialog::getColor(mSelectedColor, gpMainWindow);

    if (color.isValid())
    {
        mSelectedColor = color;
        QPalette pal(mSelectedColor);
        pal.setColor( QPalette::Foreground, mSelectedColor );
        mpExampleLabel->setPalette(pal);
    }
}


//! @brief Saves the text object into a specified dom element
//! @param rDomElement Reference to dom element to save into
void GUITextWidget::saveToDomElement(QDomElement &rDomElement)
{
    QDomElement xmlObject = appendDomElement(rDomElement, mHmfTagName);

    //Save GUI realted stuff
    QDomElement xmlGuiStuff = appendDomElement(xmlObject,HMF_HOPSANGUITAG);

    QPointF pos = mapToScene(boundingRect().topLeft());

    //xmlGuiStuff.setAttribute(HMF_POSETAG, getEndComponentName());

    QDomElement xmlPose = appendDomElement(xmlGuiStuff, HMF_POSETAG);
    xmlPose.setAttribute("x", pos.x());
    xmlPose.setAttribute("y", pos.y());

    QDomElement xmlText = appendDomElement(xmlGuiStuff, "textobject");
    xmlText.setAttribute("text", mpTextItem->toPlainText());
    xmlText.setAttribute("font", mpTextItem->font().toString());
    xmlText.setAttribute("fontcolor", mpTextItem->defaultTextColor().name());
}


//! @brief Constructor for box widget class
//! @param pos Position of box widget
//! @param rot Rotation of box widget (should normally be zero)
//! @param startSelected Initial selection status of box widget
//! @param pSystem Pointer to the GUI System where box widget is located
//! @param pParent Pointer to parent object (not required)
GUIBoxWidget::GUIBoxWidget(QPointF pos, qreal rot, selectionStatus startSelected, GUIContainerObject *pSystem, size_t widgetIndex, QGraphicsItem *pParent)
    : GUIWidget(pos, rot, startSelected, pSystem, pParent)
{
    this->mHmfTagName = HMF_BOXWIDGETTAG;

    mpRectItem = new QGraphicsRectItem(0, 0, 100, 100, this);
    QPen tempPen = mpRectItem->pen();
    tempPen.setColor(QColor("darkolivegreen"));
    tempPen.setWidth(2);
    tempPen.setStyle(Qt::SolidLine);//Qt::DotLine);
    tempPen.setCapStyle(Qt::RoundCap);
    tempPen.setJoinStyle(Qt::RoundJoin);
    mpRectItem->setPen(tempPen);
    mpRectItem->setPos(mpRectItem->pen().width()/2.0, mpRectItem->pen().width()/2.0);
    mpRectItem->show();

    this->setFlag(QGraphicsItem::ItemAcceptsInputMethod, true);

    this->resize(mpRectItem->boundingRect().width(), mpRectItem->boundingRect().height());

    mpSelectionBox->setSize(0.0, 0.0, mpRectItem->boundingRect().width(), mpRectItem->boundingRect().height());
    mWidgetIndex = widgetIndex;

    mWidthBeforeResize = mpRectItem->rect().width();
    mHeightBeforeResize = mpRectItem->rect().height();
    mPosBeforeResize = this->pos();
}




//! @brief Slot that removes text widget from all lists and then deletes it
void GUIBoxWidget::deleteMe(undoStatus undoSettings)
{
    mpParentContainerObject->removeWidget(this, undoSettings);
}


//! @brief Defines double click events (opens the box edit dialog)
void GUIBoxWidget::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
        //Open a dialog where line width and color can be selected
    mpEditBoxDialog = new QDialog(gpMainWindow);
    mpEditBoxDialog->setWindowTitle("Edit Text Box Widget");

    mpWidthLabelInDialog = new QLabel("Line Width: ");
    mpWidthBoxInDialog = new QSpinBox();
    mpWidthBoxInDialog->setMinimum(1);
    mpWidthBoxInDialog->setMaximum(100);
    mpWidthBoxInDialog->setSingleStep(1);
    mpWidthBoxInDialog->setValue(mpRectItem->pen().width());
    mpColorLabelInDialog = new QLabel("Line Color: ");
    mpColorInDialogButton = new QToolButton();
    QString redString, greenString, blueString;
    redString.setNum(mpRectItem->pen().color().red());
    greenString.setNum(mpRectItem->pen().color().green());
    blueString.setNum(mpRectItem->pen().color().blue());
    mpColorInDialogButton->setStyleSheet(QString("* { background-color: rgb(" + redString + "," + greenString + "," + blueString + ") }"));
    mpColorInDialogButton->setAutoRaise(true);

    mpStyleLabelInDialog = new QLabel("Line Style: ");
    mpStyleBoxInDialog = new QComboBox();
    mpStyleBoxInDialog->insertItem(0, "Solid Line");
    mpStyleBoxInDialog->insertItem(1, "Dashes");
    mpStyleBoxInDialog->insertItem(2, "Dots");
    mpStyleBoxInDialog->insertItem(3, "Dashes and Dots");

    if(mpRectItem->pen().style() == Qt::SolidLine)
        mpStyleBoxInDialog->setCurrentIndex(0);
    if(mpRectItem->pen().style() == Qt::DashLine)
        mpStyleBoxInDialog->setCurrentIndex(1);
    if(mpRectItem->pen().style() == Qt::DotLine)
        mpStyleBoxInDialog->setCurrentIndex(2);
    if(mpRectItem->pen().style() == Qt::DashDotLine)
        mpStyleBoxInDialog->setCurrentIndex(3);

    QGridLayout *pBoxGroupLayout = new QGridLayout();
    pBoxGroupLayout->addWidget(mpWidthLabelInDialog,0,0);
    pBoxGroupLayout->addWidget(mpWidthBoxInDialog,0,1);
    pBoxGroupLayout->addWidget(mpColorLabelInDialog,1,0);
    pBoxGroupLayout->addWidget(mpColorInDialogButton,1,1);
    pBoxGroupLayout->addWidget(mpStyleLabelInDialog, 2, 0);
    pBoxGroupLayout->addWidget(mpStyleBoxInDialog,2, 1);

    QGroupBox *pBoxGroupBox = new QGroupBox();
    pBoxGroupBox->setLayout(pBoxGroupLayout);

    mpDoneInDialogButton = new QPushButton("Done");
    mpCancelInDialogButton = new QPushButton("Cancel");
    QDialogButtonBox *pButtonBox = new QDialogButtonBox(Qt::Horizontal);
    pButtonBox->addButton(mpDoneInDialogButton, QDialogButtonBox::ActionRole);
    pButtonBox->addButton(mpCancelInDialogButton, QDialogButtonBox::ActionRole);

    QGridLayout *pDialogLayout = new QGridLayout();
    pDialogLayout->addWidget(pBoxGroupBox,0,0);
    pDialogLayout->addWidget(pButtonBox,2,0);
    mpEditBoxDialog->setLayout(pDialogLayout);
    mpEditBoxDialog->show();

    this->setZValue(0);
    this->setFlags(QGraphicsItem::ItemStacksBehindParent);

    mSelectedColor = mpRectItem->pen().color();

    connect(mpColorInDialogButton,SIGNAL(clicked()),this,SLOT(openColorDialog()));
    connect(mpDoneInDialogButton,SIGNAL(clicked()),this,SLOT(updateWidgetFromDialog()));
    connect(mpCancelInDialogButton,SIGNAL(clicked()),mpEditBoxDialog,SLOT(close()));
}


//! @brief Opens a color dialog and places the selected color in the member variable
void GUIBoxWidget::openColorDialog()
{
    QColor color;
    color = QColorDialog::getColor(mpRectItem->pen().color(), gpMainWindow);

    if (color.isValid())
    {
        mSelectedColor = color;

        QString redString, greenString, blueString;
        redString.setNum(mSelectedColor.red());
        greenString.setNum(mSelectedColor.green());
        blueString.setNum(mSelectedColor.blue());
        mpColorInDialogButton->setStyleSheet(QString("* { background-color: rgb(" + redString + "," + greenString + "," + blueString + ") }"));
    }
}


//! @brief Private function that updates the text widget from the selected values in the text edit dialog
void GUIBoxWidget::updateWidgetFromDialog()
{
    Qt::PenStyle selectedStyle;
    if(mpStyleBoxInDialog->currentIndex() == 0)
        selectedStyle = Qt::SolidLine;
    else if(mpStyleBoxInDialog->currentIndex() == 1)
        selectedStyle = Qt::DashLine;
    else if(mpStyleBoxInDialog->currentIndex() == 2)
        selectedStyle = Qt::DotLine;
    else if(mpStyleBoxInDialog->currentIndex() == 3)
        selectedStyle = Qt::DashDotLine;
    else
        selectedStyle = Qt::SolidLine;      //This shall never happen, but will supress a warning message

    mpParentContainerObject->getUndoStackPtr()->newPost();
    mpParentContainerObject->getUndoStackPtr()->registerModifiedBoxWidgetStyle(mWidgetIndex, mpRectItem->pen().width(), mpRectItem->pen().style(), mpRectItem->pen().color(),
                                                                        mpWidthBoxInDialog->value(), selectedStyle, mSelectedColor);

    QPen tempPen = mpRectItem->pen();
    tempPen.setColor(mSelectedColor);
    tempPen.setWidth(mpWidthBoxInDialog->value());
    tempPen.setStyle(selectedStyle);
    mpRectItem->setPen(tempPen);

//    delete(mpSelectionBox);
//    mpSelectionBox = new GUIObjectSelectionBox(0.0, 0.0, mpRectItem->boundingRect().width(), mpRectItem->boundingRect().height(),
//                                               QPen(QColor("red"),2*GOLDENRATIO), QPen(QColor("darkRed"),2*GOLDENRATIO), this);
    mpSelectionBox->setSize(0.0, 0.0, mpRectItem->boundingRect().width(), mpRectItem->boundingRect().height());

    if(this->isSelected())
    {
        mpSelectionBox->setActive();
    }
    this->resize(mpRectItem->boundingRect().width(), mpRectItem->boundingRect().height());
    mpEditBoxDialog->close();
}


//! @brief Defines what happens when hovering the box widget (changes cursor and defines resize areas)
void GUIBoxWidget::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    GUIObject::hoverMoveEvent(event);

    this->setCursor(Qt::ArrowCursor);
    mResizeLeft = false;
    mResizeRight = false;
    mResizeTop = false;
    mResizeBottom = false;

    int resLim = 5;

    if(event->pos().x() > boundingRect().left() && event->pos().x() < boundingRect().left()+resLim)
    {
        mResizeLeft = true;
    }
    if(event->pos().x() > boundingRect().right()-resLim && event->pos().x() < boundingRect().right())
    {
        mResizeRight = true;
    }
    if(event->pos().y() > boundingRect().top() && event->pos().y() < boundingRect().top()+resLim)
    {
        mResizeTop = true;
    }
    if(event->pos().y() > boundingRect().bottom()-resLim && event->pos().y() < boundingRect().bottom())
    {
        mResizeBottom = true;
    }

    if( (mResizeLeft && mResizeTop) || (mResizeRight && mResizeBottom) )
        this->setCursor(Qt::SizeFDiagCursor);
    else if( (mResizeTop && mResizeRight) || (mResizeBottom && mResizeLeft) )
        this->setCursor(Qt::SizeBDiagCursor);
    else if(mResizeLeft || mResizeRight)
        this->setCursor(Qt::SizeHorCursor);
    else if(mResizeTop || mResizeBottom)
        this->setCursor(Qt::SizeVerCursor);
}


//! @brief Defines what happens when clicking on the box (defines start position for resizing)
void GUIBoxWidget::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if(mResizeLeft || mResizeRight || mResizeTop || mResizeBottom)
    {
        mPosBeforeResize = this->pos();
        mWidthBeforeResize = this->mpRectItem->rect().width();
        mHeightBeforeResize = this->mpRectItem->rect().height();
        mIsResizing = true;
    }
    else
    {
        mIsResizing = false;
    }
    GUIObject::mousePressEvent(event);
}


//! @brief Defines what happens when user is moves (or is trying to move) the object with the mouse. Used for resizing.
void GUIBoxWidget::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    GUIObject::mouseMoveEvent(event);

    if(mResizeLeft && mResizeTop)
    {
        mpRectItem->setRect(mpRectItem->rect().x(), mpRectItem->rect().y(), mWidthBeforeResize+mPosBeforeResize.x()-this->pos().x(), mHeightBeforeResize+mPosBeforeResize.y()-this->pos().y());
    }
    else if(mResizeTop && mResizeRight)
    {
        mpRectItem->setRect(mpRectItem->rect().x(), mpRectItem->rect().y(), mWidthBeforeResize-mPosBeforeResize.x()+this->pos().x(), mHeightBeforeResize+mPosBeforeResize.y()-this->pos().y());
        this->setX(mPosBeforeResize.x());
    }
    else if(mResizeRight && mResizeBottom)
    {
        mpRectItem->setRect(mpRectItem->rect().x(), mpRectItem->rect().y(), mWidthBeforeResize-mPosBeforeResize.x()+this->pos().x(), mHeightBeforeResize-mPosBeforeResize.y()+this->pos().y());
        this->setX(mPosBeforeResize.x());
        this->setY(mPosBeforeResize.y());
    }
    else if(mResizeBottom && mResizeLeft)
    {
        mpRectItem->setRect(mpRectItem->rect().x(), mpRectItem->rect().y(), mWidthBeforeResize+mPosBeforeResize.x()-this->pos().x(), mHeightBeforeResize-mPosBeforeResize.y()+this->pos().y());
        this->setY(mPosBeforeResize.y());
    }
    else if(mResizeLeft)
    {
        mpRectItem->setRect(mpRectItem->rect().x(), mpRectItem->rect().y(), mWidthBeforeResize+mPosBeforeResize.x()-this->pos().x(), mpRectItem->rect().height());
        this->setY(mPosBeforeResize.y());
    }
    else if(mResizeRight)
    {
        mpRectItem->setRect(mpRectItem->rect().x(), mpRectItem->rect().y(), mWidthBeforeResize-mPosBeforeResize.x()+this->pos().x(), mpRectItem->rect().height());
        this->setPos(mPosBeforeResize);
    }
    else if(mResizeTop)
    {
        mpRectItem->setRect(mpRectItem->rect().x(), mpRectItem->rect().y(),  mpRectItem->rect().width(), mHeightBeforeResize+mPosBeforeResize.y()-this->pos().y());
        this->setX(mPosBeforeResize.x());
    }
    else if(mResizeBottom)
    {
        mpRectItem->setRect(mpRectItem->rect().x(), mpRectItem->rect().y(), mpRectItem->rect().width(), mHeightBeforeResize-mPosBeforeResize.y()+this->pos().y());
        this->setPos(mPosBeforeResize);
    }

//    delete(mpSelectionBox);
//    mpSelectionBox = new GUIObjectSelectionBox(0.0, 0.0, mpRectItem->boundingRect().width(), mpRectItem->boundingRect().height(),
//                                               QPen(QColor("red"),2*GOLDENRATIO), QPen(QColor("darkRed"),2*GOLDENRATIO), this);
    mpSelectionBox->setSize(0.0, 0.0, mpRectItem->boundingRect().width(), mpRectItem->boundingRect().height());

    mpSelectionBox->setActive();
    this->resize(mpRectItem->boundingRect().width(), mpRectItem->boundingRect().height());
    mpRectItem->setPos(mpRectItem->pen().width()/2.0, mpRectItem->pen().width()/2.0);
}



void GUIBoxWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    GUIWidget::mouseReleaseEvent(event);
    if(mWidthBeforeResize != mpRectItem->rect().width() || mHeightBeforeResize != mpRectItem->rect().height())
    {
        mpParentContainerObject->getUndoStackPtr()->newPost();
        mpParentContainerObject->getUndoStackPtr()->registerResizedBoxWidget( mWidgetIndex, mWidthBeforeResize, mHeightBeforeResize, mpRectItem->rect().width(), mpRectItem->rect().height(), mPosBeforeResize, this->pos());
        mWidthBeforeResize = mpRectItem->rect().width();
        mHeightBeforeResize = mpRectItem->rect().height();
        mPosBeforeResize = this->pos();
    }
}


//! @brief Saves the box widget into a specified dom element
//! @param rDomElement Reference to dom element to save into
void GUIBoxWidget::saveToDomElement(QDomElement &rDomElement)
{
    QDomElement xmlObject = appendDomElement(rDomElement, mHmfTagName);

    //Save GUI realted stuff
    QDomElement xmlGuiStuff = appendDomElement(xmlObject,HMF_HOPSANGUITAG);

    QPointF pos = mapToScene(mpRectItem->rect().topLeft());

    QDomElement xmlPose = appendDomElement(xmlGuiStuff, HMF_POSETAG);
    xmlPose.setAttribute("x", pos.x());
    xmlPose.setAttribute("y", pos.y());

    QDomElement xmlSize = appendDomElement(xmlGuiStuff, "size");
    xmlSize.setAttribute("width", mpRectItem->rect().width());
    xmlSize.setAttribute("height", mpRectItem->rect().height());

    QDomElement xmlLine = appendDomElement(xmlGuiStuff, "line");
    xmlLine.setAttribute("width", mpRectItem->pen().width());

    QString style;
    if(mpRectItem->pen().style() == Qt::SolidLine)
        style = "solidline";
    else if(mpRectItem->pen().style() == Qt::DashLine)
        style = "dashline";
    else if(mpRectItem->pen().style() == Qt::DotLine)
        style = "dotline";
    else if(mpRectItem->pen().style() == Qt::DashDotLine)
        style = "dashdotline";
    xmlLine.setAttribute(HMF_STYLETAG, style);
    xmlLine.setAttribute("color", mpRectItem->pen().color().name());
}


//! @brief Sets the line width of the box
//! @param value New width of the line
void GUIBoxWidget::setLineWidth(int value)
{
    QPen tempPen;
    tempPen = mpRectItem->pen();
    tempPen.setWidth(value);
    mpRectItem->setPen(tempPen);
}


//! @brief Sets the line style of the box
//! @param style Desired style (enum defined by Qt)
void GUIBoxWidget::setLineStyle(Qt::PenStyle style)
{
    QPen tempPen;
    tempPen = mpRectItem->pen();
    tempPen.setStyle(style);
    mpRectItem->setPen(tempPen);
}


//! @brief Sets the color of the box
//! @param New color
void GUIBoxWidget::setLineColor(QColor color)
{
    QPen tempPen;
    tempPen = mpRectItem->pen();
    tempPen.setColor(color);
    mpRectItem->setPen(tempPen);
}


//! @brief Sets the size of the box
//! @param w New width of the box
//! @param h New height of the box
void GUIBoxWidget::setSize(qreal w, qreal h)
{
    QPointF posBeforeResize = this->pos();
    mpRectItem->setRect(mpRectItem->rect().x(), mpRectItem->rect().y(), w, h);
    this->setPos(posBeforeResize);

    mpSelectionBox->setSize(0.0, 0.0, mpRectItem->boundingRect().width(), mpRectItem->boundingRect().height());

    mpSelectionBox->setActive();
    this->resize(mpRectItem->boundingRect().width(), mpRectItem->boundingRect().height());
    mpRectItem->setPos(mpRectItem->pen().width()/2.0, mpRectItem->pen().width()/2.0);

    mWidthBeforeResize = mpRectItem->rect().width();
    mHeightBeforeResize = mpRectItem->rect().height();
    mPosBeforeResize = this->pos();
}
