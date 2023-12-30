/// @file dudaq.c
/// @brief steering file for the Detector Unit software
/// @author C. Timmermans, RU/Nikhef
/// @date 30/12/2023

#include <stdio.h>
#include <stdlib.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include<net/if.h>
#include <arpa/inet.h>
#include <signal.h>
#include<errno.h>
#include"dudaq.h"
#include "amsg.h"
#include "scope.h"
#include "ad_shm.h"


int send_t3_event(int t3_id, int index, int evttype);

shm_struct shm_ev; //!< shared memory containing event indices, including read/write pointers
shm_struct shm_gps; //!< shared memory containing all GPS indices, including read/write pointers
shm_struct shm_cmd; //!< shared memory containing all command info, including read/write pointers
extern int errno; //!< the number of the error encountered
extern uint32_t *vadd_psddr;



int du_port;       //!<port number on which to connect to the central daq

int run=0;                //!< are you running!

int station_id  = -1;           //!< id of the LS, obtained from the ip address

fd_set sockset;           //!< socket series to be manipulated

uint32_t DU_output[MAX_OUT_MSG];  //!< memory used to store the data to be sent to the central DAQ
uint32_t DU_input[MAX_INP_MSG];   //!< memory in which to receive the socket data from the central DAQ

int32_t DU_socket = -1;   //!< main socket for accepting connections
struct sockaddr_in  DU_address; //!< structure containing address information of socket
socklen_t DU_alength;           //!< length of sending socket address
socklen_t RD_alength;           //!< length of receiving socket address
int32_t DU_comms = -1;    //!< socket for the accepted connection

unsigned int prev_mask; //!< old signal mask
unsigned int mask;      //!< signal mask

pid_t pid_scope;        //!< process id of the process reading/writing the fpga
pid_t pid_socket;       //!< process id of the process reading/writing to the central DAQ
uint8_t stop_process=0; //!< after an interrupt this flag is set so that all forked processes are killed


/*!
 * \fn remove_shared_memory()
 * \brief deletes all shared memories
 * \author C. Timmermans
*/
void remove_shared_memory()
{
    ad_shm_delete(&shm_ev);
    ad_shm_delete(&shm_gps);
    ad_shm_delete(&shm_cmd);
}

/*!
\fn void clean_stop(int signum)
* \brief kill processes
* \param signum input signal (not used)
* \author C. Timmermans
 * kill child processes and perform a clean stop to clean up shared memory
*/
void clean_stop(int signum)
{
    remove_shared_memory();
    stop_process = 1;
    kill(pid_scope,9);
    kill(pid_socket,9);
}

/*!
 \fn void block_signals(int iblock)
 * \brief block sigio/sigpipe
 * \retval 0 ok
 * \param iblock When 1, block signals. Otherwise release block
 * \author C. Timmermans
 */
void block_signals(int iblock)
{
    if(iblock == 1){
        mask = ((1<<SIGIO)+ (1<<SIGPIPE));
        sigprocmask(SIG_BLOCK,(sigset_t *)&mask,(sigset_t *)&prev_mask);
        //    printf("Blocking %x %x\n",mask,prev_mask);
    }else{
        mask = prev_mask;
        sigprocmask(SIG_SETMASK,(sigset_t *)&mask,(sigset_t *)&prev_mask);
    }
}

/*!
 \fn int set_socketoptions(int sock)
 * \brief set default socket options
 * \retval 0 ok
 * \param sock socket id
 * \author C. Timmermans
 */
int set_socketoptions(int sock)
{
    int option;
    struct timeval timeout;
    // set default socket options
    option = SOCKETS_REUSEADDR_OPT;
    if(setsockopt (sock,SOL_SOCKET, SO_REUSEADDR,&option,sizeof (int) )<0) return(-1);
    option = SOCKETS_KEEPALIVE_OPT;
    if(setsockopt (sock,SOL_SOCKET, SO_KEEPALIVE,&option,sizeof (int) ) < 0) return(-1);
    option = SOCKETS_NODELAY_OPT;
    if(setsockopt (sock,IPPROTO_TCP, TCP_NODELAY, &option,sizeof (int) )<0) return(-1);
    option = SOCKETS_BUFFER_SIZE;
    if(setsockopt (sock,SOL_SOCKET, SO_SNDBUF,&option,sizeof (int) )<0) return(-1);
    if(setsockopt (sock,SOL_SOCKET, SO_RCVBUF,&option,sizeof (int) )<0) return(-1);
    timeout.tv_usec = SOCKETS_TIMEOUT;
    timeout.tv_sec = 0;
    if(setsockopt (sock,SOL_SOCKET, SO_RCVTIMEO,&timeout,sizeof (struct timeval) )<0) return(-1);
    timeout.tv_usec = SOCKETS_TIMEOUT;
    timeout.tv_sec = 0;
    if(setsockopt (sock,SOL_SOCKET, SO_SNDTIMEO,&timeout,sizeof (struct timeval) )<0) return(-1);
    return(0);
}

