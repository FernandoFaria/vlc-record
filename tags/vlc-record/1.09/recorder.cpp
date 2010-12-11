/*********************** Information *************************\
| $HeadURL$
|
| Author: Joerg Neubert
|
| Begin: 19.01.2010 / 16:01:09
|
| Last edited by: $Author$
|
| $Id$
\*************************************************************/
#include "recorder.h"
#include "ui_recorder.h"
#include "chanlistwidgetitem.h"

/* -----------------------------------------------------------------\
|  Method: Recorder / constructor
|  Begin: 19.01.2010 / 16:01:44
|  Author: Joerg Neubert
|  Description: create recorder object, init values,
|               make connections
|
|  Parameters: pointer to translator, pointer to parent widget
|
|  Returns: --
\----------------------------------------------------------------- */
Recorder::Recorder(QTranslator *trans, QWidget *parent)
    : QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint),
    ui(new Ui::Recorder)
{
   ui->setupUi(this);

   bRecord      = true;
   bLogosReady  = false;
   pTranslator  = trans;
   iEpgOffset   = 0;
   sLogoPath    = dwnLogos.GetLogoPath();

   ui->textEpg->SetLogoDir(sLogoPath);
   // timeRec.SetLogoPath(sLogoPath);

   VlcLog.SetLogFile(QString(INI_DIR).arg(getenv(APPDATA)).toLocal8Bit().data(), "vlc-record.log");

   // if main settings aren't done, start settings dialog ...
   if ((Settings.GetPasswd() == "")
        || (Settings.GetVLCPath() == "")
        || (Settings.GetUser() == ""))
   {
      Settings.exec();
   }

   // set log level ...
   VlcLog.SetLogLevel(Settings.GetLogLevel());

   // set connection data ...
   KartinaTv.SetData(KARTINA_HOST, Settings.GetUser(), Settings.GetPasswd(), Settings.AllowEros());


   // set proxy stuff ...
   if (Settings.UseProxy())
   {
      KartinaTv.setProxy(Settings.GetProxyHost(), Settings.GetProxyPort(),
                         Settings.GetProxyUser(), Settings.GetProxyPasswd());

      dwnLogos.setProxy(Settings.GetProxyHost(), Settings.GetProxyPort(),
                        Settings.GetProxyUser(), Settings.GetProxyPasswd());
   }

   // set language as read ...
   pTranslator->load(QString("lang_%1").arg(Settings.GetLanguage ()), QApplication::applicationDirPath());

   // configure trigger and start it ...
   Trigger.SetKartinaClient(&KartinaTv);
   Trigger.start();

   // connect signals and slots ...
   connect (&KartinaTv, SIGNAL(sigError(QString)), this, SLOT(slotErr(QString)));
   connect (&KartinaTv, SIGNAL(sigGotChannelList(QString)), this, SLOT(slotChanList(QString)));
   connect (&KartinaTv, SIGNAL(sigGotStreamURL(QString)), this, SLOT(slotStreamURL(QString)));
   connect (&KartinaTv, SIGNAL(sigGotCookie()), this, SLOT(slotCookie()));
   connect (&KartinaTv, SIGNAL(sigGotEPG(QString)), this, SLOT(slotEPG(QString)));
   connect (&KartinaTv, SIGNAL(sigTimeShiftSet()), this, SLOT(slotTimeShift()));
   connect (&Refresh,   SIGNAL(timeout()), &Trigger, SLOT(slotReqChanList()));
   connect (ui->textEpg, SIGNAL(anchorClicked(QUrl)), this, SLOT(slotEpgAnchor(QUrl)));
   connect (&dwnLogos, SIGNAL(sigLogosReady()), this, SLOT(slotLogosReady()));
   connect (&Settings, SIGNAL(sigReloadLogos()), this, SLOT(slotReloadLogos()));
   connect (&KartinaTv, SIGNAL(sigGotArchivURL(QString)), this, SLOT(slotArchivURL(QString)));
   connect (&Settings, SIGNAL(sigSetServer(int)), this, SLOT(slotSetSServer(int)));
   connect (&Settings, SIGNAL(sigSetBuffer(int)), this, SLOT(slotSetHttpBuffer(int)));

   // -------------------------------------------
   // create epg nav bar ...
   // -------------------------------------------
   TouchEpgNavi (true);

   // enable button ...
   EnableDisableDlg(false);

   // request authorisation ...
   Trigger.TriggerRequest(Kartina::REQ_COOKIE);

   // start refresh timer, if needed ...
   if (Settings.DoRefresh())
   {
      Refresh.start(Settings.GetRefrInt() * 60000); // 1 minutes: (60 * 1000 msec) ...
   }
}

