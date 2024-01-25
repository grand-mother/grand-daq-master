#include <stdio.h>
#include <stdlib.h>
#include "tensorflow/lite/c/c_api.h"
#include "tflite_inference.h"

/* ADC 14 bits */
float G_quantif = 32768.0;
short G_mask14 = 1 << 13; // --- bit 14
short G_mask15 = 1 << 14; // --- bit 15
short G_mask16 = 1 << 15; // --- bit 16

#define OFFSET_0 0
#define OFFSET_1 2
#define OFFSET_2 4

/**
 * \fn TfLiteInterpreter TFLT_create*(void)
 * \brief
 *
 * \return
 */

S_TFLite*
TFLT_create (uint16_t size_trace)
{
   S_TFLite *self = NULL;

   self = (S_TFLite*) malloc (sizeof(S_TFLite));

   /*
    * Tensorflow Lite init
    *
    * */
   TfLiteModel *model = TfLiteModelCreateFromFile ("trigger_grand.tflite");
   TfLiteInterpreterOptions *options = TfLiteInterpreterOptionsCreate ();
   TfLiteInterpreterOptionsSetNumThreads (options, 1);
   /* Create the interpreter.*/
   TfLiteInterpreter *interpreter = TfLiteInterpreterCreate (model, options);
   /*Allocate tensors and populate the input tensor data.*/
   TfLiteInterpreterAllocateTensors (interpreter);
   self->p_interp = interpreter;

   /*
    * Array trace allocation
    *
    * */
   self->nb_sample = size_trace;
   self->size_byte = sizeof(float) * size_trace * 3;
   self->a_3dtraces = (float*) malloc (self->size_byte);

   return self;
}

/**
 * \fn int TFINF_delete(TfLiteInterpreter**)
 * \brief
 *
 * \param pp_interp
 * \return
 */

void TFLT_delete (S_TFLite **pself)
{
   /* Dispose of the model and interpreter objects.*/
   TfLiteInterpreterDelete ((*pself)->p_interp);
   /* #TODO:
    TfLiteInterpreterOptionsDelete (options);
    TfLiteModelDelete (model);
    */
   free ((*pself)->a_3dtraces);
   free (*pself);
   *pself = NULL;
}

/**
 * \fn void TFLT_convert_traces(uint8_t*, float*)
 * \brief
 *
 * \param a_tr_adu
 * \param a_tr_f32
 */

short TFLT_convert_adu (short adu)
{
   short value = adu;
   short bit14 = (value & (G_mask14)) >> 13;
   value = (value & (~G_mask15)) | (bit14 << 14);
   value = (value & (~G_mask16)) | (bit14 << 15);
   return value;
}

void TFLT_preprocessing (S_TFLite *const self, uint8_t *a_tr_adu)
{

   int l_s;
   short dec_adu;

   for (l_s = 0; l_s < self->nb_sample; l_s++)
   {
      dec_adu = TFLT_convert_adu (a_tr_adu[l_s + OFFSET_0]);
      self->a_3dtraces[3 * l_s] = dec_adu / G_quantif;
      dec_adu = TFLT_convert_adu (a_tr_adu[l_s + OFFSET_1]);
      self->a_3dtraces[3 * l_s + 1] = dec_adu / G_quantif;
      dec_adu = TFLT_convert_adu (a_tr_adu[l_s + OFFSET_2]);
      self->a_3dtraces[3 * l_s + 2] = dec_adu / G_quantif;
   }
}

/**
 * \fn void run_inference(TfLiteInterpreter*, float*, float*)
 * \brief
 *
 * \param interpreter
 * \param input
 * \param output_proba
 */

void TFLT_inference (S_TFLite *const self, float *output_proba)
{
   TfLiteTensor *input_tensor = TfLiteInterpreterGetInputTensor (self->p_interp, 0);
   TfLiteTensorCopyFromBuffer (input_tensor, self->a_3dtraces, self->size_byte);

   /* Execute inference */
   TfLiteInterpreterInvoke (self->p_interp);

   /* Extract the output tensor data */
   const TfLiteTensor *output_tensor = TfLiteInterpreterGetOutputTensor (self->p_interp, 0);
   TfLiteTensorCopyToBuffer (output_tensor, output_proba, sizeof(float));
}