/*!
 \fn int make_server_connection(int port)
 * \brief connect to central DAQ
 * \retval -1 Error
 * \retval 0 ok
 * \param port socket port id
 * - Create a socket
 * - Bind this socket
 * - Listen on this socket
 * - Accept a connection on this socket
 * - set default socket options
 * \author C. Timmermans
 */
int make_server_connection(int port)
{
  int ntry;
  
  printf("Trying to connect to the server\n");
  // Create the socket
  //DU_socket =  socket ( PF_INET, SOCK_DGRAM, 0 );
  DU_socket =  socket ( PF_INET, SOCK_STREAM, 0 );
  if(DU_socket < 0 ) {
    printf("make server connection: Cannot create a socket\n");
    return(-1);
  }
  set_socketoptions(DU_socket);
  DU_address.sin_family       = AF_INET;
  DU_address.sin_port         = htons (port);
  DU_address.sin_addr.s_addr  = htonl ( INADDR_ANY );
  DU_alength = sizeof(DU_address);
  if (bind ( DU_socket, (struct sockaddr*)&DU_address,DU_alength) < 0)  {
    shutdown(DU_socket,SHUT_RDWR);
    close(DU_socket);
    DU_socket = -1;
    printf("make server connection: bind does not work\n");
    return(-1);
  }
  if (listen (DU_socket, 1 ) <0) {
    shutdown(DU_socket,SHUT_RDWR);
    close(DU_socket);
    DU_socket = -1;
    printf("make server connection: listen does not work\n");
    return(-1);
  }
  DU_comms = -1;
  ntry = 0;
  while(DU_comms < 0 && ntry <10000){
    DU_comms = accept(DU_socket, (struct sockaddr*)&DU_address,&DU_alength);
    if(DU_comms <0){
      if(errno != EWOULDBLOCK && errno != EAGAIN) {
        printf("Return an error\n");
        return(-1); // an error if socket is not non-blocking
      }
    }else{
      FD_ZERO(&sockset);
      FD_SET(DU_comms, &sockset);
    }
    ntry++;
  }
  if(DU_comms < 0) return(-1);
  set_socketoptions(DU_comms);
  return(0); // all ok
}

/*!
\fn int check_server_data()
* \brief check and execute commands from central DAQ
* \retval -1 Error
* \retval 0 No new message
* \retval >0 Length of message
* - Checks if there is a message on the socket
*   - on error: shut down the connection
* - read data until all is read or there is an error
*   - on error shut down socket connection
* - interpret messages:
 * - DU_START/DU_STOP set run value to 1/0
*   - DU_INITIALIZE is stored in shared memory for interpretation by scope_check_commands()
*   - DU_GETEVENT DU_GET_MINBIAS_EVENT DU_GET_RANDOM_EVENT moves event to the central DAQ
*   - ALIVE responds with ACK_ALIVE to central DAQ
*
* \author C. Timmermans
*/