/* -----------------------------------------------------------------\
|  Method: ~Recorder / dstructor
|  Begin: 19.01.2010 / 16:04:40
|  Author: Joerg Neubert
|  Description: clean at destruction
|
|  Parameters: --
|
|  Returns: --
\----------------------------------------------------------------- */
Recorder::~Recorder()
{
   delete ui;
}

/* -----------------------------------------------------------------\
|  Method: TouchEpgNavi
|  Begin: 19.01.2010 / 16:05:00
|  Author: Joerg Neubert
|  Description: create / translate EPG navbar
|
|  Parameters: create flag
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::TouchEpgNavi (bool bCreate)
{
   QToolButton *pBtn;

   if (bCreate)
   {
      /*
        Note: We can't create the navbar in Qt Creator or
        Designer because QTabBar isn't supported there.
        Therefore we create it manually here.

        Format navbar:

        /-----------------------------------------------------\
        | <-- |\/\/| /Mon/Tue/Wed/Thu/Fri/Sat/Sun/ |\/\/| --> |
        \-----------------------------------------------------/

      */

      // create back button and set style ...
      pBtn = new QToolButton;
      pBtn->setIcon(QIcon(":png/back"));
      pBtn->setAutoRaise(true);
      pBtn->setMaximumHeight(EPG_NAVBAR_HEIGHT);
      pBtn->setToolTip(tr("1 week backward"));

      // connect signal with slot ...
      connect (pBtn, SIGNAL(clicked()), this, SLOT(slotbtnBack_clicked()));

      // add button to layout ...
      ui->hLayoutEpgNavi->addWidget(pBtn);

      // create tabbar (epg navbar) and set height ...
      pEpgNavbar = new QTabBar;
      pEpgNavbar->setMaximumHeight(EPG_NAVBAR_HEIGHT);

      // set style so tabs look like whole design ...
      pEpgNavbar->setStyleSheet(QString(NAVBAR_STYLE));

      // connect signal with slot ...
      connect (pEpgNavbar, SIGNAL(currentChanged(int)), this, SLOT(slotDayTabChanged(int)));

      // add h spacer ...
      ui->hLayoutEpgNavi->addStretch();

      // add navbar ...
      ui->hLayoutEpgNavi->addWidget(pEpgNavbar);

      // add h spacer ...
      ui->hLayoutEpgNavi->addStretch();

      // create next button and set style ...
      pBtn = new QToolButton;
      pBtn->setIcon(QIcon(":png/next"));
      pBtn->setAutoRaise(true);
      pBtn->setMaximumHeight(EPG_NAVBAR_HEIGHT);
      pBtn->setToolTip(tr("1 week forward"));

      // connect signal with slot ...
      connect (pBtn, SIGNAL(clicked()), this, SLOT(slotbtnNext_clicked()));

      // add button to layout ...
      ui->hLayoutEpgNavi->addWidget(pBtn);

      // create day tabs ...
      pEpgNavbar->addTab(tr("Mon"));
      pEpgNavbar->addTab(tr("Tue"));
      pEpgNavbar->addTab(tr("Wed"));
      pEpgNavbar->addTab(tr("Thu"));
      pEpgNavbar->addTab(tr("Fri"));
      pEpgNavbar->addTab(tr("Sat"));

      // set color for normal week days ...
      for (int i = 0; i < 5; i++)
      {
         pEpgNavbar->setTabTextColor(i, QColor("white"));
      }

      pEpgNavbar->setTabTextColor(5, QColor("blue"));
      pEpgNavbar->addTab(tr("Sun"));
      pEpgNavbar->setTabTextColor(6, QColor("red"));
   }
   else
   {
      // no creation, only translation ...
      pEpgNavbar->setTabText(0, tr("Mon"));
      pEpgNavbar->setTabText(1, tr("Tue"));
      pEpgNavbar->setTabText(2, tr("Wed"));
      pEpgNavbar->setTabText(3, tr("Thu"));
      pEpgNavbar->setTabText(4, tr("Fri"));
      pEpgNavbar->setTabText(5, tr("Sat"));
      pEpgNavbar->setTabText(6, tr("Sun"));

      // fill in tooltip for navi buttons ...
      int iIdx;
      // back button ...
      iIdx = 0;
      pBtn = (QToolButton *)ui->hLayoutEpgNavi->itemAt(iIdx)->widget();
      pBtn->setToolTip(tr("1 week backward"));

      // next button ...
      iIdx = ui->hLayoutEpgNavi->count() - 1;
      pBtn = (QToolButton *)ui->hLayoutEpgNavi->itemAt(iIdx)->widget();
      pBtn->setToolTip(tr("1 week forward"));
   }
}

