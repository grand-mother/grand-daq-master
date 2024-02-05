// Traces.c
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "Traces.h"
#define INTSIZE 4
#define SHORTSIZE 2
#include "fftw3.h"
//#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TClass.h"
#include "TSystem.h"
#include "TH1.h"
#include "TH2.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TProfile.h"
#include "TProfile2D.h"
#include "grand_util.h"

#define NDAY 10

int *filehdr=NULL;
unsigned int *event=NULL;

float *ttrace,*fmag,*fphase;
int fftlen = 0;
TH1F *hstep,*hfftstep;
TProfile *HFsum[100][3]; // One for each arm
TProfile *HBattery[100];
TProfile2D *HFTime[100][3];
int n_DU=0,DU_id[100];


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

void print_file_header()
{
  int i,additional_int;
  struct tm *mytime;
  
  additional_int = 1+(filehdr[FILE_HDR_LENGTH]/INTSIZE) - FILE_HDR_ADDITIONAL; //number of additional words in the header
  if(additional_int<0){
    printf("The header is too short!\n");
    return;
  }
  printf("Header Length is %d bytes\n",filehdr[FILE_HDR_LENGTH]);
  printf("Header Run Number is %d\n",filehdr[FILE_HDR_RUNNR]);
  printf("Header Run Mode is %d\n",filehdr[FILE_HDR_RUN_MODE]);
  printf("Header File Serial Number is %d\n",filehdr[FILE_HDR_SERIAL]);
  printf("Header First Event is %d\n",filehdr[FILE_HDR_FIRST_EVENT]);
  mytime = gmtime((const time_t *)(&filehdr[FILE_HDR_FIRST_EVENT_SEC]));
  printf("Header First Event Time is %s",asctime(mytime));
  printf("Header Last Event is %d\n",filehdr[FILE_HDR_LAST_EVENT]);
  mytime = gmtime((const time_t *)(&filehdr[FILE_HDR_LAST_EVENT_SEC]));
  printf("Header Last Event Time is %s",asctime(mytime));
  for(i=0;i<additional_int;i++){
    printf("HEADER Additional Word %d = %d\n",i,filehdr[i+FILE_HDR_ADDITIONAL]);
  }
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

void print_du(uint32_t *du)
{
  int i,idu;
  int ioff,offset;
  short value;
  short val1,val2;
  short bit14;
  int mask;
  char hname[100],fname[100];
  static int zeroday= -1;
  float fvalue;
#define DTFMIN 50
#define DTFMAX 110
#define NDTFREQ 480
  float dtfreq[NDTFREQ];
  
  float fh = (du[EVT_SECMINHOUR]&0xff)+((du[EVT_SECMINHOUR]>>8)&0xff)/60.;
  int iday = (du[EVT_DAYMONTH]>>24)&0xff;
  if(zeroday == -1) zeroday = iday;

  ioff = du[EVT_LENGTH]&0xffff;
  if(n_DU == 0){
    for(int ich=0;ich<3;ich++){
      snprintf(fname,100,"HSF%d",ich);
      snprintf(hname,100,"HSF%d",ich);
      HFsum[0][ich] = new TProfile(fname,hname,(du[EVT_TRACELENGTH]>>16),0.,250);
    }
    hstep =  new TH1F("Hstep","Hstep",2*(du[EVT_TRACELENGTH]>>16),0.,4*(du[EVT_TRACELENGTH]>>16));
    hfftstep =  new TH1F("Hfftstep","Hfftstep",(du[EVT_TRACELENGTH]>>16),0.,250);
    if(fftlen !=2*(du[EVT_TRACELENGTH]>>16)){
      fftlen = 2*(du[EVT_TRACELENGTH]>>16);
      fft_init(fftlen);
      ttrace = (float *)malloc(fftlen*sizeof(float));
      fmag = (float *)malloc(fftlen*sizeof(float));
      fphase = (float *)malloc(fftlen*sizeof(float));
    }
    for(int i=0;i<2*(du[EVT_TRACELENGTH]>>16);i++){
      if(i<(du[EVT_TRACELENGTH]>>16)) ttrace[i] = 1*(16384/(1.8*10));
      else ttrace[i] = -1*(1.8/16384);
      hstep->SetBinContent(i,ttrace[i]);
    }
    hstep->Write();
    mag_and_phase(ttrace,fmag,fphase);
    for(i=0;i<fftlen/2;i++){
      //printf("%d %g\n",i,fmag[i]);
      hfftstep->Fill(500.*(i)/fftlen,fmag[i]);
    }
    hfftstep->Write();
    dtft_init(2*(du[EVT_TRACELENGTH]>>16), 0.002, DTFMIN, DTFMAX, NDTFREQ);
    n_DU = 1;
  }
  for(idu=1;idu<n_DU;idu++){
    if(du[EVT_STATION_ID] == DU_id[idu]) break;
  }
  if(idu ==n_DU) {
    DU_id[n_DU++] = du[EVT_STATION_ID];
    for(int ich=0;ich<3;ich++){
      snprintf(fname,100,"HSF%d_%d",DU_id[idu],ich);
      snprintf(hname,100,"HSF%d_%d",DU_id[idu],ich);
      float f_off = 250./(2*(du[EVT_TRACELENGTH]>>16));
      HFsum[idu][ich] = new TProfile(fname,hname,(du[EVT_TRACELENGTH]>>16),0.-f_off,250-f_off);
      snprintf(fname,100,"HSFTime%d_%d",DU_id[idu],ich);
      snprintf(hname,100,"HSFTime%d_%d",DU_id[idu],ich);
      HFTime[idu][ich] = new TProfile2D(fname,hname,(du[EVT_TRACELENGTH]>>16),0.-f_off,250-f_off,120*NDAY,0.,24.*NDAY);
    }
    snprintf(fname,100,"HB%d",DU_id[idu]);
    snprintf(hname,100,"HB%d",DU_id[idu]);
    HBattery[idu] = new TProfile(fname,hname,14400,0.,24,0.,30.);
  }
  if(fftlen !=2*(du[EVT_TRACELENGTH]>>16)){
    fftlen = 2*(du[EVT_TRACELENGTH]>>16);
    fft_init(fftlen);
    ttrace = (float *)malloc(fftlen*sizeof(float));
    fmag = (float *)malloc(fftlen*sizeof(float));
    fphase = (float *)malloc(fftlen*sizeof(float));
  }
  HBattery[idu]->Fill(fh,(du[EVT_BATTERY]>>16)*(2.5*(18.+91.))/(18*4096));

  offset = EVT_START_ADC;
  for(int ich=0;ich<3;ich++){
    sprintf(fname,"T%d_%d_%d",du[EVT_EVT_ID],DU_id[idu],ich);
    if(((du[EVT_INP_SELECT]>>5*ich)&0x1e) != 0){
      TH1F *ht = new TH1F(fname,fname,2*(du[EVT_TRACELENGTH]>>16),0.,4*(du[EVT_TRACELENGTH]>>16));
      for(int i=0;i<(du[EVT_TRACELENGTH]>>16);i++){
        val2 =du[offset+i]>>16;
        val1 =du[offset+i]&0xffff;
        ht->SetBinContent(2*i+1,val1);
        ht->SetBinContent(2*i+2,val2);
        ttrace[2*i] = val1;
        ttrace[2*i+1] = val2;
      }
      ht->Write();
      ht->Delete();
      mag_and_phase(ttrace,fmag,fphase);
      for(i=0;i<fftlen/2;i++){
        HFsum[0][ich]->Fill(500.*(i)/fftlen,fmag[i]);
        HFsum[idu][ich]->Fill(500.*(i)/fftlen,fmag[i]);
        HFTime[idu][ich]->Fill(500.*(i)/fftlen,fh+24*(iday-zeroday),fmag[i]);
      }
      offset+=(du[EVT_TRACELENGTH]>>16);
    }
  }
}

void print_grand_event(){
  uint32_t *evdu;
  int idu = EVENT_DU;                                                      //parameter indicating start of LS
  int ev_end = event[EVENT_HDR_LENGTH]/4;
  evdu = (unsigned int *)&event[EVENT_DU];

  while(idu<ev_end){
    evdu = (uint32_t *)(&event[idu]);
    print_du((uint32_t *)evdu);
    idu +=(evdu[EVT_LENGTH]>>16);
  }
}


int main(int argc, char **argv)
{
  FILE *fp;
  int i,ich,ib;
  char fname[100],hname[100];
  int fmin,fmax,fseq;
  
  TFile g("Hist.root","RECREATE");
  fp = fopen(argv[1],"r");
  if(fp == NULL) printf("Error opening  !!%s!!\n",argv[1]);
  int nevt = 0;
  if(grand_read_file_header(fp) ){ //lets read events
    print_file_header();
    while (grand_read_event(fp) >0  ) {
      print_grand_event();
      nevt++;
    }
  }
  if (fp != NULL) fclose(fp); // close the file
  for(ib=0;ib<n_DU;ib++){
    if(HBattery[ib]!=NULL) HBattery[ib]->Write();
    for(ich = 0;ich<4;ich++){
      if(HFsum[ib][ich]!= NULL) HFsum[ib][ich]->Write();
      if(HFTime[ib][ich]!= NULL) HFTime[ib][ich]->Write();
    }
  }
  g.Close();
}
