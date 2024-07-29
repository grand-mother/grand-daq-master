/// @file du.c
/// @brief routines interfacing to the socket
/// @author C. Timmermans, Nikhef/RU
/***
 local Station interfacing
 Author: Charles Timmermans, Nikhef/Radboud University
 
 Altering the code without explicit consent of the author is forbidden
 ***/
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "Adaq.h"
#include "amsg.h"
#include "scope.h"

extern int idebug;
#define MAXLOG 8 //max 8 lines output (monitor!)
char loglines[MAXLOG][80];
uint32_t tbuf[1<<24]; //huge buffer to hold events upto 2 msec!

void coeffs(float Fnotch, float r,unsigned int *values);

#define Reg_Rate 0x1E0
extern int running;
extern int errno;

void du_send(uint32_t *bf,int du);
uint32_t du_read_initfile(int ls,uint32_t *bf);


/*!
 \func du_interpret(uint32_t *buffer)
 \brief interprets the data in the buffer
 \param buf pointer to the data to send
 Copies information in the buffer to either the event-shared memory
 or the t2 shared memory
 */
void du_interpret(uint32_t *buffer)
{
  AMSG *msg;
  int ntry;
  char filename[200];
  uint32_t *DUinfo;
  int32_t i=1;
  struct stat sb;
  FILE *fpt;
  
  while(i<buffer[0]-2){ // 2 tail words
    msg = (AMSG *)(&(buffer[i]));
    if(idebug)
      printf("DU: received message %d First word %d %d %d Length: %d\n",msg->tag,msg->body[0],msg->body[1],msg->body[2],msg->length);
    switch(msg->tag){ //based on tag, data is moved to different servers
      case DU_T2:
        if(msg->length<T2SIZE){
          // First wait until shared memory is no longer full
          ntry = 0;
          while(shm_t2.Ubuf[(*shm_t2.size)*(*shm_t2.next_write)] == 1 && ntry<10) {//infinite loop, possible problem!
            //printf("DU: Wait for T3: %d %d\n",shm_t2.Ubuf[(*shm_t2.size)*(*shm_t2.next_write)],(*shm_t2.next_write));
            ntry++;
            usleep(1000); // wait for the t3maker to be ready
          }
          if(ntry >=10 && shm_t2.Ubuf[(*shm_t2.size)*(*shm_t2.next_write)] == 1){
            printf("DU: No buffer, loosing data\n");
          }else{
            memcpy((void *)&(shm_t2.Ubuf[(*shm_t2.size)*(*shm_t2.next_write)+1]),(void *)msg,INTSIZE*msg->length);
            shm_t2.Ubuf[(*shm_t2.size)*(*shm_t2.next_write)] = 1;
            // update shared memory pointer
            *shm_t2.next_write = *shm_t2.next_write + 1;
            if(*shm_t2.next_write >= *shm_t2.nbuf) *shm_t2.next_write = 0;
          }
        } else{
          printf("DU: Error: Too much T2 information in a single message, data ignored\n");
        }
        break;
      case DU_MONITOR:
      case DU_EVENT:
        if(idebug)
          printf("Received an event\n");
      case DU_NO_EVENT:
        // wait until the shared memory is not full
        DUinfo = (uint32_t *)msg->body;
        /*sprintf(filename,"%s/%d",LOG_FOLDER,DUinfo[EVT_EVT_ID]);
        if (!(stat(filename, &sb) == 0 && S_ISDIR(sb.st_mode))) {
          mkdir(filename,S_IRWXU);
        }*/
        DUinfo[EVT_VERSION] = DUinfo[EVT_VERSION]+((ADAQ_VERSION&0xff)<<8);
        sprintf(filename,"%s/%d/%d",LOG_FOLDER,DUinfo[EVT_EVT_ID],DUinfo[EVT_STATION_ID]);
        fpt = fopen(filename,"w");
        fwrite(DUinfo,1,4*(DUinfo[EVT_LENGTH]>>16),fpt);
        fclose(fpt);
        break;
      case DU_GET:
      case DU_GETMEM:
      case ALIVE_ACK: //not acted upon
        break;
      default:
        printf("DU: Received unknown message from client %d \n",msg->tag);
    }
    if(msg->length == 0) break;
    i+=msg->length; //go to next message
  }
}

