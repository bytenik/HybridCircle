/******************************************************************************

     List Utils Extensions

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
#include "my-ctype.h"

#include "log.h"
#include "utils.h"

const char *lists_version = "1.5";

int find_insert(Var lst, Var key);
Var value_compare(Var a, Var b);
void makelowercase(char *string);

void InitListToZero(Var list)
{
  int i;
  Var z;

  z.type = TYPE_INT; z.v.num = 0;

  for(i=1; i <= list.v.list[0].v.num; i++)
    list.v.list[i]=z;
}

static package
bf_make(Var arglist, Byte next, void *vdata, Objid progr)
{
  Var ret, elt;
  int n=arglist.v.list[1].v.num, i;

  if(n < 0) {
    free_var(arglist);
    make_error_pack(E_INVARG);
  }

  ret = new_list(n);
  InitListToZero(ret);

  if(arglist.v.list[0].v.num == 2) {
    elt = var_dup(arglist.v.list[2]);
  } else {
    elt.type = TYPE_INT;
    elt.v.num = 0;
  }

  for(i = 1; i <= n; i++)
    ret.v.list[i] = var_dup(elt);

  free_var(elt);
  free_var(arglist);
  return make_var_pack(ret);
}

static package
bf_slice(Var arglist, Byte next, void *vdata, Objid progr)
{
  Var ret, list=arglist.v.list[1];
  int n=list.v.list[0].v.num, c, i;

  if(n < 0) {
    free_var(arglist);
    make_error_pack(E_INVARG);
  }

  ret = new_list(n);
  InitListToZero(ret);

  if(arglist.v.list[0].v.num == 2)
    c = arglist.v.list[2].v.num;
  else
    c = 1;

  for(i = 1; i <= n; i++)
    if( list.v.list[i].type != TYPE_LIST || list.v.list[i].v.list[0].v.num < c ) {
      free_var(ret);
      free_var(arglist);
      return make_error_pack(E_INVARG);
    } else {
      ret.v.list[i] = var_dup(list.v.list[i].v.list[c]);
    }

  free_var(arglist);
  return make_var_pack(ret);
}

/* Remove_Duplicates - from Access_Denied@LambdaMOO. */
static package
bf_remove_duplicates(Var arglist, Byte next, void *vdata, Objid progr)
{
	Var	r;
	int	i;

	r = new_list(0);
	for (i = 1; i <= arglist.v.list[1].v.list[0].v.num; i++)
		r = setadd(r, var_ref(arglist.v.list[1].v.list[i]));

	free_var(arglist);
	return make_var_pack(r);
}
/* End Remove_Duplicates */

Var
list_assoc(Var vtarget, Var vlist, int vindex)
{
  int i;

  for (i = 1; i <= vlist.v.list[0].v.num; i++) {
    if (vlist.v.list[i].type == TYPE_LIST &&
        vlist.v.list[i].v.list[0].v.num >= vindex &&
        equality(vlist.v.list[i].v.list[vindex], vtarget, 0)) {

      return var_dup(vlist.v.list[i]);
    }
  }
  return new_list(0);
}

int
list_iassoc(Var vtarget, Var vlist, int vindex)
{
  int i;

  for (i = 1; i <= vlist.v.list[0].v.num; i++) {
    if (vlist.v.list[i].type == TYPE_LIST &&
        vlist.v.list[i].v.list[0].v.num >= vindex &&
        equality(vlist.v.list[i].v.list[vindex], vtarget, 0)) {

      return i;
    }
  }
  return 0;
}


static package
bf_iassoc(Var arglist, Byte next, void *vdata, Objid progr)
{ /* (ANY, LIST[, INT]) */
  Var r;
  int index = 1;

  r.type = TYPE_INT;
  if (arglist.v.list[0].v.num == 3)
    index = arglist.v.list[3].v.num;

  if (index < 1) {
    free_var(arglist);
    return make_error_pack(E_RANGE);
  }

  r.v.num = list_iassoc(arglist.v.list[1], arglist.v.list[2], index);

  free_var(arglist);
  return make_var_pack(r);
} /* end bf_listiassoc() */

