#include <stdio.h>
#include <stdlib.h>

#define MONBUF 1
#define N_MON 7
#define MON_PRESSURE 0
#define MON_HUMIDITY 1
#define MON_AccX 2
#define MON_AccY 3
#define MON_AccZ 4
#define MON_TEMP 5
#define MON_BATTERY 6

FILE *fpmon[N_MON];

void monitor_open();
void monitor_read();
