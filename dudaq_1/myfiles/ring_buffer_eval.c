#include "ring_buffer_eval.h"

#include <stdlib.h>
#include <bits/stdint-uintn.h>

int RFE_error = 0;

/**
 * \fn RingBufferEval_struct RFE_create*(uint16_t, uint16_t)
 * \brief
 *
 *
 *
 * \param size_buffer
 * \param nb_array
 * \return
 */
RingBufferEval_struct*
RFE_create (uint16_t size_buffer, uint16_t nb_array)
{
  RingBufferEval_struct *p_rbe;
  uint64_t nb_byte;
  uint8_t *p_buf;
  float *p_prob;
  int ret_mtx;

  p_rbe = (RingBufferEval_struct*) malloc (sizeof(RingBufferEval_struct));
  if (p_rbe == NULL)
  {
    RFE_error = 1;
    return NULL;
  }
  p_rbe->inext_eval = 0;
  p_rbe->inext_trig = 0;
  p_rbe->inext_write = 0;
  p_rbe->nb_array = nb_array;
  p_rbe->idx_max = nb_array - 1;
  p_rbe->size_buffer = size_buffer;
  p_rbe->nb_eval = 0;
  p_rbe->nb_trig = 0;
  p_rbe->nb_write = nb_array;
  // alloc array of buffer
  nb_byte = (uint64_t) size_buffer * (uint64_t) nb_array;
  p_buf = (uint8_t*) malloc (nb_byte);
  if (p_buf == NULL)
  {
    RFE_error = 2;
    return NULL;
  }
  p_rbe->a_buffers = p_buf;
  p_prob = (float*) malloc (sizeof(float) * (uint64_t) nb_array);
  if (p_prob == NULL)
  {
    RFE_error = 3;
    return NULL;
  }
  p_rbe->a_prob = p_prob;
  ret_mtx = mtx_init (&(p_rbe->mutex), mtx_plain);
  if (ret_mtx != thrd_success)
  {
    RFE_error = 4;
    return NULL;
  }

  return p_rbe;
}

/**
 * \fn uint8_t RFE_delete(RingBufferEval_struct**)
 * \brief
 *
 *
 *
 * \param p_rbe
 * \return
 */
uint8_t RFE_delete (RingBufferEval_struct **pp_rbe)
{
  free ((*pp_rbe)->a_buffers);
  free ((*pp_rbe)->a_prob);
  free (*pp_rbe);
  *pp_rbe = NULL;
  return 0;
}

/**
 * \fn void RFE_update_write(RingBufferEval_struct*)
 * \brief
 *
 *
 *
 * \param p_rbe
 */
inline void RFE_update_write (RingBufferEval_struct *p_rbe)
{
  mutext_lock (&p_rbe->mutex);
  p_rbe->nb_write -= 1;
  p_rbe->nb_eval += 1;
  RFE_inc_modulo (&p_rbe->inext_write, p_rbe->idx_max);
  mutext_unlock (&p_rbe->mutex);
}

/**
 * \fn void RFE_update_eval(RingBufferEval_struct*)
 * \brief
 *
 *
 *
 * \param p_rbe
 */
inline void RFE_update_eval (RingBufferEval_struct *p_rbe)
{
  mutext_lock (&p_rbe->mutex);
  p_rbe->nb_eval -= 1;
  p_rbe->nb_trig += 1;
  RFE_inc_modulo (&p_rbe->inext_eval, p_rbe->idx_max);
  mutext_unlock (&p_rbe->mutex);
}

/**
 * \fn void RFE_update_trigger(RingBufferEval_struct*)
 * \brief
 *
 *
 *
 * \param p_rbe
 */
inline void RFE_update_trigger (RingBufferEval_struct *p_rbe)
{
  mutext_lock (&p_rbe->mutex);
  p_rbe->nb_trig -= 1;
  p_rbe->nb_write += 1;
  RFE_inc_modulo (&p_rbe->inext_trig, p_rbe->idx_max);
  mutext_unlock (&p_rbe->mutex);
}

/**
 * \fn uint16_t RFE_inc_modulo(uint16_t*)
 * \brief
 *
 *
 * \param p_int
 * \return
 */
inline void RFE_inc_modulo (uint16_t *p_int, uint16_t max_int)
{
  if (*p_int == max_int)
  {
    *p_int = 0;
  }
  else
  {
    *p_int += 1;
  }
}

void RFE_print_error (void)
{
  printf ("\nRFE module : error %d (O is OK)", RFE_error);
}
