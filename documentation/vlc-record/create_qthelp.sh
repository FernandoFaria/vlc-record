#!/bin/bash
TMPFILE=/tmp/_0815.tmp
HIDDENFEATURES=0

case $1 in 
   vlc-record)
      PROGRAM="VLC-Record"
      SERVICE="Kartina.TV"
      APISERVER="iptv.kartina.tv"
      LANGUAGES="en de ru pl"
      HIDDENFEATURES=1
      ;;
   
   kartina_tv)
      PROGRAM="Kartina.TV"
      SERVICE=$PROGRAM
      APISERVER="iptv.kartina.tv"
      LANGUAGES="en de ru"
      ;;
   
   polsky_tv)
      PROGRAM="Polsky.TV"
      SERVICE=$PROGRAM
      APISERVER="iptv.polsky.tv"
      LANGUAGES="en de pl"
      ;;
   *)
      PROGRAM="VLC-Record"
      SERVICE="Kartina.TV"
      APISERVER="iptv.kartina.tv"
      LANGUAGES="en de ru"
      ;;
   
esac

# delete qch and qhc files ...
rm -f *.qch *.qhc

for i in $LANGUAGES ; do
   # patch files ...
   sed -e "s/\[%PROGRAM%\]/$PROGRAM/g" -e "s/\[%SERVICE%\]/$SERVICE/g" -e "s/\[%APISERVER%\]/$APISERVER/g" help_$i.tmpl > help_$i.html
   sed -e "s/\[%PROGRAM%\]/$PROGRAM/g" help_$i.qhp_tmpl > help_$i.qhp
   
   if [ $HIDDENFEATURES -gt 0 ]; then
      sed -e "s/^.*\[%HIDDEN%\].*$//g" help_$i.html > $TMPFILE
      mv $TMPFILE help_$i.html
      sed -e "s/^.*\[%HIDDEN%\].*$//g" help_$i.qhp > $TMPFILE
      mv $TMPFILE help_$i.qhp
   fi

	qhelpgenerator help_$i.qhp -o help_$i.qch
	qcollectiongenerator help_$i.qhcp -o help_$i.qhc
done
