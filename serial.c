/******************************************************************
** File: serial.c
** Description: the serial part for microcom project
**
** Copyright (C)1999 Anca and Lucian Jurubita <ljurubita@hotmail.com>.
** All rights reserved.
****************************************************************************
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details at www.gnu.org
****************************************************************************
** Rev. 1.0 - Feb. 2000
** Rev. 1.01 - March 2000
** Rev. 1.02 - June 2000
****************************************************************************/

#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <assert.h>

#include "serial.h"
// #include "common_def.h"
#define LENGTH_OF(arr) (sizeof(arr) / sizeof(arr[0]))

/* static char lockfile[PATH_MAX+1] = "/var/lock/LCK.."; */
static struct termios pots; /* old port termios settings to restore */

static void _comm_init_config(struct termios *pts)
{
        /* some things we want to set arbitrarily */
        pts->c_lflag &= ~ICANON;
        pts->c_lflag &= ~(ECHO | ECHOCTL | ECHONL);
        pts->c_cflag |= HUPCL;
        pts->c_iflag |= IGNBRK;
        pts->c_cc[VMIN] = 1;
        pts->c_cc[VTIME] = 0;

        /* Standard CR/LF handling: this is a dumb terminal.
         * Do no translation:
         *  no NL -> CR/NL mapping on output, and
         *  no CR -> NL mapping on input.
         */
        pts->c_oflag &= ~ONLCR;
        pts->c_iflag &= ~ICRNL;
}

int serial_set_speed(int fd, speed_t speed)
{
	struct termios pts;	/* termios settings on port */

	tcgetattr(fd, &pts);
	cfsetospeed(&pts, speed);
	cfsetispeed(&pts, speed);
	tcsetattr(fd, TCSANOW, &pts);

	return 0;
}

int serial_set_flow(int fd, int flow)
{
	struct termios pts;	/* termios settings on port */
	tcgetattr(fd, &pts);

	switch (flow) {
	case FLOW_NONE:
		/* no flow control */
		pts.c_cflag &= ~CRTSCTS;
		pts.c_iflag &= ~(IXON | IXOFF | IXANY);
		break;
	case FLOW_HARD:
		/* hardware flow control */
		pts.c_cflag |= CRTSCTS;
		pts.c_iflag &= ~(IXON | IXOFF | IXANY);
		break;
	case FLOW_SOFT:
		/* software flow control */
		pts.c_cflag &= ~CRTSCTS;
		pts.c_iflag |= IXON | IXOFF | IXANY;
		break;
	}

	tcsetattr(fd, TCSANOW, &pts);

	return 0;
}

void serial_exit(int fd)
{
        tcsetattr(fd, TCSANOW, &pots);
        close(fd);
        /* unlink(lockfile); */
}

int serial_init(const char *device, speed_t speed, int flow)
{
	struct termios pts;	/* termios settings on port */
	int fd;
	/* char *substring; */
	/* long pid; */

	/* check lockfile */
	/* substring = strrchr(device, '/'); */
	/* if (substring) */
	/* 	substring++; */
	/* else */
	/* 	substring = device; */

	/* strncat(lockfile, substring, PATH_MAX - strlen(lockfile) - 1); */

	/* fd = open(lockfile, O_RDONLY); */
	/* if (fd >= 0) { */
	/* 	close(fd); */
	/* 	main_usage(3, "lockfile for port exists", device); */
	/* } */

	/* fd = open(lockfile, O_RDWR | O_CREAT, 0444); */
	/* if (fd < 0) */
	/* 	main_usage(3, "cannot create lockfile", device); */

	/* /\* Kermit wants binary pid *\/ */
	/* pid = getpid(); */
	/* write(fd, &pid, sizeof(long)); */
	/* close(fd); */

	/* open the device */
	fd = open(device, O_RDWR);

	if (fd < 0) {
		return fd;
	}

        tcgetattr(fd, &pts);
        memcpy(&pots, &pts, sizeof (pots));
	_comm_init_config(&pts);
	tcsetattr(fd, TCSANOW, &pts);

	serial_set_speed(fd, speed);
	serial_set_flow(fd, flow);
	
	return fd;
}

static long ispeed[] = {  0,  50,  75,  110,  134,  150,  200,  300,  600,  1200,  1800,  2400,  4800,  9600,  19200,  38400,  57600,  115200,  230400 };
static long sspeed[] = { B0, B50, B75, B110, B134, B150, B200, B300, B600, B1200, B1800, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400 };

speed_t serial_convert_speed(long speed)
{
	int i;
	for (i = 0; i < LENGTH_OF(ispeed); i++) {
		if (speed == ispeed[i]) return sspeed[i];
	}
	return B0;
}

