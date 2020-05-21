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

#ifndef QEMU_HACKING_H
#define QEMU_HACKING_H

extern GHashTable *hacking_type_table;

typedef void (printfun)(void *key, void *val);

/* print functions */
void print_type_table(void *key, void *value);
void print_properties_table(void *key, void *value);

/**
 * ghash_table_dump:
 * @hash_table: The hash table to dump.
 *
 * This function will dump contents of @hash_table
 */
void ghash_table_dump(const char *info, GHashTable *hash_table, printfun pfun);


#endif
