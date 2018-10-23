/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@rock.b11.ingr.com
                        Guy Vega, gtvega@seabass.st.usm.edu
                        Dominic Tynes, dbtynes@seabass.st.usm.edu
    Firebird Bulletin Board System
    Copyright (C) 1996, Hsien-Tsung Chang, Smallpig.bbs@bbs.cs.ccu.edu.tw
                        Peng Piaw Foong, ppfoong@csie.ncu.edu.tw

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
/*
$Id: main.c,v 1.42 1999/10/01 14:29:21 edwardc Exp $
*/

#include "bbs.h"

#define BADLOGINFILE    "logins.bad"
int     ERROR_READ_SYSTEM_FILE = NA;
int     RMSG = YEA;
int     msg_num = 0;
int     nettyNN = 0;
int     count_friends = 0, count_users = 0;
int     iscolor = 1;
int     mailXX = 0;
char   *getenv();
int     friend_login_wall();
char   *sysconf_str();
char   *Ctime();
struct user_info *t_search();
void    r_msg();
void    count_msg();
void    c_recover();
void    tlog_recover();
int     listmode;
int     numofsig = 0;
jmp_buf byebye;
extern struct BCACHE *brdshm;
FILE   *ufp;
int     talkrequest = NA;
/* int ntalkrequest = NA ; */
int     enter_uflags;
time_t  lastnote;

struct user_info uinfo;

char    netty_path[60];

#ifndef BBSD
char    tty_name[20];
#endif
char	fromhost[60];

char    BoardName[STRLEN];
char    ULIST[STRLEN];
int     utmpent = -1;
time_t  login_start_time;
int     showansi = 1;

void
log_usies(mode, mesg)
char   *mode, *mesg;
{
	time_t  now;
	char    buf[256], *fmt;
	now = time(0);
	fmt = currentuser.userid[0] ? "%s %s %-12s %s\n" : "%s %s %s%s\n";
	sprintf(buf, fmt, Ctime(&now) + 4, mode, currentuser.userid, mesg);
	file_append("usies", buf);
}

void
u_enter()
{
	enter_uflags = currentuser.flags[0];
	memset(&uinfo, 0, sizeof(uinfo));
	uinfo.active = YEA;
	uinfo.pid = getpid();
	if (HAS_PERM(PERM_LOGINCLOAK) && (currentuser.flags[0] & CLOAK_FLAG))
		uinfo.invisible = YEA;
	uinfo.mode = LOGIN;
	uinfo.pager = 0;
#ifdef BBSD
	uinfo.idle_time = time(0);
#endif
	if (DEFINE(DEF_FRIENDCALL)) {
		uinfo.pager |= FRIEND_PAGER;
	}
	if (currentuser.flags[0] & PAGER_FLAG) {
		uinfo.pager |= ALL_PAGER;
		uinfo.pager |= FRIEND_PAGER;
	}
	if (DEFINE(DEF_FRIENDMSG)) {
		uinfo.pager |= FRIENDMSG_PAGER;
	}
	if (DEFINE(DEF_ALLMSG)) {
		uinfo.pager |= ALLMSG_PAGER;
		uinfo.pager |= FRIENDMSG_PAGER;
	}
	uinfo.uid = usernum;
	strncpy(uinfo.from, fromhost, 60);
#if !defined(BBSD) && defined(SHOW_IDLE_TIME)
	strncpy(uinfo.tty, tty_name, 20);
#endif
	iscolor = (DEFINE(DEF_COLOR)) ? 1 : 0;
	strncpy(uinfo.userid, currentuser.userid, 20);
	strncpy(uinfo.realname, currentuser.realname, 20);
	strncpy(uinfo.username, currentuser.username, 40);
	getfriendstr();
	getrejectstr();
	if (HAS_PERM(PERM_EXT_IDLE))
		uinfo.ext_idle = YEA;
		
	listmode = 0;	/* 借用一下, 用来纪录到底 utmpent 卡位失败几次 */
	
	while ( 1 ) {
		utmpent = getnewutmpent(&uinfo);
		if ( utmpent >= 0 || utmpent == -1 )
			break;
		if ( utmpent == -2 && listmode <= 100 ) {
			listmode++;
			usleep(250);		/* 休息四分之一秒再接在励 */
			continue;	
		}
		if ( listmode > 100 ) {	/* 放弃吧 */
			sprintf(genbuf, "getnewutmpent(): too much times, give up.");
			report(genbuf);
			prints("getnewutmpent(): 失败太多次, 放弃. 请回报站长.\n");
			sleep(3);
			exit(0);
		}
	}
	
	if (utmpent < 0) {
		sprintf(genbuf, "Fault: No utmpent slot for %s\n", uinfo.userid);
		report(genbuf);
	}
	listmode = 0;
	digestmode = NA;
}

void
setflags(mask, value)
int     mask, value;
{
	if (((currentuser.flags[0] & mask) && 1) != value) {
		if (value)
			currentuser.flags[0] |= mask;
		else
			currentuser.flags[0] &= ~mask;
	}
}

