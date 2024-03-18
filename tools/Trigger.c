// Traces.c
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "Traces.h"
#define INTSIZE 4
#define SHORTSIZE 2

int *filehdr=NULL;
unsigned int *event=NULL;

int grand_read_file_header(FILE *fp)
{
  int i,return_code;
  int isize;
  
  if( !fread(&isize,INTSIZE,1,fp)) {
    printf("Cannot read the header length\n");
    return(0);                                                       //cannot read the header length
  }
  printf("The header length is %d bytes \n",isize);
  if(isize < FILE_HDR_ADDITIONAL){
    printf("The file header is too short, only %d integers\n",isize);
    return(0);                                                       //file header too short
  }
  if(filehdr != NULL) free((void *)filehdr);                         //in case we run several files
  filehdr = (int *)malloc(isize+INTSIZE);                            //allocate memory for the file header
  if(filehdr == NULL){
    printf("Cannot allocate enough memory to save the file header!\n");
    return(0);                                                       //cannot allocate memory for file header
  }
  filehdr[0] = isize;                                                //put the size into the header
  if((return_code = fread(&(filehdr[1]),1,isize,fp)) !=(isize)) {
    printf("Cannot read the full header (%d)\n",return_code);
    return(0);                                                       //cannot read the full header
  }
  return(1);
}


int grand_read_event(FILE *fp)
{
  int isize,return_code;
  
  if( !fread(&isize,INTSIZE,1,fp)) {
    printf("Cannot read the Event length\n");
    return(0);                                                       //cannot read the header length
  }
  //printf("The event length is %d bytes \n",isize);
  if(event != NULL) {
    if(event[0] != isize) {
      free((void *)event);                                           //free and alloc only if needed
      event = (unsigned int *)malloc(isize+INTSIZE);                          //allocate memory for the event
    }
  }
  else{
    event = (unsigned int *)malloc(isize+INTSIZE);                          //allocate memory for the event
  }
  if(event == NULL){
    printf("Cannot allocate enough memory to save the event!\n");
    return(0);                                                       //cannot allocate memory for event
  }
  event[0] = isize;
  if((return_code = fread(&(event[1]),1,isize,fp)) !=(isize)) {
    printf("Cannot read the full event (%d)\n",return_code);
    return(0);                                                       //cannot read the full event
  }
  return(1);
}

