/*
 ============================================================================
 Name        : main.c
 Author      : colley
 Version     :
 Copyright   : Your copyright notice
 Description : trigger tensor proto
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "tensorflow/lite/c/c_api.h"


#define NB_SAMPLE 1024


void get_input(float *input) {
	int l_s;
	for (l_s = 0; l_s < NB_SAMPLE; l_s++) {
		input[l_s] = (float)rand()/RAND_MAX;
	}
}


void run_inference(TfLiteInterpreter* interpreter, float* input,
		float * output_proba) {
	TfLiteTensor* input_tensor = TfLiteInterpreterGetInputTensor(interpreter,
			0);
	//TfLiteTensorCopyFromBuffer(input_tensor, "input.data()", input.size() * sizeof(float));
	TfLiteTensorCopyFromBuffer(input_tensor, "input.data()", 10);

	// Execute inference.
	TfLiteInterpreterInvoke(interpreter);

	// Extract the output tensor data.
	const TfLiteTensor* output_tensor = TfLiteInterpreterGetOutputTensor(
			interpreter, 0);
	TfLiteTensorCopyToBuffer(output_tensor, output_proba, sizeof(float));

}

int main(int argc, char *argv[]) {

	float output_proba;
	float input[NB_SAMPLE];
	int cpt;

	cpt = 0;

	TfLiteModel* model = TfLiteModelCreateFromFile("trigger_grand.tflite");

	TfLiteInterpreterOptions* options = TfLiteInterpreterOptionsCreate();

	// Create the interpreter.
	TfLiteInterpreter* interpreter = TfLiteInterpreterCreate(model, options);

	// Allocate tensors and populate the input tensor data.
	TfLiteInterpreterAllocateTensors(interpreter);

	while (1) {
		get_input(input);
		run_inference(interpreter, input, &output_proba);
		if (++cpt > 10)
			break;
	}

	// Dispose of the model and interpreter objects.
	TfLiteInterpreterDelete(interpreter);
	TfLiteInterpreterOptionsDelete(options);
	TfLiteModelDelete(model);
	return 0;
}
