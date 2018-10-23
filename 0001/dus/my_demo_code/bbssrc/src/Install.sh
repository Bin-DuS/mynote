#! /bin/sh
# Install.sh a shell script to install whole BBS
# CVS: $Id: Install.sh.in,v 1.17 1999/08/05 09:16:50 edwardc Exp $

BBS_HOME=/home/bbs
BBSUID=9999
BBSGRP=99
INSTALL="/usr/bin/install -c"
TARGET=/home/bbs/bin

echo "This script will install the whole BBS to ${BBS_HOME}..."
echo -n "Press <Enter> to continue ..."
read ans

if [ -d ${BBS_HOME} ] ; then
        echo -n "Warning: ${BBS_HOME} already exists, overwrite whole bbs [N]?"
        read ans
        ans=${ans:-N}
        case $ans in
            [Yy]) echo "Installing new bbs to ${BBS_HOME}" ;;
            *) echo "Abort ..." ; exit ;;
        esac
else
        echo "Making dir ${BBS_HOME}"
        mkdir ${BBS_HOME}
        chown -R ${BBSUID} ${BBS_HOME}
        chgrp -R ${BBSGRP} ${BBS_HOME}
fi

echo "Setup bbs directory tree ....."
( cd ../bbshome ; tar cf - * ) | ( cd ${BBS_HOME} ; tar xf - )
mv ${BBS_HOME}/BOARDS ${BBS_HOME}/.BOARDS
mv ${BBS_HOME}/badname ${BBS_HOME}/.badname
mv ${BBS_HOME}/bad_email ${BBS_HOME}/.bad_email
touch ${BBS_HOME}/.hushlogin

echo "creating necessary empty directory (user mail, user home)"

alphabet="A B C D E F G H I J K L M N O P Q R S T U V W X Y Z"
dirs="mail home"
preboards="boards vote"
boards="bbslists deleted junk newcomers notepad sysop syssecurity vote"

for x in $dirs; \
do \
	echo "in $x .."
	echo -n "   "
	for y in $alphabet; \
	do \
		echo -n " $y"
		mkdir -p ${BBS_HOME}"/$x/$y"; \
	done
	echo " done.  "
done

for x in $preboards; \
do \
	echo "in $x .."
	echo -n "  "
	for y in $boards; \
	do \
		echo -n " $y"
		mkdir -p ${BBS_HOME}"/$x/$y"; \
	done
	echo " done.  "
done

echo " "

echo -n "installing movie items (into boards/notepad) .. "
mv ${BBS_HOME}/movie.dir ${BBS_HOME}/boards/notepad/.DIR
cp ${BBS_HOME}/boards/notepad/.DIR ${BBS_HOME}/boards/notepad/.DIGEST
mv ${BBS_HOME}/movie.1 ${BBS_HOME}/boards/notepad/G.960994645.A
cp ${BBS_HOME}/boards/notepad/G.960994645.A ${BBS_HOME}/boards/notepad/M.960994645.A
mv ${BBS_HOME}/movie.2 ${BBS_HOME}/boards/notepad/G.960994659.A
cp ${BBS_HOME}/boards/notepad/G.960994659.A ${BBS_HOME}/boards/notepad/M.960994659.A
mv ${BBS_HOME}/movie.3 ${BBS_HOME}/boards/notepad/G.960994672.A
cp ${BBS_HOME}/boards/notepad/G.960994672.A ${BBS_HOME}/boards/notepad/M.960994672.A

chown -R ${BBSUID} ${BBS_HOME}
chgrp -R ${BBSGRP} ${BBS_HOME}

if [ -f bbsd ] ; then
	${INSTALL} -m 550  -s -g ${BBSGRP} -o ${BBSUID}  bbsd       ${TARGET}
else
	${INSTALL} -m 550  -s -g ${BBSGRP} -o ${BBSUID}  bbs        ${TARGET}