int check_trigger(int ich,uint32_t* du,int iprint) //ich from 0 to 2
{
  int Firmware = (du[EVT_VERSION]>>16)&0xff;
  Firmware = 3;
  //step 0: do we read out this channel
  if(((du[EVT_INP_SELECT]>>5*ich)&0x1e) == 0) return(-1);
  //Step 1: Get Trigger parameters for this channel
  int Thres1 =((du[EVT_THRES_C1+ich]>>12)&0xfff)+2; //account for bit rounding
  int Thres2 =(du[EVT_THRES_C1+ich]&0xfff)+2; //account for bit rounding
  int QuietPre = 2*((du[EVT_TRIG_C1+2*ich]>>21)&0x1ff); //samples
  int QuietPost = 2*((du[EVT_TRIG_C1+2*ich]>>12)&0x1ff); //samples
  int T2Time = 2*((du[EVT_TRIG_C1+2*ich]>>9)&0x7); //samples
  int T2Min =(du[EVT_TRIG_C1+2*ich]>>5)&0xf;
  if(Firmware == 2) T2Min = 0;
  int T2Max =du[EVT_TRIG_C1+2*ich]&0x1f;
  int BLMaxValue = 0;
  int BLSamples=0;
  //printf("Trace %d T1 %d T2 %d\n",ich,Thres1,Thres2);
  if(ich == 0) {
    BLMaxValue = du[EVT_BASELINE_12]&0x3ff;
    BLSamples = 1<<((du[EVT_BASELINE_12]>>10)&0x7);
  }
  if(ich == 1) {
    BLMaxValue = (du[EVT_BASELINE_12]>>13)&0x3ff;
    BLSamples = 1<<((du[EVT_BASELINE_12]>>23)&0x7);
  }
  if(ich == 2) {
    BLMaxValue = du[EVT_BASELINE_3]&0x3ff;
    BLSamples = 1<<((du[EVT_BASELINE_3]>>10)&0x7);
  }
  //step 2: Determine offset to ADC values to use
  int offset = EVT_START_ADC; //ch 0
  if(ich>0){
    if(((du[EVT_INP_SELECT])&0x1e) != 0) offset+=(du[EVT_TRACELENGTH]>>16); //ch 1
    if(ich>1){
      if(((du[EVT_INP_SELECT]>>5)&0x1e) != 0) offset+=(du[EVT_TRACELENGTH]>>16); //ch 2
    }
  }
  short *trace = (short *)malloc(2*2*(du[EVT_TRACELENGTH]>>16));
  short *wl = NULL;
  int nwl = 1;
  if(Firmware == 2)nwl = 2;
  wl = (short *)malloc(2*nwl*2*(du[EVT_TRACELENGTH]>>16)); //wavelet
  short *bl = (short *)malloc(2*BLSamples); //baseline
  int BLSum = 0;
  int BLptr = 0;
  int BLavg;
  memset(bl,0,2*BLSamples);
  for(int i=0;i<(du[EVT_TRACELENGTH]>>16) &&(offset+i)<(du[EVT_LENGTH]>>16);i++){
    trace[2*i] =du[offset+i]&0xffff;
    //printf("%d BLSum %d trace %d\n",2*i,BLSum, trace[2*i]);
    BLavg =(BLSum/BLSamples);
    if(trace[2*i]<=BLMaxValue&&trace[2*i]>=-BLMaxValue){
      BLSum -=bl[BLptr];
      bl[BLptr] =trace[2*i];
      BLSum+=trace[2*i];
      BLptr++;
      if(BLptr>=BLSamples) BLptr = 0;
    }
    trace[2*i] -= BLavg;
    BLavg =(BLSum/BLSamples);
    trace[2*i+1] = du[offset+i]>>16;
    //printf("%d BLSum %d trace %d\n",2*i+1,BLSum, trace[2*i+1]);
    if(trace[2*i+1]<=BLMaxValue&&trace[2*i+1]>=-BLMaxValue){
      BLSum -=bl[BLptr];
      bl[BLptr] =trace[2*i+1];
      BLSum+=trace[2*i+1];
      BLptr++;
      if(BLptr>=BLSamples) BLptr = 0;
    }
    trace[2*i+1] -= BLavg;
  }
  int TRlength =2*(du[EVT_TRACELENGTH]>>16);
  if(Firmware == 2){
    for(int i=0;i<TRlength;i+=2){
      wl[i] = (trace[i]+trace[i+1])>>1;
      wl[i+1] = -((trace[i]+trace[i+1])>>1); //wrong, should be A-B
    }
    for(int i=1;i<(TRlength-1);i+=2){
      wl[TRlength+i] = (trace[i]+trace[i+1])>>1;
      wl[TRlength+i+1] = -((trace[i]+trace[i+1])>>1); //wrong, should be A-B
    }
  }else{
    wl[0] = 0;
    for(int i=1;i<TRlength;i+=1){
      wl[i] = (trace[i-1]-trace[i])>>1;
      if(wl[i]<0) wl[i] = -wl[i];
    }
  }
  if(iprint){
    for(int i=0;i<TRlength;i++){
      if(Firmware == 2) printf("%d\t%d\t%d\t%d\n",i,trace[i],wl[i],wl[TRlength+i]);
      else printf("%d\t%d\t%d\n",i,trace[i],wl[i]);
    }
  }
  int Triggered[2] = {0,0};
  int TriggerPosition[2]={0,0};
  int ExpectedPosition =(du[EVT_TRIGGER_POS]>>16);
  int ExpectedT1 =ExpectedPosition-QuietPost;
  for(int iwl = 0;iwl<nwl;iwl++){
    int T1position = -1;
    int T1passed = 0;
    int Plength=0;
    int NT2=0;
    int T2prev = -1;
    for(int i=0;i<TRlength && Triggered[iwl] != 1;i++){
      if(i==ExpectedT1-10)Thres1 -=3; //account for bit rounding
      if(i==ExpectedT1+10)Thres1 +=3;//account for bit rounding
      if(T1passed){
        Plength++;
        int isT2 = 0;
        if(Firmware == 2){
          if(wl[i+iwl*TRlength]>Thres2 &&wl[i+iwl*TRlength]<Thres1) isT2 = 1;
        }else
          if(wl[i+iwl*TRlength]>Thres2) isT2 = 1;
        if(isT2 == 1){
          if(T2prev>0 && (i-T2prev)>T2Time) Triggered[iwl] = -1;
          T2prev = i;
          NT2++;
          if(NT2>T2Max || NT2<T2Min) Triggered[iwl] = -2;
          if(iprint) printf("T2 crossing %d %d (%d %d %d)\n",i,wl[i+iwl*TRlength],Thres2,T2Time,QuietPost);
        }
        if(Plength == QuietPost){
          T1passed = 0;
          if(Triggered[iwl] == 0) {
            Triggered[iwl] = 1;
            TriggerPosition[iwl] = i;
          }
        }
      }
      if(wl[i+iwl*TRlength]>=Thres1 && (T1position<0 ||(i-T1position)>QuietPre)){
        T1passed = 1;
        T1position = i;
        Plength = 0;
        NT2 = 0;
        T2prev = -1;
        Triggered[iwl] = 0;
      }
      if(wl[i+iwl*TRlength]>=Thres1) {
        if(iprint) printf("T1 crossing %d %d\n",T1position,i);
        T1position = i;
        NT2 = 0;
        T2prev = -1;
      }
    }
  }
  if(iprint) printf("\n Triggered:%d Trigger: (%d,%d) Position:(%d,%d) (\n",ich,
                    Triggered[0],Triggered[1],TriggerPosition[0],TriggerPosition[1]);
  free(trace);
  free(wl);
  free(bl);
  int iwl = 0;
  if(Firmware == 2){
    if((ExpectedPosition&1) &&Triggered[1] == 1 ) iwl = 1;
    if(Triggered[0] != 1) iwl = 1;
  }
  return(Triggered[iwl]*TriggerPosition[iwl]);
}

