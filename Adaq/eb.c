/***
 Event Builder
 Version:2.0
 Date: 10/1/2024
 Author: Charles Timmermans, Nikhef/Radboud University
 
 Altering the code without explicit consent of the author is forbidden
 ***/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include "Adaq.h"
#include "amsg.h"
#include "eb.h"
#include "scope.h"

char fname[400];
extern char *configfile;
int ad_init_param(char *file);


#define NDU (500*MAXDU) // maximally 5 events for each DU in memory
#define EBTIMEOUT 5 // need to have at least 5 seconds of data before writing

int running = 0;

uint32_t DUbuffer[NDU][EVSIZE];
int i_DUbuffer = 0;

int eb_event = 0;

int isub;
static int write_sub[3]={0,0,0};


FILEHDR eb_fhdr;
FILE *fpout = NULL,*fpten=NULL,*fpmb=NULL;

/**
 void eb_open(EVHDR *evhdr)
 
 opens different event streams
 writes the file headers
 */
void eb_open(EVHDR *evhdr)
{
  struct tm *gtime;
  struct timeval tp;
  struct timezone tzp;
  
  tzp.tz_minuteswest = 0;
  tzp.tz_dsttime = 0;
  gettimeofday(&tp,&tzp);
  printf("Trying to open eventfiles: %s %s %s\n",eb_dir,eb_site,eb_extra);
  gtime = gmtime((const time_t *)&tp);
  sprintf(fname,"%s/%s_%4d%02d%02d_%02d%02d_%06d_CD_%s",eb_dir,eb_site,gtime->tm_year+1900,
          gtime->tm_mon+1,gtime->tm_mday,gtime->tm_hour,gtime->tm_min,
          eb_run,eb_extra);
  printf("!!%s!!\n",fname);
  fpout = fopen(fname,"r");
  while(fpout != NULL){
    eb_run +=1;
    sprintf(fname,"%s/%s_%4d%02d%02d_%02d%02d_%06d_CD_%s",eb_dir,eb_site,gtime->tm_year+1900,
            gtime->tm_mon+1,gtime->tm_mday,gtime->tm_hour,gtime->tm_min,
            eb_run,eb_extra);
    fpout = fopen(fname,"r");
  }
  printf("File = %s\n",fname);
  fpout = fopen(fname,"w");
  sprintf(fname,"%s/%s_%4d%02d%02d_%02d%02d_%06d_MD_%s",eb_dir,eb_site,gtime->tm_year+1900,
          gtime->tm_mon+1,gtime->tm_mday,gtime->tm_hour,gtime->tm_min,
          eb_run,eb_extra);
  fpten = fopen(fname,"w");
  sprintf(fname,"%s/%s_%4d%02d%02d_%02d%02d_%06d_UD_%s",eb_dir,eb_site,gtime->tm_year+1900,
          gtime->tm_mon+1,gtime->tm_mday,gtime->tm_hour,gtime->tm_min,
          eb_run,eb_extra);
  fpmb = fopen(fname,"w");
  //sprintf(fname,"%s/%s_%4d%02d%02d_%02d%02d_%06d_MN_%s",eb_dir,eb_site,gtime->tm_year+1900,
  //        gtime->tm_mon+1,gtime->tm_mday,gtime->tm_hour,gtime->tm_min,
  //        eb_run,eb_extra);
  // next write file header
  sprintf(fname,"%s_%4d%02d%02d_%02d%02d_%06d_XX_%s",eb_site,gtime->tm_year+1900,
          gtime->tm_mon+1,gtime->tm_mday,gtime->tm_hour,gtime->tm_min,
          eb_run,eb_extra);

  eb_fhdr.length = sizeof(FILEHDR)-sizeof(int32_t);
  eb_fhdr.run_id = eb_run;
  eb_fhdr.run_mode = eb_run_mode;
  eb_fhdr.file_id = 0;
  eb_fhdr.first_event_id = evhdr->event_id;
  eb_fhdr.first_event_time = evhdr->seconds;
  eb_fhdr.last_event_id = evhdr->event_id;
  eb_fhdr.last_event_time = evhdr->seconds;
  eb_fhdr.add1 = 0;
  eb_fhdr.add2 = 0;
  fwrite(&eb_fhdr,1,sizeof(FILEHDR),fpout);
  fwrite(&eb_fhdr,1,sizeof(FILEHDR),fpten);
  fwrite(&eb_fhdr,1,sizeof(FILEHDR),fpmb);
  printf("Open: %d %d %d %d\n",eb_fhdr.first_event_id,eb_fhdr.first_event_time,eb_fhdr.last_event_id,eb_fhdr.last_event_time);
}