int check_server_data()
{
  uint32_t *msg_start;
  uint32_t msg_tag, msg_len;
  uint32_t ackalive[6]={6,3,ALIVE_ACK,station_id,GRND1,GRND2};
  int i;
  int32_t bytesRead,recvRet,ntry;
  unsigned char *bf = (unsigned char *)DU_input;
  struct timeval timeout;
  
  //printf("Checking Server data\n");
  timeout.tv_sec = 0;
  timeout.tv_usec=10;
  if(DU_comms>=0){
    FD_ZERO(&sockset);
    FD_SET(DU_comms, &sockset);
    i= select(DU_comms+1, &sockset, NULL, NULL, &timeout);
    if (i < 0){
      printf("Check server: shutting down comms\n");
      // You have an error
      shutdown(DU_comms,SHUT_RDWR);
      close(DU_comms);
      DU_comms = -1;
    }
  }
  if(i ==0) return(0);
  if(DU_comms<0){
    if(DU_socket < 0){
      if(make_server_connection(du_port)<0)
        return(-1);
    }
    DU_comms = accept(DU_socket, (struct sockaddr*)&DU_address,&DU_alength);
    if(DU_comms <0){
      if(errno != EWOULDBLOCK && errno != EAGAIN) return(-1); // an error if socket is not non-blocking
      return(0);
    }
    usleep(1000);
    FD_ZERO(&sockset);
    FD_SET(DU_comms, &sockset);
    set_socketoptions(DU_comms);
    usleep(1000);
  }
  //printf("There is data\n");
  DU_input[0] = 0;
  //printf("reading server data\n");
  while(DU_input[0] <= 1){
    DU_input[0] = -1;
    RD_alength = DU_alength;
    if((recvRet = recvfrom(DU_comms, DU_input,INTSIZE,MSG_WAITALL,(struct sockaddr*)&DU_address,&RD_alength))==INTSIZE){ //read the buffer size
      if ((DU_input[0] < 0) || (DU_input[0]>MAX_INP_MSG)) {
        printf("Check Server Data: bad buffer size when receiving socket data (%d shorts)\n", DU_input[0]);
        shutdown(DU_comms,SHUT_RDWR);
        close(DU_comms);
        DU_comms = -1;
        return(-1);
      }
      bytesRead = INTSIZE;
      ntry = 0;
      while (bytesRead < (INTSIZE*DU_input[0])&&ntry<1000) {
        RD_alength = DU_alength;
        recvRet = recvfrom(DU_comms,&bf[bytesRead],
                           INTSIZE*DU_input[0]-bytesRead,0,(struct sockaddr*)&DU_address,&RD_alength);
        if(recvRet>0) ntry=0;
        else ntry++;
        if(errno == EAGAIN && recvRet<0) continue;
        if (recvRet <= 0) {
          printf("Check Server Data: connection died when receiving socket data\n");
          shutdown(DU_comms,SHUT_RDWR);
          close(DU_comms);
          DU_comms = -1;
          return(-1);
        }
        bytesRead += recvRet;
      } // while read data
    }else{
      printf("Check Server Data: connection died before getting data\n");
      shutdown(DU_comms,SHUT_RDWR);
      close(DU_comms);
      DU_comms = -1;
      return(-1);
    }
  }
  
  i = 1;
  //printf("Message length = %d\n",DU_input[0]);
  while(i<DU_input[0]-2){
    msg_start = &(DU_input[i]);
    msg_len = msg_start[AMSG_OFFSET_LENGTH];
    msg_tag = msg_start[AMSG_OFFSET_TAG];
    //printf("%d %d\n",msg_len,msg_tag);
    if (msg_len < 2) {
      printf("Error: message is too short (no tag field)!\n");
      break;
    }
    if (msg_len > MAX_INP_MSG) {
      printf("Error: message is too long: %d\n",msg_len);
      break;
    }
    //printf("Received message %d\n",msg_tag);
    switch(msg_tag){
      case DU_START:
        run = 1;
        break;
      case DU_STOP:
        run = 0;
        break;
      case DU_INITIALIZE:
        while(shm_cmd.Ubuf[(*shm_cmd.size)*(*shm_cmd.next_write)] == 1) {
          usleep(1000); // wait for the scope to read the shm
        }
        memcpy((void *)&(shm_cmd.Ubuf[(*shm_cmd.size)*(*shm_cmd.next_write)+1]),(void *)msg_start,INTSIZE*msg_start[AMSG_OFFSET_LENGTH]);
        shm_cmd.Ubuf[(*shm_cmd.size)*(*shm_cmd.next_write)] = 1;
        *shm_cmd.next_write = *shm_cmd.next_write + 1;
        if(*shm_cmd.next_write >= *shm_cmd.nbuf) *shm_cmd.next_write = 0;
        break;
      case DU_GETEVENT:                    // calculate sec and subsec, move the event to the t3 buffer
      case DU_GET_MINBIAS_EVENT:                    // calculate sec and subsec, move the event to the t3 buffer
      case DU_GET_RANDOM_EVENT:                    // calculate sec and subsec, move the event to the t3 buffer
        if (msg_len == AMSG_OFFSET_BODY + 3 + 1) {
          //printf("request evt %d %d %d\n",msg_start[2],msg_start[3],msg_start[4]);
          send_t3_event(msg_start[3],msg_start[4],msg_tag);
        }
        else
          printf("Error: message DU_GETEVENT was wrong length (%d)\n", msg_len);
        break;
      case ALIVE:
        sendto(DU_comms, ackalive,sizeof(ackalive), 0,(struct sockaddr*)&DU_address,DU_alength);
        break;
      default:
        printf("Received unknown message %d\n",msg_tag);
    }
    i+=msg_len;
  } // End loop over input buffer
  if(msg_len<1) return(1);
  return(msg_len);
}
/*!
 \fn int socket_send(char *bf, int length)
 * \brief send buffer to the central DAQ
 * \retval number of bytes sent
 * try to send a buffer bf of size length over the socket to the central DAQ. Up to 100 tries are made
 *
 * \author C. Timmermans
 */
