/******************************************************************************

     Martian's GDBM Extension Pack

  Copyright 1998, 2000 Midgard Systems and Adam "Martian" Smyth

  Permission is granted to freely distribute and use this file for any
  non-commercial application. Commercial and Government users may also use
  it freely, but must inform the author prior to use.
  Permission is granted to change this file to suit a particular application,
  so long as a copy of the changes is provided to the original author.

  If you use this, lemme know. If you think it sucks, lemme know.
  If you want something changed, lemme know. If something doesn't
  work, lemme know.

  The latest version of this file is available at
    http://www.martian.cx/Code/MOO/extensions/ext-gdbm.c

  If you use these extensions, you may want to use the $gdbm_utils package.
  It is available at
    http://www.martian.cx/Code/MOO/gdbm_utils.moo

  Martian
  martian@midgard.org
 *****************************************************************************/


#include "bf_register.h"
#include "functions.h"

#include "structures.h"
#include "log.h"
#include "functions.h"
#include "streams.h"
#include "exceptions.h"
#include "utils.h"
#include "list.h"

#include "gdbm/gdbm.h"

const char *gdbm_ext_version = "1.1b";

/* Definitions. Change these to suit your system and tastes. */
#define FILEDIR "files/"    /* Subdirectory all files are put under */
#define TABLESTART 10       /* Initial size of gdbm_file_table */
/* End Definitions */

/* Global variables */
extern gdbm_error gdbm_errno;
extern char *gdbm_version;
static GDBM_FILE *gdbm_file_table;
static int gdbm_table_len;
/* End Global Variables */

/* Utility functions */
void gdbm_fatal_panic(char *msg)
{
  errlog("Fatal GDBM Error: %s. Panicing.\n", msg);
  panic("Fatal GDBM Error.");
}

int add_dbf_to_table(GDBM_FILE dbf)
{
  int i;

  if(!gdbm_file_table) {
    gdbm_table_len = TABLESTART;
    gdbm_file_table = (GDBM_FILE*)calloc(gdbm_table_len, sizeof(GDBM_FILE));
  }
  /*
     Could add code here to dynamically increase the size of the array
     as needed to allow more than 10 open files at once.
  */

  for(i=0;i<gdbm_table_len;i++)
    if(gdbm_file_table[i] == NULL) {
      gdbm_file_table[i] = dbf;
      return i;
    }

  return -1;
}

int bad_filename(const char *filename) {
  if((filename[0] == '\0') || (strindex(filename, "/.", 0)) || (strindex(filename, "..", 0) == 1))
    return 1;

  return 0;
}

char *expand_filename(const char *filename) {
  static Stream *s = 0;

  if(!s)
    s = new_stream(150);

  if(bad_filename(filename))
    return NULL;

  stream_add_string(s, FILEDIR);
  stream_add_string(s, ((filename[0]=='/') ? (filename+1) : filename));

  return reset_stream(s);
}

/* Builtin functions */

/* gdbm_version - Returns the version strings for this extension and the gdbm package */
/* gdbm_verison(ANY internal) => STR <version> */
static package
bf_gdbm_version(Var arglist, Byte next, void *vdata, Objid progr)
{
  Var r;
  Stream *s;

  r.type=TYPE_STR;
  s=new_stream(100);
  if(arglist.v.list[0].v.num == 1 && is_true(arglist.v.list[1])) {
    stream_printf(s, "%s", gdbm_version);
  } else {
    stream_printf(s, "Martian's GDBM Extention Pack v%s", gdbm_ext_version);
  }
  r.v.str = str_dup(reset_stream(s));
  free_stream(s);
  return make_var_pack(r);
}
/* End gdbm_version */

