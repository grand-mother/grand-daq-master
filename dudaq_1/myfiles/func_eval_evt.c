/**
 * \file
 * \brief
 *
 * \author Colley Jean-Marc, CNRS/IN2P3/LPNHE, Paris
 */

#include "func_eval_evt.h"
#include "ring_buffer_eval.h"

int G_cpt = 0;
int G_running = 1; // if 0 stop all thread

/**
 * \fn FuncEval_struct * FEEV_create(RingBufferEval_struct*, void*)
 * \brief
 *
 * \param p_rbe
 * \param p_eval
 * \return
 */
FuncEval_struct* FEEV_create (RingBufferEval_struct *p_rbe, void *p_eval)
{
   FuncEval_struct *p_feev = (FuncEval_struct*) malloc (sizeof(FuncEval_struct));
   p_feev->p_rbe = p_rbe;
   p_feev->p_eval = p_eval;
   return p_feev;
}

/**
 * \fn void FEEV_delete(FuncEval_struct**)
 * \brief
 *
 * \param pp_feev
 * \return
 */

void FEEV_delete (FuncEval_struct **pp_feev)
{
   RBE_delete(&((*pp_feev)->p_rbe));
   /* p_eval must be free before */
   assert((*pp_feev)->p_eval == NULL);
   free (*pp_feev);
   *pp_feev = NULL;
}

/**
 * \fn void FEEV_run(void*)
 * \brief
 *
 * \param p_data
 */
int FEEV_run (void *p_args)
{
   FuncEval_struct *p_fe = (FuncEval_struct*) p_args;
   RingBufferEval_struct *p_rbe = (RingBufferEval_struct*) p_fe->p_rbe;
   void *p_eval = p_fe->p_eval;

   uint16_t nb_eval;
   uint16_t idx_eval_buffer;
   uint32_t idx_eval_byte;
   struct timespec tempo =
      { .tv_nsec = 500000000 };

   while (G_running)
   {
      /* evaluation of all events without tempo between*/
      mutex_lock (&p_rbe->mutex);
      nb_eval = p_rbe->nb_eval;
      mutex_unlock (&p_rbe->mutex);
      if (nb_eval > 0)
      {
	 idx_eval_buffer = p_rbe->inext_eval;
	 /* index (in byte ) where start buffer to evaluate */
	 idx_eval_byte = ((uint32_t) idx_eval_buffer) * p_rbe->size_buffer;
	 /* eval and update ring buffer */
	 FEEV_eval (p_eval, &(p_rbe->a_buffers[idx_eval_byte]), &(p_rbe->a_prob[idx_eval_buffer]));
	 RFE_update_eval (p_rbe);
      }
      else
      {
	 /* all evaluation are done => sleep 0.1 ms */
	 thrd_sleep (&tempo, NULL);
      }
   }
   return 0;
}

/**
 * \fn void FEEV_stop(void)
 * \brief
 *
 */
void FEEV_stop (void)
{
   G_running = 0;
}

/**
 * \fn float FEEV_eval(uint8_t*)
 * \brief FAKE TRIGGER
 *
 * \param p_buf
 * \return
 */

#ifdef FEEV_IS_FAKE

void FEEV_eval (void *p_eval, uint8_t *p_buf, float *p_prob)
{
   G_cpt++;
   if (G_cpt > 100)
   {
      G_cpt = 0;
      return 0.0;
   }
   return (FEEV_cpt / 100.0);
}
#endif

#define FEEV_IS_TFLT
#ifdef FEEV_IS_TFLT

#include "tflite_inference.h"
/**
 * \fn void FEEV_eval(void*, uint8_t*, float*)
 * \brief Evaluation with neural network with tensorflow lite library
 *
 * \param p_eval
 * \param p_buf
 * \param p_prob
 */
void FEEV_eval (void *p_eval, uint8_t *p_buf, float *p_prob)
{
   TFLT_struct *p_tflt = (TFLT_struct*) p_eval;
   TFLT_preprocessing (p_tflt, p_buf);
   TFLT_inference (p_tflt, p_prob);
}
#endif