fi
${INSTALL} -m 550  -s -g ${BBSGRP} -o ${BBSUID}  chatd  	${TARGET}
${INSTALL} -m 4555 -s -g bin       -o root       bbsrf      ${TARGET}
${INSTALL} -m 550  -s -g ${BBSGRP} -o ${BBSUID}  thread     ${TARGET}
${INSTALL} -m 550  -s -g ${BBSGRP} -o ${BBSUID}  expire     ${TARGET}

cat > ${BBS_HOME}/etc/sysconf.ini << EOF
#---------------------------------------------------------------
# Here is where you adjust the BBS System Configuration
# Delete ../sysconf.img to make the change after modification
# Note: after 3.0-19990624-SNAP, system will no longer using 
# site information in sysconf.ini, replace with defined variables
#---------------------------------------------------------------

#SHOW_IDLE_TIME          = 1
KEEP_DELETED_HEADER     = 1

#---------------------------------------------------------------
# Note: after 3.0-19990706-SNAP, SHM keys has been moved into
# include/bbs.h, to improve performance. If you install multiple 
# BBS, make sure the SHM keys of different BBS will not conflict 
# with each other
#---------------------------------------------------------------

#---------------------------------------------------------------
# EMAILFILE  - Toggle the E-Mail Registration Feature
# NEWREGFILE - Toggle the 3 days no-post feature for new comers
#---------------------------------------------------------------
EMAILFILE       = "etc/mailcheck"
#NEWREGFILE     = "etc/newregister"

#---------------------------------------------------------------
# Do not modify anything below unless you know what you are doing...
#---------------------------------------------------------------
PERM_BASIC      = 0x00001
PERM_CHAT       = 0x00002
PERM_PAGE       = 0x00004
PERM_POST       = 0x00008
PERM_LOGINOK    = 0x00010
PERM_DENYPOST   = 0x00020
PERM_CLOAK      = 0x00040
PERM_SEECLOAK   = 0x00080
PERM_XEMPT      = 0x00100
PERM_WELCOME    = 0x00200
PERM_BOARDS     = 0x00400
PERM_ACCOUNTS   = 0x00800
PERM_CHATCLOAK  = 0x01000
PERM_OVOTE      = 0x02000
PERM_SYSOP      = 0x04000
PERM_POSTMASK   = 0x08000
PERM_ANNOUNCE   = 0x10000
PERM_OBOARDS    = 0x20000
PERM_ACBOARD    = 0x40000
PERM_NOZAP      = 0x80000
PERM_FORCEPAGE  = 0x100000
PERM_EXT_IDLE   = 0x200000
PERM_SPECIAL1   = 0x400000
PERM_SPECIAL2   = 0x800000
PERM_SPECIAL3   = 0x1000000
PERM_SPECIAL4   = 0x2000000
PERM_SPECIAL5   = 0x4000000
PERM_SPECIAL6   = 0x8000000
PERM_SPECIAL7   = 0x10000000
PERM_SPECIAL8   = 0x20000000

AUTOSET_PERM	= PERM_CHAT, PERM_PAGE, PERM_POST, PERM_LOGINOK

PERM_ESYSFILE  =  PERM_SYSOP,PERM_WELCOME,PERM_ACBOARD
PERM_ADMINMENU =  PERM_ACCOUNTS,PERM_OVOTE,PERM_SYSOP,PERM_OBOARDS,PERM_WELCOME,PERM_ACBOARD
PERM_BLEVELS   =  PERM_SYSOP,PERM_OBOARDS
PERM_UCLEAN    =  PERM_SYSOP,PERM_ACCOUNTS

#include "etc/menu.ini"

EOF

if test -f ../.reldate; then
  echo "cleanning CVS directories in bbshome ...."
  find ${BBS_HOME} -name "CVS" -print | xargs rm -fr
fi

echo "Install is over...."
echo "Check the configuration in ${BBS_HOME}/etc/sysconf.ini"
echo "Then login your BBS and create an account called SYSOP (case-sensitive)"