int socket_send(char *bf, int length)
{
  int bsent,rsend;
  int sentBytes = 0;
  int ntry = 0;
  while(sentBytes<length && ntry < 100){
    bsent = length-sentBytes;
    rsend = sendto(DU_comms, &(bf[sentBytes]),bsent, 0,
                   (struct sockaddr*)&DU_address,DU_alength);
    if(rsend<0 && errno != EAGAIN) {
      printf("Sending event failed %d %d %s\n",length,sentBytes,strerror(errno));
      sentBytes = rsend;
      break;
    }
    if(rsend>0) {
      sentBytes +=rsend;
      ntry = 0;
    } else{
      ntry++;
      usleep(20);
    }
  } //while sentbytes
  return(sentBytes);
}

/*!
 \fn int send_server_data()
 * \brief send t2 data to the central DAQ
 * \retval -1 Error
 * \retval 0 No connection to DAQ
 * \retval 1 All ok
 * - If there is no connection to the central DAQ, try to connect
 * - get T2 information from DDR and add to output buffer
 * - send output buffer to the central DAQ
 *
 * \author C. Timmermans
 */
int send_server_data(){
  char *bf = (char *)DU_output;
  int32_t length;
  DU_alength = sizeof(DU_address);
  
  int ievt,imsg,iten;
  uint32_t *evt0,*evt1;
  
  if(DU_comms< 0){
    if(DU_socket < 0){
      if(make_server_connection(du_port)<0)
        return(-1);
    }
    DU_comms = accept(DU_socket, (struct sockaddr*)&DU_address,&DU_alength);
    if(DU_comms <0){
      if(errno != EWOULDBLOCK && errno != EAGAIN) return(-1); // an error if socket is not non-blocking
      return(0);
    }
    FD_ZERO(&sockset);
    FD_SET(DU_comms, &sockset);
    set_socketoptions(DU_comms);
  }
  ievt=*(shm_ev.next_read);
  if(ievt != *(shm_ev.next_write)){
    imsg = 1;
    DU_output[imsg++] = 0;
    DU_output[imsg++] = DU_T2;
    evt0 =&vadd_psddr[shm_ev.Ubuf[ievt]];
    DU_output[imsg++] = station_id;
    //printf("Time: %u %x %d\n",evt0[EVT_SECOND],evt0[EVT_SECOND],ievt);
    DU_output[imsg++] = evt0[EVT_SECOND];
    do {
      evt1 =&vadd_psddr[shm_ev.Ubuf[ievt]];
      if(evt1[EVT_SECOND] == evt0[EVT_SECOND]){
        DU_output[imsg++] = evt1[EVT_NANOSEC];
        iten = 0;
        if((evt1[EVT_TRIGGER_STAT]&0x100)!= 0) iten+=4;
        if((evt1[EVT_TRIGGER_STAT]&0x7f)!= 0) iten+=2;
        DU_output[imsg++] = (ievt<<16)+(iten<<4);
        ievt++;
        if(ievt >= *shm_ev.nbuf) ievt = 0;
      }
    }while (ievt != *(shm_ev.next_write) &&imsg<(MAX_T2-4) && evt1[EVT_SECOND] == evt0[EVT_SECOND]); //each T2 adds 4 words
    *(shm_ev.next_read) = ievt;
    DU_output[imsg++] = GRND1;
    DU_output[imsg] = GRND2;
    DU_output[1] = imsg-1;
    DU_output[0] = imsg+1;
    
    length = INTSIZE*DU_output[0];
    if(run == 1) socket_send((char *)bf,length);
  }
  usleep(100);
  return(1);
}

