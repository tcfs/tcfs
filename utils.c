#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "utils.h"

int client_connect(const char *addr, uint16_t port)
{
	int s;
	struct sockaddr_in sa;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return -1;

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	if (inet_aton(addr, &sa.sin_addr) == 0) {
		struct hostent *he;

		he = gethostbyname(addr);
		if (he == NULL) {
			close(s);
			return -1;
		}
		memcpy(&sa.sin_addr, he->h_addr, sizeof(struct in_addr));
	}
	if (connect(s, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		close(s);
		return -1;
	}
	return s;
}

/*
 * Read "n" bytes from a descriptor.
 * Use in place of read() when fd is a stream socket.
 */
int readn(int fd, char *ptr, int nbytes)
{
	int nleft, nread;

	nleft = nbytes;
	while (nleft > 0) {
		nread = read(fd, ptr, nleft);
		if (nread < 0)
			return(nread);		/* error, return < 0 */
		else if (nread == 0)
			break;			/* EOF */
		nleft -= nread;
		ptr   += nread;
	}
	return(nbytes - nleft);		/* return >= 0 */
}

/*
 * Write "n" bytes to a descriptor.
 * Use in place of write() when fd is a stream socket.
 */
int writen(int fd, char *ptr, int nbytes)
{
	int	nleft, nwritten;

	nleft = nbytes;
	while (nleft > 0) {
		nwritten = write(fd, ptr, nleft);
		if (nwritten <= 0)
			return(nwritten);		/* error */
		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(nbytes - nleft);
}

/* Write "n" bytes to a descriptor using gathering write. */
ssize_t writevn(int fd, struct iovec *vector, int count, size_t n)
{
	int i;
	size_t nleft;
	ssize_t nwritten;

	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = writev(fd, vector, count)) <= 0) {
			if (errno == EINTR)
				nwritten = 0;    /* and call writev() again */
			else
				return(-1);      /* error */
		}
		nleft -= nwritten;
		if (nwritten > 0 && nleft > 0) {
			i = 0;
			while (i<count && nwritten>0) {
				if (vector[i].iov_len<=nwritten) {
					nwritten -= vector[i].iov_len;
					vector[i].iov_len = 0;
				} else {
					vector[i].iov_len -= nwritten;
					vector[i].iov_base =
						(char *)vector[i].iov_base +
						nwritten;
					nwritten = 0;
				}
				i++;
			}
		}
	}
	return(n);
}

uint32_t buf_get_uint32(const char *buf)
{
	uint32_t ret;

	memcpy(&ret, buf, 4);
	ret = ntohl(ret);
	return ret;
}

void buf_add_uint32(char *buf, uint32_t val)
{
	uint32_t nval = htonl(val);
	memcpy(buf, &nval, 4);
}
