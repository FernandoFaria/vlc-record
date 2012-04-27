/*------------------------------ Infor mation ---------------------------*//**
 *
 *  $HeadURL$
 *
 *  @file     qvlcvideowidget.cpp
 *
 *  @author   Jo2003, inspired by Tadej Novak (VLC-Qt)
 *
 *  @date     08.02.2012
 *
 *  $Id$
 *
 *///------------------------- (c) 2012 by Jo2003  --------------------------
#include "qvlcvideowidget.h"
#include <QVBoxLayout>
#include <QApplication>

// log file functions ...
extern CLogFile VlcLog;

//---------------------------------------------------------------------------
//
//! \brief   constructs QVlcVideoWidget object
//
//! \author  Jo2003
//! \date    08.02.2012
//
//! \param   parent pointer to parent widget
//
//! \return  --
//---------------------------------------------------------------------------
QVlcVideoWidget::QVlcVideoWidget(QWidget *parent) :
   QWidget(parent),
   _render(0),
   _mouseHide(0),
   _shortcuts(0)
{
   setMouseTracking(true);

   QVBoxLayout *pLayout = new QVBoxLayout();
   _render              = new QWidget(this);
   _mouseHide           = new QTimer(this);
   _render->setMouseTracking(true);
   _render->setAutoFillBackground(true);
   _render->setObjectName("renderView");
   _render->setStyleSheet("QWidget#renderView {"
                          "background-color: black;"
                          "background-image: url(:/app/kartina);"
                          "background-repeat: no-repeat;"
                          "background-position: center middle;}");
   pLayout->setMargin(0);
   pLayout->addWidget(_render);
   setLayout(pLayout);

   connect (_mouseHide, SIGNAL(timeout()), this, SLOT(hideMouse()));
}

//---------------------------------------------------------------------------
//
//! \brief   destroys QVlcVideoWidget object
//
//! \author  Jo2003
//! \date    08.02.2012
//
//! \param   --
//
//! \return  --
//---------------------------------------------------------------------------
QVlcVideoWidget::~QVlcVideoWidget()
{
   if (_mouseHide)
   {
      delete _mouseHide;
      _mouseHide = NULL;
   }

   if (_render)
   {
      delete _render;
      _render = NULL;
   }
}

//---------------------------------------------------------------------------
//
//! \brief   get window id from render view
//
//! \author  Jo2003
//! \date    08.02.2012
//
//! \param   --
//
//! \return  WId from render view
//---------------------------------------------------------------------------
WId QVlcVideoWidget::widgetId()
{
   return _render ? _render->winId() : (WId)0;
}

//---------------------------------------------------------------------------
//
//! \brief   override QWidget::mouseDoubleClickEvent()
//
//! \author  Jo2003
//! \date    08.02.2012
//
//! \param   event pointer to mouse event
//
//! \return  --
//---------------------------------------------------------------------------
void QVlcVideoWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
   event->ignore();

   emit fullScreen();
}

//---------------------------------------------------------------------------
//
//! \brief   override QWidget::mouseMoveEvent()
//
//! \author  Jo2003
//! \date    08.02.2012
//
//! \param   event pointer to mouse event
//
//! \return  --
//---------------------------------------------------------------------------
void QVlcVideoWidget::mouseMoveEvent(QMouseEvent *event)
{
   event->ignore();

   if(isFullScreen())
   {
      emit mouseShow(event->globalPos());
      QApplication::restoreOverrideCursor();
      _mouseHide->start(1000);
   }
}

//---------------------------------------------------------------------------
//
//! \brief   override QWidget::mousePressEvent()
//
//! \author  Jo2003
//! \date    08.02.2012
//
//! \param   event pointer to mouse event
//
//! \return  --
//---------------------------------------------------------------------------
void QVlcVideoWidget::mousePressEvent(QMouseEvent *event)
{
   event->ignore();

   if(event->button() == Qt::RightButton)
   {
      QApplication::restoreOverrideCursor();
      emit rightClick(event->globalPos());
   }
}

//---------------------------------------------------------------------------
//
//! \brief   override QWidget::wheelEvent()
//
//! \author  Jo2003
//! \date    08.02.2012
//
//! \param   event pointer to wheel event
//
//! \return  --
//---------------------------------------------------------------------------
void QVlcVideoWidget::wheelEvent(QWheelEvent *event)
{
   event->ignore();

   emit wheel ((event->delta() > 0) ? true : false);
}

