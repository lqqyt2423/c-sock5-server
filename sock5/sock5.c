#include "unp.h"
#include "helper.h"

//   +----+-----+-------+------+----------+----------+
//   |VER | REP |  RSV  | ATYP | BND.ADDR | BND.PORT |
//   +----+-----+-------+------+----------+----------+
//   | 1  |  1  | X'00' |  1   | Variable |    2     |
//   +----+-----+-------+------+----------+----------+

//   o  X'00' succeeded
//   o  X'01' general SOCKS server failure
//   o  X'02' connection not allowed by ruleset
//   o  X'03' Network unreachable
//   o  X'04' Host unreachable
//   o  X'05' Connection refused
//   o  X'06' TTL expired
//   o  X'07' Command not supported
//   o  X'08' Address type not supported
//   o  X'09' to X'FF' unassigned
void reply(int connfd, char *buf, char rep) {
	char head[] = { 0x05, rep, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	Write(connfd, head, sizeof(head));
}

void sock5(int connfd) {
	ssize_t n;
	char buf[MAXLINE];

	int i;

	//   +----+----------+----------+
	//   |VER | NMETHODS | METHODS  |
	//   +----+----------+----------+
	//   | 1  |    1     | 1 to 255 |
	//   +----+----------+----------+

	// as least 3 bytes
	if ( (n = Read(connfd, buf, MAXLINE)) < 3)
		return;

	if (buf[0] != 0x05) {
		err_msg("unsupported SOCKS version: %d", buf[0]);
		return;
	}

	int nmethods = buf[1];
	if (nmethods + 2 < n)
		return;

	// o  X'00' NO AUTHENTICATION REQUIRED
    // o  X'01' GSSAPI
    // o  X'02' USERNAME/PASSWORD
    // o  X'03' to X'7F' IANA ASSIGNED
    // o  X'80' to X'FE' RESERVED FOR PRIVATE METHODS
    // o  X'FF' NO ACCEPTABLE METHODS
	for (i = 2; i < nmethods + 2; i++) {
		if (buf[i] == 0x00) {
			buf[0] = 0x05;
			buf[1] = 0x00;
			Write(connfd, buf, 2);
			goto requests;
		}
	}
	err_msg("auth methods not support");
	return;

requests:
	//   +----+-----+-------+------+----------+----------+
    //   |VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
    //   +----+-----+-------+------+----------+----------+
    //   | 1  |  1  | X'00' |  1   | Variable |    2     |
    //   +----+-----+-------+------+----------+----------+

	// as least 10 bytes
	if ( (n = Read(connfd, buf, MAXLINE)) < 10)
		return;

	if (buf[0] != 0x05) {
		err_msg("unsupported SOCKS version: %d", buf[0]);
		return;
	}

	// o  CONNECT X'01'
    // o  BIND X'02'
    // o  UDP ASSOCIATE X'03'
	if (buf[1] != 0x01) {
		reply(connfd, buf, 0x07);
		return;
	}

	if (buf[2] != 0x00) {
		reply(connfd, buf, 0x07);
		return;
	}

	// show_bytes((byte_pointer)buf, n);

	int socket_family;
	int socket_type = SOCK_STREAM;
	struct sockaddr *remote_addr;
	socklen_t socket_len;
	int sockfd;

	struct sockaddr_in remote_ipv4_addr;
	struct sockaddr_in6 remote_ipv6_addr;

	// ipv4
	if (buf[3] == 0x01) {
		bzero(&remote_ipv4_addr, sizeof(remote_ipv4_addr));
		remote_ipv4_addr.sin_family = AF_INET;
		bcopy(&buf[4], &remote_ipv4_addr.sin_addr, 4);
		bcopy(&buf[8], &remote_ipv4_addr.sin_port, 2);

		remote_addr = (struct sockaddr *) &remote_ipv4_addr;
		socket_family = AF_INET;
		socket_len = sizeof(remote_ipv4_addr);
	}

	// ipv6
	else if (buf[3] == 0x04) {
		// 22 bytes
		if (n < 22)
			return;
		bzero(&remote_ipv6_addr, sizeof(remote_ipv6_addr));
		remote_ipv6_addr.sin6_family = AF_INET6;
		bcopy(&buf[4], &remote_ipv6_addr.sin6_addr, 16);
		bcopy(&buf[20], &remote_ipv6_addr.sin6_port, 2);

		remote_addr = (struct sockaddr *) &remote_ipv6_addr;
		socket_family = AF_INET6;
		socket_len = sizeof(remote_ipv6_addr);
	}

	// domain
	else if (buf[3] == 0x03) {
		int domain_len = buf[4];
		if (n < domain_len + 7)
			return;

		// port
		uint16_t port = ntohs(*((uint16_t*)&buf[domain_len+5]));
		snprintf(buf + domain_len + 6, MAXLINE - domain_len - 6 - 1, "%d", port);
		buf[domain_len+5] = 0;
		sockfd = Tcp_connect(buf + 5, buf + domain_len + 6);
		goto pipe;
	}

	else {
		err_msg("not support temporarily");
		return;
	}

	sockfd = Socket(socket_family, socket_type, 0);
	Connect(sockfd, remote_addr, socket_len);

pipe:
	reply(connfd, buf, 0x00);
	printf("connect success, begin pipe data\n");

	fd_set rset;
	int maxfdp1;
	int connfdeof, sockfdeof;
	connfdeof = 0;
	sockfdeof = 0;

	FD_ZERO(&rset);
	for ( ; ; ) {
		if (connfdeof == 0)
			FD_SET(connfd, &rset);
		if (sockfdeof == 0)
			FD_SET(sockfd, &rset);
		maxfdp1 = max(connfd, sockfd) + 1;
		Select(maxfdp1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(connfd, &rset)) {
			if ( (n = Read(connfd, buf, MAXLINE)) == 0) {
				connfdeof = 1;
				Shutdown(sockfd, SHUT_WR);
				FD_CLR(connfd, &rset);
			} else {
				Write(sockfd, buf, n);
			}
		}

		if (FD_ISSET(sockfd, &rset)) {
			if ( (n = Read(sockfd, buf, MAXLINE)) == 0) {
				sockfdeof = 1;
				Shutdown(connfd, SHUT_WR);
				FD_CLR(sockfd, &rset);
			} else {
				Write(connfd, buf, n);
			}
		}

		// finish
		if (connfdeof && sockfdeof)
			return;
	}
}
