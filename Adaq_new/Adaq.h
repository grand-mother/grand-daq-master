/***
DAQ Main project definitions
Version:1.0
Date: 17/2/2020
Author: Charles Timmermans, Nikhef/Radboud University

Altering the code without explicit consent of the author is forbidden
 ***/
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ad_shm.h"

#define ADAQ_VERSION 4
#define ERROR -1
#define NORMAL 1
#define DEFAULT_CONFIGFILE "conf/Adaq.conf"

typedef struct{
  int DUid; // station number
  char DUip[20]; // IP address
  int DUport;   // port to connect to
  int DUsock;
  time_t LSTconnect;
  struct sockaddr_in  DUaddress;
  socklen_t DUalength;
}DUInfo;


#define T3STATIONSIZE 4 //!< size of T3 request message

typedef struct{
  uint32_t DU_id;     //!< identifier of Detector Unit
  uint32_t index;     //!< index in DDR memory of the event
  uint32_t second;
  uint32_t nsec;
}T3STATION;

typedef struct{
  uint32_t event_nr;       //!< T3 event number
  T3STATION  t3station[];  //!< Information required for LS
}T3BODY;

#define MAXDU 30

#define NT2BUF (10*MAXDU) //10 per DU
#define T2SIZE 1000 //Max. size (in ints) for T2 info in 1 message

#define NT3BUF 500 // max 500 T3 buffers (small messages anyway)
#define T3SIZE (5+2*MAXDU) //Max. size (in ints) for T3 info in 1 message

#define CMDBUF 40 // leave 40 command buffers
#define CMDSIZE 5000 //Max. size (in ints) for command (should be able to hold config file)
#define CMDEBBUF 10 // leave 10 command buffers for EB
#define CMDEBSIZE 50 //Max. size (in ints) for command
#define LOG_FOLDER "/tmp/daq"

#define TCOINC 2000  // Maximum coincidence time window
#define NTRIG 2 //total at least 2 stations


#ifdef _MAINDAQ
DUInfo DUinfo[MAXDU];
int tot_du;
shm_struct shm_t2;
shm_struct shm_t3;
shm_struct shm_cmd;
shm_struct shm_ebcmd;
//next EB parameters
int eb_run = 1;
int eb_run_mode = 0;
int eb_max_evts = 10;
char eb_dir[80];
char eb_site[80];
char eb_extra[80];
//T3 parameters
int t3_rand = 0; 
int t3_stat = NTRIG;
int t3_time = TCOINC;
#else
extern DUInfo DUinfo[MAXDU];
extern int tot_du;
extern shm_struct shm_t2;
extern shm_struct shm_t3;
extern shm_struct shm_cmd;
extern shm_struct shm_ebcmd;
extern int eb_run ;
extern int eb_run_mode;
extern int eb_max_evts ;
extern char eb_dir[80]; 
extern char eb_site[80];
extern char eb_extra[80];
extern int t3_rand;
extern int t3_stat;
extern int t3_time;
#endif
