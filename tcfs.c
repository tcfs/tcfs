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
#include "encrypt.h"

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

#define SETOP(buf, opcode) ({buf_add_uint32((buf), (opcode)); 4;})

enum tcfs_op {
	GETATTR  = 0x01,
	READLINK = 0x02,
	GETDIR   = 0x03,
	MKNOD    = 0x04,
	MKDIR    = 0x05,
	SYMLINK  = 0x06,
	UNLINK   = 0x07,
	RMDIR    = 0x08,
	RENAME   = 0x09,
	CHMOD    = 0x0A,
	CHOWN    = 0x0B,
	TRUNCATE = 0x0C,
	UTIME    = 0x0D,
	OPEN     = 0x0E,
	READ     = 0x0F,
	WRITE    = 0x10,
	READDIR  = 0x11,
	RELEASE  = 0x12,
	CREATE   = 0x13,
};

struct tcfs_ctx_s {
	int sockfd;
	char buf[4096 * 1024]; /* 4 MB */
	struct encryptor *encryptor;
	int (*tcfs_recv)(struct encryptor *decryptor, int fd, char *buf);
	int (*tcfs_write)(struct encryptor *encryptor, int fd,
				char *buf, int len);
};

static int _get_reply(int fd, char *buf)
{
	int ret, n;

	ret = readn(fd, buf, 4);
	if (ret <= 0)
		return ret;
	n = buf_get_uint32(buf);
	ret = readn(fd, buf, n);
	assert(ret == n);
	return ret;
}

static int get_reply(struct encryptor *_nil, int fd, char *buf)
{
	return _get_reply(fd, buf);
}

static int decrypt_get_replay(struct encryptor *decryptor, int fd, char *buf)
{
	int ret, n;

	ret = readn(fd, buf, 4);
	if (ret <= 0)
		return ret;
	decrypt(decryptor, (uint8_t *)buf, (uint8_t *)buf, 4);
	n = buf_get_uint32(buf);
	ret = readn(fd, buf, n);
	assert(ret == n);
	decrypt(decryptor, (uint8_t *)buf, (uint8_t *)buf, n);
	return ret;
}

static int _send_msg(int fd, char *buf, int len)
{
	uint8_t len_flag[4];
	struct iovec sendbuf[2];

	len_flag[3] = len & 0xff;
	len_flag[2] = (len & 0xff00) >> 8;
	len_flag[1] = (len & 0xff0000) >> 16;
	len_flag[0] = (len & 0xff000000) >> 24;
	sendbuf[0].iov_base = len_flag;
	sendbuf[0].iov_len = 4;
	sendbuf[1].iov_base = buf;
	sendbuf[1].iov_len = len;
	int ret = writevn(fd, sendbuf, 2, len + 4);
	assert(ret == len + 4); (void)ret;
	return 0;
}

static int send_msg(struct encryptor *_nil, int fd, char *buf, int len)
{
	return _send_msg(fd, buf, len);
}

