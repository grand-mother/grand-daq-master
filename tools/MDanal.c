// Traces.c
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
//#include "amsg.h"
#include "scope.h"
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

int *filehdr=NULL;
unsigned short *event=NULL;
float *ttrace,*fmag,*fphase;
int fftlen = 0;
TProfile *HFsum[100][4]; // One for each arm
TH1F *HNoise[100][4];
TProfile *HRate[100];
TProfile *HBattery[100];
TProfile *HTemp[100];
TProfile2D *HFTime[100][4];
TH1F *Hdt;
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
      event = (unsigned short *)malloc(isize+INTSIZE);                          //allocate memory for the event
    }
  }
  else{
    event = (unsigned short *)malloc(isize+INTSIZE);                          //allocate memory for the event
  }
  if(event == NULL){
    printf("Cannot allocate enough memory to save the event!\n");
    return(0);                                                       //cannot allocate memory for event
  }
  event[0] = isize&0xffff;                                                  //put the size into the event
  event[1] = isize>>16;
  if((return_code = fread(&(event[2]),1,isize,fp)) !=(isize)) {
    printf("Cannot read the full event (%d)\n",return_code);
    return(0);                                                       //cannot read the full event
  }
  return(1);
}

void print_du(uint16_t *du)
{
  int i,ic,idu;
  int ioff;
  short value;
  short bit14;
  int mask;
  char hname[100],fname[100];
  static unsigned int prev_sec=0,prev_nsec = 0,sec_null=0,sec;
  unsigned int nsec,CTP,CTD;
  CTP = *(uint32_t *)&du[EVT_CTP];
  CTD = *(uint32_t *)&du[EVT_CTD];
  if((CTP&0x80000000) || (CTD &0x80000000) ){
    CTP &=0x7fffffff;
    CTD &=0x7fffffff;
  }
  if(CTP<480000000 ||CTP>520000000  ) CTP = 500000000;
    nsec = 1.E9*((double)CTD/CTP);
    *(uint32_t *)&du[EVT_NANOSEC] = nsec;
  /*
  printf("\t T3 ID = %u\n",du[EVT_ID]);
  printf("\t DU ID = %u\n",du[EVT_HARDWARE]);
  printf("\t DU time = %u.%09u\n",*(uint32_t *)&du[EVT_SECOND],
         *(uint32_t *)&du[EVT_NANOSEC]);
  printf("\t Trigger Position = %d T3 FLAG = 0x%x\n",
         du[EVT_TRIGGERPOS],du[EVT_T3FLAG]);
  printf("\t Atmosphere (ADC) Temp = %d Pres = %d Humidity = %d\n",
         du[EVT_ATM_TEMP],du[EVT_ATM_PRES],du[EVT_ATM_HUM]);
  printf("\t Acceleration (ADC) X = %d Y = %d Z = %d\n",
         du[EVT_ACCEL_X],du[EVT_ACCEL_Y],du[EVT_ACCEL_Z]);
  printf("\t Battery (ADC) = %d\n",du[EVT_BATTERY]);
  printf("\t Format Firmware version = %d\n",du[EVT_VERSION]);
  printf("\t ADC: sampling frequency = %d MHz, resolution=%d bits\n",
         du[EVT_MSPS],du[EVT_ADC_RES]);
  printf("\t ADC Input channels =0x%x, Enabled Channels=0x%x\n",
         du[EVT_INP_SELECT],du[EVT_CH_ENABLE]);
  printf("\t Number of ADC samples Total = %d",16*du[EVT_TOT_SAMPLES]);
  for(i=1;i<=4;i++) printf(" Ch%d = %d",i,du[EVT_TOT_SAMPLES+i]);
  printf("\n");
  printf("\t Trigger pattern=0x%x Rate=%d\n",du[EVT_TRIG_PAT],du[EVT_TRIG_RATE]);
  printf("\t Clock tick %u Nticks/sec %u\n",
         *(uint32_t *)&du[EVT_CTD],*(uint32_t *)&du[EVT_CTP]);
  printf("\t GPS: Offset=%g LeapSec=%d Status 0x%x Alarms 0x%x Warnings 0x%x\n",
         *(float *)&du[EVT_PPS_OFFSET],du[EVT_LEAP],du[EVT_GPS_STATFLAG],
         du[EVT_GPS_CRITICAL],du[EVT_GPS_WARNING]);
  float fh = (du[EVT_MINHOUR]&0xff)+((du[EVT_MINHOUR]>>8)&0xff)/60.;
  printf("\t GPS: %02d/%02d/%04d %02d:%02d:%02d (%g)\n",
         (du[EVT_DAYMONTH]>>8)&0xff,(du[EVT_DAYMONTH])&0xff,du[EVT_YEAR],
         du[EVT_MINHOUR]&0xff,(du[EVT_MINHOUR]>>8)&0xff,du[EVT_STATSEC]&0xff,fh);
  printf("\t GPS: Long = %g Lat = %g Alt = %g Chip Temp=%g\n",
         57.3*(*(double *)&du[EVT_LONGITUDE]),57.3*(*(double *)&du[EVT_LATITUDE]),
         *(double *)&du[EVT_ALTITUDE],*(float *)&du[EVT_GPS_TEMP]);
  printf("\t Digi CTRL");
  for(i=0;i<8;i++) printf(" 0x%x",du[EVT_CTRL+i]);
  printf("\n");
  printf("\t Digi Pre-Post trigger windows");
  for(i=0;i<8;i++) printf(" 0x%x",du[EVT_WINDOWS+i]);
  printf("\n");
  for(ic=1;ic<=4;ic++){
    printf("\t Ch%d properties:",ic);
    for(i=0;i<6;i++)printf(" 0x%x",du[EVT_CHANNEL+6*(ic-1)+i]);
    printf("\n");
  }
  for(ic=1;ic<=4;ic++){
    printf("\t Ch%d trigger settings:",ic);
    for(i=0;i<6;i++)printf(" 0x%x",du[EVT_TRIGGER+6*(ic-1)+i]);
    printf("\n");
  }
   */
  float fh = (du[EVT_MINHOUR]&0xff)+((du[EVT_MINHOUR]>>8)&0xff)/60.;
  ioff = du[EVT_HDRLEN];
  if(n_DU == 0){
    for(ic=0;ic<4;ic++){
      sprintf(fname,"HSF%d",ic);
      sprintf(hname,"Frequency Spectrum %d;MHz;AU",ic);
      HFsum[0][ic] = new TProfile(fname,hname,du[EVT_TOT_SAMPLES+1]/2,0.,250);
      sprintf(fname,"HSN%d",ic);
      sprintf(hname,"ADC occurances %d;ADC;Hz",ic);
      HNoise[0][ic] = new TH1F(fname,hname,8192,0.,8192);
     }
    n_DU = 1;
  }
  for(idu=1;idu<n_DU;idu++){
    if(du[EVT_HARDWARE] == DU_id[idu]) break;
  }
  for(idu=1;idu<n_DU;idu++){
    if(du[EVT_HARDWARE] == DU_id[idu]) break;
  }
  if(idu ==n_DU) {
    DU_id[n_DU++] = du[EVT_HARDWARE];
    for(ic=0;ic<4;ic++){
      sprintf(fname,"HSF%d_%d",DU_id[idu],ic);
      sprintf(hname,"Frequency Spectrum %d C %d;MHz;AU",DU_id[idu],ic);
      HFsum[idu][ic] = new TProfile(fname,hname,du[EVT_TOT_SAMPLES+1]/2,0.,250);
      sprintf(fname,"HSN%d_%d",DU_id[idu],ic);
      sprintf(hname,"ADC occurances %d;ADC;Hz",DU_id[idu],ic);
      HNoise[idu][ic] = new TH1F(fname,hname,8192,0.,8192);
      sprintf(fname,"HSFTime%d_%d",DU_id[idu],ic);
      sprintf(hname,"HSFTime%d_%d",DU_id[idu],ic);
      HFTime[idu][ic] = new TProfile2D(fname,hname,du[EVT_TOT_SAMPLES+1]/2,0.,250,240,0.,24.);
    }
    sprintf(fname,"HSR%d",DU_id[idu]);
    sprintf(hname,"Rate %d;sec;Hz",DU_id[idu]);
    HRate[idu] = new TProfile(fname,hname,500,0.,500.);
    sprintf(fname,"HB%d",DU_id[idu]);
    sprintf(hname,"HB%d",DU_id[idu]);
    HBattery[idu] = new TProfile(fname,hname,14400,0.,24,0.,30.);
    sprintf(fname,"HTmp%d",DU_id[idu]);
    sprintf(hname,"HTmp%d",DU_id[idu]);
    HTemp[idu] = new TProfile(fname,hname,14400,0.,24,0.,90.);
  }
  sec = *(uint32_t *)&du[EVT_SECOND];
  nsec =  *(uint32_t *)&du[EVT_NANOSEC];
  HBattery[idu]->Fill(fh,du[EVT_BATTERY]*(2.5*(18.+91.))/(18*4096));
  float DUtmp = *(float *)&du[EVT_GPS_TEMP];
  //printf("Temperature = %g\n",DUtmp);
  HTemp[idu]->Fill(fh,DUtmp);
  if(prev_sec != 0){
    double dt = 1000*(sec-prev_sec);
    if(prev_nsec>nsec) dt-=double((prev_nsec-nsec)/1.E6);
    else dt+=(nsec-prev_nsec)/1.E6;
    //if(dt>200) printf("%d.%09d %d.%09d %g %08x %08x\n",sec,nsec,prev_sec,prev_nsec,dt,CTD,CTP);
    Hdt->Fill(dt);
  }
  prev_nsec = nsec;
  if(prev_sec != sec){
    prev_sec = sec;
    if(sec_null == 0) sec_null = sec;
    HRate[idu]->Fill(sec-sec_null,du[EVT_TRIG_RATE]);
  }
  for(ic=1;ic<=4;ic++){
    if(du[EVT_TOT_SAMPLES+ic]>0){
      if(fftlen !=du[EVT_TOT_SAMPLES+ic]){
        fftlen = du[EVT_TOT_SAMPLES+ic];
        fft_init(fftlen);
        ttrace = (float *)malloc(fftlen*sizeof(float));
        fmag = (float *)malloc(fftlen*sizeof(float));
        fphase = (float *)malloc(fftlen*sizeof(float));
      }
      for(i=0;i<du[EVT_TOT_SAMPLES+ic];i++){
        value =(int16_t)du[ioff++];
        bit14 = (value & ( 1 << 13 )) >> 13;
        mask = 1 << 14; // --- bit 15
        value = (value & (~mask)) | (bit14 << 14);
        mask = 1 << 15; // --- bit 16
        value = (value & (~mask)) | (bit14 << 15);
        ttrace[i] = value;
        if(value<0) value = -value;
        HNoise[0][ic-1]->Fill(value);
        HNoise[idu][ic-1]->Fill(value);
      }
      mag_and_phase(ttrace,fmag,fphase);

      for(i=0;i<fftlen/2;i++){
        HFsum[0][ic-1]->Fill(500*(i+0.5)/fftlen,fmag[i]);
        HFsum[idu][ic-1]->Fill(500*(i+0.5)/fftlen,fmag[i]);
        HFTime[idu][ic-1]->Fill(500*(i+0.5)/fftlen,fh,fmag[i]);
      }

    }
  }
}