/*!
 \func int set_socketoptions(int sock)
 \brief set default socket options
 \param sock socket file descriptor
 */
int set_socketoptions(int sock)
{
  int option;
  struct timeval timeout;
  // set default socket options
  option = SOCKETS_REUSEADDR_OPT;
  if(setsockopt (sock,SOL_SOCKET, SO_REUSEADDR,&option,
                 (socklen_t)(sizeof (int)) )<0) return(ERROR);
  option = SOCKETS_KEEPALIVE_OPT;
  if(setsockopt (sock,SOL_SOCKET, SO_KEEPALIVE,&option,
                 (socklen_t)(sizeof (int)) ) < 0) return(ERROR);
  option = SOCKETS_NODELAY_OPT;
  if(setsockopt (sock,IPPROTO_TCP, TCP_NODELAY, &option,
                 (socklen_t)(sizeof (int)) )<0) return(ERROR);
  option = SOCKETS_BUFFER_SIZE;
  if(setsockopt (sock,SOL_SOCKET, SO_SNDBUF,&option,
                 (socklen_t)(sizeof (int)) )<0) return(ERROR);
  if(setsockopt (sock,SOL_SOCKET, SO_RCVBUF,&option,
                 (socklen_t)(sizeof (int)) )<0) return(ERROR);
  timeout.tv_usec = SOCKETS_TIMEOUT;
  timeout.tv_sec = 0;
  if(setsockopt (sock,SOL_SOCKET, SO_RCVTIMEO,&timeout,
                 (socklen_t)(sizeof (struct timeval)) )<0) return(ERROR);
  timeout.tv_usec = SOCKETS_TIMEOUT;
  timeout.tv_sec = 0;
  if(setsockopt (sock,SOL_SOCKET, SO_SNDTIMEO,&timeout,
                 (socklen_t)(sizeof (struct timeval))  )<0) return(ERROR);
  return(NORMAL);
}

/*!
 \func void du_init_and_run(int il)
 \brief send parameters from config file and start the run
 \param il stationlist index
 */
void du_init_and_run(int il)
{
  int length;
  uint32_t du_cmd[CMDSIZE]; // for other commands
  
  du_cmd[2] = DU_INITIALIZE;
  du_cmd[3] =  DUinfo[il].DUid;
  length = du_read_initfile(DUinfo[il].DUid,&du_cmd[4]);
  if(length>0){
    // First initialize
    du_cmd[4+length] = GRND1;
    du_cmd[5+length] = GRND2;
    du_cmd[0] = 6+length;
    du_cmd[1] = 3+length;
    du_send(du_cmd,il);
    // next start
    du_cmd[0] = 6;
    du_cmd[1] = 3;
    du_cmd[2] = DU_START;
    du_cmd[4] = GRND1;
    du_cmd[5] = GRND2;
    du_cmd[3] = DUinfo[il].DUid;
    du_send(du_cmd,il);
  }
}

/*!
 \func void du_connect()
 \brief connects to the sockets for which this is needed
 loop over all stations, when needed reconnect and initialize and start the run again
 */