static package
bf_assoc(Var arglist, Byte next, void *vdata, Objid progr)
{ /* (ANY, LIST[, INT]) */
  Var r;
  int index = 1;

  if (arglist.v.list[0].v.num == 3)
    index = arglist.v.list[3].v.num;

  if (index < 1) {
    free_var(arglist);
    return make_error_pack(E_RANGE);
  }

  r = list_assoc(arglist.v.list[1], arglist.v.list[2], index);

  free_var(arglist);
  return make_var_pack(r);
} /* end bf_listassoc() */

static package
bf_sort(Var arglist, Byte next, void *vdata, Objid progr)
{
/* sort(list) => sorts and returns list.
   sort({1,3,2}) => {1,2,3} */

/* returns E_TYPE is list is not all the same type */

	Var	sorted = new_list(0), tmp;
	Var	e;
	int	i, l;

	e.type=TYPE_NONE;

	for(i = 1; i <= arglist.v.list[1].v.list[0].v.num; i++) {
		e = var_ref(arglist.v.list[1].v.list[i]);
		l = find_insert(sorted, e);
		if(l == -10) {
			free_var(arglist);
			free_var(sorted);
			free_var(e);
			return make_error_pack(E_TYPE);
		}
		tmp = listinsert(var_ref(sorted), var_ref(e), l);
		free_var(sorted);
		sorted = var_ref(tmp);
		free_var(tmp);
	}

	free_var(arglist);
	free_var(e);
	return make_var_pack(sorted);
}

int find_insert(Var lst, Var key) {
/* find_insert(sortedlist,key) => index of first element in sortedlist
   > key.  sortedlist is assumed to bem sorted in increasing order and
   the number returned is anywhere from 1 to length(sortedlist)+1,
   inclusive. */

/* returns -10 if an E_TYPE occurs */

	Var	compare;
	int	r = lst.v.list[0].v.num, l=1, i;

	while(r >= l) {
		compare = value_compare(var_ref(key), var_ref(lst.v.list[i = ((r + l) / 2)] ) );
		if(compare.type == TYPE_ERR) {
			free_var(compare);
			return -10;
		}
		if(compare.v.num < 0) {
			r = i - 1;
		} else {
			l = i + 1;
		}
	}
	return l;
}

Var value_compare(Var a, Var b) {
	char *sa=0, *sb=0;
	Var r;

	if(a.type != b.type) {
		r.type = TYPE_ERR;
		r.v.err = E_TYPE;
		return r;
	}

	switch(a.type) {
		case TYPE_STR:
			sa=str_dup(a.v.str);
			sb=str_dup(b.v.str);
			makelowercase(sa);
			makelowercase(sb);
			r.v.num = strcmp(sa, sb);
			free(sa);
			free(sb);
			r.type = TYPE_INT;
			break;
		case TYPE_INT:
		case TYPE_FLOAT:
			r = compare_numbers(a,b);
			break;
		case TYPE_OBJ:
		case TYPE_ERR:
			a.type = b.type = TYPE_INT;
			r = compare_numbers(a,b);
			break;
		default:
			r.v.err = E_TYPE;
			r.type = TYPE_ERR;
	}

	return r;
}

void makelowercase(char *string)
{
	int i=0;
	for(;string[i];i++)
		if(string[i]>64 && string[i]<91)
			string[i]=string[i]+32;
}

void
register_listutils()
{
    oklog("Installing List Utils Extensions v%s\n", lists_version);
    register_function("iassoc", 2, 3, bf_iassoc, TYPE_ANY, TYPE_LIST, TYPE_INT);
    register_function("assoc", 2, 3, bf_assoc, TYPE_ANY, TYPE_LIST, TYPE_INT);
    register_function("sort", 1, 1, bf_sort, TYPE_LIST);
    register_function("make", 1, 2, bf_make, TYPE_INT, TYPE_ANY);
    register_function("slice", 1, 2, bf_slice, TYPE_LIST, TYPE_INT);
    register_function("remove_duplicates", 1, 1, bf_remove_duplicates, TYPE_LIST);
}


/* Revision 1.5  1996/12/19
 *  added bf_list_make and bf_list_slice
 *  move bf_remove_duplicates in from ext-martian.c
 *
 * Revision 1.0b 1996/04/18
 *  Moved list_sort and its utils from ext-martian.c
 *  Changed bf names to include _'s.  listassoc => list_assoc  listiassoc => list_iassoc
 *   I just like it better aesthetically.
 *
 * Earlier Revisions
 *  Listassoc and listiassoc stolen from a 179 patchfile, made into 180 Extention file.
 */
