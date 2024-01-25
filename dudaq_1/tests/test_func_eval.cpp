/*
 * test_buffer_eval_evt.cpp
 *
 *  Created on: Jan 12, 2024
 *      Author: grand
 */

#include "CppUTest/TestHarness.h"
#include "test_data.h"

extern "C" {
#include "func_eval_evt.h"
#include "tflite_inference.h"
#include <unistd.h>
}



TEST_GROUP(TestFuncEval) {
	void setup() {
		// This gets run before every test
	}

	void teardown() {
		// This gets run after every test
	}
};

TEST(TestFuncEval, Test_run) {
	S_TFLite* p_tf = NULL;

	p_tf = TFLT_create(NB_SAMPLE);

	printf("\n=== address struct %p", p_tf);
	printf("\n=============   end Test_create, %p", p_tf);
}
;

TEST(TestFuncEval, Test_one_trace) {
	S_TFLite* p_tflt = NULL;
	float *input;
	float proba;
	float val_ref = 0.085943;
	pthread_t thread_eval;
	int idx_evt=0;
	int ret, l_s;

	/* Create 2 ring buffer event for trigger T2 */
	S_RingBufferEval *p_rbuftrig = RBE_create (NB_SAMPLE*sizeof(float), 3);

   /* Create 2 Tensorflow Lite inference structure */
   p_tflt = TFLT_create (p_rbuftrig->size_buffer);

   /* Create 2 process of evaluation */
   S_FuncEval *p_feev = FEEV_create (p_rbuftrig, (void*) p_tflt);


   /* Create 2 thread */
   ret = pthread_create (&thread_eval, NULL,FEEV_run, p_feev);
   if (ret != 0)
   {
	  printf ("[scope_create_thread_T2] Failed create thread 1, ret=%d ", ret);
   }
   usleep (500);

    /* Write */
   	p_rbuftrig->a_prob[idx_evt] = 10.0;
   	printf("\n=== before proba: %f", p_rbuftrig->a_prob[idx_evt]);
	input = (float *)p_rbuftrig->a_buffers;

	for (l_s = 0; l_s < NB_SAMPLE; l_s++) {
		input[3 * l_s] = trace_1[l_s];
		input[3 * l_s + 1] = trace_2[l_s];
		input[3 * l_s + 2] = trace_3[l_s];
	}
	RBE_after_write(p_rbuftrig);

	/* Eval*/
	while(p_rbuftrig->nb_eval == 1)
	{
		printf("\nSleep now");
		usleep(10);
	}

	//DOUBLES_EQUAL(val_ref, proba, 1e-5);
	printf("\n=== after proba: %f", p_rbuftrig->a_prob[idx_evt]);
	FEEV_stop();

	/* trigger */
	printf("\n=============   end Test_run");
}
;