/**
 void eb_close()
 
 Closes the different file streams and run a monitoring program on the files
 */
void eb_close()
{
  char cmd[400];
  // first update file header
  printf("Close: %d %d %d %d\n",eb_fhdr.first_event_id,eb_fhdr.first_event_time,eb_fhdr.last_event_id,eb_fhdr.last_event_time);
  if(fpout != NULL){
    rewind(fpout);
    fwrite(&eb_fhdr,1,sizeof(FILEHDR),fpout);
    fclose(fpout);
  }
  if(fpten != NULL){
    rewind(fpten);
    fwrite(&eb_fhdr,1,sizeof(FILEHDR),fpten);
    fclose(fpten);
  }
  if(fpmb != NULL){
    rewind(fpmb);
    fwrite(&eb_fhdr,1,sizeof(FILEHDR),fpmb);
    fclose(fpmb);
  }
  fpout = NULL;
  fpten = NULL;
  fpmb = NULL;
}

/**
 int eb_DUcompare(const void *a, const void *b)
 
 routine used for sorting
 a<b return 1
 a>b return -1
 --> reverse ordering
 */
int eb_DUcompare(const void *a, const void *b)
{ /* sorting in REVERSE order, easy removal of older data */
  uint32_t  *t1,*t2;
  uint32_t t1sec,t2sec;
  uint32_t t1nsec,t2nsec;
  t1 = (uint32_t *)(a);
  t2 = (uint32_t *)(b);
  /**if(t1[EVT_ID] < t2[EVT_ID]) {
    if((t1[EVT_ID]+1000) < t2[EVT_ID]) return(-1);
    else return(1);
  }
  if(t1[EVT_ID] > t2[EVT_ID]) {
    if(t1[EVT_ID] > (t2[EVT_ID]+1000)) return(1);
    else return(-1);
  }**/
  t1sec = *(uint32_t *)&t1[EVT_SECOND];
  t1nsec = *(uint32_t *)&t1[EVT_NANOSEC];
  t2sec = *(uint32_t *)&t2[EVT_SECOND];
  t2nsec = *(uint32_t *)&t2[EVT_NANOSEC];
  if(t1sec < t2sec) return(1);
  if(t1sec == t2sec){
    if(t1nsec < t2nsec) return(1);
    else if(t2nsec < t1nsec) return(-1);
    else return(0);
  }
  return(-1);
}

/**
 void eb_getui()
 
 get data from the user interface
 commands read are DU_STOP and DU_START; these will stop and start the EB as well
 */
void eb_getui()
{
  AMSG *msg;
  
  
  while((shm_ebcmd.Ubuf[(*shm_ebcmd.size)*(*shm_ebcmd.next_readb)]) !=  0){ // loop over the UI input
    if(((shm_ebcmd.Ubuf[(*shm_ebcmd.size)*(*shm_ebcmd.next_readb)]) &2) ==  2){ // loop over the UI input
      msg = (AMSG *)(&(shm_ebcmd.Ubuf[(*shm_ebcmd.size)*(*shm_ebcmd.next_readb)+1]));
      printf("EB: A GUI command\n");
      if(msg->tag == DU_STOP){
        running = 0;
        if(fpout != NULL) eb_close();
      }
      else if(msg->tag == DU_START){
        ad_init_param(configfile);
        printf("EB: Starting the run\n");
        running = 1;
        i_DUbuffer = 0; // get rid of old data
      }
      shm_ebcmd.Ubuf[(*shm_ebcmd.size)*(*shm_ebcmd.next_readb)] &= ~2;
    }
    *shm_ebcmd.next_readb = (*shm_ebcmd.next_readb) + 1;
    if( *shm_ebcmd.next_readb >= *shm_ebcmd.nbuf) *shm_ebcmd.next_readb = 0;
  }
}