void du_connect()
{
  int i,ilog;
  int iret;
  struct timeval tnow;
  struct timezone tzone;
  struct tm *tlocal;
  ssize_t recvRet;
  socklen_t RDalength;
  uint32_t buffer[2];
  char line[800];

  gettimeofday(&tnow,&tzone);
  tlocal = localtime(&tnow.tv_sec);
  ilog = 0;
  sprintf(line,"%d/%d %02d:%02d:%02d Connection to station ",
          tlocal->tm_mday,tlocal->tm_mon+1,tlocal->tm_hour,tlocal->tm_min,tlocal->tm_sec);
  for(i=0;i<tot_du;i++){ // loop over all stations
    if(DUinfo[i].DUsock >= 0) continue; // nothing needs to be done
    ilog = 1;
    sprintf(line,"%s %d",line,DUinfo[i].DUid);
    //1. Create the socket
    DUinfo[i].DUsock =  socket ( PF_INET, SOCK_STREAM, 0 );
    DUinfo[i].LSTconnect = tnow.tv_sec;
    if(DUinfo[i].DUsock < 0 ) {
      continue;//cannot connect, go to the next one
    }
    //2. Set the socket properties
    if(set_socketoptions(DUinfo[i].DUsock) == ERROR){
      shutdown(DUinfo[i].DUsock,SHUT_RDWR);
      close(DUinfo[i].DUsock);
      DUinfo[i].DUsock = -1;
      DUinfo[i].LSTconnect = 0;
      continue;
    }
    DUinfo[i].DUaddress.sin_family= AF_INET;
    DUinfo[i].DUaddress.sin_port= htons (DUinfo[i].DUport);
    DUinfo[i].DUaddress.sin_addr.s_addr= inet_addr(DUinfo[i].DUip);
    DUinfo[i].DUalength = sizeof(DUinfo[i].DUaddress);
    //3. connect if possible
    if(connect(DUinfo[i].DUsock,
               (struct sockaddr*)&DUinfo[i].DUaddress,DUinfo[i].DUalength)< 0){
      shutdown(DUinfo[i].DUsock,SHUT_RDWR);
      close(DUinfo[i].DUsock);
      DUinfo[i].DUsock = -1;
      DUinfo[i].LSTconnect = 0;
      continue;
    }
    //4. flush the socket
    RDalength = DUinfo[i].DUalength;
    while((recvRet = recvfrom(DUinfo[i].DUsock,buffer,2,0,
                              (struct sockaddr*)&DUinfo[i].DUaddress,&RDalength))==2) usleep(10);

    //5. continue the run when needed
    if(running == 1) du_init_and_run(i);
  }
  if(ilog == 1){
    if((i=strlen(line))>=80){
      line[78]='\n';
      line[79] = 0;
    }else{
      line[i]='\n';
      line[i+1] = 0;
    }
    for(ilog=MAXLOG-1;ilog>0;ilog--) strncpy(loglines[ilog],loglines[ilog-1],80);
    strncpy(loglines[0],line,80);
  }
}

/*!
 \func void du_read()
 \brief read data from all sockets
 loop over all DUs
 check if there is a socket connection
 read data into a local buffer (1<<24) ints long!)
 interpret data
 send an ALIVE message every second
 */
