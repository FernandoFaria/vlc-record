/*********************** Information *************************\
| $HeadURL$
|
| Author: Jo2003
|
| Begin: 20.06.2010 / 19:50:35
|
| Last edited by: $Author$
|
| $Id$
\*************************************************************/
#include "cvideoframe.h"

// log file functions ...
extern CLogFile VlcLog;

/* -----------------------------------------------------------------\
|  Method: CVideoFrame / constructor
|  Begin: 20.06.2010 / 19:05:00
|  Author: Jo2003
|  Description: create object, init values
|
|  Parameters: pointer to parent window, windows flags
|
|  Returns: --
\----------------------------------------------------------------- */
CVideoFrame::CVideoFrame(QWidget * parent, Qt::WindowFlags f)
#ifdef Q_OS_MAC
   : CVideoWidget(NULL, parent)
#else
   : CVideoWidget(parent, f)
#endif
{
   pvShortcuts = NULL;

   // set mouse hide timer to single shot timer ...
   tMouseHide.setSingleShot (true);

   // ... with 1000 ms. interval ...
   tMouseHide.setInterval (1000);

   // hide mouse when timer has timeout ...
   connect (&tMouseHide, SIGNAL(timeout()), this, SLOT(slotHideMouse()));

#ifndef Q_OS_MAC
   setFrameShape(QFrame::NoFrame);
   setFrameShadow(QFrame::Plain);
   setLineWidth(0);
#endif // Q_OS_MAC
}

/* -----------------------------------------------------------------\
|  Method: ~CVideoFrame / destructor
|  Begin: 22.06.2010 / 13:05:00
|  Author: Jo2003
|  Description: clean at destruction
|
|  Parameters: --
|
|  Returns: --
\----------------------------------------------------------------- */
CVideoFrame::~CVideoFrame()
{

}

/* -----------------------------------------------------------------\
|  Method: mouseDoubleClickEvent
|  Begin: 22.06.2010 / 19:05:00
|  Author: Jo2003
|  Description: toggle fullscreen on double click
|
|  Parameters: pointer to event ...
|
|  Returns: --
\----------------------------------------------------------------- */
void CVideoFrame::mouseDoubleClickEvent(QMouseEvent *pEvent)
{
   emit sigToggleFullscreen();
   pEvent->accept();
}

/* -----------------------------------------------------------------\
|  Method: mouseMoveEvent
|  Begin: 27.07.2010 / 11:45:00
|  Author: Jo2003
|  Description: mouse was moved, special handling to hide mouse
|               if we are in fullScreen ...
|  Parameters: pointer to event ...
|
|  Returns: --
\----------------------------------------------------------------- */
void CVideoFrame::mouseMoveEvent (QMouseEvent *pEvent)
{
   // is cursor hidden ... ?
   if (cursor().shape() == Qt::BlankCursor)
   {
      // remove blank cursor (means unhide cursor) ...
      unsetCursor ();
   }

   // if we're in fullscreen, start mouse hide timer ...
   if (parentWidget()->isFullScreen())
   {
      // trigger or re-trigger mouse hide timer ...
      tMouseHide.start ();
   }

   // always accept event ...
   pEvent->accept ();
}

/* -----------------------------------------------------------------\
|  Method: slotHideMouse
|  Begin: 27.07.2010 / 11:45:00
|  Author: Jo2003
|  Description: hide mouse cursore if we're in fullscreen
|
|  Parameters: --
|
|  Returns: --
\----------------------------------------------------------------- */
void CVideoFrame::slotHideMouse()
{
   // hide mouse if we're in fullscreen ...
   if (parentWidget()->isFullScreen())
   {
      // hide cursor if not hidden ...
      if (cursor().shape() != Qt::BlankCursor)
      {
         setCursor(QCursor(Qt::BlankCursor));
      }
   }
}

/* -----------------------------------------------------------------\
|  Method: keyPressEvent
|  Begin: 23.03.2010 / 22:46:10
|  Author: Jo2003
|  Description: catch keypress events to emulate shortcuts
|
|  Parameters: pointer to event
|
|  Returns: --
\----------------------------------------------------------------- */
void CVideoFrame::keyPressEvent(QKeyEvent *pEvent)
{
   QKeySequence seq;

   // make key sequence from key event ...
   if (!keyEventToKeySequence(pEvent, seq))
   {
      // success --> activate shortcut ...
      if (!fakeShortCut(seq))
      {
         // success --> accept event ...
         pEvent->accept();
      }
   }
   else
   {
      // no key sequence --> handle by base class ...
      QWidget::keyPressEvent(pEvent);
   }
}

/* -----------------------------------------------------------------\
|  Method: setShortCuts
|  Begin: 24.03.2010 / 14:17:51
|  Author: Jo2003
|  Description: store a pointer to shortcuts vector
|
|  Parameters: pointer to shortcuts vector
|
|  Returns: --
\----------------------------------------------------------------- */
void CVideoFrame::setShortCuts(QVector<CShortcutEx *> *pvSc)
{
   pvShortcuts = pvSc;
}

/* -----------------------------------------------------------------\
|  Method: fakeShortCut
|  Begin: 24.03.2010 / 14:30:51
|  Author: Jo2003
|  Description: fake shortcut press if needed
|
|  Parameters: key sequence
|
|  Returns: 0 --> shortcut sent
|          -1 --> not handled
\----------------------------------------------------------------- */
int CVideoFrame::fakeShortCut (const QKeySequence &seq)
{
   int iRV = -1;
   QVector<CShortcutEx *>::const_iterator cit;

   if (pvShortcuts)
   {
      // test all shortcuts if one matches the now incoming ...
      for (cit = pvShortcuts->constBegin(); (cit != pvShortcuts->constEnd()) && iRV; cit ++)
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

/* -----------------------------------------------------------------\
|  Method: keyEventToKeySequence
|  Begin: 25.03.2011 / 8:30
|  Author: Jo2003
|  Description: make keysequence from key event
|
|  Parameters: key event, ref. to key sequence
|
|  Returns: 0 --> key sequence filled
|          -1 --> not a key sequence
\----------------------------------------------------------------- */
int CVideoFrame::keyEventToKeySequence(QKeyEvent *pEvent, QKeySequence &seq)
{
   int iKey                    = pEvent->key();
   Qt::KeyboardModifiers state = pEvent->modifiers();
   QString text                = pEvent->text();

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

/************************* History ***************************\
| $Log$
\*************************************************************/
