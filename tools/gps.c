// Traces.c
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
//#include "amsg.h"
#include "scope.h"
#include "Traces.h"
#define INTSIZE 4
#define SHORTSIZE 2

int *filehdr=NULL;
unsigned short *event=NULL;

int grand_read_file_header(FILE *fp)
{
  int i,return_code;
  int isize;
  
  if( !fread(&isize,INTSIZE,1,fp)) {
    printf("Cannot read the header length\n");
    return(0);                                                       //cannot read the header length
  }
  //printf("The header length is %d bytes \n",isize);
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
  /*printf("Header Length is %d bytes\n",filehdr[FILE_HDR_LENGTH]);
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
  }*/
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



void print_grand_event(){
  uint16_t *evdu;
  unsigned int *evptr = (unsigned int *)event;
  uint32_t evsec,dusec;
  int mask = 0;
  int idu = EVENT_DU;                                                      //parameter indicating start of LS
  int ev_end = ((int)(event[EVENT_HDR_LENGTH+1]<<16)+(int)(event[EVENT_HDR_LENGTH]))/SHORTSIZE;
  evptr +=2;
  //printf("Event Size = %d\n",*evptr++);
  //printf("      Run Number = %d\n",*evptr++);
  printf("      Event Number = %d\n",*evptr++);
  evptr+=2;
  //printf("      T3 Number = %d\n",*evptr++);
  //printf("      First DU = %d\n",*evptr++);
  evsec = *evptr;
  //printf("      Time Seconds = %u\n",evsec);
  //printf("      Time Nano Seconds = %d\n",*evptr++);
  /*evdu = (uint16_t *)evptr;
  printf("      Event Type = ");
  if((evdu[0] &TRIGGER_T3_MINBIAS)) printf("10 second trigger\n");
  else if((evdu[0] &TRIGGER_T3_RANDOM)) printf("random trigger\n");
  else printf("Shower event\n");
  ++evdu;
  printf("      Event Version = %d\n",*evdu);
  evptr +=3;
  printf("      Number of DU's = %d\n",*evptr);*/
  while(idu<ev_end){
    evdu = (uint16_t *)(&event[idu]);
    //print_du(evdu);
    dusec =*(uint32_t *)&evdu[EVT_SECOND];
    if(dusec != evsec) mask = 1;
    idu +=(evdu[EVT_LENGTH]);
  }
  if(mask == 1){
    printf("      Time Seconds = %u\n",evsec);
    idu = EVENT_DU;
    while(idu<ev_end){
      evdu = (uint16_t *)(&event[idu]);
      //print_du(evdu);
      dusec =*(uint32_t *)&evdu[EVT_SECOND];
      if(dusec != evsec) mask = 1;
      printf("\t DU ID = %u\n",evdu[EVT_HARDWARE]);
      printf("\t DU time = %u\n",dusec);
      idu +=(evdu[EVT_LENGTH]);
    }
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
    print_file_header();
    while (grand_read_event(fp) >0 ) {
      print_grand_event();
    }
  }
  if (fp != NULL) fclose(fp); // close the file
}
