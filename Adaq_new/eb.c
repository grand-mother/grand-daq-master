/***
 Event Builder
 Version:3.0
 Date: 26/7/2024
 Author: Charles Timmermans, Nikhef/Radboud University
 
 Altering the code without explicit consent of the author is forbidden
 ***/

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
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


int running = 0;

int i_DUbuffer = 0;

int eb_event = 0;

int isub;
static int write_sub[3]={0,0,0};


FILEHDR eb_fhdr;
FILE *fpout = NULL,*fpten=NULL,*fpmb=NULL,*fptr=NULL;

#define T3_BUFSIZE 1000 // max 1000000 integers (=a lot of 10 station events (23 words each)
uint32_t t3_buffer[T3_BUFSIZE];
#define N_T3 100 // store a maximum of 100 addresses
uint32_t t3_address[N_T3];
int t3_write = 0;
int t3_read = 0;


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
  sprintf(fname,"%s/%s_%4d%02d%02d_%02d%02d%02d_RUN%06d_CD_%s.bin",eb_dir,eb_site,gtime->tm_year+1900,
          gtime->tm_mon+1,gtime->tm_mday,gtime->tm_hour,gtime->tm_min,gtime->tm_sec,
          eb_run,eb_extra);
  printf("!!%s!!\n",fname);
  fpout = fopen(fname,"r");
  while(fpout != NULL){
    eb_run +=1;
    sprintf(fname,"%s/%s_%4d%02d%02d_%02d%02d%02d_RUN%06d_CD_%s.bin",eb_dir,eb_site,gtime->tm_year+1900,
            gtime->tm_mon+1,gtime->tm_mday,gtime->tm_hour,gtime->tm_min,gtime->tm_sec,
            eb_run,eb_extra);
    fpout = fopen(fname,"r");
  }
  printf("File = %s\n",fname);
  fpout = fopen(fname,"w");
  sprintf(fname,"%s/%s_%4d%02d%02d_%02d%02d%02d_RUN%06d_MD_%s.bin",eb_dir,eb_site,gtime->tm_year+1900,
          gtime->tm_mon+1,gtime->tm_mday,gtime->tm_hour,gtime->tm_min,gtime->tm_sec,
          eb_run,eb_extra);
  fpten = fopen(fname,"w");
  sprintf(fname,"%s/%s_%4d%02d%02d_%02d%02d%02d_RUN%06d_UD_%s.bin",eb_dir,eb_site,gtime->tm_year+1900,
          gtime->tm_mon+1,gtime->tm_mday,gtime->tm_hour,gtime->tm_min,gtime->tm_sec,
          eb_run,eb_extra);
  fpmb = fopen(fname,"w");
  sprintf(fname,"%s/%s_%4d%02d%02d_%02d%02d%02d_RUN%06d_TR_%s.bin",eb_dir,eb_site,gtime->tm_year+1900,
          gtime->tm_mon+1,gtime->tm_mday,gtime->tm_hour,gtime->tm_min,gtime->tm_sec,
          eb_run,eb_extra);
  fptr = fopen(fname,"w");
  //sprintf(fname,"%s/%s_%4d%02d%02d_%02d%02d_%06d_MN_%s",eb_dir,eb_site,gtime->tm_year+1900,
  //        gtime->tm_mon+1,gtime->tm_mday,gtime->tm_hour,gtime->tm_min,
  //        eb_run,eb_extra);
  // next write file header
  sprintf(fname,"%s_%4d%02d%02d_%02d%02d%02d_RUN%06d_XX_%s.bin",eb_site,gtime->tm_year+1900,
          gtime->tm_mon+1,gtime->tm_mday,gtime->tm_hour,gtime->tm_min,gtime->tm_sec,
          eb_run,eb_extra);

  eb_fhdr.length = sizeof(FILEHDR)-sizeof(int32_t);
  eb_fhdr.run_id = eb_run;
  eb_fhdr.run_mode = eb_run_mode;
  eb_fhdr.file_id = 0;
  //eb_fhdr.first_event_id = evhdr->event_id;
  //eb_fhdr.first_event_time = evhdr->seconds;
  //eb_fhdr.last_event_id = evhdr->event_id;
  //eb_fhdr.last_event_time = evhdr->seconds;
  eb_fhdr.add1 = 0;
  eb_fhdr.add2 = 0;
  fwrite(&eb_fhdr,1,sizeof(FILEHDR),fpout);
  fwrite(&eb_fhdr,1,sizeof(FILEHDR),fpten);
  fwrite(&eb_fhdr,1,sizeof(FILEHDR),fpmb);
  fwrite(&eb_fhdr,1,sizeof(FILEHDR),fptr);
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
  if(fptr != NULL){
    rewind(fptr);
    fwrite(&eb_fhdr,1,sizeof(FILEHDR),fptr);
    fclose(fptr);
  }
  fpout = NULL;
  fpten = NULL;
  fpmb = NULL;
  fptr = NULL;
}