static int encrypt_send_msg(struct encryptor *encryptor, int fd,
				char *buf, int len)
{
	uint8_t len_flag[4];
	struct iovec sendbuf[2];

	len_flag[3] = len & 0xff;
	len_flag[2] = (len & 0xff00) >> 8;
	len_flag[1] = (len & 0xff0000) >> 16;
	len_flag[0] = (len & 0xff000000) >> 24;
	encrypt(encryptor, len_flag, len_flag, 4);
	encrypt(encryptor, (uint8_t *)buf, (uint8_t *)buf, len);
	sendbuf[0].iov_base = len_flag;
	sendbuf[0].iov_len = 4;
	sendbuf[1].iov_base = buf;
	sendbuf[1].iov_len = len;
	int ret = writevn(fd, sendbuf, 2, len + 4);
	assert(ret == len + 4); (void)ret;
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
	len = SETOP(tc->buf, GETATTR);
	len += sprintf(tc->buf + len, "%s", path);
	tc->tcfs_write(tc->encryptor, sock, tc->buf, len);
	ret = tc->tcfs_recv(tc->encryptor, sock, tc->buf);
	retstat = buf_get_uint32(tc->buf);
	memset(statbuf, 0, sizeof(*statbuf));
	if (retstat != 0)
		return retstat;
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
	int retstat;
	int len;
	struct tcfs_ctx_s *tc = fuse_get_context()->private_data;
	int sock = tc->sockfd;
	ssize_t ret;

	debug_print("path: %s", path);
	len = SETOP(tc->buf, MKDIR);
	buf_add_uint32(tc->buf + len, mode);
	len += 4;
	len += sprintf(tc->buf + len, "%s", path);
	tc->tcfs_write(tc->encryptor, sock, tc->buf, len);
	ret = tc->tcfs_recv(tc->encryptor, sock, tc->buf);
	retstat = buf_get_uint32(tc->buf);
	if (retstat != 0)
		return retstat;
	assert(ret == 4); (void)ret;
	return 0;
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
	int retstat;
	int len;
	struct tcfs_ctx_s *tc = fuse_get_context()->private_data;
	int sock = tc->sockfd;
	ssize_t ret;

	debug_print("path: %s", path);
	len = SETOP(tc->buf, UNLINK);
	len += sprintf(tc->buf + len, "%s", path);
	tc->tcfs_write(tc->encryptor, sock, tc->buf, len);
	ret = tc->tcfs_recv(tc->encryptor, sock, tc->buf);
	retstat = buf_get_uint32(tc->buf);
	if (retstat != 0)
		return retstat;
	assert(ret == 4); (void)ret;
	return 0;
}

static int tcfs_rmdir(const char *path)
{
	int retstat;
	int len;
	struct tcfs_ctx_s *tc = fuse_get_context()->private_data;
	int sock = tc->sockfd;
	ssize_t ret;

	debug_print("path: %s", path);
	len = SETOP(tc->buf, RMDIR);
	len += sprintf(tc->buf + len, "%s", path);
	tc->tcfs_write(tc->encryptor, sock, tc->buf, len);
	ret = tc->tcfs_recv(tc->encryptor, sock, tc->buf);
	retstat = buf_get_uint32(tc->buf);
	if (retstat != 0)
		return retstat;
	assert(ret == 4); (void)ret;
	return 0;
}

static int tcfs_rename(const char *path, const char *newpath)
{
	int retstat = -1;

	/* TODO */
	debug_print("path: %s, newpath %s", path, newpath);
	return retstat;
}

static int tcfs_chmod(const char *path, mode_t mode)
{
	int retstat;
	int len;
	struct tcfs_ctx_s *tc = fuse_get_context()->private_data;
	int sock = tc->sockfd;
	ssize_t ret;

	debug_print("path: %s", path);
	len = SETOP(tc->buf, CHMOD);
	buf_add_uint32(tc->buf + len, mode);
	len += 4;
	len += sprintf(tc->buf + len, "%s", path);
	tc->tcfs_write(tc->encryptor, sock, tc->buf, len);
	ret = tc->tcfs_recv(tc->encryptor, sock, tc->buf);
	retstat = buf_get_uint32(tc->buf);
	if (retstat != 0) {
		errno = retstat;
		return -1;
	}
	assert(ret == 4); (void)ret;
	return 0;
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

	debug_print("path: %s", path);
	len = SETOP(tc->buf, TRUNCATE);
	buf_add_uint32(tc->buf + len, newsize);
	len += 4;
	len += sprintf(tc->buf + len, "%s", path);
	tc->tcfs_write(tc->encryptor, sock, tc->buf, len);
	ret = tc->tcfs_recv(tc->encryptor, sock, tc->buf);
	assert(ret == 4); (void)ret;
	retstat = buf_get_uint32(tc->buf);
	return retstat;
}