void analyze_du(uint32_t *du)
{
  int ipos;
  printf("Event %d DU %d Trigger %04x Position %d",du[EVT_EVT_ID],du[EVT_STATION_ID ],du[EVT_TRIGGER_STAT]&0xffff,du[EVT_TRIGGER_POS]>>16);
  for(int ich = 0;ich<3;ich++){
    if((du[EVT_TRIG_SELECT]&(1<<ich))!=0){ // channel needs to be analyzed
      ipos = check_trigger(ich,du,0);
      if(ipos > 0) printf("\t Channel %d %d --> %d",ich+1,ipos,(du[EVT_TRIGGER_POS]>>16)-ipos);
      else if((du[EVT_TRIGGER_STAT]&0xffff)&(1<<ich)) check_trigger(ich,du,1);
    }
  }
  printf("\n");
}

void analyze_grand_event(){
  uint32_t *evdu;
  int idu = EVENT_DU;                                                      //parameter indicating start of LS
  int ev_end = event[EVENT_HDR_LENGTH]/4;
  evdu = (unsigned int *)&event[EVENT_DU];
  while(idu<ev_end){
    evdu = (uint32_t *)(&event[idu]);
    analyze_du((uint32_t *)evdu);
    idu +=(evdu[EVT_LENGTH]>>16);
  }
}


int main(int argc, char **argv)
{
  FILE *fp;
  int i,ich,ib;
  char fname[100],hname[100];
  
  fp = fopen(argv[1],"r");
  if(fp == NULL) printf("Error opening  !!%s!!\n",argv[1]);
  
  if(grand_read_file_header(fp) ){ //lets read events
    while (grand_read_event(fp) >0 ) {
      analyze_grand_event();
    }
  }
  if (fp != NULL) fclose(fp); // close the file
}