/* -----------------------------------------------------------------\
|  Method: changeEvent
|  Begin: 19.01.2010 / 16:05:00
|  Author: Joerg Neubert
|  Description: catch language change event
|
|  Parameters: pointer to event
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);

        // translate manual created navbar ...
        TouchEpgNavi (false);
        break;
    default:
        break;
    }
}

/* -----------------------------------------------------------------\
|  Method: FillChannelList
|  Begin: 19.01.2010 / 16:05:24
|  Author: Joerg Neubert
|  Description: fill channel list
|
|  Parameters: vector with channel elements
|
|  Returns: 0
\----------------------------------------------------------------- */
int Recorder::FillChannelList (const QVector<cparser::SChan> &chanlist)
{
   QString  sLine;
   QString  sToolTip;
   QString  sLogoFile;
   CChanListWidgetItem *pItem;
   int      iRow, iRowGroup;
   QPixmap  Pix(16, 16);

   // get current item ...
   iRow      = ui->listWidget->currentRow();
   iRow      = (iRow <= 0) ? 1 : iRow;
   iRowGroup = ui->cbxChannelGroup->currentIndex();
   iRowGroup = (iRowGroup < 0) ? 0 : iRowGroup;

   // clear channel list ...
   ui->listWidget->clear();

   // clear channel group list ...
   ui->cbxChannelGroup->clear();

   for (int i = 0; i < chanlist.size(); i++)
   {
      // is this a channel group ... ?
      if ((chanlist[i].iIdx == -1) && (chanlist[i].iId == -1))
      {
         pItem = new CChanListWidgetItem (chanlist[i].sName, -1, ui->listWidget);
         pItem->setBackgroundColor(QColor(chanlist[i].sProgramm));

         // add channel group entry ...
         Pix.fill(QColor(chanlist[i].sProgramm));
         ui->cbxChannelGroup->addItem(QIcon(Pix), chanlist[i].sName, QVariant(i));
      }
      else
      {
         sLogoFile = QString("%1/%2.gif").arg(sLogoPath).arg(chanlist[i].iId);
         sLine     = QString("%1. %2").arg(chanlist[i].iIdx).arg(chanlist[i].sName);

         if (QFile::exists(sLogoFile))
         {
            pItem = new CChanListWidgetItem (QIcon(sLogoFile), sLine, chanlist[i].iId, chanlist[i].sName, ui->listWidget);
         }
         else
         {
            pItem = new CChanListWidgetItem (sLine, chanlist[i].iId, chanlist[i].sName, ui->listWidget);
         }

         // create tool tip with programm info ...
         sToolTip = tr("<b style='color: red;'>%1</b><br>\n"
                       "<b>Programm:</b> %2<br>\n"
                       "<b>Start:</b> %3<br>\n<b>End:</b> %4")
                       .arg(chanlist[i].sName).arg(chanlist[i].sProgramm)
                       .arg(chanlist[i].sStart).arg(chanlist[i].sEnd);

         pItem->setToolTip(sToolTip);

         pItem->SetStartEnd(chanlist[i].sStart, chanlist[i].sEnd);
      }
   }

   ui->cbxChannelGroup->setCurrentIndex(iRowGroup);
   ui->listWidget->setCurrentRow (iRow);
   ui->listWidget->setFocus(Qt::OtherFocusReason);

   return 0;
}

/* -----------------------------------------------------------------\
|  Method: StartVlcRec
|  Begin: 19.01.2010 / 16:06:16
|  Author: Joerg Neubert
|  Description: start VLC to record stream
|
|  Parameters: stream url, channel name, archiv flag
|
|  Returns: 0
\----------------------------------------------------------------- */
int Recorder::StartVlcRec (const QString &sURL, const QString &sChannel, int iCacheTime, bool bArchiv)
{
   QDateTime now    = QDateTime::currentDateTime();
   QString sTarget  = QString("%1\\%2_%3").arg(Settings.GetTargetDir()).arg(sChannel).arg(now.toString("yyyy-MM-dd_hh-mm-ss"));
   QString fileName = QFileDialog::getSaveFileName(this, tr("Save Stream as"),
                       sTarget, tr("Transport Stream (*.ts);;AVI File (*.avi)"));
   QRegExp rx("^.*[.]{1}([a-zA-Z]*)$");
   if(rx.indexIn(fileName) > -1)
   {
      QString sCmdLine = VLC_REC_TEMPL;
      sCmdLine.replace(TMPL_VLC, Settings.GetVLCPath());
      sCmdLine.replace(TMPL_URL, sURL);
      sCmdLine.replace(TMPL_MUX, rx.cap(1));
      sCmdLine.replace(TMPL_DST, fileName);

      if (bArchiv)
      {
         // standard cache time: 3000 ... but give a little more ...
         sCmdLine += " --rtsp-tcp --rtsp-caching 5000";
      }
      else
      {
         // add buffer value from kartina ...
         sCmdLine += QString(" --http-caching %1 --no-http-reconnect").arg(iCacheTime);
      }

      VlcLog.LogInfo(QString("Starting VLC using following command line:\n%1\n").arg(sCmdLine));

      // Start the QProcess instance.
      QProcess::startDetached(sCmdLine);
   }
   else
   {
      QMessageBox::critical(this, tr("Error!"), tr("Can't recognice file extension!"));
   }

   /* O.k., everything is fine now, leave the Qt application. The external program
    * will continue running.
    */
   return 0;
}

