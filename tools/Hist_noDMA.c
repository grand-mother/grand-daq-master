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

#define NDAY 10

int *filehdr=NULL;
unsigned short *event=NULL;
float *ttrace,*fmag,*fphase;
int fftlen = 0;
TProfile *HFsum[100][4],*HDtfreq[100][4]; // One for each arm
TProfile *HBattery[100];
TProfile2D *HFTime[100][4];
//,*HDTime[100][4];
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
  static int zeroday= -1;
  float fvalue;
#define DTFMIN 50
#define DTFMAX 110
#define NDTFREQ 480
  float dtfreq[NDTFREQ];
  
  //printf("\t T3 ID = %u\n",du[EVT_ID]);
  //printf("\t DU ID = %u\n",du[EVT_HARDWARE]);
  //printf("\t DU time = %u.%09u\n",*(uint32_t *)&du[EVT_SECOND],
  //       *(uint32_t *)&du[EVT_NANOSEC]);
  //printf("\t Trigger Position = %d T3 FLAG = 0x%x\n",
  //       du[EVT_TRIGGERPOS],du[EVT_T3FLAG]);
  //printf("\t Atmosphere (ADC) Temp = %d Pres = %d Humidity = %d\n",
  //       du[EVT_ATM_TEMP],du[EVT_ATM_PRES],du[EVT_ATM_HUM]);
  //printf("\t Acceleration (ADC) X = %d Y = %d Z = %d\n",
  //       du[EVT_ACCEL_X],du[EVT_ACCEL_Y],du[EVT_ACCEL_Z]);
  //printf("\t Battery (ADC) = %d %g\n",du[EVT_BATTERY],du[EVT_BATTERY]*(2.5*(18.+91.))/(18*4096));
  //printf("\t Format Firmware version = %d\n",du[EVT_VERSION]);
  //printf("\t ADC: sampling frequency = %d MHz, resolution=%d bits\n",
  //       du[EVT_MSPS],du[EVT_ADC_RES]);
  //printf("\t ADC Input channels =0x%x, Enabled Channels=0x%x\n",
  //       du[EVT_INP_SELECT],du[EVT_CH_ENABLE]);
  //printf("\t Number of ADC samples Total = %d",16*du[EVT_TOT_SAMPLES]);
  //for(i=1;i<=4;i++) printf(" Ch%d = %d",i,du[EVT_TOT_SAMPLES+i]);
  //printf("\n");
  //printf("\t Trigger pattern=0x%x Rate=%d\n",du[EVT_TRIG_PAT],du[EVT_TRIG_RATE]);
  //printf("\t Clock tick %u Nticks/sec %u\n",
  //       *(uint32_t *)&du[EVT_CTD],(*(uint32_t *)&du[EVT_CTP])&0x7fffffff);
  //printf("\t GPS: Offset=%g LeapSec=%d Status 0x%x Alarms 0x%x Warnings 0x%x\n",
  //       *(float *)&du[EVT_PPS_OFFSET],du[EVT_LEAP],du[EVT_GPS_STATFLAG],
  //       du[EVT_GPS_CRITICAL],du[EVT_GPS_WARNING]);
  float fh = (du[EVT_MINHOUR]&0xff)+((du[EVT_MINHOUR]>>8)&0xff)/60.;
  int iday = (du[EVT_DAYMONTH]>>8)&0xff;
  if(zeroday == -1) zeroday = iday;
  //printf("\t GPS: %02d/%02d/%04d %02d:%02d:%02d (%g)\n",
  //       (du[EVT_DAYMONTH]>>8)&0xff,(du[EVT_DAYMONTH])&0xff,du[EVT_YEAR],
 //        du[EVT_MINHOUR]&0xff,(du[EVT_MINHOUR]>>8)&0xff,du[EVT_STATSEC]&0xff,fh);
  //printf("\t GPS: Long = %g Lat = %g Alt = %g Chip Temp=%g\n",
   //      57.3*(*(double *)&du[EVT_LONGITUDE]),57.3*(*(double *)&du[EVT_LATITUDE]),
     //    *(double *)&du[EVT_ALTITUDE],*(float *)&du[EVT_GPS_TEMP]);
  //printf("\t Digi CTRL");
  //for(i=0;i<8;i++) printf(" 0x%x",du[EVT_CTRL+i]);
  //printf("\n");
  //printf("\t Digi Pre-Post trigger windows");
  //for(i=0;i<8;i++) printf(" 0x%x",du[EVT_WINDOWS+i]);
  //printf("\n");
  //for(ic=1;ic<=4;ic++){
  //  printf("\t Ch%d properties:",ic);
  //    for(i=0;i<6;i++)printf(" 0x%x",du[EVT_CHANNEL+6*(ic-1)+i]);
  //  printf("\n");
  //}
  //for(ic=1;ic<=4;ic++){
  //  printf("\t Ch%d trigger settings:",ic);
 //   for(i=0;i<6;i++)printf(" 0x%x",du[EVT_TRIGGER+6*(ic-1)+i]);