void
u_exit()
{
	setflags(PAGER_FLAG, (uinfo.pager & ALL_PAGER));
	if (HAS_PERM(PERM_LOGINCLOAK))
		setflags(CLOAK_FLAG, uinfo.invisible);

	if (currentuser.flags[0] != enter_uflags && !ERROR_READ_SYSTEM_FILE) {
		set_safe_record();
		substitute_record(PASSFILE, &currentuser, sizeof(currentuser), usernum);
	}
	uinfo.active = NA;
	uinfo.pid = 0;
	uinfo.invisible = YEA;
	uinfo.sockactive = NA;
	uinfo.sockaddr = 0;
	uinfo.destuid = 0;
#if !defined(BBSD) && defined(SHOW_IDLE_TIME)
	strcpy(uinfo.tty, "NoTTY");
#endif
	update_utmp();
}

int
cmpuids(uid, up)
char   *uid;
struct userec *up;
{
	return !ci_strncmp(uid, up->userid, sizeof(up->userid));
}

int
dosearchuser(userid)
char   *userid;
{
	int     id;
	if ((id = getuser(userid)) != 0) {
		if (cmpuids(userid, &lookupuser)) {
			memcpy(&currentuser, &lookupuser, sizeof(currentuser));
			return usernum = id;
		}
	}
	memset(&currentuser, 0, sizeof(currentuser));
	return usernum = 0;
}

int     started = 0;

void
talk_request()
{
	signal(SIGUSR1, talk_request);
	talkrequest = YEA;
	bell();
	bell();
	bell();
	sleep(1);
	bell();
	bell();
	bell();
	bell();
	bell();
	return;
}

void
abort_bbs()
{
	time_t  stay;


	if (uinfo.mode == POSTING || uinfo.mode == SMAIL || uinfo.mode == EDIT
		|| uinfo.mode == EDITUFILE || uinfo.mode == EDITSFILE || uinfo.mode == EDITANN)
		keep_fail_post();
	if (started) {
		stay = time(0) - login_start_time;
		sprintf(genbuf, "Stay: %3ld (%s)", stay / 60, currentuser.username);
		log_usies("AXXED", genbuf);
		u_exit();
	}
	exit(0);
}

int
cmpuids2(unum, urec)
int     unum;
struct user_info *urec;
{
	return (unum == urec->uid);
}

int
count_multi(uentp)
struct user_info *uentp;
{
	static int count;
	if (uentp == NULL) {
		int     num = count;
		count = 0;
		return num;
	}
	if (!uentp->active || !uentp->pid)
		return 0;
	if (uentp->uid == usernum)
		count++;
	return 1;
}

int
count_user()
{
	count_multi(NULL);
	apply_ulist(count_multi);
	return count_multi(NULL);
}

void
multi_user_check()
{
	struct user_info uin;
	char    buffer[40];
	int     logins;
	if (HAS_PERM(PERM_MULTILOG))
		return;		/* don't check sysops */

	/* allow multiple guest user */
	logins = count_user();

	if (heavyload() && logins) {
		prints("[1;33m抱歉, 目前系统负荷过重, 请勿重覆 Login。[m\n");
		oflush();
		sleep(3);
		exit(1);
	}
	if (!strcasecmp("guest", currentuser.userid)) {
		if (logins > MAXGUEST) {
			prints("[1;33m抱歉, 目前已有太多 [1;36mguest[33m, 请稍後再试。[m\n");
			oflush();
			sleep(3);
			exit(1);
		}
		return;
	} else if (logins >= 5) {
		prints("[1;33m很抱歉, 您已 Login 相同帐号五次, 为确保他人上站权益,\n 此连线将被取消。[m\n");
		oflush();
		sleep(3);
		exit(1);
	}
	if (!search_ulist(&uin, cmpuids2, usernum))
		return;		/* user isn't logged in */

	if (!uin.active || (kill(uin.pid, 0) == -1))
		return;		/* stale entry in utmp file */

	getdata(0, 0, "[1;37m您想删除重复的 login 吗 (Y/N)? [N][m", genbuf, 4,
		DOECHO, YEA);

	if (genbuf[0] == 'Y' || genbuf[0] == 'y') {
		kill(uin.pid, 9);
		sprintf(buffer, "kicked (multi-login)");
		report(buffer);
		log_usies("KICK ", currentuser.username);
	}
}

