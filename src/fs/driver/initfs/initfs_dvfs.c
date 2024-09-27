/**
 * @file
 * @details Read-only filesystem with direct address space mapping.
 *
 * @date 7 May 2015
 * @author Anton Bondarev
 *	        - initial implementation
 * @author Nikolay Korotky
 *	        - rework using vfs
 * @author Eldar Abusalimov
 *	        - rework mount to use cpio_parse_entry
 * @author Denis Deryugin
 *              - port from old VFS
 *
 * @note   Initfs is based on CPIO archive format. By design, this format
 *         has no directory abstraction, as all files are stored with full
 *         path names. Because of this it could be tricky to handle some
 *         VFS calls.
 */
#include <util/log.h>

#include <stdint.h>
#include <cpio.h>
//#include <limits.h>
#include <string.h>

#include <fs/inode.h>
#include <fs/super_block.h>

#include "initfs.h"

struct inode *initfs_lookup(char const *name, struct inode const *dir) {
	extern char _initfs_start;
	char *cpio = &_initfs_start;
	struct cpio_entry entry;
	struct inode *node = NULL;
	struct initfs_file_info *fi = inode_priv(dir);

	while ((cpio = cpio_parse_entry(cpio, &entry))) {
		if (fi->path && memcmp(fi->path, entry.name, fi->path_len)) {
			continue;
		}
		if (!strcmp(name,
		             entry.name + fi->path_len + (*(entry.name + fi->path_len) == '/' ? 1 : 0)) &&
			strrchr(entry.name + fi->path_len + 1, '/') == NULL) {

			if (!S_ISDIR(entry.mode) && !S_ISREG(entry.mode)) {
				log_error("Unknown inode type in cpio\n");
				break;
			}

			node = dvfs_alloc_inode(dir->i_sb);
			if (node == NULL) {
				break;
			}

			if (0 > initfs_fill_inode(node, cpio, &entry)) {
				dvfs_destroy_inode(node);
				return NULL;
			}

			return node;
		}
	}

	return NULL;
}

extern struct idesc *dvfs_file_open_idesc(struct lookup *lookup, int __oflag);

struct super_block_operations initfs_sbops = {
	.open_idesc = dvfs_file_open_idesc,
	.destroy_inode = initfs_destroy_inode,
};
