/*
 * tflite_inference.h
 *
 *  Created on: 5 déc. 2023
 *      Author: jcolley
 */

#ifndef DUDAQ_1_MYFILES_TFLITE_INFERENCE_H_
#define DUDAQ_1_MYFILES_TFLITE_INFERENCE_H_

#include "tensorflow/lite/c/c_api.h"

#define TFLT_SAMPLE_IN_TRACE 1024

typedef struct
{
   TfLiteInterpreter *p_interp; // tensorflow lite structure for inference
   TfLiteInterpreterOptions *p_options; // tensorflow lite structure for inference
   TfLiteModel *p_model; // tensorflow lite structure for inference
   float *a_3dtraces; // array of 3d traces
   uint64_t size_byte; // size of array of 3d traces
   uint16_t nb_sample; // in one trace
} S_TFLite;

S_TFLite* TFLT_create (int nb_thread);

void TFLT_delete (S_TFLite **pself);

void TFLT_preprocessing (S_TFLite *const self, const uint32_t *const a_tr_adu);

void TFLT_inference (S_TFLite *const self, float *const p_proba);

#endif /* DUDAQ_1_MYFILES_TFLITE_INFERENCE_H_ */