int
simplepasswd(str, check)
char   *str;
int     check;
{
	char    ch;
	while ((ch = *str++) != '\0') {
		if (check == 1) {
			if (!(ch >= 'a' && ch <= 'z'))
				return 0;
		} else if (!(ch >= '0' && ch <= '9'))
			return 0;
	}
	return 1;
}
#ifndef BBSD
void
system_init(argc, argv)
int     argc;
char  **argv;
#else
void
system_init()
#endif
{
#ifndef BBSD
	char   *rhost;
#endif

	login_start_time = time(0);
	gethostname(genbuf, 256);
#ifdef SINGLE
	if (strcmp(genbuf, SINGLE)) {
		printf("Not on a valid machine!\n");
		exit(-1);
	}
#endif
	sprintf(ULIST, "%s.%s", ULIST_BASE, genbuf);

#ifndef BBSD
	if (argc >= 3) {
		strncpy(fromhost, argv[2], 60);
	} else {
		fromhost[0] = '\0';
	}
	if ((rhost = getenv("REMOTEHOST")) != NULL)
		strncpy(fromhost, rhost, 60);
	fromhost[59] = '\0';
#if defined(SHOW_IDLE_TIME)
	if (argc >= 4) {
		strncpy(tty_name, argv[3], 20);
	} else {
		tty_name[0] = '\0';
	}
#endif
#endif

#ifndef lint
	signal(SIGHUP, abort_bbs);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
#ifdef DOTIMEOUT
	init_alarm();
	uinfo.mode = LOGIN;
	alarm(LOGIN_TIMEOUT);
#else
	signal(SIGALRM, SIG_SIG);
#endif
	signal(SIGTERM, SIG_IGN);
	signal(SIGURG, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
#endif
	signal(SIGTTOU, count_msg);
	signal(SIGUSR1, talk_request);
	signal(SIGUSR2, r_msg);
}

void
system_abort()
{
	if (started) {
		log_usies("ABORT", currentuser.username);
		u_exit();
	}
	clear();
	refresh();
	printf("谢谢光临, 记得常来喔 !\n");
	exit(0);
}

void
logattempt(uid, frm)
char   *uid, *frm;
{
	char    fname[STRLEN];
	sprintf(genbuf, "%-12.12s  %-30s %s\n",
		uid, Ctime(&login_start_time), frm);
	file_append(BADLOGINFILE, genbuf);
	sethomefile(fname, uid, BADLOGINFILE);
	file_append(fname, genbuf);

}

void
login_query()
{
	char    uid[IDLEN + 2], passbuf[PASSLEN];
	int     curr_login_num;
	int     attempts;
#ifdef MAGIC_PASS
	int     magic;
	char    buf[PASSLEN];
#endif
	char    genbuf[STRLEN];
#ifndef BBSNAME
	char   *ptr;
#endif
	extern struct UTMPFILE *utmpshm;


	curr_login_num = num_active_users();
	if (curr_login_num >= MAXACTIVE) {
		ansimore("etc/loginfull", NA);
		oflush();
		sleep(1);
		exit(1);
	}
#ifdef BBSNAME
	strcpy(BoardName, BBSNAME);
	/* edwardc.990508 copy from defined variable */
#else
	ptr = sysconf_str("BBSNAME");
	if (ptr == NULL)
		ptr = "尚未命名测试站";
	strcpy(BoardName, ptr);
#endif

	if (fill_shmfile(1, "etc/issue", "ISSUE_SHMKEY")) {
		show_issue();	/* is anibanner ready, remark this and put
				 * \n\n */
	}
	prints("[1;35m欢迎光临[1;40;33m【 %s 】[0;1;32m。 本站一共可以让 [1;36m%d[0;1;32m 人注册使用。[m\n",
		BoardName, MAXUSERS);
	resolve_utmp();
	if (utmpshm->usersum == 0)
		utmpshm->usersum = allusers();
	if (utmpshm->max_login_num < curr_login_num)
		utmpshm->max_login_num = curr_login_num;

#ifndef VERSION_ID
	prints("[1;36m目前上站人数:[1;40;37m [%d/%d] [0;1;32m。 最高人数记录: [[1;36m%d[1;32m]。 目前已有 [1;36m%d[32m 个注册帐号。[m\n",
		curr_login_num, MAXACTIVE, utmpshm->max_login_num, utmpshm->usersum);
#else
	prints("[1;36m目前上站人数:[1;40;37m [%d/%d] [0;1;32m。 目前已有 [1;36m%d[32m 个注册帐号。 ”%s〔[m\n",
		curr_login_num, MAXACTIVE, utmpshm->usersum, VERSION_ID);
#endif

#ifdef MUDCHECK_BEFORELOGIN

	prints("[1;33m为防止使用程式上站，请按 [1;36mCTRL + C[m : ");
	
	genbuf[0] = igetkey();
	
	if ( genbuf[0] != Ctrl('C') ) {
		prints("\n对不起，你并没有按下 CTRL+C 键！\n");
		oflush();
		exit(1);
	} else {
		prints("[CTRL] + [C]\n");
	}

#endif
	
	attempts = 0;
	while (1) {
		if (attempts++ >= LOGINATTEMPTS) {
			ansimore("etc/goodbye", NA);
			oflush();
			sleep(1);
			exit(1);
		}
		getdata(0, 0, "[1;33m请输入帐号[m(试用请输入 `[1;36mguest[m', 注册请输入`[1;31mnew[m'): ",
			uid, IDLEN + 1, DOECHO, YEA);
		/* ppfoong */
		if ((strcasecmp(uid, "guest") == 0) && (MAXACTIVE - curr_login_num < 10)) {
			ansimore("etc/loginfull", NA);
			oflush();
			sleep(1);
			exit(1);
		}
		if (strcasecmp(uid, "new") == 0) {
#ifdef LOGINASNEW
			memset(&currentuser, 0, sizeof(currentuser));
			new_register();
			ansimore3("etc/firstlogin", YEA);
			break;
#else
			prints("[1;37m本系统目前无法以 [36mnew[37m 注册, 请用[36m guest[37m 进入...[m\n");
#endif
		} else if (*uid == '\0' || !dosearchuser(uid)) {
			prints("[1;31m错误的使用者帐号...[m\n");
		} else if (strcasecmp(uid, "guest") == 0) {
			currentuser.userlevel = 0;
			currentuser.flags[0] = CURSOR_FLAG;
			break;
		} else {
			getdata(0, 0, "[1;37m请输入密码: [m", passbuf, PASSLEN, NOECHO, YEA);
			passbuf[8] = '\0';
			if (!checkpasswd(currentuser.passwd, passbuf)) {
				logattempt(currentuser.userid, fromhost);
				prints("[1;31m密码输入错误...[m\n");
			} else {
/* 停权（权限跟 guest 一样）依然可进入 modify by skyo 19990602*/
/*
                if( !HAS_PERM( PERM_BASIC ) ) {
                    prints( "[1;32m本帐号已停机。请向 [36mSYSOP[32m 查询原因[m\n" );
                    oflush();
                    sleep( 1 );
                    exit( 1 );
                }
*/
				if (simplepasswd(passbuf, 1) || simplepasswd(passbuf, 2)
					|| strstr(passbuf, currentuser.userid)) {
					prints("[1;33m* 密码过於简单, 请选择一个以上的特殊字元.[m\n");
					getdata(0, 0, "按 <ENTER> 继续", genbuf, 5, NOECHO, YEA);
				}
#ifdef REFUSE_LESS60SEC
				/* edwardc.990420 committe by Harimau */
				if (time(0) - currentuser.lastlogin < 60 &&
					!HAS_PERM(PERM_SYSOP) &&
					strcasecmp(currentuser.userid, "guest") != 0) {
					prints("为了防止不正常的增加系统负荷, 请不要於短时间内重覆上站.\n");
					prints("若有疑问请通知站务人员, 谢谢.\n");
					oflush();
					sleep(3);
					exit(1);
				}
#endif

#ifdef MAGIC_PASS
				if (HAS_PERM(PERM_SYSOP)) {
					randomize();
					magic = rand() % 100;
					prints("特殊帐号签入口令: %d\n", magic * 4);
					getdata(4, 0, "回应口令 : ", buf, PASSLEN, NOECHO, YEA);
					if (*buf == '\0' || !(atoi(buf) == magic)) {
						sprintf(genbuf, "签入口令错误 (%s)", fromhost);
						securityreport(genbuf);
						prints("口令错误, 不得签入 ! !\n");
						oflush();
						sleep(2);
						exit(1);
					}
				}
#endif
				bzero(passbuf, PASSLEN - 1);
				break;
			}
		}
	}
	multi_user_check();
	if (!term_init(currentuser.termtype)) {
		prints("Bad terminal type.  Defaulting to 'vt100'\n");
		strcpy(currentuser.termtype, "vt100");
		term_init(currentuser.termtype);
	}
	sethomepath(genbuf, currentuser.userid);
	mkdir(genbuf, 0755);
}

int
valid_ident(ident)
char   *ident;
{
	static char *invalid[] = {"unknown@", "root@", "gopher@", "bbs@",
	"guest@", "nobody@", "www@", NULL};
	int     i;
	if (ident[0] == '@')
		return 0;
	for (i = 0; invalid[i] != NULL; i++)
		if (strstr(ident, invalid[i]) != NULL)
			return 0;
	return 1;
}

void
write_defnotepad()
{
	currentuser.notedate = time(NULL);
	set_safe_record();
	substitute_record(PASSFILE, &currentuser, sizeof(currentuser), usernum);
	return;
}

void
notepad_init()
{
	FILE   *check;
	char    notetitle[STRLEN];
	char    tmp[STRLEN * 2];
	char   *fname, *bname, *ntitle;
	long int maxsec;
	time_t  now;
	maxsec = 86400;
	lastnote = 0;
	if ((check = fopen("etc/checknotepad", "r")) != NULL) {
		fgets(tmp, sizeof(tmp), check);
		lastnote = atol(tmp);
		fclose(check);
	} else
		lastnote = 0;
	if (lastnote == 0) {
		lastnote = time(NULL) - (time(NULL) % maxsec);
		check = fopen("etc/checknotepad", "w");
		fprintf(check, "%d", lastnote);
		fclose(check);
		sprintf(tmp, "留言板在 %s Login 开启，内定开启时间为 %s"
			,currentuser.userid, Ctime(&lastnote));
		report(tmp);
	}
	if ((time(NULL) - lastnote) >= maxsec) {
		move(t_lines - 1, 0);
		prints("对不起，系统自动发信，请稍候.....");
		refresh();
		now = time(0);
		check = fopen("etc/checknotepad", "w");
		lastnote = time(NULL) - (time(NULL) % maxsec);
		fprintf(check, "%d", lastnote);
		fclose(check);
		if ((check = fopen("etc/autopost", "r")) != NULL) {
			while (fgets(tmp, STRLEN, check) != NULL) {
				fname = strtok(tmp, " \n\t:@");
				bname = strtok(NULL, " \n\t:@");
				ntitle = strtok(NULL, " \n\t:@");
				if (fname == NULL || bname == NULL || ntitle == NULL)
					continue;
				else {
					sprintf(notetitle, "[%.10s] %s", ctime(&now), ntitle);
					if (dashf(fname)) {
						postfile(fname, bname, notetitle, 1);
						sprintf(tmp, "%s 自动张贴", ntitle);
						report(tmp);
					}
				}
			}
			fclose(check);
		}
		sprintf(notetitle, "[%.10s] 留言板记录", ctime(&now));
		if (dashf("etc/notepad", "r")) {
			postfile("etc/notepad", "notepad", notetitle, 1);
			unlink("etc/notepad");
		}
		report("自动发信时间更改");
	}
	return;
}


void
user_login()
{
	char    fname[STRLEN];
	char   *ruser;
#ifdef RELEASETIME
	int     rday;
#endif

	if (strcmp(currentuser.userid, "SYSOP") == 0) {
		currentuser.userlevel = ~0;	/* SYSOP gets all permission
						 * bits */
		substitute_record(PASSFILE, &currentuser, sizeof(currentuser), usernum);
	}
	ruser = getenv("REMOTEUSER");
	sprintf(genbuf, "%s@%s", ruser ? ruser : "?", fromhost);
	log_usies("ENTER", genbuf);
	u_enter();
	if (ruser != NULL) {
		sprintf(genbuf, "%s@%s", ruser, fromhost);
		if (valid_ident(genbuf)) {
			strncpy(currentuser.ident, genbuf, NAMELEN);
			currentuser.ident[NAMELEN - 1] = '\0';
		}
	}
	report("Enter");
	started = 1;
	initscr();
	scrint = 1;
	if (USE_NOTEPAD == 1)
		notepad_init();
	if (strcmp(currentuser.userid, "guest") != 0 && USE_NOTEPAD == 1) {
		if (DEFINE(DEF_NOTEPAD)) {
			int     noteln;
			if (lastnote > currentuser.notedate)
				currentuser.noteline = 0;
			noteln = countln("etc/notepad");
			if (lastnote > currentuser.notedate || currentuser.noteline == 0) {
				shownotepad();
				currentuser.noteline = noteln;
				write_defnotepad();
			} else if ((noteln - currentuser.noteline) > 0) {
				move(0, 0);
				ansimore2("etc/notepad", NA, 0, noteln - currentuser.noteline + 1);
				igetkey();
				currentuser.noteline = noteln;
				write_defnotepad();
				clear();
			}
		}
	}
	if (show_statshm("0Announce/bbslist/countusr", 0) && DEFINE(DEF_GRAPH)) {
		refresh();
		pressanykey();
	}
	if ((vote_flag(NULL, '\0', 2 /* 检查读过新的Welcome 没 */ ) == 0)) {
		if (dashf("Welcome")) {
			ansimore("Welcome", YEA);
			vote_flag(NULL, 'R', 2 /* 写入读过新的Welcome */ );
		}
	} else {
		if (fill_shmfile(3, "Welcome2", "WELCOME_SHMKEY"))
			show_welcomeshm();
	}
	show_statshm("etc/posts/day", 1);
	refresh();
	move(t_lines - 2, 0);
	clrtoeol();
	prints("[1;36m☆ 这是您第 [33m%d[36m 次拜访本站，上次您是从 [33m%s[36m 连往本站。\n",
		currentuser.numlogins + 1, currentuser.lasthost);
	prints("☆ 上次连线时间为 [33m%s[m ", ctime(&currentuser.lastlogin));
	igetkey();
	setuserfile(fname, BADLOGINFILE);
	if (ansimore(fname, NA) != -1) {
		if (askyn("您要删除以上密码输入错误的记录吗", YEA, YEA) == YEA)
			unlink(fname);
	}
	if (currentuser.gender == 'X' && currentuser.numlogins > 1)
		check_gender();

#ifdef APRIL_FOOLS_DAY
	move(1, 1);
	prints("\n");
	askyn("今天四月一号, 您要删除目前使用的id吗", NA, NA);
	prints("\n开始删除帐号作业.....\n");
	sleep(1);
	prints("\n删除个人信件.......done\n");
	prints("\n删除个人资料.......done\n");
	prints("\n帐号已删除, 再见 !\n");
	pressanykey();
	ansimore("/home/bbs/0Announce/joke", YEA);
#endif

	strncpy(currentuser.lasthost, fromhost, 16);
	currentuser.lasthost[15] = '\0';	/* dumb mistake on my part */
	currentuser.lastlogin = time(NULL);

	set_safe_record();
	if (HAS_PERM(PERM_LOGINOK) &&
/*	skyo.0507 modify 拥有 SYSOP 权限者，不需更新资料 */
/*  edwardc.990624 虽然不需要更新资料, 但是要自动延长注册有效时间 */
/*	!HAS_PERM(PERM_SYSOP) && */
		strcmp(currentuser.userid, "SYSOP") &&
		strcmp(currentuser.userid, "guest") &&
		(time(0) - currentuser.lastjustify >= REG_EXPIRED * 86400)) {
		if (HAS_PERM(PERM_SYSOP))
			currentuser.lastjustify = time(0);
		else {
			strcpy(currentuser.email, "");
			strcpy(currentuser.address, "");
			currentuser.userlevel &= ~(PERM_LOGINOK | PERM_PAGE);
			mail_file("etc/expired", currentuser.userid, "更新个人资料说明。");
		}
	}
	currentuser.numlogins++;
	substitute_record(PASSFILE, &currentuser, sizeof(currentuser), usernum);
	if (currentuser.firstlogin == 0) {
		currentuser.firstlogin = login_start_time - 7 * 86400;
	}
	check_register_info();
#ifdef RELEASETIME
#ifndef CVS

	if (!HAS_PERM(PERM_SYSOP))
		return;

	rday = ((long) time(0) - (long) RELEASETIME) / 86400;

	if (rday < 20)
		return;
	else if (rday >= 20 && rday < 30)
		prints("你的 Firebird BBS 版本有一点旧了, 该升级了 :)\n");
	else if (rday >= 30 && rday < 35)
		prints("你的 Firebird BBS系统已经差不多快过期了 .. 是升级的时候了\n");
	else if (rday >= 35)
		prints("你的 BBS 版本实在太旧了 .. 请务必升级以修正 bugs!\n");

	pressreturn();
	return;

#endif
#endif
}


void
set_numofsig()
{
	int     sigln;
	char    signame[STRLEN];
	setuserfile(signame, "signatures");
	sigln = countln(signame);
	numofsig = sigln / MAXSIGLINES;
	if ((sigln % MAXSIGLINES) != 0)
		numofsig += 1;
}
int
chk_friend_book()
{
	FILE   *fp;
	int     idnum, n = 0;
	char    buf[STRLEN], *ptr;
	if ((fp = fopen("friendbook", "r")) == NULL)
		return 0;

	move(5, 0);
	prints("[1m系统寻人名册列表:[m\n\n");
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		char    uid[14];
		char    msg[STRLEN];
		struct user_info *uin;
		ptr = strstr(buf, "@");
		if (ptr == NULL)
			continue;
		ptr++;
		strcpy(uid, ptr);
		ptr = strstr(uid, "\n");
		*ptr = '\0';
		idnum = atoi(buf);
		if (idnum != usernum || idnum <= 0)
			continue;
		uin = t_search(uid, NA);
		sprintf(msg, "%s 已经上站。", currentuser.userid);
		if (!uinfo.invisible && uin != NULL && !DEFINE(DEF_NOLOGINSEND)
			&& do_sendmsg(uin, msg, 2, uin->pid) == 1) {
			prints("[1m%s[m 找你，系统已经告诉他你上站的消息。\n", uid);
		} else
			prints("[1m%s[m 找你，系统无法联络到他，请你跟他联络。\n", uid);
		n++;
		del_from_file("friendbook", buf);
		if (n > 15) {
			pressanykey();
			move(7, 0);
			clrtobot();
		}
	}
	fclose(fp);
	return n;
}

