
/*
 * Hacking stuff for QEMU
 *
 * Authors:
 *  Tong Chen   <yihect@gmail.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#include "qemu/osdep.h"
#include "hacking/hacking.h"


void ghash_table_dump(GHashTable *hash_table, printfun pfun)
{
    GHashTableIter iter;
    gpointer key;
    gpointer value;

    g_assert(pfun);
    g_hash_table_iter_init(&iter, hash_table);
    while (g_hash_table_iter_next(&iter, &key, &value))
    {
	pfun(key, value);
    }
}