/* gdbm_open - Open a database file. Return a handle for the other functions to use */
/* gdbm_open(STR Name, STR Mode, [ANY Slow]) => INT Handle */
static package
bf_gdbm_open(Var arglist, Byte next, void *vdata, Objid progr)
{
  Var r;
  GDBM_FILE dbf;
  int read_write;
  char *filename;

  if(!is_wizard(progr)) {
    free_var(arglist);
    return make_error_pack(E_PERM);
  }

  if(!mystrcasecmp(arglist.v.list[2].v.str, "write")) {
    if(arglist.v.list[0].v.num == 3 && is_true(arglist.v.list[3]))
      read_write = GDBM_WRCREAT;
    else
      read_write = GDBM_WRCREAT | GDBM_FAST;
  } else
    read_write = GDBM_READER;

  filename = expand_filename(arglist.v.list[1].v.str);
  free_var(arglist);
  if(filename == NULL)
    return make_raise_pack(E_INVARG, "Invalid filename", zero);

  dbf = gdbm_open(filename, 512, read_write, 0664, gdbm_fatal_panic);
  if(dbf == NULL)
    return make_raise_pack(E_INVARG, gdbm_strerror(gdbm_errno), zero);

  r.type=TYPE_INT;
  if((r.v.num = add_dbf_to_table(dbf))<0) {
    gdbm_close(dbf);
    return make_raise_pack(E_QUOTA, "Can't allocate storage.", zero);
  }

  return make_var_pack(r);
}
/* End gdbm_open */

/* gdbm_close - Close a database file. */
/* gdbm_close(INT Handle, [ANY Compact]) => 0 [success] or E_INVARG [failure] */
static package
bf_gdbm_close(Var arglist, Byte next, void *vdata, Objid progr)
{
  int which = arglist.v.list[1].v.num;

  free_var(arglist);

  if(!is_wizard(progr))
    return make_error_pack(E_PERM);

  if(which >= gdbm_table_len || gdbm_file_table[which]==NULL)
    return make_raise_pack(E_INVARG, "Invalid database handle", zero);

  if(arglist.v.list[0].v.num == 2 && is_true(arglist.v.list[2]))
    gdbm_reorganize(gdbm_file_table[which]);

  gdbm_close(gdbm_file_table[which]);
  gdbm_file_table[which] = NULL;

  return no_var_pack();
}
/* End gbdm_close */

/* gdbm_store - Store a string in the database. */
/* gdbm_store(INT Handle, STR key, STR data, [ANY NoReplace]) => 0 [success] or E_INVARG [failure] */
static package
bf_gdbm_store(Var arglist, Byte next, void *vdata, Objid progr)
{
  int which = arglist.v.list[1].v.num, flag=GDBM_REPLACE, ret;
  datum key, data;

  if(!is_wizard(progr)) {
    free_var(arglist);
    return make_error_pack(E_PERM);
  }

  if(which >= gdbm_table_len || gdbm_file_table[which]==NULL) {
    free_var(arglist);
    return make_raise_pack(E_INVARG, "Invalid database handle", zero);
  }

  if(arglist.v.list[0].v.num == 4 && is_true(arglist.v.list[4]))
    flag = GDBM_INSERT;

  key.dptr = (char*)arglist.v.list[2].v.str;
  key.dsize = strlen(key.dptr)+1;

  data.dptr = (char*)arglist.v.list[3].v.str;
  data.dsize = strlen(data.dptr)+1;

  ret = gdbm_store(gdbm_file_table[which], key, data, flag);

  free_var(arglist);

  if(ret < 0)
    return make_raise_pack(E_INVARG, "Wrong file mode", zero);
  else if(ret > 0)
    return make_raise_pack(E_INVARG, "Duplicate key", zero);
  else
    return no_var_pack();
}
/* End gbdm_store */

/* gdbm_fetch - Fetch the string associated with a given key. */
/* gdbm_fetch(INT Handle, STR key) => STR data [success] or E_INVARG [failure] */
static package
bf_gdbm_fetch(Var arglist, Byte next, void *vdata, Objid progr)
{
  int which = arglist.v.list[1].v.num;
  datum key, data;
  Var r;

  if(!is_wizard(progr)) {
    free_var(arglist);
    return make_error_pack(E_PERM);
  }

  if(which >= gdbm_table_len || gdbm_file_table[which]==NULL) {
    free_var(arglist);
    return make_raise_pack(E_INVARG, "Invalid database handle", zero);
  }

  key.dptr = (char*)arglist.v.list[2].v.str;
  key.dsize = strlen(key.dptr)+1;

  data = gdbm_fetch(gdbm_file_table[which], key);

  free_var(arglist);

  if(data.dptr == NULL)
    return make_raise_pack(E_INVARG, "Key not found", zero);
  else {
    r.type = TYPE_STR;
    r.v.str = str_dup(data.dptr);
    free(data.dptr);
    return make_var_pack(r);
  }
}
/* End gbdm_fetch */

