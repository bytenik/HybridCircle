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

#include "my-ctype.h"
#include "my-string.h"

#include "bf_register.h"
#include "config.h"
#include "exceptions.h"
#include "functions.h"
#include "hash.h"
#include "log.h"
#include "list.h"
#include "md5.h"
#include "options.h"
#include "pattern.h"
#include "random.h"
#include "ref_count.h"
#include "streams.h"
#include "storage.h"
#include "structures.h"
#include "unparse.h"
#include "utils.h"

/* I'm not thrilled with the function names, but I tried to mimic the rest
 * of LambdaMOO's coding style.  How I wish LambdaMOO used glib.
 *
 * Known issues:
 *
 * - hash loops (ie, inserting a hash into itself) are hazardous.  Don't do
 *   it.  You'll crash your server.
 *   - the code still lets you do it, however; I haven't decided if there's
 *     a good way to allow loops, so I haven't written code that prevents it
 * - do_hash isn't portable or very efficient.  portable is easy, efficient
 *   might be harder.
 */

#define HASH_DEF_SIZE 11

static void hashresize(Var v);
static int32 do_hash(Var v);

Var
new_hash(void)
{
    Var v;

    v.type = TYPE_HASH;
    v.v.hash = (Hash *) mymalloc(1 * sizeof(Hash), M_HASH);
    v.v.hash->size = HASH_DEF_SIZE;
    v.v.hash->nnodes = 0;
    v.v.hash->frozen = 0;
    v.v.hash->nodes =
        (HashNode **) mymalloc(v.v.hash->size * sizeof(HashNode *), M_VM);
    memset(v.v.hash->nodes, 0, v.v.hash->size * sizeof(HashNode *));

    return v;
}

static void
destroy_hashnode(HashNode *n)
{
    free_var(n->key);
    free_var(n->value);
    myfree(n, M_VM);
}

void
destroy_hash(Var v)
{
    int n;

    for (n = 0; n < v.v.hash->size; n++) {
        while (v.v.hash->nodes[n]) {
            HashNode *next = v.v.hash->nodes[n]->next;
            destroy_hashnode(v.v.hash->nodes[n]);
            v.v.hash->nodes[n] = next;
        }
    }

    myfree(v.v.hash->nodes, M_VM);
    myfree(v.v.hash, M_HASH);
}

/* Iterate over the entire hash, calling the function once per key/value 
 * pair */
void
hashforeach(Var v, hashfunc func, void *data)
{
    int n;
    int first = 1;
    HashNode *hn;

    for (n = 0; n < v.v.hash->size; n++) {
        for (hn = v.v.hash->nodes[n]; hn; hn = hn->next) {
            (* func)(hn->key, hn->value, data, first);
            first = 0;
        }
    }
}

static HashNode *
new_hashnode(Var key, Var value)
{
    HashNode *n;

    n = (HashNode *) mymalloc(1 * sizeof(HashNode), M_VM);
    n->key = key;
    n->value = value;
    n->next = NULL;
 
    return n;
}

static void
do_hash_hash(Var key, Var value, void *data, int32 first)
{
    int32 *h = (int32 *)data;

    *h *= do_hash(key);
    *h *= do_hash(value);
}

/* FIXME: these are really just for testing; they should be replaced with
 * portable, more reasonable hashes. */
static int32
do_hash(Var v)
{
    int t = v.type;

    switch (t) {
    case TYPE_STR: {
        unsigned32 h = str_hash(v.v.str);

        return h;
        
        break;
    }
    case TYPE_OBJ:
    case TYPE_ERR:
    case TYPE_INT:
        return v.v.num;
        break;
    case TYPE_FLOAT:
        return (int)(*v.v.fnum);
        break;
    case TYPE_HASH:
        {
            int32 value = 1;

            hashforeach(v, do_hash_hash, &value);

            return value;
        }
        break;
    case TYPE_LIST:
        {
            int32 value = 1;
            int n;

            for (n = 0; n <= v.v.list[0].v.num; n++ ) {
                value *= do_hash(v.v.list[n]);
            }
            return value;
        }
        break;
    default:
        errlog("DO_HASH: Unknown type (%d)\n", t);
        return 0;
	break;
    }
}

static int32
do_compare(Var a, Var b)
{
    return (equality(a, b, 0) == 0);
}

static inline HashNode **
do_hashlookup(Var v, Var key)
{
    HashNode **n;

    int32 index = do_hash(key) % v.v.hash->size;
    n = &(v.v.hash->nodes[index]);

    while (*n && do_compare((*n)->key, key))
        n = &(*n)->next;

    return n;
}

int
hashremove(Var v, Var key)
{
    HashNode **n, *d;

    n = do_hashlookup(v, key);

    if (*n) {
        d = *n;

        *n = d->next;
        destroy_hashnode(d);
        v.v.hash->nnodes--;
        if (!v.v.hash->frozen) {
            hashresize(v);
        }
        return 1;
    } else {
        return 0;
    }
}

