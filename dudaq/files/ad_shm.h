/*** \file ad_shm.h
DAQ shared memory definitions
Version:2.0 updated for integers, not shorts
Date: 17/12/2020
Author: Charles Timmermans, Nikhef/Radboud University

Altering the code without explicit consent of the author is forbidden
 ***/
#include <stdint.h>
#include <unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/shm.h>

typedef struct{
  int shmid; /**< shared memory identifier */
  int *next_read;  /**< pointer to the next block to read out in Ubuf */
  int *next_readb; /**< pointer to the next block to read out in Ubuf by a second process */
  int *next_write; /**< Pointer to the next position where to write a block of data */
  int *nbuf; /**< number of data blocks in the circular buffer */
  int *size; /**< size (in ints) of a data block */
  char *buf; /**< pointer to the buffer */
  uint32_t *Ubuf; /**< our data is in uint32, so a uint32 pointer */
}shm_struct;

int ad_shm_create(shm_struct *ptr,int nbuf,int size);
void ad_shm_delete(shm_struct *ptr);
