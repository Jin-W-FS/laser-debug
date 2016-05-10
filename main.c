#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "laser.h"

int stop = 0;
void sighandler(int signo) {
	stop = 1;
}

const char* input(const char* prompt)
{
	static char line[80];
	printf("%s", prompt);
	fflush(stdout);
	fgets(line, sizeof(line), stdin);
	return line;
}

#define CONFIRM(call) if (call < 0) { perror(#call); exit(3); }
int main(int argc, char* argv[])
{
	if (argc == 1) return 1;
	const char* s = NULL;
	const char* dev = argv[1];
	Laser* laser = laser_open(dev);
	if (laser == NULL) {
		perror("laeser_open");
		return 2;
	}
	
	// setup address
	unsigned char addr, light, tmpr;
	CONFIRM(laser_get_params(laser, &addr, &light, &tmpr));
	printf("Addr %x, Light %d, Tmpr: %d\n", addr, light, tmpr);
	s = input("Change address? ");
	int iaddr;
	if (sscanf(s, "%x", &iaddr) == 1) {
		addr = (unsigned char)iaddr;
		CONFIRM(laser_config_addr(laser, addr));
		printf("Address change to %x\n", addr);
	} else {
		printf("Address unchanged\n");
	}
	
	// calibrate
	CONFIRM(laser_config_range(laser, 5));
	CONFIRM(laser_config_resolution(laser, LASER_RESOL_0_1MM));
	
	signal(SIGINT, sighandler);

	s = input("Adjust [y/N]? ");
	if (s[0] == 'y' || s[0] == 'Y') {
		int delta = 0;
		CONFIRM(laser_adjust_distance(laser, delta));	// reset to 0
		while (!stop) {
			// CONFIRM(laser_wakeup(laser));
			float dist = laser_mesure_once(laser);
			// CONFIRM(laser_sleep(laser));
			printf("Distance: %f mm\n", dist * 1000);
			const char* s = input("Adjust +x mm, or [c]ancel: ");
			if (s[0] == 'C' || s[0] == 'c') break;
			int d = atoi(s);
			delta += d;
			if (d) CONFIRM(laser_adjust_distance(laser, delta));	// reset to 0
		}
	}
	
	// continious test
	while (!stop) {
		float dist = laser_mesure_once(laser);
		printf("Distance: %f m\n", dist);
	}
	
	laser_close(laser);
	return 0;
}