static int tcfs_utime(const char *path, struct utimbuf *ubuf)
{
	int retstat;
	int len;
	struct tcfs_ctx_s *tc = fuse_get_context()->private_data;
	int sock = tc->sockfd;
	ssize_t ret;

	debug_print("path: %s", path);
	len = SETOP(tc->buf, UTIME);
	buf_add_uint64(tc->buf + len, ubuf->actime);
	len += 8;
	buf_add_uint64(tc->buf + len, ubuf->modtime);
	len += 8;
	len += sprintf(tc->buf + len, "%s", path);
	tc->tcfs_write(tc->encryptor, sock, tc->buf, len);
	ret = tc->tcfs_recv(tc->encryptor, sock, tc->buf);
	assert(ret == 4); (void)ret;
	retstat = buf_get_uint32(tc->buf);
	return retstat;
}

static int tcfs_open(const char *path, struct fuse_file_info *fi)
{
	int retstat;
	int len;
	struct tcfs_ctx_s *tc = fuse_get_context()->private_data;
	int sock = tc->sockfd;
	ssize_t ret;

	debug_print("path: %s", path);
	len = SETOP(tc->buf, OPEN);
	buf_add_uint32(tc->buf + len, fi->flags);
	len += 4;
	len += sprintf(tc->buf + len, "%s", path);
	tc->tcfs_write(tc->encryptor, sock, tc->buf, len);
	ret = tc->tcfs_recv(tc->encryptor, sock, tc->buf);
	retstat = buf_get_uint32(tc->buf);
	if (retstat != 0) {
		fi->fh = -1;
		return retstat;
	}
	assert(ret == 8); (void)ret;
	fi->fh = buf_get_uint32(tc->buf + 4);
	assert(fi->fh >= 0);
	return 0;
}

static int tcfs_read(const char *path, char *rbuf, size_t size, off_t offset,
			struct fuse_file_info *fi)
{
	int len;
	struct tcfs_ctx_s *tc = fuse_get_context()->private_data;
	int sock = tc->sockfd;
	ssize_t ret;
	int readed;

	debug_print("path: %s", path);
	len = SETOP(tc->buf, READ);
	buf_add_uint32(tc->buf + len, fi->fh);
	buf_add_uint32(tc->buf + len + 4, offset);
	buf_add_uint32(tc->buf + len + 8, size);
	len += 12;
	len += sprintf(tc->buf + len, "%s", path);
	tc->tcfs_write(tc->encryptor, sock, tc->buf, len);
	ret = tc->tcfs_recv(tc->encryptor, sock, tc->buf);
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
	ssize_t ret;
	int readed;

	debug_print("path: %s", path);
	len = SETOP(tc->buf, WRITE);
	buf_add_uint32(tc->buf + len, fi->fh);
	buf_add_uint32(tc->buf + len + 4, offset);
	buf_add_uint32(tc->buf + len + 8, size);
	debug_print("size: %d", (int)size);
	len += 12;
	memcpy(tc->buf + len, wbuf, size);
	len += size;
	tc->tcfs_write(tc->encryptor, sock, tc->buf, len);
	ret = tc->tcfs_recv(tc->encryptor, sock, tc->buf);
	assert(ret >= 4); (void)ret;
	readed = buf_get_uint32(tc->buf);
	return readed;
}

