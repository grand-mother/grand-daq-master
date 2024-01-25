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
} S_TFLite;

S_TFLite* TFLT_create (uint16_t size_trace);

void TFLT_delete (S_TFLite **pself);

void TFLT_preprocessing (S_TFLite *const self, uint8_t *a_tr_adu);

void TFLT_inference (S_TFLite *const self, float *output_proba);

#endif /* DUDAQ_1_MYFILES_TFLITE_INFERENCE_H_ */