void print_grand_event(){
  static unsigned int sec_prev,nsec_prev,sec,nsec;
  double dt;
  uint16_t *evdu;
  unsigned int *evptr = (unsigned int *)event;
  int idu = EVENT_DU;                                                      //parameter indicating start of LS
  int ev_end = ((int)(event[EVENT_HDR_LENGTH+1]<<16)+(int)(event[EVENT_HDR_LENGTH]))/SHORTSIZE;
  if(n_DU == 0){
    Hdt = new TH1F("Hdt","Time difference events;ms",10000,0.,1000);
    sec_prev = 0;
    nsec_prev = 0;
  }
  /*printf("Event Size = %d\n",*evptr++);
  printf("      Run Number = %d\n",*evptr++);
  printf("      Event Number = %d\n",*evptr++);
  printf("      T3 Number = %d\n",*evptr++);
  printf("      First DU = %d\n",*evptr++);*/
  evptr+=5;
  sec =*evptr++;
  nsec =*evptr++;
  /*if(n_DU!= 0){
    dt = 1000*(sec-sec_prev);
    if(nsec_prev>nsec) dt+=((nsec_prev-nsec)/1.E6)-1000;
    else dt+=(nsec-nsec_prev)/1.E6;
    //if(dt<10) printf("%d %d %d %d\n",sec,nsec,sec_prev,nsec_prev);
    Hdt->Fill(dt);
    nsec_prev = nsec;
    sec_prev = sec;
  }*/
  evdu = (uint16_t *)evptr;
  /*printf("      Event Type = ");
  if((evdu[0] &TRIGGER_T3_MINBIAS)) printf("10 second trigger\n");
  else if((evdu[0] &TRIGGER_T3_RANDOM)) printf("random trigger\n");
  else printf("Shower event\n");*/
  ++evdu;
  //printf("      Event Version = %d\n",*evdu);
  evptr +=3;
  //printf("      Number of DU's = %d\n",*evptr);
  while(idu<ev_end){
    evdu = (uint16_t *)(&event[idu]);
    print_du(evdu);
    idu +=(evdu[EVT_LENGTH]);
  }
}