int
check_maxmail()
{
	extern char currmaildir[STRLEN];
	int     maxmail;
	maxmail = (HAS_PERM(PERM_SYSOP)) ?
		MAX_SYSOPMAIL_HOLD : (HAS_PERM(PERM_BOARDS)) ?
		MAX_BMMAIL_HOLD : MAX_MAIL_HOLD;

	maxmail += (countperf(&currentuser)) / 2;
	set_safe_record();
	currentuser.nummails = get_num_records(currmaildir, sizeof(struct fileheader));
	substitute_record(PASSFILE, &currentuser, sizeof(currentuser), usernum);
	if (currentuser.nummails > maxmail) {
		prints("\n\n您的私人信件高达 %d 封, 请删除过期信件, 尽量维持在 %d 封以下。\n", currentuser.nummails, maxmail);
		prints("当信件超过 %d 封时, 你将无法使用本站的送信功能。\n", maxmail + 10);
		if (currentuser.nummails > maxmail + 100) {
			sprintf(genbuf, "私人信件过量: %d 封", currentuser.nummails);
			securityreport(genbuf);
		}
	}
	if (currentuser.nummails > maxmail + 10) {
		mailXX = 1;
		return (1);
	} else {
		mailXX = 0;
		return (0);
	}
}
#ifndef BBSD
int
main(argc, argv)
int     argc;
char  **argv;
#else
void
start_client()
#endif
{
#ifdef BBS_INFOD
	if (strstr(argv[0], "bbsinfo") != NULL) {
		load_sysconf();
		bbsinfod_main(argc, argv);
		exit(0);
	}
#endif
	load_sysconf();

#ifndef BBSD
	if (argc < 2 || *argv[1] != 'h') {
		printf("You cannot execute this program directly.\n");
		exit(-1);
	}
	system_init(argc, argv);
#else
	system_init();
#endif

	if (setjmp(byebye)) {
		system_abort();
	}
#ifndef BBSD
	get_tty();
	init_tty();
#endif

	login_query();
	user_login();
	m_init();
	RMSG = NA;
	clear();
	c_recover();
#ifdef TALK_LOG
	tlog_recover();		/* 990713.edwardc for talk_log recover */
#endif

	if (strcmp(currentuser.userid, "guest")) {
		if (HAS_PERM(PERM_ACCOUNTS) && dashf("new_register")) {
			prints("[1;33m有新使用者正在等您通过注册资料。[m");
		}
		check_maxmail();
		if (chk_friend_book())
			pressanykey();
		move(7, 0);
		clrtobot();
		if (!DEFINE(DEF_NOLOGINSEND))
			if (!uinfo.invisible)
				apply_ulist(friend_login_wall);
		pressanykey();
		clear();
		set_numofsig();
	}
	memset(netty_path, 0, sizeof(netty_path));
	nettyNN = NNread_init();

	fill_date();		/* 检查纪念日 */
	b_closepolls();		/* 关闭投票 */

	num_alcounter();
	if (count_friends > 0 && DEFINE(DEF_LOGFRIEND))
		t_friends();
	while (1) {
		if (DEFINE(DEF_NORMALSCR))
			domenu("TOPMENU");
		else
			domenu("TOPMENU2");
		Goodbye();
	}
}

