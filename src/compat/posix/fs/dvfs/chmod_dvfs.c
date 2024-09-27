/**
 * @file
 * @brief
 *
 * @date 2.02.2016
 * @author Alex Kalmuk
 */

#include <sys/stat.h>
#include <errno.h>
#include <fs/dvfs.h>

int chmod(const char *path, mode_t mode) {
	struct lookup l = {};
	int res = dvfs_lookup(path, &l);
	struct dentry *d = l.item;

	if (res < 0) {
		errno = -res;
		return -1;
	}
	assert(d);
	assert(d->d_inode);

	d->d_inode->i_mode = mode;

	return 0;
}
