/******************************************************************************

     Martian's MOO Extension Pack

  No copyright. Copyrights are Capitalist things. Caplitalism is BAD.

  If you use this, lemme know. If you think it sucks, lemme know.
  If you want something changed, lemme know. If something doesn't
  work, lemme know.

  Martian
  martian@midgard.solgate.com
 *****************************************************************************/


#include "bf_register.h"
#include "functions.h"
#include "my-unistd.h"

#include "exceptions.h"
#include "log.h"
#include "net_multi.h"
#include "storage.h"
#include "tasks.h"
#include "utils.h"
#include "db.h"
#include "list.h"
#ifdef WIN32
#include <time.h>
#include "my-unistd.h"
#else
#include <sys/time.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <string.h>
#include "my-string.h"
#include "my-stdlib.h"
#include "db.h"
#include "db_private.h"
#include "numbers.h"
#include "random.h"
#include "structures.h"
#include "log.h"

const char *martian_version = "1.2";

/* Logical XOR - one and only one of args[1] and args[2] can be true. */
static package
bf_xor(Var arglist, Byte next, void *vdata, Objid progr)
{
    Var r;
    r.type = TYPE_INT;
    r.v.num = (is_true(arglist.v.list[1]) && (!is_true(arglist.v.list[2]))) || ((!is_true(arglist.v.list[1])) && is_true(arglist.v.list[2]));
    free_var(arglist);
    return make_var_pack(r);
}
/* End XOR */

/* us_time - return a float of time() with microseconds */
#ifndef WIN32
static package
bf_us_time(Var arglist, Byte next, void *vdata, Objid progr)
{
        Var r; 
        struct timeval *tv = (struct timeval *)malloc(sizeof(struct timeval));
        struct timezone *tz=0;
        
        gettimeofday(tv,tz);

	r=new_float( ( (double)tv->tv_sec) + ( (double)tv->tv_usec / 1000000 ) );

        free(tv);
        free_var(arglist);
        return make_var_pack(r);
}
#endif
/* End us_time */

/* Proc_Size - The current process size. Requires the /proc virtual file system */
static package
bf_proc_size(Var arglist, Byte next, void *vdata, Objid progr)
{
        Var r;
	char state;
        int pid, ppid, pgrp, session, tty, tpgid, utime, stime, cutime, cstime, counter, priority, starttime, signal, blocked, sigignore, sigcatch;
	unsigned int flags, minflt, cminflt, majflt, cmajflt, timeout, itrealvalue, vsize, rss, rlim, startcode, endcode, startstack, kstkesp, kstkeip, wchan;
	FILE *fp;
	char inputtext[240];

        r.type=TYPE_INT;

        if((fp=fopen("/proc/self/stat","r")) == NULL)
		return make_error_pack(E_NACC);

	fgets(inputtext,239,fp);
        sscanf(inputtext, "%d %*s %c %d %d %d %d %d %u %u %u %u %u %d %d %d %d %d %d %u %u %d %u %u %u %u %u %u %u %u %d %d %d %d %u",
	&pid, &state, &ppid, &pgrp, &session, &tty, &tpgid, &flags,
	&minflt, &cminflt, &majflt, &cmajflt, &utime, &stime, &cutime, 
	&cstime, &counter, &priority, &timeout, &itrealvalue, &starttime,
	&vsize, &rss, &rlim, &startcode, &endcode, &startstack, &kstkesp,
	&kstkeip, &signal, &blocked, &sigignore, &sigcatch, &wchan);

	fclose(fp);

        r.v.num=(int)vsize;
        free_var(arglist);
        return make_var_pack(r);
}
/* End Proc_Size */

/* nprogs - Number of programmed verbs. */
static package
bf_nprogs(Var arglist, Byte next, void *vdata, Objid progr)
{
	Objid	oid, max_oid=db_last_used_objid();
	int	nprogs=0;
	Verbdef	*v;
	Var	r;

	free_var(arglist);

	for(oid = 0; oid <= max_oid; oid++)
		if(valid(oid))
			for(v = dbpriv_find_object(oid)->verbdefs; v; v = v->next)
				if(v->program)
					nprogs++;

	r.type=TYPE_INT;
	r.v.num=nprogs;
        return make_var_pack(r);
}
/* End nprogs */


/* Random_Of - Random item of a list. */
static package
bf_random_of(Var arglist, Byte next, void *vdata, Objid progr)
{
	Var 	r;
	int	n;

	n = arglist.v.list[1].v.list[0].v.num;

	if(!n) {
		free_var(arglist);
		return make_error_pack(E_INVARG);
	}

	n = RANDOM() % n + 1;
	r = var_ref(arglist.v.list[1].v.list[n]);

	free_var(arglist);
        return make_var_pack(r);
}
/* End Random_Of */