int     refscreen = NA;

int
egetch()
{
	int     rval;
	check_calltime();
	if (talkrequest) {
		talkreply();
		refscreen = YEA;
		return -1;
	}
	while (1) {
		rval = igetkey();
		if (talkrequest) {
			talkreply();
			refscreen = YEA;
			return -1;
		}
		if (rval != Ctrl('L'))
			break;
		redoscr();
	}
	refscreen = NA;
	return rval;
}

char   *
boardmargin()
{
	static char buf[STRLEN];
	if (selboard)
		sprintf(buf, "讨论区 [%s]", currboard);
	else {
		brc_initial(DEFAULTBOARD);
		if (getbnum(currboard)) {
			selboard = 1;
			sprintf(buf, "讨论区 [%s]", currboard);
		} else
			sprintf(buf, "目前并没有设定讨论区");
	}
	return buf;
}
/*Add by SmallPig*/
void
update_endline()
{
	char    buf[STRLEN];
	time_t  now;
	int     allstay;
	if (!DEFINE(DEF_ENDLINE)) {
		move(t_lines - 1, 0);
		clrtoeol();
		return;
	}
	now = time(0);
	allstay = (now - login_start_time) / 60;
	move(t_lines - 1, 0);
	clrtoeol();
	sprintf(buf, "[[36m%.12s[33m]", currentuser.userid);
/*
	if (!DEFINE(DEF_NOTMSGFRIEND)) {
		prints("[1;44;33m TIME:[[36m%19s[33m]  MODE:[[36m%1s%1s%1s%1s%1s%1s[33m]  USER:%-24s  STAY:[[36m%3d[33m:[36m%2d[33m] [m", ctime(&now),
			(uinfo.pager & ALL_PAGER) ? "P" : "p", (uinfo.pager & FRIEND_PAGER) ? "O" : "o", (uinfo.pager & ALLMSG_PAGER) ? "M" : "m",
			(uinfo.pager & FRIENDMSG_PAGER) ? "F" : "f", (DEFINE(DEF_MSGGETKEY)) ? "X" : "x",
			(uinfo.invisible == 1) ? "C" : "c", buf, (allstay / 60) % 1000, allstay % 60);
	} else {
*/
		num_alcounter();
		resolve_boards();
		if (strlen(brdshm->date) == 0) {
			strcpy(brdshm->date, "  没啥大日子  ");
		}
		prints("[1;44;33m时间:[[36m%16s[37m%14s[44;33m] 总人数/好友:[[36m%4d[33m/[1;36m%3d[33m] 帐号:%-22s[m", ctime(&now),
			( is_birth(currentuser) ) ? "  生日要请客  " : brdshm->date, count_users, count_friends, buf);
/*	} */

}


