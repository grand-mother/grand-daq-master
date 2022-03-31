#include "ad_shm.h"
#include "du_monitor.h"

extern shm_struct shm_mon;

void monitor_open()
{
  fpmon[MON_PRESSURE] = fopen("/sys/bus/iio/devices/iio:device1/in_voltage0_raw","r");
  fpmon[MON_HUMIDITY] = fopen("/sys/bus/iio/devices/iio:device1/in_voltage1_raw","r");
  fpmon[MON_AccX] = fopen("/sys/bus/iio/devices/iio:device1/in_voltage2_raw","r");
  fpmon[MON_AccY] = fopen("/sys/bus/iio/devices/iio:device1/in_voltage3_raw","r");
  fpmon[MON_AccZ] = fopen("/sys/bus/iio/devices/iio:device1/in_voltage4_raw","r");
  fpmon[MON_TEMP] = fopen("/sys/bus/iio/devices/iio:device1/in_voltage5_raw","r");
  fpmon[MON_BATTERY] = fopen("/sys/bus/iio/devices/iio:device1/in_voltage6_raw","r");
  
}

void monitor_read()
{
  char bla[20000];
  int i,size;
  short *mon_val=shm_mon.Ubuf;
  
  for(i=0;i<N_MON;i++){
    if(fpmon[i] == NULL){
      sprintf(bla,"/sys/bus/iio/devices/iio:device1/in_voltage%d_raw",i);
      fpmon[i] = fopen(bla,"r");
      if(fpmon[i] == NULL) continue;
    }
    fseek(fpmon[i],0,SEEK_END);
    size = ftell(fpmon[i]);
    rewind(fpmon[i]);
    fread(bla,1,size,fpmon[i]);
    bla[size]=0;
    sscanf(bla,"%d",&mon_val[i]);
    //printf("%d\t", mon_val[i]);
  }
  //printf("\n");
  sleep(1);
}
