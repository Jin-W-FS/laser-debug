#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "serial.h"
#include "laser.h"

typedef signed char s8;
typedef unsigned char u8;

struct laser 
{
	int fd; // serial port
	u8 addr; // device address
	u8 awaken; // weather is sleep or wakeup
};

static u8 checksum(u8 data[], int len)
{
	u8 s = 0, *p;
	for (p = data; p < data + len; ++p) {
		s += *p;
	}
	return -s;
}

typedef struct laser_msg_s {
	s8 len;
	u8 data[15];
} laser_msg_t;

void print_msg(const char* title, const laser_msg_t* msg)
{
	const u8* p;
	if (title && title[0]) printf("%s ", title);
	printf("[%d]:", (int)msg->len);
	for (p = msg->data; p < msg->data + msg->len; ++p) {
		printf(" %02x", (int)(*p));
	}
	printf("\n");
}

#ifdef DEBUG
#define DBGMSG(title, msg) print_msg(title, msg)
#else
#define DBGMSG(...)
#endif

int laser_transport(int fd, const laser_msg_t* msg, laser_msg_t* ret)
{
	// TODO: check return val of read/write to ensure all's read/wrote
	DBGMSG("W", msg);
	write(fd, msg->data, msg->len); // TODO: 
	if (!ret) return 0;
	// usleep(10*1000); // wait for at least 12 bytes data under B9600
	ret->len = read(fd, ret->data, sizeof(ret->data)); // TODO:
	DBGMSG("R", ret);
	if (ret->len < 0) return -1;
	return 0;
}


// interfaces
Laser* laser_open(const char* device)
{
	Laser* laser = (Laser*)malloc(sizeof(*laser));
	// open port
	laser->fd = serial_init(device, B9600, FLOW_NONE);
	if (laser->fd < 0) goto error;
	// setup params
	if (laser_get_params(laser, &(laser->addr), NULL, NULL) < 0) goto error;
	if (laser_sleep(laser) < 0) goto error;
	laser->awaken = 0;
	// return
	return laser;
error:
	if (laser->fd < 0) {
		serial_exit(laser->fd);
	} else { // error after open port
		errno = ECOMM;
	}
	free(laser);
	return NULL;
}

int laser_close(Laser* laser)
{
	if (laser) {
		if (laser->awaken) laser_sleep(laser);
		serial_exit(laser->fd);
		free(laser);
	}
}

int laser_get_params(Laser* laser, u8* addr, u8* light, u8* temperature) 
{
	laser_msg_t msg = { 4, { 0xfa, 0x06, 0x01, 0xff } };
	laser_msg_t ret = { 0 };
	if (laser_transport(laser->fd, &msg, &ret) < 0) {
		return -1;
	}
	if (ret.len != 7 || checksum(ret.data, ret.len) != 0) {
		return -2;
	}
	if (addr) *addr = ret.data[3];
	if (light) *light = ret.data[4];
	if (temperature) *temperature = ret.data[5];
	return 0;
}

int laser_sleep(Laser* laser) 
{

	return 0;
}

#ifdef TEST
int main(int argc, char* argv[])
{
	if (argc == 1) return 1;
	const char* dev = argv[1];
	Laser* laser = laser_open(dev);
	if (laser == NULL) {
		perror("laeser_open");
		return 2;
	}
	laser_close(laser);
	return 0;
}
#endif