void du_read()
{
  int i,ntry;
  int bytesRead,nread;
  ssize_t recvRet,rsend;
  uint8_t *bf = (uint8_t *)tbuf;
  struct timeval tnow;
  struct timezone tzone;
  socklen_t RDalength;
  
  gettimeofday(&tnow,&tzone);
  for(i=0;i<tot_du;i++){
    RDalength = DUinfo[i].DUalength;
    if(DUinfo[i].DUsock < 0) continue;
    // first read length of buffer
    tbuf[0] = 0;
    while((recvRet = recvfrom(DUinfo[i].DUsock,tbuf,INTSIZE,0,
                              (struct sockaddr*)&DUinfo[i].DUaddress,&RDalength))==INTSIZE){
      RDalength = DUinfo[i].DUalength;
      if (tbuf[0] == 0) {
        printf("DU: du_read: The buffer from station %d cannot be handled size=%d\n",DUinfo[i].DUid,tbuf[0]);
        ntry = 0;
        while((recvRet = recvfrom(DUinfo[i].DUsock,tbuf,INTSIZE,0,
                                  (struct sockaddr*)&DUinfo[i].DUaddress,&RDalength))==INTSIZE) {
          if(ntry<10) printf("Data = 0x%08x %d\n",tbuf[0],tbuf[0]);
          ntry++;
          usleep(10);
        }
        continue;
      }
      // read remaining data
      bytesRead = INTSIZE;
      ntry = 0;
      //printf("Socket reading-before loop  %d %d\n",bytesRead,(INTSIZE*tbuf[0]));
      while (bytesRead < (INTSIZE*tbuf[0])) { //size is in ints, including the first word!
        nread = INTSIZE*tbuf[0]-bytesRead;
        //printf("Socket reading %d %d %d\n",bytesRead,(INTSIZE*tbuf[0]),nread);
        errno = 0;
        recvRet = recvfrom(DUinfo[i].DUsock,&bf[bytesRead],
                           nread,0,(struct sockaddr*)&DUinfo[i].DUaddress,&RDalength);
        //printf("After Socket reading %zd %d\n",recvRet,errno);
        RDalength = DUinfo[i].DUalength;
        if(errno == EAGAIN ) {
          if(recvRet>0) {
            bytesRead+=recvRet;
            ntry = 0;
          }else{
            ntry++;
            usleep(10);
          }
          if(ntry == 20) {
            printf("Socket read error %d %d\n",4*tbuf[0],bytesRead);
            tbuf[0] = 0;
            shutdown(DUinfo[i].DUsock,SHUT_RDWR);
            close(DUinfo[i].DUsock);
            DUinfo[i].DUsock = -1;
            DUinfo[i].LSTconnect = 0;
            break;
          }
        }else if (recvRet < 0) {
          printf("DU: du_read: The buffer from station %d cannot be read, shutting down socket\n",DUinfo[i].DUid);
          tbuf[0] = 0;
          shutdown(DUinfo[i].DUsock,SHUT_RDWR);
          close(DUinfo[i].DUsock);
          DUinfo[i].DUsock = -1;
          DUinfo[i].LSTconnect = 0;
          break;
        }else {
          if(recvRet>0){
            bytesRead += recvRet;
            ntry = 0;
          }else{
            ntry++;
          }
          usleep(10);
        }
      } // while read data
      if(tbuf[0] > 0 &&bytesRead == INTSIZE*(tbuf[0])&&recvRet>0) {
        du_interpret(tbuf);
      }else{
        printf("DU Error in Receive %d %d %ld\n",bytesRead,4*tbuf[0],recvRet);
      }
      DUinfo[i].LSTconnect = tnow.tv_sec;
    }
    if(errno != EAGAIN && recvRet<0){
      printf("DU: Problem with connection to station %d Error = %d (%s)\n",DUinfo[i].DUid,errno,strerror(errno));
      shutdown(DUinfo[i].DUsock,SHUT_RDWR);
      close(DUinfo[i].DUsock);
      DUinfo[i].DUsock = -1;
      DUinfo[i].LSTconnect = 0;
    }
    // send an ALIVE message if the latest event was more than 1 sec ago!
    if((tnow.tv_sec-DUinfo[i].LSTconnect) > 1){
      tbuf[0] = 6;
      tbuf[1] = 3;
      tbuf[2] = ALIVE;
      tbuf[3] = DUinfo[i].DUid;
      tbuf[4] = GRND1;
      tbuf[5] = GRND2;
      //printf("Sending ALIVE\n");
      rsend = sendto(DUinfo[i].DUsock,tbuf,INTSIZE*tbuf[0], 0,(struct sockaddr*)&DUinfo[i].DUaddress,
                     DUinfo[i].DUalength);
      if(rsend<0 && errno != EAGAIN ) {
        shutdown(DUinfo[i].DUsock,SHUT_RDWR);
        close(DUinfo[i].DUsock);
        DUinfo[i].DUsock = -1;
        DUinfo[i].LSTconnect = 0;
        printf("DU: (sending alive) Socket to station %d has died Error = %d\n",DUinfo[i].DUid,errno);
      } else
        DUinfo[i].LSTconnect = tnow.tv_sec;
    }
  }
}

/*!
 \func void du_send(uint32_t *bf,int ls)
 \brief write data to socket
 */
void du_send(uint32_t *bf,int du)
{
  ssize_t rsend;
  int sentbytes,length;
  int ntry;
  struct timeval tnow;
  struct timezone tzone;
  
  gettimeofday(&tnow,&tzone);
  
  sentbytes = 0;
  length = INTSIZE*bf[0];
  
  ntry = 0;
  while(sentbytes<length){
    rsend = sendto(DUinfo[du].DUsock,bf,length-sentbytes, 0,(struct sockaddr*)&DUinfo[du].DUaddress,
                   DUinfo[du].DUalength);
    if(rsend<0 && errno != EAGAIN) {
      sentbytes = rsend;
      break;
    }
    if(rsend>0) sentbytes +=rsend;
    if(sentbytes<length) usleep(10);
    if((ntry++)>100) break; //at most 100 loops
  }
  if(sentbytes != length)
    printf("DU: Sending ERROR %d bytes sent for %d required on socket %d\n",sentbytes,length,DUinfo[du].DUsock);
  if(rsend<0 && errno != EAGAIN) {
    shutdown(DUinfo[du].DUsock,SHUT_RDWR);
    close(DUinfo[du].DUsock);
    DUinfo[du].DUsock = -1;
    DUinfo[du].LSTconnect = 0;
    printf("DU: Send Socket to station %d has died\n",DUinfo[du].DUid);
  }else{
    DUinfo[du].LSTconnect = tnow.tv_sec;
  }
}