/* gdbm_delete - Delete the given key (and it's data) from the database. */
/* gdbm_delete(INT Handle, STR key) => 0 [success] or E_INVARG [failure] */
static package
bf_gdbm_delete(Var arglist, Byte next, void *vdata, Objid progr)
{
  int which = arglist.v.list[1].v.num, ret;
  datum key;

  if(!is_wizard(progr)) {
    free_var(arglist);
    return make_error_pack(E_PERM);
  }

  if(which >= gdbm_table_len || gdbm_file_table[which]==NULL) {
    free_var(arglist);
    return make_raise_pack(E_INVARG, "Invalid database handle", zero);
  }

  key.dptr = (char*)arglist.v.list[2].v.str;
  key.dsize = strlen(key.dptr)+1;

  ret = gdbm_delete(gdbm_file_table[which], key);

  free_var(arglist);

  if(ret < 0)
    return make_raise_pack(E_INVARG, "Key not found or Invalid file mode", zero);
  else
    return no_var_pack();
}
/* End gbdm_delete */

/* gdbm_exists - Checks if the given key exists in the database. */
/* gdbm_exists(INT Handle, STR key) => 1 [key found], 0 [key not found] or E_INVARG [error] */
static package
bf_gdbm_exists(Var arglist, Byte next, void *vdata, Objid progr)
{
  int which = arglist.v.list[1].v.num, ret;
  datum key;
  Var r;

  if(!is_wizard(progr)) {
    free_var(arglist);
    return make_error_pack(E_PERM);
  }

  if(which >= gdbm_table_len || gdbm_file_table[which]==NULL) {
    free_var(arglist);
    return make_raise_pack(E_INVARG, "Invalid database handle", zero);
  }

  key.dptr = (char*)arglist.v.list[2].v.str;
  key.dsize = strlen(key.dptr)+1;

  ret = gdbm_exists(gdbm_file_table[which], key);

  free_var(arglist);

  r.type = TYPE_INT;
  r.v.num = !!ret;
  return make_var_pack(r);
}
/* End gbdm_exists */

/* gdbm_sync - Writes the given database to disk. */
/* gdbm_sync(INT Handle) => 0 [success] or E_INVARG [failure] */
static package
bf_gdbm_sync(Var arglist, Byte next, void *vdata, Objid progr)
{
  int which = arglist.v.list[1].v.num;

  free_var(arglist);

  if(!is_wizard(progr))
    return make_error_pack(E_PERM);

  if(which >= gdbm_table_len || gdbm_file_table[which]==NULL)
    return make_raise_pack(E_INVARG, "Invalid database handle", zero);

  gdbm_sync(gdbm_file_table[which]);

  return no_var_pack();
}
/* End gbdm_sync */

/* gdbm_closeall - Close all open databases. */
/* gdbm_closeall() => 0 */
static package
bf_gdbm_closeall(Var arglist, Byte next, void *vdata, Objid progr)
{
  int i;

  free_var(arglist);

  if(!is_wizard(progr))
    return make_error_pack(E_PERM);

  for(i=0;i<gdbm_table_len;i++)
    if(gdbm_file_table[i] != NULL) {
      gdbm_close(gdbm_file_table[i]);
      gdbm_file_table[i] = NULL;
    }

  return no_var_pack();
}
/* End gbdm_closeall */

/* gdbm_find_keys - Returns a list of all the keys in the database which match the
                    prefix or all keys, if the prefix is blank or not given */