/*!
 \fn int send_t3_event()
 * \brief send stored T3 event to the central DAQ
 * \retval -1 Error
 * \retval 0 No connection to DAQ
 * \retval 1 All ok
 * - If there is no connection to the central DAQ, try to connect
 * - Check if the link given by the central DAQ corresponds to a valid event
 * - set the appropriate T3 trigger flag
 * - create and send header of event message
 * - send the event data
 * - send the tail of the message
 *
 * \author C. Timmermans
 */

int send_t3_event(int t3_id, int index, int evttype)
{
  int32_t length;
  short trflag;
  DU_alength= sizeof(DU_address);
  
  if(DU_comms< 0){
    if(DU_socket < 0){
      if(make_server_connection(du_port)<0)
        return(-1);
    }
    DU_comms = accept(DU_socket, (struct sockaddr*)&DU_address,&DU_alength);
    if(DU_comms <=0){
      if(errno != EWOULDBLOCK && errno != EAGAIN) return(-1); // an error if socket is not non-blocking
      return(0);
    }
    FD_ZERO(&sockset);
    FD_SET(DU_comms, &sockset);
    set_socketoptions(DU_comms);
  }
  uint32_t *evt = &vadd_psddr[shm_ev.Ubuf[index]];
  if((evt[EVT_LENGTH]&0xffff)!= EVT_HDR_LENGTH){
    printf("Error requesting event. Not a proper pointer %08x ",vadd_psddr[shm_ev.Ubuf[index]]);
    if(index> 0) printf("( %08x ",vadd_psddr[shm_ev.Ubuf[index-1]]);
    else  printf("( %08x ",vadd_psddr[shm_ev.Ubuf[BUFSIZE-1]]);
    if(index < (BUFSIZE-1)) printf( " %08x)\n",vadd_psddr[shm_ev.Ubuf[index+1]]);
    else printf( " %08x)\n",vadd_psddr[shm_ev.Ubuf[0]]);
    return(0);
  }
  evt[EVT_EVT_ID] = t3_id;
  trflag = 0;
  if(evttype == DU_GET_MINBIAS_EVENT)trflag = TRIGGER_T3_MINBIAS;
  if(evttype == DU_GET_RANDOM_EVENT)trflag = TRIGGER_T3_RANDOM;
  length =evt[EVT_LENGTH]>>16;
  evt[EVT_TRIGGER_STAT] = ((trflag&0xffff)<<16)+(evt[EVT_TRIGGER_STAT]&0xffff);
  if(run == 1){
    DU_output[1] = length+2; //event + 2 words (length+tag)
    DU_output[2] = DU_EVENT; // tag
    DU_output[0] = DU_output[1]+3; //add 2 tail words, and length word
    //printf("Sending T3 event %d\n",DU_output[1]);
    socket_send((char *)DU_output,3*INTSIZE);
    socket_send((char *)evt,length*INTSIZE);
    //memcpy(&DU_output[3],evt,length*sizeof(uint32_t));
    //printf("Send_T3 %d %d %x %x\n",DU_output[1],DU_output[2],DU_output[3],DU_output[4]);
    DU_output[0] = GRND1;
    DU_output[1] = GRND2;
    socket_send((char *)&DU_output[0],2*INTSIZE);
    //length = 2*DU_output[0]+2; //next we need the length in bytes
  }
  return(1);
}

/*!
 \fn void du_scope_check_commands()
 * \brief interpreting DAQ commands for fpga
 * - while there is a command in shared memory
 *    - get the command tag
 *      - on DU_BOOT, DU_RESET and DU_INITIALIZE 
 *                 - creates file /Dudef.sh with all required devmem commands to initialize the fpga
 *                 - sources this file
 *                 - file is also used when booting the board
 *      - no other commands are implemented here
 *    - remove "to be executed" mark for this command
 *    - go to next command in shared memory
 *
 * \author C. Timmermans
 */

