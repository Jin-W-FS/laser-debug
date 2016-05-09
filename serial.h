#ifndef _SERIAL_DEFAULT_INIT_H
#define _SERIAL_DEFAULT_INIT_H

#include <termios.h>

#define FLOW_NONE       0
#define FLOW_SOFT       1
#define FLOW_HARD       2

int serial_set_speed(int fd, speed_t speed);
int serial_set_flow(int fd, int flow);

speed_t serial_convert_speed(long speed);

int serial_init(const char *device, speed_t speed, int flow);
void serial_exit(int fd);

#endif