/* gdbm_find_keys(INT Handle, [STR prefix]) => LIST <keys> [success] or E_INVARG [failure] */
static package
bf_gdbm_find_keys(Var arglist, Byte next, void *vdata, Objid progr)
{
  int which = arglist.v.list[1].v.num, checks=0;
  Var s, k, keys;
  datum key, nextkey;

  if(!is_wizard(progr)) {
    free_var(arglist);
    return make_error_pack(E_PERM);
  }

  if(arglist.v.list[0].v.num == 2) {
    s = var_ref(arglist.v.list[2]);
    checks = strlen(s.v.str);
  }

  free_var(arglist);

  if(which >= gdbm_table_len || gdbm_file_table[which]==NULL)
    return make_raise_pack(E_INVARG, "Invalid database handle", zero);

  keys = new_list(0);

  key = gdbm_firstkey(gdbm_file_table[which]);
  while (key.dptr) {
    if(!checks || !mystrncasecmp(s.v.str, key.dptr, checks)) {
      k.type = TYPE_STR;
      k.v.str = str_dup(key.dptr);
      keys = listappend(keys, k);
    }
    nextkey = gdbm_nextkey(gdbm_file_table[which], key);
    free(key.dptr);
    key=nextkey;
  }

  return make_var_pack(keys);
}
/* End gbdm_find_keys */

/* gdbm_num_keys - Returns a the number of keys in the database */
/* gdbm_num_keys(INT Handle) => INT <count> [success] or E_INVARG [failure] */
static package
bf_gdbm_num_keys(Var arglist, Byte next, void *vdata, Objid progr)
{
  int which = arglist.v.list[1].v.num;
  Var keys;
  datum key, nextkey;

  if(!is_wizard(progr)) {
    free_var(arglist);
    return make_error_pack(E_PERM);
  }

  free_var(arglist);

  if(which >= gdbm_table_len || gdbm_file_table[which]==NULL)
    return make_raise_pack(E_INVARG, "Invalid database handle", zero);

  keys.type = TYPE_INT;
  keys.v.num = 0;

  key = gdbm_firstkey(gdbm_file_table[which]);
  while (key.dptr) {
    keys.v.num++;
    nextkey = gdbm_nextkey(gdbm_file_table[which], key);
    free(key.dptr);
    key=nextkey;
  }

  return make_var_pack(keys);
}
/* End gbdm_num_keys */

/* End Builtin functions */

void
register_gdbm()
{
  oklog("Installing Martian's GDBM Extention Pack v%s\n", gdbm_ext_version);
  register_function("gdbm_version", 0, 1, bf_gdbm_version, TYPE_ANY);
  register_function("gdbm_open", 2, 3, bf_gdbm_open, TYPE_STR, TYPE_STR, TYPE_ANY);
  register_function("gdbm_close", 1, 1, bf_gdbm_close, TYPE_INT);
  register_function("gdbm_store", 3, 4, bf_gdbm_store, TYPE_INT, TYPE_STR, TYPE_STR, TYPE_ANY);
  register_function("gdbm_fetch", 2, 2, bf_gdbm_fetch, TYPE_INT, TYPE_STR);
  register_function("gdbm_delete", 2, 2, bf_gdbm_delete, TYPE_INT, TYPE_STR);
  register_function("gdbm_exists", 2, 2, bf_gdbm_exists, TYPE_INT, TYPE_STR);
  register_function("gdbm_sync", 1, 1, bf_gdbm_sync, TYPE_INT);
  register_function("gdbm_closeall", 0, 0, bf_gdbm_closeall);
  register_function("gdbm_find_keys", 1, 2, bf_gdbm_find_keys, TYPE_INT, TYPE_STR);
  register_function("gdbm_num_keys", 1, 1, bf_gdbm_num_keys, TYPE_INT);

  gdbm_file_table = NULL;
  gdbm_table_len = 0;
}

/* Version 1.2 1998/04/30
 * Added gdbm_num_keys
 *
 * Version 1.1b 1998/04/30
 * First production version
 *
 * Version 1.0b 1998/04/30
 * First usable version
 *
 * Version 1.0a 1998/04/28
 * First working version
 *
 * Version 0.1a 1998/04/27
 * First attempt at implementation
 */