void du_scope_check_commands()
{
  uint32_t *msg_start;
  uint32_t msg_tag,msg_len,addr;
  uint32_t il;
  FILE *fp;
  
  //printf("Check cmds %d\n",*shm_cmd.next_read);
  while(((shm_cmd.Ubuf[(*shm_cmd.size)*(*shm_cmd.next_read)]) &1) ==  1){ // loop over the T3 input
    //printf("Received command\n");
    msg_start = (uint32_t *)(&(shm_cmd.Ubuf[(*shm_cmd.size)*(*shm_cmd.next_read)+1]));
    msg_len = msg_start[AMSG_OFFSET_LENGTH];
    msg_tag = msg_start[AMSG_OFFSET_TAG];
    
    switch(msg_tag){
      case DU_BOOT:
      case DU_RESET:
      case DU_INITIALIZE:
        il = 2;
        fp = fopen("/DUdef.sh","w");
        fprintf(fp,"#/bin/sh\n");
        while(il<(msg_len-2)){ 
          addr = msg_start[il+1]>>1;
          //if((addr&1))addr -=1;
          //else addr+=1;
          //if(msg_start[il+1]<127) sl[addr] = msg_start[il+2];
          fprintf(fp,"devmem 0x%x 32 0x%08x\n",0x80000000+(msg_start[il+1]>>16),msg_start[il+2]);
          fprintf(fp,"sleep 0.1\n");
          il+=2;
        }
        fclose(fp);
        system("source /DUdef.sh\n"); 
        sleep(5);
        printf("Initializing scope\n");
        scope_initialize(); // resets and initialize scope
        break;
      default:
        printf("Received unimplemented message %d\n",msg_tag);
    }
    shm_cmd.Ubuf[(*shm_cmd.size)*(*shm_cmd.next_read)] &= ~1;
    *shm_cmd.next_read = (*shm_cmd.next_read) + 1;
    if( *shm_cmd.next_read >= *shm_cmd.nbuf) *shm_cmd.next_read = 0;
  }
}

/*!
 \fn void du_scope_main()
 * \brief main fpga handling routine
 *
 * - continuous while loop
 *    - when running:
 *      - read data,
 *      - check if there has been a command from the DAQ
 *
 * \author C. Timmermans
 */

void du_scope_main()
{
  int i;
  
  while(stop_process == 0){
    if((i =scope_read()) < 0){
      //scope_flush();
      printf("Error reading scope %d\n",i);  // read out the scope
    }
    du_scope_check_commands();
  }
}


/*!
 \fn void du_get_station_id()
 * Obtains station id from the ip address of the machine.
 * The ip address can be obtained from the eth0 interface
 * If the network address does not start with 10., the routine returns -1
 *
 * \author C. Timmermans
 */
void du_get_station_id()
{
  unsigned char ip_address[15];
  int fd;
  struct ifreq ifr;
  
  FILE *fpn;
  char line[100],wrd[20];
  int n1,n2,n3,n4;
  
  station_id = -1;
  
  /*AF_INET - to define network interface IPv4*/
  /*Creating soket for it.*/
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  /*AF_INET - to define IPv4 Address type.*/
  ifr.ifr_addr.sa_family = AF_INET;
  /*eth0 - define the ifr_name - port name
   where network attached.*/
  memcpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);
  /*Accessing network interface information by
   passing address using ioctl.*/
  ioctl(fd, SIOCGIFADDR, &ifr);
  /*closing fd*/
  close(fd);
  strcpy((char *)ip_address, inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr));
  
  printf("System IP Address is: %s\n", ip_address);
  if(sscanf((char *)ip_address,"%d.%d.%d.%d",&n1,&n2,&n3,&n4) == 4){
    station_id = n4;
  }
  if(n1 != 10) station_id = -1; //dangerous!
  if(station_id ==0) station_id = -1;
  printf("Read station %d\n",station_id);
  return;
  fpn = fopen("/etc/network/interfaces","r");
  if(fpn == NULL) return;
  while(line == fgets(line,199,fpn)){
    if(sscanf(line,"%s %d.%d.%d.%d",wrd,&n1,&n2,&n3,&n4) == 5){
      if(strncmp(wrd,"address",7) == 0){
        station_id = n4;
        break;
      }
    }
  }
  fclose(fpn);
  if(station_id ==0) station_id = -1;
  printf("Read station %d\n",station_id);
}

/*!
 \fn void du_socket_main()
 * \brief Handles all communication with the IP socket.
 * - Opens a connection to the central DAQ
 *  - In an infinite loop
 *     - Every 0.2 seconds
 *        - Check commands from the central DAQ
 *       - send T2-data to the central DAQ
 *     - If there has not been contact for 13 seconds
 *       - close and reopen the socket
 *
 * \author C. Timmermans
 */
