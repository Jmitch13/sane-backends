/* sane - Scanner Access Now Easy.
   Copyright (C) 2006 Tower Technologies
   Author: Alessandro Zummo <a.zummo@towertech.it>
   This file is part of the SANE package.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

   As a special exception, the authors of SANE give permission for
   additional uses of the libraries contained in this release of SANE.

   The exception is that, if you link a SANE library with other files
   to produce an executable, this does not by itself cause the
   resulting executable to be covered by the GNU General Public
   License.  Your use of that executable is in no way restricted on
   account of linking the SANE library code into it.

   This exception does not, however, invalidate any other reasons why
   the executable file might be covered by the GNU General Public
   License.

   If you submit changes to SANE to the maintainers to be included in
   a subsequent release, you agree by submitting the changes that
   those changes may be distributed with this exception intact.

   If you write modifications of your own for SANE, it is your choice
   whether to permit this exception to apply to your modifications.
   If you do not wish that, delete this exception notice.  */

#include "../include/sane/config.h"

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

#define BACKEND_NAME sanei_udp

#include "../include/sane/sane.h"
#include "../include/sane/sanei_debug.h"
#include "../include/sane/sanei_udp.h"

static SANE_Status
sanei_udp_socket(int *fdp, int broadcast)
{
	int fd;

	if ((fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		return SANE_STATUS_INVAL;

	if (broadcast) {
		int opt = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST,
			       (char *) &opt, sizeof(opt)) < 0) {
			close(fd);
			return SANE_STATUS_INVAL;
		}
	}

	*fdp = fd;

	return SANE_STATUS_GOOD;
}

static SANE_Status
sanei_udp_connect(int fd, const char *host, int port)
{
	int err;
	struct sockaddr_in saddr;
	struct hostent *h;

	h = gethostbyname(host);

	if (h == NULL || h->h_addr_list[0] == NULL
	    || h->h_addrtype != AF_INET)
		return SANE_STATUS_INVAL;

	memset(&saddr, 0x00, sizeof(struct sockaddr_in));

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	memcpy(&saddr.sin_addr, h->h_addr_list[0], h->h_length);

	if ((err = connect(fd, (struct sockaddr *) &saddr,
		     sizeof(struct sockaddr_in))) != 0) {
		return SANE_STATUS_INVAL;
	}

	return SANE_STATUS_GOOD;
}

SANE_Status
sanei_udp_open(const char *host, int port, int *fdp)
{
	int status;
#ifdef HAVE_WINSOCK2_H
	WSADATA wsaData;
#endif

	DBG_INIT();
	DBG(1, "%s\n", __func__);

#ifdef HAVE_WINSOCK2_H
	status = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (status != 0)
	    return SANE_STATUS_INVAL;
#endif

	status = sanei_udp_socket(fdp, 0);
	if (status != SANE_STATUS_GOOD)
		return status;

	status = sanei_udp_connect(*fdp, host, port);
	if (status != SANE_STATUS_GOOD) {
		close(*fdp);
		return status;
	}

	return status;
}

SANE_Status
sanei_udp_open_broadcast(int *fdp)
{
	int status;

	DBG_INIT();
	DBG(1, "%s\n", __func__);

	status = sanei_udp_socket(fdp, 1);
	if (status != SANE_STATUS_GOOD)
		return status;

	return status;
}

void
sanei_udp_close(int fd)
{
	close(fd);
#ifdef HAVE_WINSOCK2_H
	WSACleanup();
#endif
}

ssize_t
sanei_udp_write(int fd, const u_char * buf, int count)
{
	return send(fd, buf, count, 0);
}

ssize_t
sanei_udp_write_broadcast(int fd, int port, const u_char * buf, int count)
{
	struct sockaddr_in saddr;

	memset(&saddr, 0x00, sizeof(struct sockaddr_in));

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	return sendto(fd, buf, count, 0,
		(struct sockaddr *)&saddr, sizeof(saddr));
}

void
sanei_udp_set_nonblock(int fd, SANE_Bool nonblock)
{
#ifdef HAVE_WINSOCK2_H
	u_long mode=nonblock;

	ioctlsocket(fd, FIONBIO, &mode);
#else
	long flags;

	flags = fcntl(fd, F_GETFL, 0L);
	if (nonblock)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;
	fcntl(fd, F_SETFL, flags);
#endif
}

ssize_t
sanei_udp_read(int fd, u_char * buf, int count)
{
	return recv(fd, buf, count, 0);
}

ssize_t
sanei_udp_recvfrom(int fd, u_char * buf, int count, char **fromp)
{
	ssize_t l;
	socklen_t fl;
	struct sockaddr_in from;

	fl = sizeof(from);

	l = recvfrom(fd, buf, count, 0, (struct sockaddr *) &from, &fl);

	if (l > 0 && fromp) {
		*fromp = inet_ntoa(from.sin_addr);
	}

	return l;
}