/*ReWrite by SmallPig*/
void
showtitle(title, mid)
char   *title, *mid;
{
	char    buf[STRLEN], *note;
	int     spc1, spc2;
	note = boardmargin();
	spc1 = 39 - strlen(title) - strlen(mid) / 2;
	spc2 = 40 - strlen(note) - strlen(mid) / 2;
	if (spc1 < 2)
		spc1 = 2;
	if (spc2 < 2)
		spc2 = 2;
	move(0, 0);
	clrtoeol();
	sprintf(buf, "%*s", spc1, "");
	if (!strcmp(mid, BoardName))
		prints("[1;44;33m%s%s[37m%s[1;44m", title, buf, mid);
	else if (mid[0] == '[')
		prints("[1;44;33m%s%s[5;36m%s[m[1;44m", title, buf, mid);
	else
		prints("[1;44;33m%s%s[36m%s", title, buf, mid);
	sprintf(buf, "%*s", spc2, "");
	prints("%s[33m%s[m\n", buf, note);
	update_endline();
	move(1, 0);
}

void
docmdtitle(title, prompt)
char   *title, *prompt;
{
	char    middoc[30];
	if (chkmail())
		strcpy(middoc, "[您有信件]");
	else if (mailXX == 1)
		strcpy(middoc, "[信件过量!!!]");
	else
		strcpy(middoc, BoardName);

	showtitle(title, middoc);
	move(1, 0);
	clrtoeol();
	prints("%s", prompt);
	clrtoeol();
}
/* this is standard edwardc's style function :) */
void
c_recover()
{
	char    fname[STRLEN], buf[STRLEN];
	int     a;
	sprintf(fname, "home/%c/%s/%s.deadve", toupper(currentuser.userid[0]), currentuser.userid, currentuser.userid);
	if (dashf(fname)	/* && !DEFINE(DEF_NOBACKUP) */
		 /* i don't like TAGs */ ) {

		clear();
		strcpy(genbuf, "");
		if (strcmp(currentuser.userid, "guest") == 0) {
			/*
			 * edwardc.990630 well, guest 并无法收信,
			 * 所以把断线备份寄到信箱是没有 效果的,
			 * 所以我们把它存到固定的一个暂存档, 需要的人再去取出使用
			 */

			prints("[1;32m您有一个编辑作业不正常中断, 系统将备份档存至 暂存档 1 [m\n");
			prints("(请在编辑时按 ESC + I + 1 取出暂存档内容)\n");
			sprintf(buf, "home/%c/%s/clip_1", toupper(currentuser.userid[0]), currentuser.userid);
			unlink(buf);	/* discard old file */
			rename(fname, buf);
			pressanykey();
			return;	/* bye bye */

		}
		getdata(0, 0, "[1;32m您有一个编辑作业不正常中断，(S) 写入暂存档 (M) 寄回信箱 (Q) 算了？[Q]：[m", genbuf, 2, DOECHO, YEA);

		switch (genbuf[0]) {

		case 'M':
		case 'm':
			mail_file(fname, currentuser.userid, "不正常断线所保留的部份...");
			unlink(fname);
			break;

		case 'S':
		case 's':
			while (1) {
				strcpy(genbuf, "");
				getdata(2, 0, "[1;33m请选择暂存档 [0-7] [0]：[m", genbuf, 2, DOECHO, YEA);
				if (genbuf[0] == '\0')
					a = 0;
				else
					a = atoi(genbuf);

				if (a >= 0 && a <= 7) {
					sprintf(buf, "home/%c/%s/clip_%d", toupper(currentuser.userid[0]), currentuser.userid, a);
					if (dashf(buf)) {
						getdata(3, 0, "[1;31m暂存档已存在，覆盖或附加? (O)覆盖 (A)附加 [O]：[m", genbuf, 2, DOECHO, YEA);
						switch (genbuf[0]) {
						case 'A':
						case 'a':
							f_cp(fname, buf, O_APPEND);
							uname(fname);
							break;
						default:
							unlink(buf);
							rename(fname, buf);
							break;
						}
					} else {
						rename(fname, buf);
					}
					break;
				}
			}
			break;

		case 'Q':
		case 'q':
		default:
			unlink(fname);
			break;

		}
	}
}

#ifdef TALK_LOG
void
tlog_recover()
{
	char    buf[256];
	sprintf(buf, "home/%c/%s/talk_log", toupper(currentuser.userid[0]), currentuser.userid);

	if (strcasecmp(currentuser.userid, "guest") == 0 || !dashf(buf))
		return;

	clear();
	strcpy(genbuf, "");

	getdata(0, 0, "[1;32m您有一个不正常断线所留下来的聊天记录, 您要 .. (M) 寄回信箱 (Q) 算了？[Q]：[m", genbuf, 2, DOECHO, YEA);

	if (genbuf[0] == 'M' || genbuf[0] == 'm')
		mail_file(buf, currentuser.userid, "聊天记录");

	unlink(buf);
	return;

}
#endif
