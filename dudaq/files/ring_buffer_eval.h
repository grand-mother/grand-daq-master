/**
 * \struct
 * \brief
 *
 */
#ifndef DUDAQ_1_MYFILES_RING_BUFFER_H_
#define DUDAQ_1_MYFILES_RING_BUFFER_H_

#include <pthread.h>
#include <stdint.h>
#include <string.h>

typedef struct
{
   /* dimension of array of buffer */
   uint16_t size_buffer_byte; // size of one buffer in byte
   uint16_t nb_elet_buffer; // number of elet in buffer
   uint16_t nb_buffer; // number of buffer in ring buffers
   uint16_t idx_max; // index max , ie nb_array-1

   /* index in array, nb_array > inext_trig >= 0*/
   uint16_t inext_write; // index next write
   uint16_t inext_eval; // index next evaluation
   uint16_t inext_trig; // index next trigger

   /* number of buffer */
   /* nb_write+nb_eval+nb_trig=nb_array*/
   uint16_t nb_write; //nb buffer available
   uint16_t nb_eval; // nb buffer to evaluate
   uint16_t nb_trig; // nb buffer to trig

   /* array of buffer*/
   uint32_t *a_buffers; // ring buffer : array of all buffers
   float *a_prob; // array of probability for each buffer

   /* mutex */
   pthread_mutex_t mutex;
} S_RingBufferEval;

S_RingBufferEval*
RBE_create (uint16_t nb_elet_buffer, uint16_t nb_array);

void
RBE_delete (S_RingBufferEval **pself);

void
RBE_write (S_RingBufferEval *self, const void *p_buf);

void
RBE_after_write (S_RingBufferEval *const self);

void
RBE_after_eval (S_RingBufferEval *self);

void
RBE_after_trigger (S_RingBufferEval *self);

void
RBE_inc_modulo (uint16_t *p_int, uint16_t max_int);

void
RBE_print_error (void);

#endif