/**
 void eb_gett3i()
 get data from the T3 Maker
 Information is not used. We could remove this aspect!
 */
void eb_gett3(){
  AMSG *msg;
  T3BODY *T3info;
  int n_t3_du;
  
  while((shm_t3.Ubuf[(*shm_t3.size)*(*shm_t3.next_readb)])  !=  0){ // loop over the input
    if(((shm_t3.Ubuf[(*shm_t3.size)*(*shm_t3.next_readb)]) &2) ==  2){ // loop over the input
      msg = (AMSG *)(&(shm_t3.Ubuf[(*shm_t3.size)*(*shm_t3.next_readb)+1]));
      T3info = (T3BODY *)(&(msg->body[0])); //set the T3 info pointer
      n_t3_du = (msg->length-3)/T3STATIONSIZE; // msg == length+tag+eventnr+T3stations
      //printf("EB: T3 event = %d; NDU = %d \n",T3info->event_nr,n_t3_du);
      shm_t3.Ubuf[(*shm_t3.size)*(*shm_t3.next_readb)] &= ~2;
    }
    *shm_t3.next_readb = (*shm_t3.next_readb) + 1;
    if( *shm_t3.next_readb >= *shm_t3.nbuf) *shm_t3.next_readb = 0;
  }
}

/**
 void eb_getdata()
 
 interpret the data from the detector units
 copy events into a local buffer (DUbuffer)
 sort the event fragments in memory (latest first)
 */
void eb_getdata(){
  AMSG *msg;
  uint32_t *DUinfo;
  int inew=0;
  
  while(((shm_eb.Ubuf[(*shm_eb.size)*(*shm_eb.next_read)]) &1) ==  1){ // loop over the inputb
    //printf("EB: Get Data %d %d\n",*shm_eb.next_read,shm_eb.Ubuf[(*shm_eb.size)*(*shm_eb.next_read)]);
    if(i_DUbuffer >= NDU) {
      printf("EB: Cannot accept more data\n");
      break;
    }
    inew = 1;
    msg = (AMSG *)(&(shm_eb.Ubuf[(*shm_eb.size)*(*shm_eb.next_read)+1]));
    //printf("EB getdata: loop over input. TAG = %d\n",msg->tag);
    if(msg->tag == DU_EVENT){
      DUinfo = (uint32_t *)msg->body;
      //printf("Trying to copy event of length %d (max is %d) %d\n",(DUinfo[EVT_LENGTH]>>16),EVSIZE,running);
      //printf("EB: ADC = %08x %08x %08x %08x\n",DUinfo[EVT_HDR_LENGTH], DUinfo[EVT_HDR_LENGTH+1]
      //       , DUinfo[EVT_HDR_LENGTH+2], DUinfo[EVT_HDR_LENGTH+3]);
      if((DUinfo[EVT_LENGTH]>>16) < EVSIZE){
        DUinfo[EVT_VERSION] = DUinfo[EVT_VERSION]+((ADAQ_VERSION&0xff)<<8);
        if(i_DUbuffer < NDU) memcpy((void *)&DUbuffer[i_DUbuffer],(void *)DUinfo,INTSIZE*(DUinfo[EVT_LENGTH]>>16));
        if(running ==1) i_DUbuffer +=1;
        if(i_DUbuffer >= NDU) {
          printf("EB: Filling the buffer, loosing data\n");
          i_DUbuffer = NDU-1;
        }
      }
    }
    shm_eb.Ubuf[(*shm_eb.size)*(*shm_eb.next_read)] = 0;
    *shm_eb.next_read = (*shm_eb.next_read) + 1;
    if( *shm_eb.next_read >= *shm_eb.nbuf) *shm_eb.next_read = 0;
  }
  if(i_DUbuffer >= NDU) {
    printf("EB: Filling the buffer, loosing data\n");
    i_DUbuffer = NDU-1;
  }
  if(i_DUbuffer>0 && inew == 1) {
    qsort(DUbuffer[0],i_DUbuffer,INTSIZE*EVSIZE,eb_DUcompare);
  }
}

