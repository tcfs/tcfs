#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include <fuse.h>

#include "utils.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define debug_print(fmt, ...) \
	do { \
		fprintf(stderr, ANSI_COLOR_GREEN"debug_print: %s: %d: %s():" \
			fmt "\n"ANSI_COLOR_RESET, __FILE__, __LINE__, __func__,\
			##__VA_ARGS__); \
	} while (0)

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

struct tcfs_ctx_s {
	int sockfd;
	char buf[4096];
};

static int get_reply(int fd, char *buf)
{
	int ret, n;

	ret = readn(fd, buf, 2);
	if (ret <= 0)
		return ret;
	n = *buf * 256 + *(buf+1);
	ret = readn(fd, buf, n);
	assert(ret == n);
	return ret;
}

static int send_msg(int fd, char *buf, int len)
{
	char len_flag[2];
	struct iovec sendbuf[2];

	len_flag[1] = len & 0xff;
	len_flag[0] = (len & 0xff00) >> 8;
	sendbuf[0].iov_base = len_flag;
	sendbuf[0].iov_len = 2;
	sendbuf[1].iov_base = buf;
	sendbuf[1].iov_len = len;
	int ret = writevn(fd, sendbuf, 2, len + 2);
	assert(ret == len + 2); (void)ret;
	return 0;
}

static int tcfs_getattr(const char *path, struct stat *statbuf)
{
	int retstat;
	struct tcfs_ctx_s *tc = fuse_get_context()->private_data;
	int sock = tc->sockfd;
	int len;
	ssize_t ret;

	debug_print("path: %s", path);
	len = sprintf(tc->buf, "getattr%s", path);
	(void)send_msg(sock, tc->buf, len);
	ret = get_reply(sock, tc->buf);
	retstat = buf_get_uint32(tc->buf);
	memset(statbuf, 0, sizeof(*statbuf));
	if (retstat != 0)
		return -1;
	assert(ret == 44); (void)ret;
	statbuf->st_dev   = buf_get_uint32(tc->buf + 4);
	statbuf->st_ino   = buf_get_uint32(tc->buf + 8);
	statbuf->st_mode  = buf_get_uint32(tc->buf + 12);
	statbuf->st_nlink = buf_get_uint32(tc->buf + 16);
	statbuf->st_uid   = buf_get_uint32(tc->buf + 20);
	statbuf->st_gid   = buf_get_uint32(tc->buf + 24);
	statbuf->st_size  = buf_get_uint32(tc->buf + 28);
	statbuf->st_atime = buf_get_uint32(tc->buf + 32);
	statbuf->st_mtime = buf_get_uint32(tc->buf + 36);
	statbuf->st_ctime = buf_get_uint32(tc->buf + 40);
	return retstat;
}

static int tcfs_readlink(const char *path, char *link, size_t size)
{
	int retstat = -1;

	/* TODO */
	debug_print("path: %s", path);
	return retstat;
}

static int tcfs_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler)
{
	int retstat = -1;

	/* TODO */
	debug_print("path: %s", path);
	return retstat;
}

static int tcfs_mknod(const char *path, mode_t mode, dev_t dev)
{
	int retstat = -1;

	/* TODO */
	debug_print("path: %s", path);
	return retstat;
}

static int tcfs_mkdir(const char *path, mode_t mode)
{
	int retstat = -1;

	/* TODO */
	debug_print("path: %s", path);
	return retstat;
}

static int tcfs_symlink(const char *path, const char *link)
{
	int retstat = -1;

	/* TODO */
	debug_print("path: %s", path);
	return retstat;
}

static int tcfs_unlink(const char *path)
{
	int retstat = -1;

	/* TODO */
	debug_print("path: %s", path);
	return retstat;
}

static int tcfs_rmdir(const char *path)
{
	int retstat = -1;

	/* TODO */
	debug_print("path: %s", path);
	return retstat;
}

static int tcfs_rename(const char *path, const char *newpath)
{
	int retstat = -1;

	/* TODO */
	debug_print("path: %s", path);
	return retstat;
}

static int tcfs_chmod(const char *path, mode_t mode)
{
	int retstat = -1;

	/* TODO */
	debug_print("path: %s", path);
	return retstat;
}

static int tcfs_chown(const char *path, uid_t uid, gid_t gid)
{
	int retstat = -1;

	/* TODO */
	debug_print("path: %s", path);
	return retstat;
}

static int tcfs_truncate(const char *path, off_t newsize)
{
	int retstat;
	int len;
	struct tcfs_ctx_s *tc = fuse_get_context()->private_data;
	int sock = tc->sockfd;
	ssize_t ret;

	len = sprintf(tc->buf, "truncate");
	buf_add_uint32(tc->buf + len, newsize);
	len += 4;
	len += sprintf(tc->buf + len, "%s", path);
	(void)send_msg(sock, tc->buf, len);
	ret = get_reply(sock, tc->buf);
	assert(ret == 4); (void)ret;
	retstat = buf_get_uint32(tc->buf);
	return retstat;
}

static int tcfs_utime(const char *path, struct utimbuf *ubuf)
{
	int retstat = -1;

	/* TODO */
	debug_print("path: %s", path);
	return retstat;
}

static int tcfs_open(const char *path, struct fuse_file_info *fi)
{
	int retstat;
	int len;
	struct tcfs_ctx_s *tc = fuse_get_context()->private_data;
	int sock = tc->sockfd;
	ssize_t ret;

	len = sprintf(tc->buf, "open");
	buf_add_uint32(tc->buf + len, fi->flags);
	len += 4;
	len += sprintf(tc->buf + len, "%s", path);
	(void)send_msg(sock, tc->buf, len);
	ret = get_reply(sock, tc->buf);
	retstat = buf_get_uint32(tc->buf);
	if (retstat != 0) {
		fi->fh = -1;
		return -1;
	}
	assert(ret == 8); (void)ret;
	fi->fh = buf_get_uint32(tc->buf + 4);
	if (fi->fh < 0)
		return -1;
	return 0;
}

