#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

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