int
hashlookup(Var v, Var key, Var *value)
{
    HashNode **n = do_hashlookup(v, key);

    if (*n) {
        if (value) {
            *value = (*n)->value;
        }
        return 1;
    } else {
        return 0;
    }
}

/* Based loosely on glib's hash resizing heuristics.  I don't bother finding
 * the nearest prime, though a motivated reader may be interested in doing
 * so. */
static void
hashresize(Var v)
{
    double npl;
    int new_size, n;
    HashNode **new_nodes;

    npl = (double) v.v.hash->nnodes / (double) v.v.hash->size;

    if ((npl > 0.3 || v.v.hash->size <= HASH_DEF_SIZE) && npl < 3.0)
        return;

    if (npl > 1.0) {
        new_size = v.v.hash->size * 2;
    } else {
        new_size = v.v.hash->size / 2;
    }

    new_nodes = (HashNode **) mymalloc(new_size * sizeof(HashNode *), M_VM);
    memset(new_nodes, 0, new_size * sizeof(HashNode *));

    for (n = 0; n < v.v.hash->size; n++) {
        HashNode *node, *next;

        for (node = v.v.hash->nodes[n]; node; node = next) {
            int32 index = do_hash(node->key) % new_size;

            next = node->next;
            node->next = new_nodes[index];
            new_nodes[index] = node;
        }
    }

    myfree(v.v.hash->nodes, M_VM);
    v.v.hash->nodes = new_nodes;
    v.v.hash->size = new_size;
}

void
hashinsert(Var v, Var key, Var value)
{
    HashNode **n;

    n = do_hashlookup(v, key);
    if (*n) {
        var_ref(value);
        free_var((*n)->value);
        (*n)->value = value;
    } else {
        var_ref(key);
        var_ref(value);
        *n = new_hashnode(key, value);
        v.v.hash->nnodes++;
        if (!v.v.hash->frozen) {
            hashresize(v);
        }
    }
}

unsigned32
hashsize(Var v)
{
    return v.v.hash->size;
}

unsigned32
hashnodes(Var v)
{
    return v.v.hash->nnodes;
}

int32
hash_equal(Var *a, Var *b)
{
    int n;
    HashNode *hn;

    /* If they have different numbers of nodes, they can't be identical. */
    if (a->v.hash->nnodes != b->v.hash->nnodes)
        return 0;

    /* otherwise, for each node in a, do a lookup in b */
    for (n = 0; n < a->v.hash->size; n++) {
        for (hn = a->v.hash->nodes[n]; hn; hn = hn->next) {
            Var value;

            if (!hashlookup(*b, hn->key, &value)) {
                return 0;
            }
            if (!equality(hn->value, value, 0)) {
                return 0;
            }
        }
    }

    return 1;
}

#ifdef DEBUG
void
dumphash(Var v)
{
    int n;
    HashNode *hn;

    for (n = 0; n < v.v.hash->size; n++) {
        for (hn = v.v.hash->nodes[n]; hn; hn = hn->next) {
            errlog("key type: %d  value type: %d\n",
                   hn->key.type, hn->value.type);
        }
    }
}
#endif

static package
bf_hash_remove(Var arglist, Byte next, void *vdata, Objid progr)
{
	Var key	= arglist.v.list[2],
		r	= var_dup(arglist.v.list[1]);	/* Modified from original patch; now makes a
											   copy of the hash and returns it instead of a status bit */
	if (!hashremove(r, key))
	{
		free_var(arglist);
		return make_error_pack(E_INVARG);
	}

	free_var(arglist);

	return make_var_pack(r);
}

static package
bf_hashslice(Var arglist, Byte next, void *vdata, Objid progr)
{
	int			n,
				nCounter	= 0;
    HashNode	*hn;
	Var			res,
				v			= arglist.v.list[1];
	
	free_var(arglist);

	if(v.type != TYPE_HASH)
		return make_error_pack(E_TYPE);
	
	res = new_list(v.v.hash->nnodes);
	
	for (n = 0; n < v.v.hash->nnodes; n++)
	{
		hn = v.v.hash->nodes[n];
		res.v.list[n] = new_list(2);
		res.v.list[n].v.list[1] = var_dup(hn->key);
		res.v.list[n].v.list[2] = var_dup(hn->value);
		oklog("Loop #%d good...\n", n);
	}

	return make_var_pack(res);
}

void
register_hash(void)
{
    register_function("hashdelete", 2, 2, bf_hash_remove, TYPE_HASH,
                      TYPE_ANY);
	register_function("hash_remove", 2, 2, bf_hash_remove, TYPE_HASH,
                      TYPE_ANY);
	register_function("hashslice", 1, 1, bf_hashslice, TYPE_HASH);
}
