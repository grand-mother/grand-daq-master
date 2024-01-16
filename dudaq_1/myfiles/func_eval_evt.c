/**
 * \file
 * \brief
 *
 * \author Colley Jean-Marc, CNRS/IN2P3/LPNHE, Paris
 */

#include "func_eval_evt.h"
#include "ring_buffer_eval.h"
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>

int G_cpt = 0;
int G_running = 1; // if 0 stop all thread

/**
 * \fn S_FuncEval * FEEV_create(S_RingBufferEval*, void*)
 * \brief
 *
 * \param p_rbe
 * \param p_eval
 * \return
 */
S_FuncEval* FEEV_create (S_RingBufferEval *p_rbe, void *p_eval)
{
   S_FuncEval *p_feev = (S_FuncEval*) malloc (sizeof(S_FuncEval));
   p_feev->p_rbe = p_rbe;
   p_feev->p_eval = p_eval;
   return p_feev;
}

/**
 * \fn void FEEV_delete(S_FuncEval**)
 * \brief
 *
 * \param pp_feev
 * \return
 */

void FEEV_delete (S_FuncEval **pself)
{
   RBE_delete(&((*pself)->p_rbe));
   /* p_eval must be free before */
   assert((*pself)->p_eval == NULL);
   free (*pself);
   *pself = NULL;
}

/**
 * \fn void FEEV_run(void*)
 * \brief
 *
 * \param p_data
 */
void *FEEV_run (void *p_args)
{
   S_FuncEval *self = (S_FuncEval*) p_args;
   S_RingBufferEval *p_rbe = (S_RingBufferEval*) self->p_rbe;
   void *p_eval = self->p_eval;

   uint16_t nb_eval;
   uint16_t idx_eval_buffer;
   uint32_t idx_eval_byte;


   while (G_running)
   {
      /* evaluation of all events without tempo between*/
      pthread_mutex_lock (&p_rbe->mutex);
      nb_eval = p_rbe->nb_eval;
      pthread_mutex_unlock (&p_rbe->mutex);
      if (nb_eval > 0)
      {
	 idx_eval_buffer = p_rbe->inext_eval;
	 /* index (in byte ) where start buffer to evaluate */
	 idx_eval_byte = ((uint32_t) idx_eval_buffer) * p_rbe->size_buffer;
	 /* eval and update ring buffer */
	 FEEV_eval (p_eval,
			 	p_rbe->a_buffers + idx_eval_byte,
				p_rbe->a_prob + idx_eval_buffer);
	 RBE_after_eval (p_rbe);
      }
      else
      {
	 /* all evaluation are done => sleep 0.1 ms */
	 usleep (100);
	 printf("Wake up !!!");
      }
   }
   pthread_exit(NULL);
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
   S_TFLite *p_tflt = (S_TFLite*) p_eval;
   TFLT_preprocessing (p_tflt, p_buf);
   TFLT_inference (p_tflt, p_prob);
}
#endif