int eb_directory_age(const char *path)
{
  time_t rawtime;
  struct stat statbuf;
  time(&rawtime);
  
  if (!stat(path, &statbuf)) {
    return(rawtime - statbuf.st_mtime);
  }
  return(0);
}

int eb_count_files(const char *path)
{
  DIR *d = opendir(path);
  size_t path_len = strlen(path);
  time_t rawtime;
  time(&rawtime);
  int r = 0;
  if (d) {
    struct dirent *p;
    r = 0;
    while (!r && (p=readdir(d))) {
      int r2 = 0;
      char *buf;
      size_t len;
      /* Skip the names "." and ".." as we don't want to recurse on them. */
      if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
        continue;
      len = path_len + strlen(p->d_name) + 2;
      buf = malloc(len);
      if (buf) {
        struct stat statbuf;
        snprintf(buf, len, "%s/%s", path, p->d_name);
        if (!stat(buf, &statbuf)) {
          //printf("%ld %ld %ld\n",rawtime,statbuf.st_atime,rawtime - statbuf.st_atime);
          if ((!S_ISDIR(statbuf.st_mode) &&(rawtime - statbuf.st_mtime)>1))
            r2 = 1;
        }
        free(buf);
      }
      r += r2;
    }
    closedir(d);
  }
  return(r);
}

int eb_directory_size(const char *path) {
  DIR *d = opendir(path);
  size_t path_len = strlen(path);
  int r = -1;
  if (d) {
    struct dirent *p;
    r = 0;
    while (!r && (p=readdir(d))) {
      int r2 = 0;
      char *buf;
      size_t len;
      /* Skip the names "." and ".." as we don't want to recurse on them. */
      if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
        continue;
      len = path_len + strlen(p->d_name) + 2;
      buf = malloc(len);
      if (buf) {
        struct stat statbuf;
        snprintf(buf, len, "%s/%s", path, p->d_name);
        if (!stat(buf, &statbuf)) {
          if (S_ISDIR(statbuf.st_mode))
            r2 = eb_directory_size(buf);
          else
            r2 = statbuf.st_size;
        }
        free(buf);
      }
      r += r2;
    }
    closedir(d);
  }
  return(r);
}

int eb_remove_directory(const char *path) {
  DIR *d = opendir(path);
  size_t path_len = strlen(path);
  int r = -1;
  if (d) {
    struct dirent *p;
    r = 0;
    while (!r && (p=readdir(d))) {
      int r2 = -1;
      char *buf;
      size_t len;
      /* Skip the names "." and ".." as we don't want to recurse on them. */
      if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
        continue;
      len = path_len + strlen(p->d_name) + 2;
      buf = malloc(len);
      if (buf) {
        struct stat statbuf;
        snprintf(buf, len, "%s/%s", path, p->d_name);
        if (!stat(buf, &statbuf)) {
          if (S_ISDIR(statbuf.st_mode))
            r2 = eb_remove_directory(buf);
          else
            r2 = unlink(buf);
        }
        free(buf);
      }
      r = r2;
    }
    closedir(d);
  }
  if (d) //was if(!r)
    r = rmdir(path);
  return(r);
}

