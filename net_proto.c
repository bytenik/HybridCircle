/***********************************************************\
|	HybridCircle - by the Hybrid Development Team			|
|                  ByteNik Solutions						|
|															|
|	Copyright (c) 2002 by ByteNik Solutions					|
|															|
|	HybridCircle is distributed under the GNU Lesser		|
|	General Public License (LGPL), and is the intellectual	|
|	property of ByteNik Solutions. All rights not granted	|
|	explicitly by the LGPL are reserved. This product is	|
|	protected by various international copyright laws and	|
|	treaties and falls under jurisdiction of the United		|
|	States government.										|
|															|
|	All distributions of this code, whether modified or in	|
|	their original form, must maintain this licence at the	|
|	top. Additionally, all new additions to HybridCircle	|
|	may only be distributed if they feature this licence at	|
|	the beginning of their code and are distributed under	|
|	the LGPL.												|
|															|
|	ByteNik Solutions does not claim ownership to any code	|
|	originating from the Xerox PARC laboratory or any other	|
|	patches written by third parties for the LambdaMOO		|
|	platform. LambdaMOO, from which this server is based,	|
|	is not owned by ByteNik Solutions. However, any and all	|
|	changes made by ByteNik Solutions are their sole		|
|	property. The original Xerox licence agreement should	|
|	be distributed with the HybridCircle source code along	|
|	with HybridCircle's comprehensive copyright and licence	|
|	agreement.												|
|															|
|	The latest version of HybridCircle and the HybridCircle	|
|	source code should be available at HybridCircle's		|
|	website, at:											|
|		--- http://www.hybrid-moo.net/hybridcircle			|
|	or at HybridSphere's SourceForge project, at:			|
|		--- http://sourceforge.net/projects/hybridsphere	|
\***********************************************************/

#include "options.h"

#  if NETWORK_PROTOCOL == NP_TCP
#    if NETWORK_STYLE == NS_BSD
#      include "net_bsd_tcp.c"
#    endif
#    if NETWORK_STYLE == NS_SYSV
#      include "net_sysv_tcp.c"
#    endif
#  endif

#  if NETWORK_PROTOCOL == NP_LOCAL
#    if NETWORK_STYLE == NS_BSD
#      include "net_bsd_lcl.c"
#    endif
#    if NETWORK_STYLE == NS_SYSV
#      include "net_sysv_lcl.c"
#    endif
#  endif

char rcsid_net_proto[] = "$Id: net_proto.c,v 1.2 2002/06/11 22:57:39 bytenik Exp $";

/* 
 * $Log: net_proto.c,v $
 * Revision 1.2  2002/06/11 22:57:39  bytenik
 * Fixed various compiler warnings.
 *
 * Revision 1.1.1.1  2002/02/22 19:17:42  bytenik
 * Initial import of HybridCircle 2.1i-beta1
 *
 * Revision 1.1.1.1  2001/01/28 16:41:46  bytenik
 *
 *
 * Revision 1.2  1998/12/14 13:18:33  nop
 * Merge UNSAFE_OPTS (ref fixups); fix Log tag placement to fit CVS whims
 *
 * Revision 1.1.1.1  1997/03/03 03:45:00  nop
 * LambdaMOO 1.8.0p5
 *
 * Revision 2.1  1996/02/08  06:58:12  pavel
 * Updated copyright notice for 1996.  Release 1.8.0beta1.
 *
 * Revision 2.0  1995/11/30  04:28:43  pavel
 * New baseline version, corresponding to release 1.8.0alpha1.
 *
 * Revision 1.1  1995/11/30  04:28:36  pavel
 * Initial revision
 */
