/*
 * tflite_inference.h
 *
 *  Created on: 5 déc. 2023
 *      Author: jcolley
 */

#ifndef DUDAQ_1_MYFILES_TFLITE_INFERENCE_H_
#define DUDAQ_1_MYFILES_TFLITE_INFERENCE_H_

#include "tensorflow/lite/c/c_api.h"


typedef struct
{
      TfLiteInterpreter *p_interp; // tensor flow lite structure for inference
      float *a_3dtraces; // array of 3d traces
      uint64_t size_byte; // size of array of 3d traces
      uint16_t nb_sample; // in one trace
} TFLT_struct;

TFLT_struct* TFLT_create (uint16_t size_trace);

void TFLT_delete (TFLT_struct **pp_tflt);

void TFLT_preprocessing (TFLT_struct *p_tflt, uint8_t *a_tr_adu);

void TFLT_inference (TFLT_struct *p_tflt, float *output_proba);

#endif /* DUDAQ_1_MYFILES_TFLITE_INFERENCE_H_ */
