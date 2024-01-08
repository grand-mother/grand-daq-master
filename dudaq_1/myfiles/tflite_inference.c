#include <stdio.h>
#include <stdlib.h>
#include "tensorflow/lite/c/c_api.h"
#include "tflite_inference.h"

/* ADC 14 bits */
float G_quantif = 32768.0;
short G_mask14 = 1 << 13; // --- bit 14
short G_mask15 = 1 << 14; // --- bit 15
short G_mask16 = 1 << 15; // --- bit 16

/**
 * \fn TfLiteInterpreter TFLT_create*(void)
 * \brief
 *
 * \return
 */

TFLT_struct*
TFLT_create(uint16_t size_trace) {
	TFLT_struct *p_tflt = (TFLT_struct*) malloc(sizeof(TFLT_struct));
	p_tflt->nb_sample = size_trace;
	p_tflt->size_byte = sizeof(float) * size_trace * 3;
	float *p_3dt = (float*) malloc(p_tflt->size_byte);

	TfLiteModel *model = TfLiteModelCreateFromFile("trigger_grand.tflite");
	TfLiteInterpreterOptions *options = TfLiteInterpreterOptionsCreate();
	TfLiteInterpreterOptionsSetNumThreads(options, 1);

	/* Create the interpreter.*/
	TfLiteInterpreter *interpreter = TfLiteInterpreterCreate(model, options);

	/*Allocate tensors and populate the input tensor data.*/
	TfLiteInterpreterAllocateTensors(interpreter);

	p_tflt->p_interp = interpreter;
	p_tflt->a_3dtraces = p_3dt;

	return p_tflt;
}

/**
 * \fn int TFINF_delete(TfLiteInterpreter**)
 * \brief
 *
 * \param pp_interp
 * \return
 */

void TFLT_delete(TFLT_struct **pp_tflt) {
	/* Dispose of the model and interpreter objects.*/
	TfLiteInterpreter *interpreter = (*pp_tflt)->p_interp;
	TfLiteInterpreterDelete(interpreter);
	/* #TODO:
	 TfLiteInterpreterOptionsDelete (options);
	 TfLiteModelDelete (model);
	 */
	free((*pp_tflt)->a_3dtraces);
	free(*pp_tflt);
	*pp_tflt = NULL;
}

/**
 * \fn void TFLT_convert_traces(uint8_t*, float*)
 * \brief
 *
 * \param a_tr_adu
 * \param a_tr_f32
 */

short TFLT_convert_adu(short adu) {
	short value = adu;
	short bit14 = (value & (G_mask14)) >> 13;
	value = (value & (~G_mask15)) | (bit14 << 14);
	value = (value & (~G_mask16)) | (bit14 << 15);
	return value;
}

void TFLT_preprocessing(TFLT_struct *p_tflt, uint16_t *a_tr_adu) {

	int l_s;
	short dec_adu;

	for (l_s = 0; l_s < p_tflt->nb_sample; l_s++) {
		dec_adu = TFLT_convert_adu(a_tr_adu[l_s]);
		p_tflt->a_3dtraces[3 * l_s] = dec_adu / G_quantif;
		dec_adu = TFLT_convert_adu(a_tr_adu[l_s + OFFSET_1]);
		p_tflt->a_3dtraces[3 * l_s + 1] = dec_adu / G_quantif;
		dec_adu = TFLT_convert_adu(a_tr_adu[l_s + OFFSET_2]);
		p_tflt->a_3dtraces[3 * l_s + 2] = dec_adu / G_quantif;
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

void TFLT_inference(TFLT_struct *p_tflt, float *output_proba) {
	TfLiteTensor *input_tensor = TfLiteInterpreterGetInputTensor(
			p_tflt->p_interp, 0);
	TfLiteTensorCopyFromBuffer(input_tensor, p_tflt->a_3dtraces,
			p_tflt->size_byte);

	/* Execute inference */
	TfLiteInterpreterInvoke(p_tflt->p_interp);

	/* Extract the output tensor data */
	const TfLiteTensor *output_tensor = TfLiteInterpreterGetOutputTensor(
			p_tflt->p_interp, 0);
	TfLiteTensorCopyToBuffer(output_tensor, output_proba, sizeof(float));
}