int eb_directory_add(FILE *fpout, const char *path) {

  DIR *d = opendir(path);
  FILE *fp;
  size_t path_len = strlen(path);
  int r = 0;
  int rdlen;
  int bf[2000];
  if (d) {
    struct dirent *p;
    r = 0;
    while (!r && (p=readdir(d))) {
      int r2 = 0;
      char *buf;
      size_t len;
      /* Skip the names "." and ".." as we don't want to recurse on them. */
      if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
        continue;
      len = path_len + strlen(p->d_name) + 2;
      buf = malloc(len);
      if (buf) {
        struct stat statbuf;
        snprintf(buf, len, "%s/%s", path, p->d_name);
        if (!stat(buf, &statbuf)) {
          if (S_ISDIR(statbuf.st_mode))
            r2 = eb_directory_size(buf);
          else{
            fp = fopen(buf,"r");
            if(fp != NULL){
              while((rdlen = fread(bf, INTSIZE, 2000, fp)) >0) fwrite(bf,INTSIZE,rdlen,fpout);
              fclose(fp);
            }
          }
        }
        free(buf);
      }
      r += r2;
    }
    closedir(d);
  }
  return(r);
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
 */
void eb_gett3(){
  AMSG *msg;
  T3BODY *T3info;
  int n_t3_du;
  int length;
  char foldername[100];
  
  while((shm_t3.Ubuf[(*shm_t3.size)*(*shm_t3.next_readb)])  !=  0){ // loop over the input
    if(((shm_t3.Ubuf[(*shm_t3.size)*(*shm_t3.next_readb)]) &2) ==  2){ // loop over the input
      msg = (AMSG *)(&(shm_t3.Ubuf[(*shm_t3.size)*(*shm_t3.next_readb)+1]));
      memcpy((void *)(&t3_buffer[t3_address[t3_write]]),msg,(msg->length)*INTSIZE);
      t3_write++;
      if(t3_write >= N_T3) t3_write = 0;
      else t3_address[t3_write] = t3_address[t3_write-1]+(msg->length);
      T3info = (T3BODY *)(&(msg->body[0])); //set the T3 info pointer
      n_t3_du = (msg->length-3)/T3STATIONSIZE; // msg == length+tag+eventnr+T3stations
      //sprintf(foldername,"%s/%d",LOG_FOLDER,T3info->event_nr);
      //eb_remove_directory(foldername);
      //printf("EB: T3 event = %d; NDU = %d \n",T3info->event_nr,n_t3_du);
      if(fptr != NULL){
        length = 3+3*n_t3_du;
        fwrite(&length,INTSIZE,1,fptr);
        fwrite(&(T3info->event_nr),INTSIZE,1,fptr);
        length = 0;
        if(msg->tag == DU_GET_MINBIAS_EVENT) length = TRIGGER_T3_MINBIAS;
        if(msg->tag == DU_GET_RANDOM_EVENT) length = TRIGGER_T3_RANDOM;
        fwrite(&length,INTSIZE,1,fptr);
        for(int i=0;i<n_t3_du;i++){
          fwrite(&(T3info->t3station[i].DU_id),INTSIZE,1,fptr);
          fwrite(&(T3info->t3station[i].second),INTSIZE,1,fptr);
          fwrite(&(T3info->t3station[i].nsec),INTSIZE,1,fptr);
        }
      }
      shm_t3.Ubuf[(*shm_t3.size)*(*shm_t3.next_readb)] &= ~2;
    }
    *shm_t3.next_readb = (*shm_t3.next_readb) + 1;
    if( *shm_t3.next_readb >= *shm_t3.nbuf) *shm_t3.next_readb = 0;
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
  char foldername[100];
  int n_stat,n_data;
  AMSG *msg;
  T3BODY *T3info;
  
  if(t3_read == t3_write) return; //no buffers in memory
  msg = (AMSG *) (&t3_buffer[t3_address[t3_read]]);
  T3info = (T3BODY *)(&(msg->body[0])); //set the T3 info pointer
  n_stat = (msg->length-3)/T3STATIONSIZE;
  sprintf(foldername,"%s/%d",LOG_FOLDER,T3info->event_nr);
  //printf("%s\n",foldername);
  n_data = eb_count_files(foldername);
  if(n_data<0){
    printf("Error reading folder %s\n",foldername);
    t3_read++;
    if(t3_read >=N_T3) t3_read = 0;
  }
  //printf("Write: %s %d %d %d\n",foldername,n_stat,n_data,eb_directory_age(foldername));
  if(n_stat == n_data ||eb_directory_age(foldername)>30){ //all date or older than 30 sec
    evhdr.t3_id = T3info->event_nr;
    evhdr.DU_count = n_data;
    evhdr.length = 40+eb_directory_size(foldername);
    //printf("Event size = %d\n",evhdr.length);
    evhdr.run_id = eb_run;
    evhdr.event_id = eb_event++;
    evhdr.first_DU = T3info->t3station[0].DU_id;
    evhdr.seconds = T3info->t3station[0].second;
    evhdr.nanosec = T3info->t3station[0].nsec;
    //evhdr.version=DUinfo[EVT_VERSION];
    if(fpout == NULL){
      eb_open(&evhdr);
      n_written = 0;
      for(isub=0;isub<3;isub++) write_sub[isub] = 0;
    }
    fp = fpout;
    evhdr.type = 0;
    if(msg->tag == DU_GET_MINBIAS_EVENT) {
      fp = fpten;
      evhdr.type = TRIGGER_T3_MINBIAS;
    }
    if(msg->tag == DU_GET_RANDOM_EVENT) {
      evhdr.type = TRIGGER_T3_RANDOM;
      fp = fpmb;
    }
    if(n_data != 0){
      fwrite(&evhdr,1,44,fp);
      eb_directory_add(fp, foldername);
      n_written++;
      write_sub[isub]++;
      if(n_written >=eb_max_evts) { // do we still need this?????
        eb_close();
        //printf("EB: after closing  files %d\n",(int) fpout);
        //printf("EB: after closing  files %d\n",(int) fpout);
      }
    }
    t3_read++;
    if(t3_read >=N_T3) t3_read = 0;
    eb_remove_directory(foldername);
  }
  return;
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
  AMSG *msg;
  T3BODY *T3info;
  char foldername[100];

  sprintf(flname,"%s/eb",LOG_FOLDER);
  fp_log = fopen(flname,"w");
  printf("Starting EB\n");
  t3_address[0] = 0; // initialize first address
  while(1) {
    fseek(fp_log,0,SEEK_SET);
    eb_getui();
    eb_gett3();
    if(running == 1) eb_write_events();
    else{
      while(t3_read != t3_write){//clean up!
        msg = (AMSG *) (&t3_buffer[t3_address[t3_read]]);
        T3info = (T3BODY *)(&(msg->body[0])); //set the T3 info pointer
        sprintf(foldername,"%s/%d",LOG_FOLDER,T3info->event_nr);
        eb_remove_directory(foldername);
        t3_read++;
        if(t3_read >=N_T3) t3_read = 0;
      }
    }
    fprintf(fp_log,"%s\n",fname);
    fprintf(fp_log,"CD events: %5d\n",write_sub[0]);
    fprintf(fp_log,"UD events: %5d\n",write_sub[2]);
    fprintf(fp_log,"MD events: %5d\n",write_sub[1]);
    usleep(1000);
  }
  fclose(fp_log);
}