/* -----------------------------------------------------------------\
|  Method: StartVlcPlay
|  Begin: 19.01.2010 / 16:06:16
|  Author: Joerg Neubert
|  Description: start VLC to show stream
|
|  Parameters: stream url, archiv flag
|
|  Returns: 0
\----------------------------------------------------------------- */
int Recorder::StartVlcPlay (const QString &sURL, int iCacheTime, bool bArchiv)
{
   QString sCmdLine = VLC_PLAY_TEMPL;
   sCmdLine.replace(TMPL_VLC, Settings.GetVLCPath());
   sCmdLine.replace(TMPL_URL, sURL);

   if (bArchiv)
   {
      // standard cache time: 3000 ... but give a little more ...
      sCmdLine += " --rtsp-tcp --rtsp-caching 5000";
   }
   else
   {
      // add buffer value from kartina ...
      sCmdLine += QString(" --http-caching %1 --no-http-reconnect").arg(iCacheTime);
   }

   VlcLog.LogInfo(QString("Starting VLC using following command line:\n%1\n").arg(sCmdLine));

   // Start the QProcess instance.
   QProcess::startDetached(sCmdLine);

   /* O.k., everything is fine now, leave the Qt application. The external program
    * will continue running.
    */
   return 0;
}

/* -----------------------------------------------------------------\
|  Method: on_pushSettings_clicked
|  Begin: 19.01.2010 / 16:07:19
|  Author: Joerg Neubert
|  Description: show settings dialog
|
|  Parameters: --
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::on_pushSettings_clicked()
{
   if (Settings.exec() == QDialog::Accepted)
   {
      // if changes where saved, accept it here ...
      VlcLog.SetLogLevel(Settings.GetLogLevel());

      ui->listWidget->clear();
      KartinaTv.abort();

      // update connection data ...
      KartinaTv.SetData(KARTINA_HOST, Settings.GetUser(), Settings.GetPasswd(), Settings.AllowEros());

      // set proxy ...
      if (Settings.UseProxy())
      {
         KartinaTv.setProxy(Settings.GetProxyHost(), Settings.GetProxyPort(),
                            Settings.GetProxyUser(), Settings.GetProxyPasswd());

         dwnLogos.setProxy(Settings.GetProxyHost(), Settings.GetProxyPort(),
                           Settings.GetProxyUser(), Settings.GetProxyPasswd());
      }

      // set language as read ...
      pTranslator->load(QString("lang_%1").arg(Settings.GetLanguage ()), QApplication::applicationDirPath());

      EnableDisableDlg(false);

      // authenticate ...
      Trigger.TriggerRequest(Kartina::REQ_COOKIE);

      // set refresh timer ...
      if (Settings.DoRefresh())
      {
         if (!Refresh.isActive())
         {
            Refresh.start(Settings.GetRefrInt() * 60000); // 1 minutes: (60 * 1000 msec) ...
         }
      }
      else
      {
         if (Refresh.isActive())
         {
            Refresh.stop();
         }
      }
   }
}

/* -----------------------------------------------------------------\
|  Method: slotErr
|  Begin: 19.01.2010 / 16:08:51
|  Author: Joerg Neubert
|  Description: display errors signaled by other threads
|
|  Parameters: error string
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::slotErr(QString str)
{
   QMessageBox::critical(this, tr("Error"),
                         tr("Kartina.tv Client API reports some errors: %1").arg(str));
   EnableDisableDlg();
}

/* -----------------------------------------------------------------\
|  Method: slotChanList
|  Begin: 19.01.2010 / 16:09:23
|  Author: Joerg Neubert
|  Description: handle requested channel list
|
|  Parameters: channel list (xml)
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::slotChanList (QString str)
{
   QVector<cparser::SChan> chanList;
   XMLParser.SetByteArray(str.toUtf8());

   chanList = XMLParser.ParseChannelList(Settings.FixTime());

   // set timeshift in epg browser ...
   ui->textEpg->SetTimeShift(XMLParser.GetTimeShift());

   FillChannelList(chanList);

   // set channel list in timeRec class ...
   // timeRec.SetTimeShift(XMLParser.GetTimeShift());
   // timeRec.SetChanList(chanList);

   // only download channel logos, if they aren't there ...
   if (!dwnLogos.IsRunning() && !bLogosReady)
   {
      dwnLogos.SetChanList(chanList);
   }

   // mark current timeshift value ...
   int iIdx = ui->cbxTimeShift->findText(QString::number(XMLParser.GetTimeShift()));
   ui->cbxTimeShift->setCurrentIndex((iIdx < 0) ? 0 : iIdx);

   EnableDisableDlg();
}

/* -----------------------------------------------------------------\
|  Method: slotStreamURL
|  Begin: 19.01.2010 / 16:09:54
|  Author: Joerg Neubert
|  Description: handle requested stream url
|
|  Parameters: stream url (xml)
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::slotStreamURL(QString str)
{
   QString              sChan, sUrl;
   int                  iCacheTime;
   CChanListWidgetItem *pItem = (CChanListWidgetItem *)ui->listWidget->currentItem();

   XMLParser.SetByteArray(str.toUtf8());

   sUrl = XMLParser.ParseURL(iCacheTime);

   if (pItem)
   {
      sChan = pItem->text();
   }

   if (bRecord)
   {
      StartVlcRec(sUrl, sChan, iCacheTime);
   }
   else
   {
      StartVlcPlay(sUrl, iCacheTime);
   }

   EnableDisableDlg();
}

/* -----------------------------------------------------------------\
|  Method: slotCookie
|  Begin: 19.01.2010 / 16:10:23
|  Author: Joerg Neubert
|  Description: authentication done, request channel list
|
|  Parameters: --
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::slotCookie()
{
   Trigger.TriggerRequest(Kartina::REQ_CHANNELLIST);
}

/* -----------------------------------------------------------------\
|  Method: slotEPG
|  Begin: 19.01.2010 / 16:10:49
|  Author: Joerg Neubert
|  Description: handle requested epg info
|
|  Parameters: epg (xml)
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::slotEPG(QString str)
{
   QString                sDlgTitle;
   QVector<cparser::SEpg> epg;
   int                    cid     = 0;
   uint                   uiGmt   = 0;
   bool                   bArchiv = false;
   QDateTime              epgTime = QDateTime::currentDateTime().addDays(iEpgOffset);


   CChanListWidgetItem *pItem = (CChanListWidgetItem *)ui->listWidget->currentItem();

   if (pItem)
   {
      sDlgTitle = pItem->GetName();
   }

   XMLParser.SetByteArray(str.toUtf8());

   epg = XMLParser.ParseEpg(cid, uiGmt, bArchiv);

   ui->textEpg->DisplayEpg(epg, sDlgTitle, cid, epgTime.toTime_t(), bArchiv);

   // fill epg controll ...
   if (QFile::exists(QString("%1/%2.gif").arg(sLogoPath).arg(cid)))
   {
      QPixmap pic;
      pic.load(QString("%1/%2.gif").arg(sLogoPath).arg(cid));
      ui->labChanIcon->setPixmap(pic);
   }

   ui->labChanName->setText(sDlgTitle);
   ui->labCurrDay->setText(epgTime.toString("dd. MMM. yyyy"));

   pEpgNavbar->setCurrentIndex(epgTime.date().dayOfWeek() - 1);

   EnableDisableDlg();
   ui->listWidget->setFocus(Qt::OtherFocusReason);
}

/* -----------------------------------------------------------------\
|  Method: slotTimeShift
|  Begin: 19.01.2010 / 16:11:30
|  Author: Joerg Neubert
|  Description: time shift set, reload channel list
|
|  Parameters: --
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::slotTimeShift ()
{
   Trigger.TriggerRequest(Kartina::REQ_CHANNELLIST);
}

/* -----------------------------------------------------------------\
|  Method: on_pushRecord_clicked
|  Begin: 19.01.2010 / 16:11:52
|  Author: Joerg Neubert
|  Description: request stream url for record
|
|  Parameters: --
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::on_pushRecord_clicked()
{
   CChanListWidgetItem *pItem = (CChanListWidgetItem *)ui->listWidget->currentItem();

   if (pItem && (pItem->GetId() != -1))
   {
      EnableDisableDlg(false);
      bRecord = true;
      Trigger.TriggerRequest(Kartina::REQ_STREAM, pItem->GetId());
   }
}

/* -----------------------------------------------------------------\
|  Method: on_pushPlay_clicked
|  Begin: 19.01.2010 / 16:12:20
|  Author: Joerg Neubert
|  Description: request stream url for play
|
|  Parameters: --
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::on_pushPlay_clicked()
{
   CChanListWidgetItem *pItem = (CChanListWidgetItem *)ui->listWidget->currentItem();

   if (pItem && (pItem->GetId() != -1))
   {
      EnableDisableDlg(false);
      bRecord = false;
      Trigger.TriggerRequest(Kartina::REQ_STREAM, pItem->GetId());
   }
}

/* -----------------------------------------------------------------\
|  Method: on_cbxTimeShift_currentIndexChanged
|  Begin: 19.01.2010 / 16:13:03
|  Author: Joerg Neubert
|  Description: set new timeshift
|
|  Parameters: --
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::on_cbxTimeShift_currentIndexChanged(QString str)
{
   EnableDisableDlg(false);
   Trigger.TriggerRequest(Kartina::REQ_TIMESHIFT, str.toInt());
}

/* -----------------------------------------------------------------\
|  Method: EnableDisableDlg
|  Begin: 19.01.2010 / 16:13:30
|  Author: Joerg Neubert
|  Description: enable / disable buttons
|
|  Parameters: enable flag
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::EnableDisableDlg (bool bEnable)
{
   ui->cbxTimeShift->setEnabled(bEnable);
   ui->listWidget->setEnabled(bEnable);
   ui->pushPlay->setEnabled(bEnable);
   ui->pushRecord->setEnabled(bEnable);
}

/* -----------------------------------------------------------------\
|  Method: on_listWidget_currentRowChanged
|  Begin: 19.01.2010 / 16:13:56
|  Author: Joerg Neubert
|  Description: channel changed, request epg if needed
|
|  Parameters: slected row index
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::on_listWidget_currentRowChanged(int currentRow)
{
   CChanListWidgetItem *pItem = (CChanListWidgetItem *)ui->listWidget->item(currentRow);
   if (pItem && (pItem->GetId() != -1))
   {
      ui->textEpgShort->setHtml(QString(TMPL_BACKCOLOR).arg("rgb(255, 254, 212)").arg(pItem->toolTip()));
      SetProgress (pItem->GetStartTime(), pItem->GetEndTime());

      // was this a refresh or was channel changed ... ?
      if (pItem->GetId() != ui->textEpg->GetCid())
      {
         // channel changed ...
         iEpgOffset = 0;

         // load epg ...
         Trigger.TriggerRequest(Kartina::REQ_EPG, pItem->GetId());
      }
      else // same channel ...
      {
         // refresh epg only, if we view current day in epg ...
         if (iEpgOffset == 0) // 0 means today!
         {
            Trigger.TriggerRequest(Kartina::REQ_EPG, pItem->GetId());
         }
      }
   }
}

/* -----------------------------------------------------------------\
|  Method: on_cbxChannelGroup_activated
|  Begin: 19.01.2010 / 16:14:40
|  Author: Joerg Neubert
|  Description: jump tp other channel group
|
|  Parameters: new channel group index
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::on_cbxChannelGroup_activated(int index)
{
   int iListIdx = ui->cbxChannelGroup->itemData(index).toInt();

   CChanListWidgetItem *pItem = (CChanListWidgetItem *)ui->listWidget->item(iListIdx);

   ui->listWidget->scrollToItem(pItem, QAbstractItemView::PositionAtTop);
   ui->listWidget->setCurrentRow(iListIdx + 1);
   ui->listWidget->setFocus(Qt::OtherFocusReason);
}

/* -----------------------------------------------------------------\
|  Method: SetProgress
|  Begin: 19.01.2010 / 16:15:17
|  Author: Joerg Neubert
|  Description: set progress bar (program time)
|
|  Parameters: start and stop time as string
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::SetProgress(const QString &start, const QString &end)
{
   int iPercent = 0;

   if ((start != "") && (end != ""))
   {
      QDateTime dtStart  = QDateTime::fromString(start, DEF_TIME_FORMAT);
      QDateTime dtEnd    = QDateTime::fromString(end, DEF_TIME_FORMAT);
      QDateTime dtNow    = QDateTime::currentDateTime();

      int       iLength  = (int)(dtEnd.toTime_t() - dtStart.toTime_t());
      int       iNow     = (int)(dtNow.toTime_t() - dtStart.toTime_t());

      // error check (div / 0 PC doesn't like ;-) ) ...
      if ((iNow > 0) && (iLength > 0))
      {
         // get percent ...
         iPercent        = (int)((iNow * 100) / iLength);
      }
   }

   ui->progressBar->setValue(iPercent);
}

/* -----------------------------------------------------------------\
|  Method: on_pushAbout_clicked
|  Begin: 19.01.2010 / 16:15:56
|  Author: Joerg Neubert
|  Description: show about dialog
|
|  Parameters: --
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::on_pushAbout_clicked()
{
   CAboutDialog dlg;
   dlg.exec();
}

/* -----------------------------------------------------------------\
|  Method: slotEpgAnchor
|  Begin: 19.01.2010 / 16:16:17
|  Author: Joerg Neubert
|  Description: link in epg browser was clicked,
|               parse and handle request
|  Parameters: clicked link
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::slotEpgAnchor (const QUrl &link)
{
   // create request string ...
   QString action = link.encodedQueryItemValue(QByteArray("action"));
   bool    ok     = false;

   if (action == "archivrec")
   {
      ok      = true;
      bRecord = true;
   }
   else if (action == "archivplay")
   {
      ok      = true;
      bRecord = false;
   }
   else if(action == "timerrec")
   {
      /*
      uint uiStart = link.encodedQueryItemValue(QByteArray("start")).toUInt();
      uint uiEnd   = link.encodedQueryItemValue(QByteArray("end")).toUInt();
      int  iChan   = link.encodedQueryItemValue(QByteArray("cid")).toInt();

      timeRec.SetRecInfo(uiStart, uiEnd, iChan);
      timeRec.exec();
      */
   }

   if (ok)
   {
      QString cid    = link.encodedQueryItemValue(QByteArray("cid"));
      QString gmt    = link.encodedQueryItemValue(QByteArray("gmt"));
      QString req    = QString("m=channels&act=get_stream_url&cid=%1&gmt=%2")
                       .arg(cid.toInt()).arg(gmt.toUInt());

      Trigger.TriggerRequest(Kartina::REQ_ARCHIV, req);
   }
}

