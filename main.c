#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "laser.h"

int stop = 0;
void sighandler(int signo) {
  stop = 1;
}

#define CONFIRM(call) if (call < 0) { perror(#call); exit(3); }
int main(int argc, char* argv[])
{
	if (argc == 1) return 1;
	const char* dev = argv[1];
	Laser* laser = laser_open(dev);
	if (laser == NULL) {
		perror("laeser_open");
		return 2;
	}
	// CONFIRM(laser_config_addr(laser, 0x80));
	// unsigned char addr, light, tmpr;
	// CONFIRM(laser_get_params(laser, &addr, &light, &tmpr));
	// printf("Addr %x, Light %d, Tmpr: %d\n", addr, light, tmpr);
	CONFIRM(laser_config_range(laser, 5));
	CONFIRM(laser_config_resolution(laser, LASER_RESOL_0_1MM));
	signal(SIGINT, sighandler);
	int v = 0;
	while (!stop) {
	  v = !v;
	  // CONFIRM(laser_wakeup(laser));
	  CONFIRM(laser_config_base_pos(laser, v));
	  float dist = laser_mesure_once(laser);
	  printf("Distance: %f\n", dist);
	  // CONFIRM(laser_sleep(laser));
	}
	laser_close(laser);
	return 0;
}