/**
 void eb_write_events()
 
 write events to disk
 
 check if there is any buffer in memory
 create an event header
 check how many segments should be added to the event
 open the data files when needed
 write the event in the proper event stream
 make a new event header
 */
void eb_write_events(){
  FILE *fp;
  EVHDR evhdr;
  uint32_t *DUinfo,*DUn;
  uint32_t du_sec,du_nsec;
  int i,ils,il_start,DTime;
  static int n_written=0;
  
  if(i_DUbuffer == 0) return; //no buffers in memory
  //printf("EB: A buffer in memory\n");
  DUinfo = (uint32_t *)DUbuffer[i_DUbuffer-1];
  il_start = i_DUbuffer-1;
  DUn = (uint32_t *)DUbuffer[0];
  //printf("Write event %u %u\n",DUn[EVT_SECOND],DUinfo[EVT_SECOND]);
  if(DUn[EVT_SECOND]<DUinfo[EVT_SECOND]) {
    printf("EB: Second sorting went wrong %u %u\n",DUn[EVT_SECOND],DUinfo[EVT_SECOND]);
    i_DUbuffer = 0;
    return;
  }
  if((DUn[EVT_SECOND]==DUinfo[EVT_SECOND])  &&(i_DUbuffer > (0.8*NDU))) {
    printf("EB: filling 80 percent of the data buffer, aborting event writing\n");
    i_DUbuffer = 0;
    return;
  }
  if(((DUn[EVT_SECOND]-DUinfo[EVT_SECOND])<EBTIMEOUT) &&(i_DUbuffer < (0.8*NDU)) ) return;
  evhdr.t3_id = DUinfo[EVT_EVT_ID];
  evhdr.DU_count = 1;
  evhdr.length = 40+4*(DUinfo[EVT_LENGTH]>>16);
  evhdr.run_id = eb_run;
  evhdr.event_id = eb_event;
  evhdr.first_DU = DUinfo[EVT_STATION_ID];
  evhdr.seconds = DUinfo[EVT_SECOND];
  evhdr.nanosec = DUinfo[EVT_NANOSEC];
  evhdr.type = DUinfo[EVT_TRIGGER_STAT]>>16;
  if(DUinfo[EVT_TRIGGER_STAT]&0x100) evhdr.type |= TRIGGER_T3_MINBIAS;
  evhdr.version=DUinfo[EVT_VERSION];
  //printf("Event Type %d Version %d Time %u\n",evhdr.type,evhdr.version,evhdr.seconds);
  for(i=(i_DUbuffer-2);i>=0;i--){
    DUn = (uint32_t *)DUbuffer[i];
    du_sec = *(uint32_t *)&DUn[EVT_SECOND];
    du_nsec =*(uint32_t *)&DUn[EVT_NANOSEC];
    if(du_sec == evhdr.seconds){
      if(du_nsec >= evhdr.nanosec) DTime = du_nsec-evhdr.nanosec;
      else {
        DTime = evhdr.nanosec-du_nsec;
        printf("EB: sorting error (ns)\n");
      }
    }else if (du_sec > evhdr.seconds){
      DTime = du_sec-evhdr.seconds;
      if(DTime > 1) DTime = 1E9;
      else{
        if(evhdr.nanosec>du_nsec)
          DTime = 1E9-(evhdr.nanosec-du_nsec);
        else
          DTime = 1E9+(du_nsec-evhdr.nanosec);
      }
    } else{
      printf("EB: sorting error (s)\n");
      DTime = evhdr.seconds-du_sec;
      if(DTime > 1) DTime = 1E9;
      else{
        if(du_nsec>evhdr.nanosec)
          DTime = 1E9+(du_nsec-evhdr.nanosec);
        else
          DTime = 1E9-(evhdr.nanosec-du_nsec);
      }
    }
    if(DUn[EVT_EVT_ID] == evhdr.t3_id && DTime < 5E6){
      evhdr.DU_count ++;
      evhdr.length += 4*(DUn[EVT_LENGTH]>>16);
    }else{
      //printf("EB: Found event %d with %d DU Length = %d (%d, %d %d)\n",evhdr.t3_id,evhdr.DU_count,evhdr.length,i_DUbuffer,du_sec,evhdr.seconds);
      eb_fhdr.last_event_id = evhdr.event_id;
      eb_fhdr.last_event_time = evhdr.seconds;
      if(fpout == NULL) {
        eb_open(&evhdr);
        n_written = 0;
        for(isub=0;isub<3;isub++) write_sub[isub] = 0;
      }
      isub = 0;
      if((evhdr.type &TRIGGER_T3_MINBIAS) != 0){ // untriggered
        fp = fpten;
        isub = 1;
      }
      else{
        if((evhdr.type& TRIGGER_T3_RANDOM) != 0) {
          fp=fpmb; //single station triggered
          isub = 2;
        }
        else {
          if(evhdr.DU_count == 1) {
            fp = fpmb;
            isub = 2;
            evhdr.type |= TRIGGER_T3_RANDOM;
          } else
            fp = fpout;
        }
      }
      //printf("EB: Writing event %d\n",(int)fp);
      if(evhdr.DU_count == 0) return;
      fwrite(&evhdr,1,44,fp);
      for(ils=il_start;ils>(il_start-evhdr.DU_count);ils--){
        DUn = (uint32_t *)DUbuffer[ils];
        fwrite(DUn,1,4*(DUn[EVT_LENGTH]>>16),fp);
        *(uint32_t *)&DUn[EVT_SECOND] = 0;
      }// that is it, start a new event
      n_written++;
      write_sub[isub]++;
      if(n_written >=eb_max_evts) { // do we still need this?????
        eb_close();
        //printf("EB: after closing  files %d\n",(int) fpout);
        //printf("EB: after closing  files %d\n",(int) fpout);
      }
      eb_event++;
      DUinfo = (uint32_t *)DUbuffer[i];
      DUn = (uint32_t *)DUbuffer[i-1];
      evhdr.t3_id = DUinfo[EVT_EVT_ID];
      evhdr.DU_count = 1;
      evhdr.length = 40+4*(DUinfo[EVT_LENGTH]>>16);
      evhdr.run_id = eb_run;
      evhdr.event_id = eb_event;
      evhdr.first_DU = DUinfo[EVT_STATION_ID];
      evhdr.seconds = DUinfo[EVT_SECOND];
      evhdr.nanosec = DUinfo[EVT_NANOSEC];
      evhdr.type = DUinfo[EVT_TRIGGER_STAT]>>16;
      if(DUinfo[EVT_TRIGGER_STAT]&0x100) evhdr.type |= TRIGGER_T3_MINBIAS;
      evhdr.version=DUinfo[EVT_VERSION];
      il_start = i;
      i_DUbuffer = i+1;
      break;

    }
  }
}
/**
 void eb_main()
 
 main eventbuilder loop.
 Get data from the (graphical) user interface
 Get data from the T3Maker
 Get data from the DUs
 Write events to file
 */
void eb_main()
{
  char flname[100];
  FILE *fp_log;
  
  sprintf(flname,"%s/eb",LOG_FOLDER);
  fp_log = fopen(flname,"w");
  printf("Starting EB\n");
  while(1) {
    fseek(fp_log,0,SEEK_SET);
    eb_getui();
    eb_gett3();
    eb_getdata();
    //if(nloop == 1000) {
    //  printf("Running.... %d\n",running);
    //  nloop = 0;
    //}
    if(running == 1) eb_write_events();
    fprintf(fp_log,"%s\n",fname);
    fprintf(fp_log,"CD events: %5d\n",write_sub[0]);
    fprintf(fp_log,"UD events: %5d\n",write_sub[2]);
    fprintf(fp_log,"MD events: %5d\n",write_sub[1]);
    usleep(1000);
  }
  fclose(fp_log);
}