void du_socket_main()
{
  int du_port=DU_PORT;
  struct timeval tprev,tnow,tcontact;
  struct timezone tzone;
  float tdif,tdifc;
  int prev_msg = 0;

  printf("Opening Connection %d\n",du_port);
  if(make_server_connection(du_port) < 0) {  // connect to DAQ
    printf("Cannot open sockets\n");
    exit(-1);
  }
  printf("Connection opened\n");
  gettimeofday(&tprev,&tzone);
  tcontact.tv_sec = tprev.tv_sec;
  tcontact.tv_usec = tprev.tv_usec;
  while(stop_process == 0){
    //printf("In the main loop\n");
    gettimeofday(&tnow,&tzone);
    tdif = (float)(tnow.tv_sec-tprev.tv_sec)+(float)( tnow.tv_usec-tprev.tv_usec)/1000000.;
    if(tdif>=0.2 ){//|| prev_msg == 1 ){                          // every 0.2 seconds, this is really only needed for phase2
      prev_msg = 0;
      //printf("Check server data\n");
      while(check_server_data() > 0){  // check on an Adaq command
        //printf("handled server data\n");
        tcontact.tv_sec = tnow.tv_sec;
        tcontact.tv_usec = tnow.tv_usec;
        prev_msg = 1;
      }
      if(send_server_data() > 0){                  // send data to Adaq
        tcontact.tv_sec = tnow.tv_sec;
        tcontact.tv_usec = tnow.tv_usec;
      }
      tprev.tv_sec = tnow.tv_sec;
      tprev.tv_usec = tnow.tv_usec;
    }
    tdifc = (float)(tnow.tv_sec-tcontact.tv_sec)+(float)( tnow.tv_usec-tcontact.tv_usec)/1000000.;
    if(tdifc>13.){ // no contact for 13 seconds!
      printf("Shutting down the comms\n");
      if(DU_comms >=0){
        shutdown(DU_comms,SHUT_RDWR);
        close(DU_comms);
        DU_comms = -1;
      }
      if(DU_socket >=0){
        shutdown(DU_socket,SHUT_RDWR);
        close(DU_socket);
        DU_socket = -1;
      }
      sleep(1);
      make_server_connection(du_port);
      tcontact.tv_sec = tnow.tv_sec;
    }
    usleep(1000);
  }
}

/*!
 \fn int main(int argc, char **argv)
 * main program:
 * - Initializes responses to signals
 * - get station ID from ip-address, start with 10.
 * - Opens the connection to the DDR memory, this way this memory is available to all child processes
 * - Create shared memories
 * - Create child processes for:
 *   - Reading/writing IP-sockets
 *   - Reading/writing the scope
 * - Restart these processes when needed
 * - After a stop is requested through an interrupt, clean up shared memories
 *
 \param argc Number of arguments when starting the program (not used)
 \param argv character string of arguments (not used)
 \author C. Timmermans
 */
int main(int argc, char **argv)
{
  pid_t pid;
  int32_t status,i;
  
  signal(SIGHUP,clean_stop);
  signal(SIGINT,clean_stop);
  signal(SIGTERM,clean_stop);
  signal(SIGABRT,clean_stop);
  signal(SIGKILL,clean_stop);
  
  i = 0;
  while(station_id <= 0 && i < 60) {
    du_get_station_id();
    if(station_id <=0) sleep(2);
    i++;
  }

  if(ad_shm_create(&shm_ev,BUFSIZE,1) <0){
    printf("Cannot create T3  shared memory !!\n");
    exit(-1);
  }
  *(shm_ev.next_read) = 0;
  *(shm_ev.next_write) = 0;
  if(ad_shm_create(&shm_gps,GPSSIZE,1) <0){
    printf("Cannot create GPS shared memory !!\n");
    exit(-1);
  }
  *(shm_gps.next_read) = 0;
  *(shm_gps.next_write) = 0;
  if(ad_shm_create(&shm_cmd,MSGSTOR,sizeof(int)) <0){
    printf("Cannot create CMD shared memory !!\n");
    exit(-1);
  }
  scope_open();
  if((pid_scope = fork()) == 0) du_scope_main();
  if((pid_socket = fork()) == 0) du_socket_main();
  while(stop_process == 0){
    pid = waitpid (WAIT_ANY, &status, 0);
    if(pid == pid_scope && stop_process == 0) {
      if((pid_scope = fork()) == 0) du_scope_main();
    }
    if(pid == pid_socket && stop_process == 0) {
      if((pid_socket = fork()) == 0) du_socket_main();
    }
    sleep(1);
  }
  remove_shared_memory();
}
