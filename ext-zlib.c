/******************************************************************************

     Martian's ZLib Interface Extension

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
    http://www.martian.cx/Code/MOO/extensions/ext-zlib.c

  Martian
  martian@midgard.org
 *****************************************************************************/

/* Compilation note: zconf.h may redefine 'Byte',
   previously defined in 'program.h'.
   Just ignore the compiler warning. */


#include "bf_register.h"
#include "functions.h"

#include "structures.h"
#include "log.h"
#include "streams.h"
#include "utils.h"

#include "storage.h"

#include <zlib.h>

const char *zlib_ext_version = "1.0";

/* zlib_version - Returns version information */
/* zlib_version(ANY internal) => STR version */
static package
bf_zlib_version(Var arglist, Byte next, void *vdata, Objid progr)
{
  Var r;
  Stream *s;

  r.type=TYPE_STR;
  s=new_stream(100);
  if(arglist.v.list[0].v.num == 1 && is_true(arglist.v.list[1])) {
    stream_printf(s, "Zlib Version %s", ZLIB_VERSION);
  } else {
    stream_printf(s, "Martian's ZLib Interface Extention v%s", zlib_ext_version);
  }
  r.v.str = str_dup(reset_stream(s));
  free_stream(s);
  return make_var_pack(r);
}
/* End zlib_version */

/* zlib_compress - Compress a binary string */
/* zlib_compress(STR Uncompressed) => STR Compressed */
static package
bf_zlib_compress(Var arglist, Byte next, void *vdata, Objid progr)
{
/*
ZEXTERN int ZEXPORT compress2 OF((Bytef *dest,   uLongf *destLen,
                                  const Bytef *source, uLong sourceLen,
                                  int level));
*/
  Var r;
  Stream *dest;
  const char *source;
  uLongf destLen, sourceLen;
  int result, level;

  if(!is_wizard(progr)) {
    free_var(arglist);
    return make_error_pack(E_PERM);
  }

  level = (arglist.v.list[0].v.num == 2) ? arglist.v.list[2].v.num : Z_DEFAULT_COMPRESSION;
  if(level < -1 || level > 9) {
    free_var(arglist);
    return make_raise_pack(E_INVARG, "Invalid compression setting.", zero);
  }


  source = binary_to_raw_bytes(arglist.v.list[1].v.str, (int*) &sourceLen);
  free_var(arglist);
  destLen = ((sourceLen+20) * 1005) / 1000;
  dest = new_stream(destLen);

  while((result = compress2(dest->buffer, &destLen, source, sourceLen, level)) == Z_BUF_ERROR) {
    destLen = destLen * 2;
    free_stream(dest);
    dest = new_stream(destLen);
  }
  if(result == Z_OK) {
    r.type = TYPE_STR;
    r.v.str = str_dup(raw_bytes_to_binary(dest->buffer, destLen));
    free_stream(dest);
    return make_var_pack(r);
  } else {
    free_stream(dest);
    return make_raise_pack(E_QUOTA, "Error while compressing", zero);
  }
}
/* End zlib_compress */

/* zlib_uncompress - Uncompress a binary string */
/* zlib_uncompress(STR compressed) => STR Uncompressed */
static package
bf_zlib_uncompress(Var arglist, Byte next, void *vdata, Objid progr)
{
/*
ZEXTERN int ZEXPORT uncompress OF((Bytef *dest,   uLongf *destLen,
                                   const Bytef *source, uLong sourceLen));
*/
  Var r;
  Stream *dest;
  const char *source;
  uLongf destLen, sourceLen;
  int result;

  if(!is_wizard(progr)) {
    free_var(arglist);
    return make_error_pack(E_PERM);
  }

  source = binary_to_raw_bytes(arglist.v.list[1].v.str, (int*) &sourceLen);
  free_var(arglist);
  destLen = sourceLen * 2;
  dest = new_stream(destLen);

  while((result = uncompress(dest->buffer, &destLen, source, sourceLen)) == Z_BUF_ERROR) {
    destLen = destLen * 2;
    free_stream(dest);
    dest = new_stream(destLen);
  }
  if(result == Z_OK) {
    r.type = TYPE_STR;
    r.v.str = str_dup(raw_bytes_to_binary(dest->buffer, destLen));
    free_stream(dest);
    return make_var_pack(r);
  } else if(result == Z_DATA_ERROR) {
    free_stream(dest);
    return make_raise_pack(E_INVARG, "Input data corrupted", zero);
  } else {
    free_stream(dest);
    return make_raise_pack(E_QUOTA, "Error while compressing", zero);
  }
}
/* End zlib_uncompress */

/* End Builtin functions */

void
register_zlib()
{
  oklog("Installing Martian's ZLib Interface Extentions v%s\n", zlib_ext_version);
  register_function("zlib_version", 0, 1, bf_zlib_version, TYPE_ANY);
  register_function("zlib_compress", 1, 2, bf_zlib_compress, TYPE_STR, TYPE_INT);
  register_function("zlib_uncompress", 1, 1, bf_zlib_uncompress, TYPE_STR);
}

/* Version 1.1 1998/08/08
 * MAJOR bugfixes
 * One must take extra special care against letting
 * the system free a static Stream's buffer.
 *
 * Version 1.0 1998/08/07
 * First production version
 *
 * Version 0.1a 1998/08/07
 * First attempt at implementation
 */
