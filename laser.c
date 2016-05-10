#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>

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

static ssize_t read_timed(int fd, void* buf, size_t count, int ms)
{
	if (ms >= 0) {
		struct timeval tv = { ms / 1000, ms % 1000 * 1000 };
		
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		
		int ret = select(fd+1, &rfds, NULL, NULL, &tv);
		if (ret <= 0) return ret;
	}
	return read(fd, buf, count);
}

static ssize_t laser_read(int fd, void* buf, size_t count)
{
	ssize_t len = 0;
	int timeout = -1;
	while (1) {
		ssize_t ret = read_timed(fd, buf + len, count, timeout);
		if (ret < 0) return ret;
		if (ret == 0) break;
		len += ret;
		count -= ret;
		if (count < 1) count = 1;
		timeout = 10;
	}
	return len;
}

static int laser_transport(int fd, const laser_msg_t* msg, laser_msg_t* ret)
{
	DBGMSG("W", msg);
	write(fd, msg->data, msg->len);
	if (!ret) return 0;
	// usleep(1000*1000); // wait for at least 12 bytes data under B9600
	ret->len = laser_read(fd, ret->data, sizeof(ret->data));
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

static int laser_config(Laser* laser, u8 type, u8 value)
{
	laser_msg_t msg = { 5, { 0xfa, 0x04, type, value } };
	laser_msg_t ret = { 0 };
	msg.data[msg.len-1] = checksum(msg.data, msg.len-1);
	if (laser_transport(laser->fd, &msg, &ret) < 0) {
		return -1;
	}
	if (!(ret.len == 4 || ret.len == 5) ||
	    checksum(ret.data, ret.len) != 0) {
		return -2;
	}
	if (ret.len == 4 && ret.data[1] == 0x04) { // succeed
		return 0;
	} else { // failed
		return -3;
	}

}

#ifdef DEBUG
static int _config_check_amoungst(u8 val, const u8 arr[], int len) {
	int i;
	for (i = 0; i < len; i++) {
		if (val == arr[i]) return 0;
	}
	return -1;
}
#define config_check_amoungst(val, arr)				\
	if (_config_check_amoungst(val, arr, sizeof(arr) / sizeof(arr[0])) < 0) return -1
#else
#define config_check_amoungst(...)
#endif

int laser_config_addr(Laser* laser, unsigned char addr)
{
	if ((addr & 0xf0) == 0xf0) return -1;	// reserved addr
	int ret = laser_config(laser, 0x01, addr);
	if (ret == 0) laser->addr = addr;
	return ret;
}

int laser_config_base_pos(Laser* laser, int value)
{
	const u8 valid[] = { LASER_BASE_TAIL, LASER_BASE_HEAD };
	config_check_amoungst(value, valid);
	return laser_config(laser, 0x08, value);
}

int laser_config_range(Laser* laser, int value)
{
	const u8 valid[] = { 5, 10, 30, 50, 80 };
	config_check_amoungst(value, valid);
	return laser_config(laser, 0x09, value);
}

int laser_config_freq(Laser* laser, int value)
{
	const u8 valid[] = { 5, 10, 20 };
	config_check_amoungst(value, valid);
	return laser_config(laser, 0x0a, value);
}

int laser_config_resolution(Laser* laser, int value)
{
	const u8 valid[] = { LASER_RESOL_1MM, LASER_RESOL_0_1MM };
	config_check_amoungst(value, valid);
	return laser_config(laser, 0x0c, value);
}


static int laser_sleep_wake(Laser* laser, u8 wake) 
{	
	laser_msg_t msg = { 5, { laser->addr, 0x06, 0x05, wake } };
	laser_msg_t ret = { 0 };
	msg.data[msg.len-1] = checksum(msg.data, msg.len-1);
	if (laser_transport(laser->fd, &msg, &ret) < 0) {
		return -1;
	}
	if (ret.len != 5 || checksum(ret.data, ret.len) != 0) {
		return -2;
	}
	if (ret.data[3] == 0x01) { // succeed
		return 0;
	} else { // failed
		return -3;
	}
}

int laser_sleep(Laser* laser)
{
	return laser_sleep_wake(laser, 0);
}

int laser_wakeup(Laser* laser)
{
	return laser_sleep_wake(laser, 1);
}


float laser_mesure_once(Laser* laser)
{
	// if (!laser->awaken) laser_wakeup(laser); // tested: no need
	laser_msg_t msg = { 4, { laser->addr, 0x06, 0x02 } };
	laser_msg_t ret = { 0 };
	msg.data[msg.len-1] = checksum(msg.data, msg.len-1);
	if (laser_transport(laser->fd, &msg, &ret) < 0) {
		return -1;
	}
	if (ret.len < 11 || checksum(ret.data, ret.len) != 0) {
		return -2;
	}
	ret.data[ret.len-1] = '\0';
	if (ret.data[3] != 'E') {
		return atof(ret.data + 3);
	} else { // "ERR--xx" / "ERR---xx"
		return -10 * atof(ret.data + ret.len - 2);
	}
}

