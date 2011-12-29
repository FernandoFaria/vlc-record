/*********************** Information *************************\
| $HeadURL$
|
| Author: Jo2003
|
| Begin: 06.02.2010 / 09:20:35
|
| Last edited by: $Author$
|
| $Id$
\*************************************************************/
#ifndef __02062010__CUSTOMIZATION_H
   #define __02062010__CUSTOMIZATION_H

#ifdef _CUST_RUSS_TELEK
   #define COMPANY_NAME "RussianTelek.com"
   #define VERSION_APPENDIX " rt"
   #define _IS_OEM
   #define COMPANY_LINK \
      "<a href='http://www.russiantelek.com'>" COMPANY_NAME "</a>"
/*
#elif defined _CUST_XXX_XXX
   #define COMPANY_NAME "Whatever.com"
   #define VERSION_APPENDIX " w"
   #define COMPANY_LINK \
      "<a href='http://www.whatever.com'>" COMPANY_NAME "</a>"
*/
#elif defined _CUST_RUSS_SERVICES
   #define COMPANY_NAME "Russian Services"
   #define VERSION_APPENDIX " rs"
   #define _IS_OEM
   #define COMPANY_LINK \
            "<a href='http://www.russian-services.com/tvshop'>" COMPANY_NAME "</a>"
#else
   #define COMPANY_NAME "Kartina.TV"
   #define VERSION_APPENDIX
   #define COMPANY_LINK \
      "<a href='http://www.kartina.tv'>" COMPANY_NAME "</a>"
#endif

#endif /* __02062010__CUSTOMIZATION_H */
/************************* History ***************************\
| $Log$
\*************************************************************/
