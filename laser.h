#ifndef LASER_DISTANCE_MEASURE
#define LASER_DISTANCE_MEASURE

struct laser;
typedef struct laser Laser;

//// open and close
// device => FILE like object or NULL
Laser* laser_open(const char* device);
int laser_close(Laser* laser);

//// configuration
// get address, light, and temperature
int laser_get_params(Laser* laser, unsigned char* addr, unsigned char* light, unsigned char* temperature);

// setup start position
enum LASER_BASE_POS { LASER_BASE_TAIL = 0, LASER_BASE_HEAD = 1 };
int laser_config_base_pos(Laser* laser, int from);

// supports 5, 10, 39, 50, 80 m
int laser_config_range(Laser* laser, int range);

// supports 5, 10, 20 Hz
int laser_config_freq(Laser* laser, int freq);

// supports 1(1mm), 2(0.1mm)
enum LASER_RESOL { LASER_RESOL_1MM = 1, LASER_RESOL_0_1MM = 2 };
int laser_config_resolution(Laser* laser, int res);

// setup address (uint8_t, can't be 0xFA/0xF*)
int laser_config_addr(Laser* laser, unsigned char addr);

//// sleep and wakeup
int laser_sleep(Laser* laser);
int laser_wakeup(Laser* laser);


//// mesure
// return distance or fail (-1)
float laser_mesure(Laser* laser, int res, int continuous);

#endif
