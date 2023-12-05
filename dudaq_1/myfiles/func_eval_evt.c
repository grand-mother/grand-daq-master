#include "func_eval_evt.h"
#include "ring_buffer_eval.h"

int FEEV_cpt = 0;
int FEEV_stop = 0;

void FEEV_main (void  *p_data)
{
  RingBufferEval_struct *p_rbe = (RingBufferEval_struct *)p_data;
  uint16_t nb_eval;
  uint32_t idx_eval;
  struct timespec tempo =
    { .tv_nsec = 500000000 };

  while (FEEV_stop == 0)
  {
    while (1)
    {
// we must do evaluation all evaluation without tempo between
      mutex_lock (&p_rbe->mutex);
      nb_eval = p_rbe->nb_eval;
      mutex_unlock (&p_rbe->mutex);
      if (nb_eval > 0)
      {
	idx_eval = (uint32_t) p_rbe->inext_eval * (uint32_t) p_rbe->size_buffer;
	p_rbe->a_prob[idx_eval] = FEEV_eval (p_rbe->a_buffers[idx_eval]);
	RFE_update_eval (p_rbe);
      }
      else
      {
	break;
      }
    }
// all evaluation are done, => sleep 0.1 ms
    thrd_sleep (&tempo, NULL);
  }
}

// fake eval
float FEEV_eval (uint8_t *p_buf)
{
  FEEV_cpt++;
  if (FEEV_cpt > 100)
  {
    FEEV_cpt = 0;
    return 0.0;
  }
  return (FEEV_cpt / 100.0);
}

