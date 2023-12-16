#ifndef DUDAQ_1_MYFILES_FUNC_EVAL_EVT_H_
#define DUDAQ_1_MYFILES_FUNC_EVAL_EVT_H_


#include "ring_buffer_eval.h"

typedef struct
{
      RingBufferEval_struct *p_rbe; // data
      void *p_eval; // for processing
} FuncEval_struct;

FuncEval_struct *FEEV_create(RingBufferEval_struct *p_rbe, void *p_eval);

void *FEEV_run (void *p_args);

void FEEV_stop (void);

void FEEV_eval (void *p_eval, uint8_t *p_buf, float *p_prob);

#endif