/* Enlist - Make args[1] into a list, if it isn't. */
static package
bf_enlist(Var arglist, Byte next, void *vdata, Objid progr)
{
	Var r;

	if(arglist.v.list[1].type != TYPE_LIST) {
		r = new_list(1);
		r.v.list[1] = var_ref(arglist.v.list[1]);
	} else {
		r = var_ref(arglist.v.list[1]);
	}
        free_var(arglist);
	return make_var_pack(r);
}

/* Explode - Parse a string into sections broken by some character. */
/*/// START LUKE-JR RECYCLE BLOCK /////
              EVIL CODE
static package
bf_explode(Var arglist, Byte next, void *vdata, Objid progr)
{
	Var str, r, s;
	int slen=0, i=0, a=1;
	const char *exp;

	str = var_ref(arglist.v.list[1]);
	slen = strlen(str.v.str);

	if (arglist.v.list[0].v.num > 1) {
		s = substr(var_ref(arglist.v.list[2]),1,1);
		exp = s.v.str;
		free_var(s);

//		exp = (char *) substr(var_dup(arglist.v.list[2]),1,1).v.str;
	} else {
		exp = str_ref(str_dup(" "));
	}
	
	r = new_list(0);
	while(a <= slen) {
		if(!(i=strindex(substr(var_ref(str),a,slen).v.str, exp, 0))) {
			s = substr(var_ref(str), a, slen);
			r = listappend(r, s);
			a = slen;
			break;
		} else if(i==1) {
			a = a + 1;
		} else {
			s = substr(var_ref(str), a, i+a-2);
			r = listappend(r, s);
			a = a + i;
		}
	}
	free_var(arglist);
	free_var(str);
//	free_var(s);
	free_str(exp);
	return make_var_pack(r);
}
/////  END LUKE-JR RECYCLE BLOCK ///*/

/* Panic - panic the server with the given message. */
static package
bf_panic(Var arglist, Byte next, void *vdata, Objid progr)
{
  const char *msg;

  if(!is_wizard(progr))
    return make_error_pack(E_PERM);

  if(arglist.v.list[0].v.num) {
    msg=str_dup(arglist.v.list[1].v.str);
  } else {
    msg="Intentional Server Panic";
  }

  free_var(arglist);
  panic(msg);

  return make_error_pack(E_NONE);
}

static package
bf_isa(Var arglist, Byte next, void *vdata, Objid progr)  
{
  Objid what=arglist.v.list[1].v.obj, targ=arglist.v.list[2].v.obj;
  Var r;

  free_var(arglist);
  r.type = TYPE_INT;
  r.v.num = 0;
  
  while(valid(what)) {
    if(what == targ) {
      r.v.num = 1;
      return make_var_pack(r);
    }
    what = db_object_parent(what);
  }
  return make_var_pack(r);
}

void
register_martian()
{
    oklog("Installing Martian's MOO Extention Pack v%s\n", martian_version);
    register_function("xor", 2, 2, bf_xor, TYPE_ANY, TYPE_ANY);
#ifndef WIN32
    register_function("us_time", 0, 0, bf_us_time);
#endif
    register_function("proc_size", 0, 0, bf_proc_size);
    register_function("nprogs", 0, 0, bf_nprogs);
    register_function("random_of", 1, 1, bf_random_of, TYPE_LIST);
    register_function("enlist", 1, 1, bf_enlist, TYPE_ANY);
/*/// BEGIN LUKE-JR RECYCLE BLOCK /////
    register_function("explode", 1, 2, bf_explode, TYPE_STR, TYPE_STR);
/////  END LUKE-JR RECYCLE BLOCK  ///*/
    register_function("panic", 0, 1, bf_panic, TYPE_STR);
    register_function("isa", 2, 2, bf_isa, TYPE_OBJ, TYPE_OBJ);
}

/* Version 1.2 1996/12/18
 *  Added bf_explode.
 *  Moved bf_remove_duplicates to ext-lists.c
 * 
 * Revision 1.0b2 1996/04/18
 *  Changed xor from TYPE_INT to TYPE_ANY, using is_true
 *  Moved list_sort and it's utils to ext-lists.c
 * 
 * Revision 1.0b  1996/03/18
 *  Martian's MOO Extension Pack (MExt)
 *  First (supposedly) bug-free version. At least it doesn't leak memory
 *   like the Titanic. (Well, not anymore)
 *
 * Earlier Revisions  1995/12/?? - 1996/03/18
 *  Don't ask. Really. I mean it.
 */