/* -----------------------------------------------------------------\
|  Method: slotLogosReady
|  Begin: 19.01.2010 / 16:17:23
|  Author: Joerg Neubert
|  Description: logo downloader told us that logos are ready ...
|
|  Parameters: --
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::slotLogosReady()
{
   // downloader sayd ... logos are there ...
   bLogosReady = true;
}

/* -----------------------------------------------------------------\
|  Method: slotReloadLogos
|  Begin: 19.01.2010 / 16:17:54
|  Author: Joerg Neubert
|  Description: trigger reload of channel logos
|
|  Parameters: --
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::slotReloadLogos()
{
   bLogosReady = false;

   if (!dwnLogos.IsRunning())
   {
      QVector<cparser::SChan> tmpChanList;
      cparser::SChan          listEntry;
      CChanListWidgetItem    *pItem = NULL;

      // create tmp channel list with channels from listWidget ...
      for (int i = 0; i < ui->listWidget->count(); i++)
      {
         pItem = (CChanListWidgetItem *)ui->listWidget->item(i);

         if (pItem->GetId() > -1)
         {
            listEntry.iId  = pItem->GetId();
            listEntry.iIdx = 0;

            tmpChanList.push_back(listEntry);
         }
      }

      dwnLogos.SetChanList(tmpChanList);
   }
}

/* -----------------------------------------------------------------\
|  Method: slotbtnBack_clicked
|  Begin: 19.01.2010 / 16:18:30
|  Author: Joerg Neubert
|  Description: one week backward in epg
|
|  Parameters: --
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::slotbtnBack_clicked()
{
   CChanListWidgetItem *pItem = (CChanListWidgetItem *)ui->listWidget->currentItem();
   if (pItem && (pItem->GetId() != -1))
   {
      iEpgOffset = iEpgOffset - 7;
      Trigger.TriggerRequest(Kartina::REQ_EPG, pItem->GetId(), iEpgOffset);
   }
}

/* -----------------------------------------------------------------\
|  Method: slotbtnNext_clicked
|  Begin: 19.01.2010 / 16:18:30
|  Author: Joerg Neubert
|  Description: one week forward in epg
|
|  Parameters: --
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::slotbtnNext_clicked()
{
   CChanListWidgetItem *pItem = (CChanListWidgetItem *)ui->listWidget->currentItem();
   if (pItem && (pItem->GetId() != -1))
   {
      iEpgOffset = iEpgOffset + 7;
      Trigger.TriggerRequest(Kartina::REQ_EPG, pItem->GetId(), iEpgOffset);
   }
}

/* -----------------------------------------------------------------\
|  Method: slotArchivURL
|  Begin: 19.01.2010 / 16:19:25
|  Author: Joerg Neubert
|  Description: got requested archiv url, start play / record
|
|  Parameters: archiv url (xml)
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::slotArchivURL(QString str)
{
   QString              sChan, sUrl;

   CChanListWidgetItem *pItem = (CChanListWidgetItem *)ui->listWidget->currentItem();

   XMLParser.SetByteArray(str.toUtf8());

   sUrl = XMLParser.ParseArchivURL();

   VlcLog.LogInfo(sUrl + "\n");

   if (pItem)
   {
      sChan = pItem->text();
   }

   if (bRecord)
   {
      StartVlcRec(sUrl, sChan, 0, true);
   }
   else
   {
      StartVlcPlay(sUrl, 0, true);
   }

   EnableDisableDlg();
}

/* -----------------------------------------------------------------\
|  Method: slotDayTabChanged
|  Begin: 19.01.2010 / 16:19:25
|  Author: Joerg Neubert
|  Description: day in epg navi bar changed ...
|
|  Parameters: day index (0 - 6)
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::slotDayTabChanged(int iIdx)
{
   CChanListWidgetItem *pItem = (CChanListWidgetItem *)ui->listWidget->currentItem();
   if (pItem && (pItem->GetId() != -1))
   {
      QDateTime epgTime = QDateTime::currentDateTime().addDays(iEpgOffset);
      int       iDay    = epgTime.date().dayOfWeek() - 1;

      // earlier or later ... ?
      if (iIdx < iDay)
      {
         // earlier ...
         iEpgOffset -= iDay - iIdx;
      }
      else if (iIdx > iDay)
      {
         // later ...
         iEpgOffset += iIdx - iDay;
      }

      // get epg for requested day ...
      if (iIdx != iDay)
      {
         Trigger.TriggerRequest(Kartina::REQ_EPG, pItem->GetId(), iEpgOffset);
      }
   }
}

/* -----------------------------------------------------------------\
|  Method: on_listWidget_itemDoubleClicked
|  Begin: 19.01.2010 / 16:19:25
|  Author: Joerg Neubert
|  Description: double click on channel list -> start play channel
|
|  Parameters: pointer to listwidgetitem ...
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::on_listWidget_itemDoubleClicked(QListWidgetItem* item)
{
   CChanListWidgetItem *pItem = (CChanListWidgetItem *)item;

   if (pItem && (pItem->GetId() != -1))
   {
      bRecord = false;
      EnableDisableDlg(false);
      Trigger.TriggerRequest(Kartina::REQ_STREAM, pItem->GetId());
   }
}

/* -----------------------------------------------------------------\
|  Method: slotSetSServer
|  Begin: 21.01.2010 / 12:19:25
|  Author: Joerg Neubert
|  Description: set stream server request
|
|  Parameters: new server number
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::slotSetSServer(int iSrv)
{
   Trigger.TriggerRequest(Kartina::REQ_SERVER, iSrv);
}

/* -----------------------------------------------------------------\
|  Method: slotSetHttpBuffer
|  Begin: 21.01.2010 / 12:20:25
|  Author: Joerg Neubert
|  Description: set http buffer size request
|
|  Parameters: new buffer time
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::slotSetHttpBuffer(int iTime)
{
   Trigger.TriggerRequest(Kartina::REQ_HTTPBUFF, iTime);
}

/* -----------------------------------------------------------------\
|  Method: on_btnSearch_clicked
|  Begin: 21.01.2010 / 13:58:25
|  Author: Joerg Neubert
|  Description: search button was pressed
|
|  Parameters: --
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::on_btnSearch_clicked()
{
   if(!ui->textEpg->find(ui->lineSearch->text()))
   {
      // not found --> set cursor to document start ...
      ui->textEpg->moveCursor(QTextCursor::Start);
   }
}

/* -----------------------------------------------------------------\
|  Method: on_lineSearch_returnPressed
|  Begin: 21.01.2010 / 13:58:25
|  Author: Joerg Neubert
|  Description: search line enter pressed --> search
|
|  Parameters: --
|
|  Returns: --
\----------------------------------------------------------------- */
void Recorder::on_lineSearch_returnPressed()
{
   if(!ui->textEpg->find(ui->lineSearch->text()))
   {
      // not found --> set cursor to document start ...
      ui->textEpg->moveCursor(QTextCursor::Start);
   }
}

/************************* History ***************************\
| $Log$
\*************************************************************/