//---------------------------------------------------------------------------
//
//! \brief   switch between normal screen and full screen [slot]
//
//! \author  Jo2003
//! \date    08.02.2012
//
//! \param   event pointer to wheel event
//
//! \return  --
//---------------------------------------------------------------------------
void QVlcVideoWidget::toggleFullScreen()
{
   if (!isFullScreen())
   {
      // make it Window ...
      setWindowFlags(windowFlags() | Qt::Window);

      // show fullscreen ...
      showFullScreen();

      // make sure render window is raised ...
      _render->show();
      _render->activateWindow();
      _render->raise();

      // start mouse hiding ...
      _mouseHide->start(1000);
   }
   else
   {
      // embedd it ...
      setWindowFlags(windowFlags() & ~(Qt::WindowFlags)Qt::Window);

      // end fullscreen ...
      showNormal();

      QApplication::restoreOverrideCursor();
   }
}

//---------------------------------------------------------------------------
//
//! \brief   hide mouse when fullScreen [slot]
//
//! \author  Jo2003
//! \date    08.02.2012
//
//! \param   --
//
//! \return  --
//---------------------------------------------------------------------------
void QVlcVideoWidget::hideMouse()
{
   if(isFullScreen())
   {
      QApplication::setOverrideCursor(Qt::BlankCursor);
      _mouseHide->stop();
      emit mouseHide();
   }
}

//---------------------------------------------------------------------------
//
//! \brief   override QWidget::keyPressEvent()
//
//! \author  Jo2003
//! \date    08.02.2012
//
//! \param   event pointer to key event
//
//! \return  --
//---------------------------------------------------------------------------
void QVlcVideoWidget::keyPressEvent(QKeyEvent *event)
{
   QKeySequence seq;

   event->ignore();

   // make key sequence from key event ...
   if (!keyEventToKeySequence(event, seq))
   {
      fakeShortCut(seq);
   }
}

//---------------------------------------------------------------------------
//
//! \brief   store a pointer to shortcuts vector
//
//! \author  Jo2003
//! \date    08.02.2012
//
//! \param   pvSc pointer to shortcuts vector
//
//! \return  --
//---------------------------------------------------------------------------
void QVlcVideoWidget::setShortCuts(QVector<CShortcutEx *> *pvSc)
{
   _shortcuts = pvSc;
}

//---------------------------------------------------------------------------
//
//! \brief   fake shortcut press if needed
//
//! \author  Jo2003
//! \date    08.02.2012
//
//! \param   seq ref. to key sequence
//
//! \return  --
//---------------------------------------------------------------------------
int QVlcVideoWidget::fakeShortCut (const QKeySequence &seq)
{
   int iRV = -1;
   QVector<CShortcutEx *>::const_iterator cit;

   if (_shortcuts)
   {
      // test all shortcuts if one matches the now incoming ...
      for (cit = _shortcuts->constBegin(); (cit != _shortcuts->constEnd()) && iRV; cit ++)
      {
         // is key sequence equal ... ?
         if ((*cit)->key() == seq)
         {
            // this shortcut matches ...
            mInfo (tr("Activate shortcut: %1").arg(seq.toString()));

            // fake shortcut keypress ...
            (*cit)->activate();

            // only one shortcut should match this sequence ...
            // so we're done!
            iRV = 0;
         }
      }
   }

   return iRV;
}

//---------------------------------------------------------------------------
//
//! \brief   fake shortcut press if needed
//
//! \author  Jo2003
//! \date    08.02.2012
//
//! \param   event pointer to key event
//! \param   seq ref. to key sequence
//
//! \return  0 --> key sequence filled
//! \return -1 --> not a key sequence
//---------------------------------------------------------------------------
int QVlcVideoWidget::keyEventToKeySequence(QKeyEvent *event, QKeySequence &seq)
{
   int iKey                    = event->key();
   Qt::KeyboardModifiers state = event->modifiers();
   QString text                = event->text();

   if ((iKey == Qt::Key_Control) || (iKey == Qt::Key_Shift) ||
       (iKey == Qt::Key_Meta)    || (iKey == Qt::Key_Alt)   ||
       (iKey == Qt::Key_Super_L) || (iKey == Qt::Key_AltGr))
   {
      // a single modifier can't be a shortcut / key sequence ...
      return -1;
   }

   // does shift modifier make sense ... ?
   if ((state & Qt::ShiftModifier) && ((text.size() == 0) || !text.at(0).isPrint() || text.at(0).isLetter() || text.at(0).isSpace()))
   {
      iKey |= Qt::SHIFT;
   }

   if (state & Qt::ControlModifier)
   {
      iKey |= Qt::CTRL;
   }

   if (state & Qt::MetaModifier)
   {
      iKey |= Qt::META;
   }

   if (state & Qt::AltModifier)
   {
      iKey |= Qt::ALT;
   }

   seq = QKeySequence(iKey);

   return 0;
}

//---------------------------------------------------------------------------
//
//! \brief   raise render view
//
//! \author  Jo2003
//! \date    27.04.2012
//
//! \return  --
//---------------------------------------------------------------------------
void QVlcVideoWidget::raiseRender()
{
   if (_render)
   {
      _render->raise();
   }
}