//    printf("\n");
  //}
  ioff = du[EVT_HDRLEN];
  if(n_DU == 0){
    for(ic=0;ic<4;ic++){
      snprintf(fname,100,"HSF%d",ic);
      snprintf(hname,100,"HSF%d",ic);
      HFsum[0][ic] = new TProfile(fname,hname,du[EVT_TOT_SAMPLES+1]/2,0.,250);
    }
    dtft_init(du[EVT_TOT_SAMPLES+1], 0.002, DTFMIN, DTFMAX, NDTFREQ);
    n_DU = 1;
  }
  for(idu=1;idu<n_DU;idu++){
    if(du[EVT_HARDWARE] == DU_id[idu]) break;
  }
  if(idu ==n_DU) {
    DU_id[n_DU++] = du[EVT_HARDWARE];
    for(ic=0;ic<4;ic++){
      snprintf(fname,100,"HSF%d_%d",DU_id[idu],ic);
      snprintf(hname,100,"HSF%d_%d",DU_id[idu],ic);
      float f_off = 250./du[EVT_TOT_SAMPLES+1];
      HFsum[idu][ic] = new TProfile(fname,hname,du[EVT_TOT_SAMPLES+1]/2,0.-f_off,250-f_off);
      snprintf(fname,100,"HDF%d_%d",DU_id[idu],ic);
      snprintf(hname,100,"HDF%d_%d",DU_id[idu],ic);
      HDtfreq[idu][ic] = new TProfile(fname,hname,NDTFREQ,DTFMIN,DTFMAX);
      snprintf(fname,100,"HSFTime%d_%d",DU_id[idu],ic);
      snprintf(hname,100,"HSFTime%d_%d",DU_id[idu],ic);
      HFTime[idu][ic] = new TProfile2D(fname,hname,du[EVT_TOT_SAMPLES+1]/2,0.-f_off,250-f_off,120*NDAY,0.,24.*NDAY);
    }
    snprintf(fname,100,"HDFTime%d_%d",DU_id[idu],ic);
    snprintf(hname,100,"HDFTime%d_%d",DU_id[idu],ic);
    //HDTime[idu][ic] = new TProfile2D(fname,hname,NDTFREQ,DTFMIN,DTFMAX,24*10*NDAY,0.,24.*NDAY);
    snprintf(fname,100,"HB%d",DU_id[idu]);
    snprintf(hname,100,"HB%d",DU_id[idu]);
    HBattery[idu] = new TProfile(fname,hname,14400,0.,24,0.,30.);
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
      snprintf(fname,100,"H%dT%dD%d",du[EVT_ID],ic,du[EVT_HARDWARE]);
      snprintf(hname,100,"H%dT%dD%d",du[EVT_ID],ic,du[EVT_HARDWARE]);
      //TH1F *Hist = new TH1F(fname,hname,du[EVT_TOT_SAMPLES+ic],0.,2*du[EVT_TOT_SAMPLES+ic]);
      for(i=0;i<du[EVT_TOT_SAMPLES+ic];i++){
        value =(int16_t)du[ioff++];
        bit14 = (value & ( 1 << 13 )) >> 13;
        mask = 1 << 14; // --- bit 15
        value = (value & (~mask)) | (bit14 << 14);
        mask = 1 << 15; // --- bit 16
        value = (value & (~mask)) | (bit14 << 15);
       // Hist->SetBinContent(i+1,value);
        ttrace[i] = value;
      }
      /*dtft_run(ttrace,dtfreq);
      for(i=0;i<NDTFREQ;i++){
        float fr = DTFMIN+(i+0.5)*(DTFMAX-DTFMIN)/NDTFREQ;
        HDtfreq[idu][ic-1]->Fill(fr,dtfreq[i]);
        //fvalue = dtft_single(&(ttrace[0]),
        //                     du[EVT_TOT_SAMPLES+ic],2.e-3,fr);
        //HDtfreq[idu][ic-1]->Fill(fr,fvalue);
      }*/
      HBattery[idu]->Fill(fh,du[EVT_BATTERY]*(2.5*(18.+91.))/(18*4096));
      //Hist->Write();
      // Hist->Delete();
      mag_and_phase(ttrace,fmag,fphase);
      snprintf(fname,100,"H%dF%dD%d",du[EVT_ID],ic,du[EVT_HARDWARE]);
      snprintf(hname,100,"H%dF%dD%d",du[EVT_ID],ic,du[EVT_HARDWARE]);
      //Hist = new TH1F(fname,hname,fftlen/2,0.,250);

      for(i=0;i<fftlen/2;i++){
       // Hist->SetBinContent(i+1,fmag[i]);
        HFsum[0][ic-1]->Fill(500.*(i)/fftlen,fmag[i]);
        HFsum[idu][ic-1]->Fill(500.*(i)/fftlen,fmag[i]);
        HFTime[idu][ic-1]->Fill(500.*(i)/fftlen,fh+24*(iday-zeroday),fmag[i]);
      }
     // Hist->Write();
     // Hist->Delete();

    }
  }
}

void print_grand_event(){
  uint16_t *evdu;
  unsigned int *evptr = (unsigned int *)event;
  int idu = EVENT_DU;                                                      //parameter indicating start of LS
  int ev_end = ((int)(event[EVENT_HDR_LENGTH+1]<<16)+(int)(event[EVENT_HDR_LENGTH]))/SHORTSIZE;
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
  char fname[100],hname[100];
  int fmin,fmax,fseq;
  
  TFile g("Hist.root","RECREATE");
  sscanf(argv[2],"%d",&fmin);
  sscanf(argv[3],"%d",&fmax);
  for(fseq = fmin;fseq<=fmax;fseq++){
    snprintf(fname,100,"%s.f%04d",argv[1],fseq);
    fp = fopen(fname,"r");
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
  }
  for(ib=0;ib<n_DU;ib++){
    if(HBattery[ib]!=NULL) HBattery[ib]->Write();
    for(ich = 0;ich<4;ich++){
      if(HFsum[ib][ich]!= NULL) HFsum[ib][ich]->Write();
      if(HDtfreq[ib][ich]!= NULL) HDtfreq[ib][ich]->Write();
      if(HFTime[ib][ich]!= NULL) HFTime[ib][ich]->Write();
     // if(HDTime[ib][ich]!= NULL) HFTime[ib][ich]->Write();
    }
  }
  g.Close();
}
