/**
 * \struct
 * \brief
 *
 */
#ifndef DUDAQ_1_MYFILES_RING_BUFFER_H_
#define DUDAQ_1_MYFILES_RING_BUFFER_H_

#include  <pthread.h>

typedef struct
{
      /* dimension of array of buffer */
      uint16_t size_buffer; // size of one buffer in byte
      uint16_t nb_array; // number of buffer in array
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
      uint8_t *a_buffers; // array of all buffers
      float *a_prob; // array of probability for each buffer

      /* mutex */
      pthread_mutex_t mutex;
} RingBufferEval_struct;

RingBufferEval_struct*
RBE_create (uint16_t size_buffer, uint16_t nb_array);

void
RBE_delete (RingBufferEval_struct **pp_rbe);

inline void
RBE_update_write (RingBufferEval_struct *p_rbe);

inline void
RBE_update_eval (RingBufferEval_struct *p_rbe);

inline void
RBE_update_trigger (RingBufferEval_struct *p_rbe);

inline void
RBE_inc_modulo (uint16_t *p_int, uint16_t max_int);

void
RBE_print_error (void);

#endif