static int tcfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			off_t offset, struct fuse_file_info *fi)
{
	int retstat;
	int len;
	struct tcfs_ctx_s *tc = fuse_get_context()->private_data;
	int sock = tc->sockfd;
	ssize_t ret;

	debug_print("path: %s", path);
	len = SETOP(tc->buf, READDIR);
	len += sprintf(tc->buf + len, "%s", path);
	tc->tcfs_write(tc->encryptor, sock, tc->buf, len);
	ret = tc->tcfs_recv(tc->encryptor, sock, tc->buf);
	retstat = buf_get_uint32(tc->buf);
	if (retstat != 0)
		return retstat;
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

static int tcfs_release(const char *path, struct fuse_file_info *fi)
{
	int retstat;
	int len;
	struct tcfs_ctx_s *tc = fuse_get_context()->private_data;
	int sock = tc->sockfd;
	ssize_t ret;

	debug_print("path: %s", path);
	len = SETOP(tc->buf, RELEASE);
	buf_add_uint32(tc->buf + len, fi->fh);
	len += 4;
	tc->tcfs_write(tc->encryptor, sock, tc->buf, len);
	ret = tc->tcfs_recv(tc->encryptor, sock, tc->buf);
	assert(ret == 4); (void)ret;
	retstat = buf_get_uint32(tc->buf);
	return retstat;
}

static int tcfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int retstat;
	int len;
	struct tcfs_ctx_s *tc = fuse_get_context()->private_data;
	int sock = tc->sockfd;
	ssize_t ret;

	debug_print("create: %s", path);
	len = SETOP(tc->buf, CREATE);
	buf_add_uint32(tc->buf + len, mode);
	len += 4;
	len += sprintf(tc->buf + len, "%s", path);
	tc->tcfs_write(tc->encryptor, sock, tc->buf, len);
	ret = tc->tcfs_recv(tc->encryptor, sock, tc->buf);
	retstat = buf_get_uint32(tc->buf);
	if (retstat != 0) {
		fi->fh = -1;
		return retstat;
	}
	assert(ret == 8); (void)ret;
	fi->fh = buf_get_uint32(tc->buf + 4);
	assert(fi->fh >= 0);
	return 0;
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
	.create   = tcfs_create,
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
			" -m --method <rc4>        Specify encryption method\n"
			" -k --key <password>      Specify password\n"
			"\n", argv[0]);
}

static struct option long_opts[] = {
	{"help",        no_argument, 0, 'h'},
	{"debug",	no_argument, 0, 'd'},
	{"server",	required_argument, 0, 's'},
	{"port",	required_argument, 0, 'p'},
	{"method",	required_argument, 0, 'm'},
	{"key",		required_argument, 0, 'k'},
	{0, 0, 0, 0}
};

struct tcfs_ctx_s *create_tcfs(const char *server_ip, uint16_t server_port,
			enum encrypt_method encry_method, const char *key)
{
	struct tcfs_ctx_s *tcfs_data = calloc(1, sizeof(*tcfs_data));
	if (tcfs_data == NULL) {
		perror("calloc");
		exit(errno);
	}
	tcfs_data->sockfd = client_connect(server_ip, server_port);
	if (tcfs_data->sockfd < 0) {
		fprintf(stderr, "client_connect failed: %s\n", strerror(errno));
		exit(errno);
	}
	if (encry_method != NO_ENCRYPT && key != NULL) {
		tcfs_data->encryptor = create_encryptor(encry_method,
							(const uint8_t *)key,
							strlen(key));
		tcfs_data->tcfs_recv = decrypt_get_replay;
		tcfs_data->tcfs_write = encrypt_send_msg;
	} else {
		tcfs_data->tcfs_recv = get_reply;
		tcfs_data->tcfs_write = send_msg;
	}
	return tcfs_data;
}

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
	enum encrypt_method encry_method = NO_ENCRYPT;
	const char *key = NULL;

	while ((opt = getopt_long(argc, argv,
			"hds:p:m:k:", long_opts, &index)) != -1) {
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
		case 'm':
			if (!strcmp("rc4", optarg))
				encry_method = RC4_METHOD;
			break;
		case 'k':
			key = optarg;
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
	tcfs_data = create_tcfs(server, port, encry_method, key);
	fuse_argc = 0;
	fuse_argv[fuse_argc++] = "tcfs";
	fuse_argv[fuse_argc++] = "-s"; /* single thread */
	if (is_debug)
		fuse_argv[fuse_argc++] = "-d"; /* debug and core dump */
	fuse_argv[fuse_argc++] = mount_point;
	fuse_stat = fuse_main(fuse_argc, fuse_argv, &tcfs_oper, tcfs_data);
	return fuse_stat;
}
