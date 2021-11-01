// io.c - adapted for basrcpd project 2018 - 2021 by Rainer Müller

/***************************************************************************
                          io.c  -  description
                             -------------------
    begin                : Wed Jul 4 2001
    copyright            : (C) 2001 by Dipl.-Ing. Frank Schmischke
    email                : frank.schmischke@t-online.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*
  changes:

    25.11.2007 Frank Schmischke
    in isvalidchar() change 'char' to 'unsigned char' to avoid compiler
			warning
*/

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <sys/ioctl.h>

#include "config-srcpd.h"
#include "io.h"
#include "syslogmessage.h"


int fdfb = -1;	// file descriptor for serial feedback port

int read_comport(bus_t bus, ssize_t maxbytes, unsigned char *bytes)
{
    ssize_t i = 0;

	if (fdfb <= 0) return -1;
    if (ioctl(fdfb, FIONREAD, &i) == -1) {
        syslog_bus(bus, DBG_ERROR, "read_comport ioctl() failed: %s (errno = %d)\n",
                       strerror(errno), errno);
        return -1;
    }
    /* read only if there is really an input to avoid a blocking read */
    if (i > 0) {
    	if (i > maxbytes) i = maxbytes;
        i = read(fdfb, bytes, i);
        if (i == -1) {
            syslog_bus(bus, DBG_ERROR, "read_comport read() failed: %s (errno = %d)\n",
                           strerror(errno), errno);
        }
    }
    if (i > 0) syslog_bus(bus, DBG_DEBUG, "read_comport read %d bytes", i);
    return i;
}

#if 0
// TODO: check if UART code could be reused, eg for Railcom
void writeByte(bus_t bus, const unsigned char b, unsigned long msecs)
{
    ssize_t i = 0;
    char byte = b;

    if (buses[bus].debuglevel <= DBG_DEBUG) {
        i = write(buses[bus].device.file.fd, &byte, 1);
        syslog_bus(bus, DBG_DEBUG, "(FD: %d) %i byte sent: 0x%02x (%d)\n",
                   buses[bus].device.file.fd, i, b, b);
        if (i < 0) {
            syslog_bus(bus, DBG_ERROR, "(FD: %d) write failed: %s "
                       "(errno = %d)\n",
                       buses[bus].device.file.fd, strerror(errno), errno);
        }
        tcdrain(buses[bus].device.file.fd);
    }
    else {
        syslog_bus(bus, DBG_DEBUG, "(FD: %d) %i byte sent: 0x%02x (%d)\n",
                   buses[bus].device.file.fd, i, b, b);
    }
    if (usleep(msecs * 1000) == -1) {
        syslog_bus(bus, DBG_ERROR,
                   "usleep() failed: %s (errno = %d)\n",
                   strerror(errno), errno);
    }
}

void writeString(bus_t bus, const char *s, unsigned long msecs)
{
    size_t l = strlen(s);
    size_t i;

    for (i = 0; i < l; i++) {
        writeByte(bus, s[i], msecs);
    }
}
#endif

void open_comport (bus_t bus, const char *device)
{
	if ((fdfb = open(device, O_RDWR|O_NOCTTY)) == -1)
        syslog_bus(bus, DBG_ERROR, "Open feedback port %s failed: %s (errno = %d).",
			device, strerror(errno), errno);
    else
    	syslog_bus(bus, DBG_INFO, "DDL will use %s for feedback.", device);
}

int rxstartwait_comport(struct termios *config)
{
	if (fdfb <= 0) return -1;
	tcflush(fdfb, TCIFLUSH);
	config->c_cflag |= (CLOCAL | CREAD);
	config->c_cflag &= ~CRTSCTS;
	return tcsetattr(fdfb, TCSAFLUSH, config);
}

#if 0
void restore_comport(bus_t bus)
{
    int fd;

    syslog_bus(bus, DBG_INFO, "Restoring attributes for serial line %s",
               buses[bus].device.file.path);
    fd = open(buses[bus].device.file.path, O_RDWR);
    if (fd == -1) {
        syslog_bus(bus, DBG_ERROR,
                   "Open serial line failed: %s (errno = %d).\n",
                   strerror(errno), errno);
    }
    else {
        syslog_bus(bus, DBG_INFO, "Restoring old values...");
        tcsetattr(fd, TCSANOW, &buses[bus].device.file.devicesettings);
        close(fd);
        syslog_bus(bus, DBG_INFO, "Old values successfully restored");
    }
}
#endif

void close_comport(bus_t bus)
{
//    struct termios interface;
	if (fdfb > 0) {
    	syslog_bus(bus, DBG_INFO, "Closing serial feedback port.");

//    tcgetattr(buses[bus].device.file.fd, &interface);
//    cfsetispeed(&interface, B0);
//    cfsetospeed(&interface, B0);
//    tcsetattr(buses[bus].device.file.fd, TCSANOW, &interface);
    	close(fdfb);
    }
}

// ssplitstr splits a string str into n tokens by assigning n pointers
// if the nr of tokens is bigger than the nr of pointers, last points to remainder
// original string is modified with term zeros, retval is nr of assigned pointers
int ssplitstr(char * str, int n, ...)
{
	int s = 1, i = 0;
	unsigned char c;

    va_list vl;
    va_start(vl,n);
	while ((c = *str)) {
		if (s && (c > 0x20)) {
			*(va_arg(vl, char **)) = str;	// fill pointer
			if(++i == n) break; 			// no more pointer available
		}
		if ((s = (c <= 0x20))) *str = 0;	// end of token terminated with 0
		str++;
	}
	while (n-- > i) 						// no more token available
		*(va_arg(vl, char **)) = str; 		// point to terminating zero
	va_end(vl);
	return i;
}

/* Zeilenweises Lesen vom Socket      */
/* nicht eben trivial!                */
static int isvalidchar(unsigned char c)
{
    return ((c >= 0x20 && c <= 127) || c == 0x09 || c == '\n');
}

/*
 * Read a text line from socket descriptor including newline character
 * (like fgets()).
 * return values
 *   -1: error
 *    0: end of file (EOF), client terminated connection
 *   >0: number of read characters
 * */
ssize_t socket_readline(int Socket, char *line, int len)
{
    char c;
    int i = 0;
    ssize_t bytes_read;

  again:
    bytes_read = read(Socket, &c, 1);
    if (bytes_read == -1) {

        /* handle interrupt */
        if (errno == EINTR)
            goto again;

        /* normal read error */
        else
            return -1;
    }

    /*EOF detected, client closed connection */
    else if (bytes_read == 0) {
        return 0;
    }

    /*normal operation */
    else {
        if (isvalidchar(c))
            line[i++] = c;
        /* die Reihenfolge beachten! */
        /*TODO: handle (errno == EINTR), message part will get lost */
        while (read(Socket, &c, 1) > 0) {
            if (isvalidchar(c) && (i < len - 1))
                line[i++] = c;
            /* stop at newline character */
            if (c == '\n')
                break;
        }
    }
    line[i++] = 0x00;
    return (i - 1);
}

/* Write "n" bytes to a descriptor. Stevens, UNP;
 * srcp messages must end with '\n' to use this function directly
 * return values:
 *   -1: write error
 *  >=0: number of written characters */
ssize_t writen(int fd, const void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            /* if EINTR call write() again */
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;
            /* a real error */
            else
                return (-1);
        }

        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}
