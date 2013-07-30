/*------------------------------ Infor mation ---------------------------*//**
 *
 *  $HeadURL$
 *
 *  @file     qstringfilterwidgetaction.h
 *
 *  @author   Jo2003
 *
 *  @date     29.07.2013
 *
 *  $Id$
 *
 *///------------------------- (c) 2013 by Jo2003  --------------------------
#ifndef __20130729_QSTRINGFILTERWIDGETACTION_H
   #define __20130729_QSTRINGFILTERWIDGETACTION_H

#include <QWidgetAction>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QEvent>

//---------------------------------------------------------------------------
//! \class   QStringFilterWidgetAction
//! \date    29.07.2013
//! \author  Jo2003
//! \brief   widget for channel filter
//---------------------------------------------------------------------------
class QStringFilterWidgetAction : public QWidgetAction
{
   Q_OBJECT

public:
   //---------------------------------------------------------------------------
   //
   //! \brief   constructs QStringFilterWidgetAction object
   //
   //! \author  Jo2003
   //! \date    29.07.2013
   //
   //! \param   parent (QObject *) pointer to parent object
   //
   //! \return  --
   //---------------------------------------------------------------------------
   QStringFilterWidgetAction(QObject *parent = 0)
      : QWidgetAction(parent), _line(NULL), _chk(NULL)
   {

   }

   //---------------------------------------------------------------------------
   //
   //! \brief   clear filter string
   //
   //! \author  Jo2003
   //! \date    29.07.2013
   //
   //! \param   --
   //
   //! \return  --
   //---------------------------------------------------------------------------
   void cleanFilter()
   {
      _line->clear();
      _chk->setChecked(false);
   }

private:
   QLineEdit *_line;
   QCheckBox *_chk;

protected:
   //---------------------------------------------------------------------------
   //
   //! \brief   create channel filter widget
   //
   //! \author  Jo2003
   //! \date    29.07.2013
   //
   //! \param   parent (QWidget *) pointer to parent widget
   //
   //! \return  --
   //---------------------------------------------------------------------------
   virtual QWidget* createWidget (QWidget * parent)
   {
      QWidget     *w = new QWidget(parent);
      QHBoxLayout *l = new QHBoxLayout();

      _line          = new QLineEdit(w);
      _chk           = new QCheckBox(tr("Filter Channels"), w);

      _chk->setToolTip(tr("enable / disable filter"));

      l->setSpacing(2);
      l->setMargin(4);

      l->addWidget(_chk);
      l->addWidget(_line, 10);

      w->setLayout(l);

      connect(_chk , SIGNAL(clicked(bool))  , this, SLOT(slotFilterToggled(bool)));
      connect(_line, SIGNAL(returnPressed()), this, SLOT(slotEnter()));

      return w;
   }

   //---------------------------------------------------------------------------
   //
   //! \brief   handle language switch event
   //
   //! \author  Jo2003
   //! \date    29.07.2013
   //
   //! \param   e (QEvent *) pointer to event
   //
   //! \return  true -> handled; false -> not handled
   //---------------------------------------------------------------------------
   virtual bool event(QEvent *e)
   {
      bool rv = false;

      if (e->type() == QEvent::LanguageChange)
      {
         // make sure we already created
         // control elements ...
         if (_chk)
         {
            _chk->setToolTip(tr("enable / disable filter"));
            _chk->setText(tr("Filter Channels"));

            rv = true;
         }
      }
      else
      {
         rv = QWidgetAction::event(e);
      }

      return rv;
   }

private slots:
   //---------------------------------------------------------------------------
   //
   //! \brief   go button was pressed, trigger sigFilter
   //
   //! \author  Jo2003
   //! \date    29.07.2013
   //
   //! \param   b (bool) enabled or disabled
   //
   //! \return  --
   //---------------------------------------------------------------------------
   void slotFilterToggled(bool b)
   {
      // slot is reached only if we've created
      // control elements already ...
      if (b)
      {
         if (!_line->text().isEmpty())
         {
            emit sigFilter(_line->text());
         }
      }
      else
      {
         emit sigFilter(QString());
      }
   }

   //---------------------------------------------------------------------------
   //
   //! \brief   enter pressed, trigger sigFilter
   //
   //! \author  Jo2003
   //! \date    29.07.2013
   //
   //! \param   --
   //
   //! \return  --
   //---------------------------------------------------------------------------
   void slotEnter()
   {
      // slot is reached only if we've created
      // control elements already ...
      _chk->setChecked(!_line->text().isEmpty());

      emit sigFilter(_line->text());
   }

signals:
   //---------------------------------------------------------------------------
   //
   //! \brief   signal filter string
   //
   //! \author  Jo2003
   //! \date    29.07.2013
   //
   //! \param   QString filter string
   //
   //! \return  --
   //---------------------------------------------------------------------------
   void sigFilter(QString);
};

#endif // __20130729_QSTRINGFILTERWIDGETACTION_H