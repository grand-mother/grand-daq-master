/*** \file ad_shm.c
 DAQ shared memory
 Version:1.0
 Date: 17/2/2020
 Author: Charles Timmermans, Nikhef/Radboud University

 Altering the code without explicit consent of the author is forbidden
 ***/

#include "ad_shm.h"

#include <stdio.h>
#include <stdlib.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include<errno.h>
#include"dudaq.h"
#include "amsg.h"
#include "scope.h"
#include "ad_shm.h"
#include "du_monitor.h"

/**
 * \fn int ad_shm_create(shm_struct*, int, int)
 * \brief
 * int ad_shm_create(shm_struct *ptr,int nbuf,int size)
 *
 * create a shared memory of useable size "(size+1)*nbuf" shorts
 * The pointer to the shm_struct (defined in ad_shm.h) must be provided
 *
 *
 * \pre
 * \post
 * \param ptr
 * \param nbuf number of element buffer in share memory
 * \param size number of int16 in element buffer
 * \return 1 OK, -1 NOK
 */
int
ad_shm_create (shm_struct *ptr, int nbuf, int size)
{
  int sz_int = sizeof(int);
  size_t isize = (size + 1) * nbuf * sizeof(uint16_t) + 5 * sz_int;
  //
  // why size + 1 ?
  // may by ... to prevent, critical section when update 'new write' before modulo MAX
  // JM Colley
  //
  key_t key = IPC_PRIVATE;

  ptr->shmid = 0;
  ptr->buf = NULL;
  ptr->Ubuf = NULL;
  ptr->next_write = NULL;
  ptr->next_read = NULL;
  ptr->next_readb = NULL;
  ptr->nbuf = NULL;
  ptr->size = NULL;
  ptr->shmid = shmget (key, isize, IPC_CREAT | 0666);
  if (ptr->shmid < 0)
    return (-1);
  // buf is a pointer of char (1 byte)
  ptr->buf = shmat (ptr->shmid, NULL, 0600);
  memset ((void*) ptr->buf, 0, isize);
  // buf is header(5 int) + Ubuf
  ptr->Ubuf = (uint16_t*) (&(ptr->buf[5 * sz_int]));
  // fill header : 5 fields
  ptr->next_write = (int*) &(ptr->buf[0]);
  ptr->next_read = (int*) &(ptr->buf[sz_int]);
  ptr->next_readb = (int*) &(ptr->buf[2 * sz_int]);
  ptr->nbuf = (int*) &(ptr->buf[3 * sz_int]);
  ptr->size = (int*) &(ptr->buf[4 * sz_int]);
  *(ptr->next_write) = 0;
  *(ptr->next_read) = 0;
  *(ptr->next_readb) = 0;
  *(ptr->nbuf) = nbuf;
  *(ptr->size) = size;
  return (1);
}

/**
 void ad_shm_delete(shm_struct *ptr)

 deletes the shared memory pointed to by ptr
 */
void
ad_shm_delete (shm_struct *ptr)
{
  shmdt (ptr->buf);
  shmctl (ptr->shmid, IPC_RMID, NULL);
  ptr->shmid = 0;
  ptr->buf = NULL;
  ptr->Ubuf = NULL;
  ptr->next_write = NULL;
  ptr->next_read = NULL;
  ptr->next_readb = NULL;
  ptr->nbuf = NULL;
  ptr->size = NULL;
}