int main(int argc, char **argv)
{
  FILE *fp;
  int i,ich,ib;
  int nsample;
  double tottime,val;
  char fname[100],hname[100];
  int irun,ifile,ifile_l,ifile_f;
  
  sscanf(argv[2],"%d",&irun);
  sscanf(argv[3],"%d",&ifile_f);
  if(argc ==5 )sscanf(argv[4],"%d",&ifile_l);
  else ifile_l=ifile_f;

  sprintf(fname,"MDanal_%d_%d.root",irun,ifile_f);
  TFile g(fname,"RECREATE");
  for(ifile=ifile_f;ifile<=ifile_l;ifile++){
    sprintf(fname,"%s/MD/md%06d.f%04d",argv[1],irun,ifile);
    fp = fopen(fname,"r");
    if(fp == NULL) printf("Error opening  !!%s!!\n",fname);
    
    if(grand_read_file_header(fp) ){ //lets read events
      print_file_header();
      while (grand_read_event(fp) >0 ) {
        print_grand_event();
      }
    }
    if (fp != NULL) fclose(fp); // close the file
  }
  for(ib=0;ib<n_DU;ib++){
    if(HBattery[ib]!=NULL) HBattery[ib]->Write();
    if(HTemp[ib]!=NULL) HTemp[ib]->Write();
    for(ich = 0;ich<4;ich++){
      if(HFsum[ib][ich]!= NULL) HFsum[ib][ich]->Write();
      if(HFTime[ib][ich]!= NULL) HFTime[ib][ich]->Write();
      if(HNoise[ib][ich]!= NULL) {
        nsample =HNoise[ib][ich]->GetEntries();
        tottime = nsample*2/1.E9;
        //printf("%d %g\n",nsample,tottime);
        for(i=0;i<=8192;i++){
          val = HNoise[ib][ich]->GetBinContent(i);
          HNoise[ib][ich]->SetBinContent(i,val/tottime);
        }
        HNoise[ib][ich]->Write();
      }
    }
    if(ib != 0)HRate[ib]->Write();
  }
  Hdt->Write();
  g.Close();
}