/*!
 \func uint32_t du_read_initfile(int ls,uint32_t *bf)
 \brief reads the configuration file. Also calculates the filter parameters from the input file!
 note: does not check the size of the buffer (bf)!
 */
uint32_t du_read_initfile(int ls,uint32_t *bf)
{
#define XTRA_PIPE 4
  uint32_t rcode = 0;
  FILE *fp;
  char line[200];
  char fname[200];
  int axi,reg,val;
  int i;
  double freq,width;
  unsigned int a[5];
  
  sprintf(fname,"conf/DU/grand_%03d",ls);
  fp = fopen(fname,"r");
  if(fp == NULL) {
    fp = fopen("conf/DU.conf","r");
    if(fp == NULL) {
      printf("There is no configuration file for DU %d\n",ls);
      return(rcode);
    }
  }
  while(fgets(line,199,fp) == line){
    if(line[0]==0 || line[0] == '#') continue;
    if(sscanf(line,"%d 0x%x 0x%x",&axi,&reg,&val) == 3){
      if(axi < 25 || reg == Reg_Rate){ // normal registers
        bf[rcode++] = (reg<<16)+(axi&0xffff);
        bf[rcode++] = val;
      }
    }else{// filter
      if(sscanf(line,"%d 0x%x %lg %lg",&axi,&reg,&freq,&width) == 4){
        coeffs(freq, width,a);
        printf("%x %x %x %x %x\n",a[0],a[1],a[2],a[3],a[4]);
        bf[rcode++] = (reg<<16)+(axi&0xffff);
        bf[rcode++] = a[0];
        bf[rcode++] = ((reg+4)<<16)+((axi+1)&0xffff);;
        bf[rcode++] = a[1];
        bf[rcode++] = ((reg+8)<<16)+((axi+2)&0xffff);;
        bf[rcode++] = a[2];
        bf[rcode++] = ((reg+12)<<16)+((axi+3)&0xffff);;
        bf[rcode++] = a[3];
        bf[rcode++] = ((reg+16)<<16)+((axi+4)&0xffff);;
        bf[rcode++] = a[4];
      }
    }
  }
  fclose(fp);
  printf("code After initfile: %d\n",rcode);
  return(rcode);
}

/*!
 \func void du_write()
 \brief write
 */
