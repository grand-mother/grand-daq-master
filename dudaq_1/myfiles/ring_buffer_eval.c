#include "ring_buffer_eval.h"

#include <stdlib.h>
#include <bits/stdint-uintn.h>

int RBE_error = 0;

/**
 * \fn RingBufferEval_struct RBE_create*(uint16_t, uint16_t)
 * \brief
 *
 *
 *
 * \param size_buffer
 * \param nb_array
 * \return
 */
RingBufferEval_struct*
RBE_create (uint16_t size_buffer, uint16_t nb_array)
{
   RingBufferEval_struct *p_rbe;
   uint64_t nb_byte;
   uint8_t *p_buf;
   float *p_prob;
   int ret_mtx;

   p_rbe = (RingBufferEval_struct*) malloc (sizeof(RingBufferEval_struct));
   if (p_rbe == NULL)
   {
      RBE_error = 1;
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
      RBE_error = 2;
      return NULL;
   }
   p_rbe->a_buffers = p_buf;
   p_prob = (float*) malloc (sizeof(float) * (uint64_t) nb_array);
   if (p_prob == NULL)
   {
      RBE_error = 3;
      return NULL;
   }
   p_rbe->a_prob = p_prob;
   ret_mtx = mtx_init (&(p_rbe->mutex), mtx_plain);
   if (ret_mtx != thrd_success)
   {
      RBE_error = 4;
      return NULL;
   }

   return p_rbe;
}

/**
 * \fn uint8_t RBE_delete(RingBufferEval_struct**)
 * \brief
 *
 *
 *
 * \param p_rbe
 * \return
 */
void RBE_delete (RingBufferEval_struct **pp_rbe)
{
   free ((*pp_rbe)->a_buffers);
   free ((*pp_rbe)->a_prob);
   mtx_destroy (&((*pp_rbe)->mutex));
   free (*pp_rbe);
   *pp_rbe = NULL;
}

/**
 * \fn void RBE_write(RingBufferEval_struct*, void *p_buf)
 * \brief
 *
 *
 *
 * \param p_rbe
 */
inline void RBE_write (RingBufferEval_struct *p_rbe, void *p_buf)
{
   uint32_t idx_eval_buffer = p_rbe->inext_eval;
   /* index (in byte ) where start buffer to evaluate */
   uint32_t idx_eval_byte = ((uint32_t) idx_eval_buffer) * p_rbe->size_buffer;
   memcpy(&p_rbe->a_buffers[idx_eval_byte], p_buf, p_rbe->size_buffer);
   p_rbe->nb_write -= 1;
   p_rbe->nb_eval += 1;
   RBE_inc_modulo (&p_rbe->inext_write, p_rbe->idx_max);
}

/**
 * \fn void RBE_update_write(RingBufferEval_struct*)
 * \brief
 *
 *
 *
 * \param p_rbe
 */
inline void RBE_update_write (RingBufferEval_struct *p_rbe)
{
   mutext_lock (&p_rbe->mutex);
   p_rbe->nb_write -= 1;
   p_rbe->nb_eval += 1;
   RBE_inc_modulo (&p_rbe->inext_write, p_rbe->idx_max);
   mutext_unlock (&p_rbe->mutex);
}

/**
 * \fn void RBE_update_eval(RingBufferEval_struct*)
 * \brief
 *
 *
 *
 * \param p_rbe
 */
inline void RBE_update_eval (RingBufferEval_struct *p_rbe)
{
   mutext_lock (&p_rbe->mutex);
   p_rbe->nb_eval -= 1;
   p_rbe->nb_trig += 1;
   RBE_inc_modulo (&p_rbe->inext_eval, p_rbe->idx_max);
   mutext_unlock (&p_rbe->mutex);
}

/**
 * \fn void RBE_update_trigger(RingBufferEval_struct*)
 * \brief
 *
 *
 *
 * \param p_rbe
 */
inline void RBE_update_trigger (RingBufferEval_struct *p_rbe)
{
   mutext_lock (&p_rbe->mutex);
   p_rbe->nb_trig -= 1;
   p_rbe->nb_write += 1;
   RBE_inc_modulo (&p_rbe->inext_trig, p_rbe->idx_max);
   mutext_unlock (&p_rbe->mutex);
}

/**
 * \fn uint16_t RBE_inc_modulo(uint16_t*)
 * \brief
 *
 *
 * \param p_int
 * \return
 */

inline void RBE_inc_modulo (uint16_t *p_int, uint16_t max_int)
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

void RBE_print_error (void)
{
   printf ("\nRFE module : error %d (O is OK)", RBE_error);
}