static int tcfs_read(const char *path, char *rbuf, size_t size, off_t offset,
			struct fuse_file_info *fi)
{
	int len;
	struct tcfs_ctx_s *tc = fuse_get_context()->private_data;
	int sock = tc->sockfd;
	ssize_t ret, readed;

	len = sprintf(tc->buf, "read");
	buf_add_uint32(tc->buf + len, fi->fh);
	buf_add_uint32(tc->buf + len + 4, offset);
	buf_add_uint32(tc->buf + len + 8, size);
	len += 12;
	len += sprintf(tc->buf + len, "%s", path);
	(void)send_msg(sock, tc->buf, len);
	ret = get_reply(sock, tc->buf);
	assert(ret >= 4); (void)ret;
	readed = buf_get_uint32(tc->buf);
	if (readed <= 0)
		return readed;
	memcpy(rbuf, tc->buf + 4, readed);
	return readed;
}

static int tcfs_write(const char *path, const char *wbuf, size_t size,
			off_t offset, struct fuse_file_info *fi)
{
	int len;
	struct tcfs_ctx_s *tc = fuse_get_context()->private_data;
	int sock = tc->sockfd;
	ssize_t ret, readed;

	len = sprintf(tc->buf, "write");
	buf_add_uint32(tc->buf + len, fi->fh);
	buf_add_uint32(tc->buf + len + 4, offset);
	buf_add_uint32(tc->buf + len + 8, size);
	len += 12;
	memcpy(tc->buf + len, wbuf, size);
	len += size;
	(void)send_msg(sock, tc->buf, len);
	ret = get_reply(sock, tc->buf);
	assert(ret >= 4); (void)ret;
	readed = buf_get_uint32(tc->buf);
	return readed;
}

int tcfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi)
{
	int retstat;
	int len;
	struct tcfs_ctx_s *tc = fuse_get_context()->private_data;
	int sock = tc->sockfd;
	ssize_t ret;

	debug_print("path: %s", path);
	len = sprintf(tc->buf, "readdir%s", path);
	(void)send_msg(sock, tc->buf, len);
	ret = get_reply(sock, tc->buf);
	retstat = buf_get_uint32(tc->buf);
	if (retstat != 0)
		return -1;
	tc->buf[ret] = '\0';
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	const char *p;
	for (p = tc->buf + 4;
	     p - tc->buf < ret;
	     p += strnlen(p, ret - (p - tc->buf)) + 1) {
		debug_print("readdir: %s", p);
		filler(buf, p, NULL, 0);
	}
	return retstat;
}

int tcfs_release(const char *path, struct fuse_file_info *fi)
{
	int retstat;
	int len;
	struct tcfs_ctx_s *tc = fuse_get_context()->private_data;
	int sock = tc->sockfd;
	ssize_t ret;

	len = sprintf(tc->buf, "release");
	buf_add_uint32(tc->buf + len, fi->fh);
	len += 4;
	(void)send_msg(sock, tc->buf, len);
	ret = get_reply(sock, tc->buf);
	assert(ret == 4); (void)ret;
	retstat = buf_get_uint32(tc->buf);
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
	.readdir  = tcfs_readdir,
	.release  = tcfs_release,
};

static void usage(char **argv)
{
	fprintf(stderr, "Usage: %s [Options] <mountpoint>\n"
			"\n"
			"Options:\n"
			" -h --help                Show this help\n"
			" -d --debug               Show debug info\n"
			" -s --server <server-ip>  Specify server ipaddr\n"
			" -p --port <server-port>  Specify server port\n"
			"\n", argv[0]);
}

static struct option long_opts[] = {
	{"help",        no_argument, 0, 'h'},
	{"debug",	no_argument, 0, 'd'},
	{"server",	required_argument, 0, 's'},
	{"port",	required_argument, 0, 'p'},
	{0, 0, 0, 0}
};

int main(int argc, char **argv)
{
	int fuse_stat, fuse_argc;
	const char *server = "127.0.0.1";
	uint16_t port = 9876;
	char *fuse_argv[4];
	char *mount_point = NULL;
	struct tcfs_ctx_s *tcfs_data;
	int opt, index;
	bool is_debug = false;

	while ((opt = getopt_long(argc, argv,
			"hds:p:", long_opts, &index)) != -1) {
		switch (opt) {
		case 's':
			server = optarg;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'd':
			is_debug = true;
			break;
		case '0':
		case 'h':
		default:
			usage(argv);
			exit(0);
		}
	}
	if (argc <= optind) {
		usage(argv);
		exit(1);
	} else
		mount_point = argv[optind];
	tcfs_data = calloc(1, sizeof(*tcfs_data));
	if (tcfs_data == NULL) {
		perror("calloc");
		exit(1);
	}
	tcfs_data->sockfd = client_connect(server, port);
	assert(tcfs_data->sockfd > 0);
	fuse_argc = 0;
	fuse_argv[fuse_argc++] = "tcfs";
	fuse_argv[fuse_argc++] = "-s"; /* single thread */
	if (is_debug)
		fuse_argv[fuse_argc++] = "-d"; /* debug and core dump */
	fuse_argv[fuse_argc++] = mount_point;
	fuse_stat = fuse_main(fuse_argc, fuse_argv, &tcfs_oper, tcfs_data);
	return fuse_stat;
}