void du_write()
{
  AMSG *msg;
  T3BODY *T3info;
  uint32_t get_t3event[9]={9,6,DU_GETEVENT,0,0,0,0,GRND1,GRND2}; //last word will be magic at some point
  uint32_t du_cmd[CMDSIZE]; // for other commands
  int n_t3_du,it3;
  unsigned int ssec;
  int il,length;
  
  // read t3 request from memory and send to DU
  while((shm_t3.Ubuf[(*shm_t3.size)*(*shm_t3.next_read)])  !=  0){ // loop over the T3 input
    if(((shm_t3.Ubuf[(*shm_t3.size)*(*shm_t3.next_read)]) &1) ==  1) {
      msg = (AMSG *)(&(shm_t3.Ubuf[(*shm_t3.size)*(*shm_t3.next_read)+1]));
      T3info = (T3BODY *)(&(msg->body[0])); //set the T3 info pointer
      n_t3_du = (msg->length-3)/T3STATIONSIZE; // msg == length+tag+eventnr+T3stations
      for(it3=0;it3<n_t3_du;it3++){ // loop over all stations in T3 list
        //printf("DU: Need to request a T3 %d %d %d\n",*shm_t3.next_read,
        //       T3info->t3station[it3].DU_id,T3info->t3station[it3].index);
        for(il=0;il<tot_du;il++){ //loop over all DU in the DAQ
          if(T3info->t3station[it3].DU_id== 0 || T3info->t3station[it3].DU_id == DUinfo[il].DUid){
            // request event from station
            get_t3event[2] =  msg->tag;
            get_t3event[3] =  DUinfo[il].DUid;
            get_t3event[4] =  T3info->event_nr;
            get_t3event[5] =  T3info->t3station[it3].index;
            //printf("DU: Requesting a T3 %d %d %d\n",get_t3event[3],get_t3event[4],get_t3event[5]);
            du_send(get_t3event,il);
          }
        }
      }
      //if(loglevel >=3) printf("DU: Done sending T3 request\n");
      shm_t3.Ubuf[(*shm_t3.size)*(*shm_t3.next_read)] &= ~1;
    }
    *shm_t3.next_read = (*shm_t3.next_read) + 1;
    if( *shm_t3.next_read >= *shm_t3.nbuf) *shm_t3.next_read = 0;
  }
  // and the same for the command line
  while((shm_cmd.Ubuf[(*shm_cmd.size)*(*shm_cmd.next_read)])  !=  0){ // loop over the UI input
    if(((shm_cmd.Ubuf[(*shm_cmd.size)*(*shm_cmd.next_read)]) &1) ==  1){ // loop over the UI input
      msg = (AMSG *)(&(shm_cmd.Ubuf[(*shm_cmd.size)*(*shm_cmd.next_read)+1]));
      if(idebug) printf("DU: sending commandline command  %d with length %d\n",msg->tag,msg->length);
      if(msg->tag == DU_START) running = 1;
      if(msg->tag == DU_STOP) running = 0;
      if(msg->tag == DU_STOP || msg->tag == DU_START){
        du_cmd[0] = 6;
        du_cmd[1] = 3;
        du_cmd[2] = msg->tag;
        du_cmd[4] = GRND1;
        du_cmd[5] = GRND2;
        if(idebug) printf("DU: Changing run %d\n",msg->tag);
        for(il=0;il<tot_du;il++){
          du_cmd[3] = DUinfo[il].DUid;
          du_send(du_cmd,il);
        }
      } else if(msg->tag == DU_INITIALIZE){
        for(il=0;il<tot_du;il++){
          if(msg->body[0] == DUinfo[il].DUid || msg->body[0] == 0){ // go for initialization
            du_cmd[2] = DU_INITIALIZE;
            du_cmd[3] =  DUinfo[il].DUid;
            length = du_read_initfile(DUinfo[il].DUid,&du_cmd[4]);
            if(length>0){
              du_cmd[4+length] = GRND1;
              du_cmd[5+length] = GRND2;
              du_cmd[0] = 6+length;
              du_cmd[1] = 3+length;
              du_send(du_cmd,il);
            }
          }
        }
      }
      shm_cmd.Ubuf[(*shm_cmd.size)*(*shm_cmd.next_read)] &= ~1;
    }
    *shm_cmd.next_read = (*shm_cmd.next_read) + 1;
    if( *shm_cmd.next_read >= *shm_cmd.nbuf) *shm_cmd.next_read = 0;
  }
}

/*!
 \func void du_main()
 \brief main steering routine for the socket handling
 ignore SIGPIPE
 connecting to all detector units
 read from detector units
 write to detector units
 connect to all
 */
void du_main()
{
  int i;
  struct sigaction svec;
  char fname[100];
  FILE *fp_log;
 
  
  svec.sa_handler = SIG_IGN;
  sigemptyset(&svec.sa_mask);
  svec.sa_flags = 0;
  sigaction(SIGPIPE,&svec,NULL);
  for(i=0;i<tot_du;i++) {
    DUinfo[i].DUsock = -1; // all sockets need connecting!
    DUinfo[i].LSTconnect = 0;
  }
  sprintf(fname,"%s/du",LOG_FOLDER);
  fp_log = fopen(fname,"w");
  du_connect();
  fclose(fp_log);
  while(1) {
    usleep(100);
    du_read();
    du_write();
    du_connect(); // perform regular reconnection attempts
    //fp_log = freopen(fname,"w",fp_log);
    for(i=0;i<MAXLOG;i++)fputs(loglines[i],fp_log);
    //fflush(fp_log);
    //fclose(fp_log);
  }
}
