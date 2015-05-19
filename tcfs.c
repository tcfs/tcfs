#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <fuse.h>

#include "utils.h"

struct tcfs_ctx {
	int sockfd;
};

static int tcfs_getattr(const char *path, struct stat *statbuf)
{
	int retstat = 0;

	/* TODO */
	return retstat;
}

static int tcfs_readlink(const char *path, char *link, size_t size)
{
	int retstat = 0;

	/* TODO */
	return retstat;
}

static int tcfs_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler)
{
	int retstat = 0;

	/* TODO */
	return retstat;
}

static int tcfs_mknod(const char *path, mode_t mode, dev_t dev)
{
	int retstat = 0;

	/* TODO */
	return retstat;
}

static int tcfs_mkdir(const char *path, mode_t mode)
{
	int retstat = 0;

	/* TODO */
	return retstat;
}

static int tcfs_symlink(const char *path, const char *link)
{
	int retstat = 0;

	/* TODO */
	return retstat;
}

static int tcfs_unlink(const char *path)
{
	int retstat = 0;

	/* TODO */
	return retstat;
}

static int tcfs_rmdir(const char *path)
{
	int retstat = 0;

	/* TODO */
	return retstat;
}

static int tcfs_rename(const char *path, const char *newpath)
{
	int retstat = 0;

	/* TODO */
	return retstat;
}

static int tcfs_chmod(const char *path, mode_t mode)
{
	int retstat = 0;

	/* TODO */
	return retstat;
}

static int tcfs_chown(const char *path, uid_t uid, gid_t gid)
{
	int retstat = 0;

	/* TODO */
	return retstat;
}

static int tcfs_truncate(const char *path, off_t newsize)
{
	int retstat = 0;

	/* TODO */
	return retstat;
}

static int tcfs_utime(const char *path, struct utimbuf *ubuf)
{
	int retstat = 0;

	/* TODO */
	return retstat;
}

static int tcfs_open(const char *path, struct fuse_file_info *fi)
{
	int retstat = 0;

	/* TODO */
	return retstat;
}

static int tcfs_read(const char *path, char *rbuf, size_t size, off_t offset,
			struct fuse_file_info *fi)
{
	int retstat = 0;

	/* TODO */
	return retstat;
}

static int tcfs_write(const char *path, const char *wbuf, size_t size,
			off_t offset, struct fuse_file_info *fi)
{
	int retstat = 0;

	/* TODO */
	return retstat;
}

struct fuse_operations tcfs_oper = {
	.getattr  = tcfs_getattr,
	.readlink = tcfs_readlink,
	.getdir   = tcfs_getdir,
	.mknod    = tcfs_mknod,
	.mkdir    = tcfs_mkdir,
	.symlink  = tcfs_symlink,
	.unlink   = tcfs_unlink,
	.rmdir    = tcfs_rmdir,
	.rename   = tcfs_rename,
	.chmod    = tcfs_chmod,
	.chown    = tcfs_chown,
	.truncate = tcfs_truncate,
	.utime    = tcfs_utime,
	.open     = tcfs_open,
	.read     = tcfs_read,
	.write    = tcfs_write,
};

int main(int argc, char **argv)
{
	int fuse_stat, fuse_argc;
	char *fuse_argv[4];
	char *mount_point;
	struct tcfs_ctx *tcfs_data;

	if (argc < 2) {
		fprintf(stderr, "usage: %s mount_point\n", argv[0]);
		exit(1);
	}
	mount_point = argv[1];

	tcfs_data = calloc(1, sizeof(*tcfs_data));
	if (tcfs_data == NULL) {
		perror("calloc");
		exit(1);
	}
	tcfs_data->sockfd = client_connect("127.0.0.1", 9876);
	assert(tcfs_data->sockfd > 0);
	fuse_argc = 0;
	fuse_argv[fuse_argc++] = "tcfs";
	fuse_argv[fuse_argc++] = "-s"; /* single thread */
	fuse_argv[fuse_argc++] = "-d"; /* debug and core dump */
	fuse_argv[fuse_argc++] = mount_point;
	fuse_stat = fuse_main(fuse_argc, fuse_argv, &tcfs_oper, tcfs_data);
	return fuse_stat;
}
