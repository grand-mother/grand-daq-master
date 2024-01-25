/*
 * test_buffer_eval_evt.cpp
 *
 *  Created on: Jan 12, 2024
 *      Author: grand
 */

#include "CppUTest/TestHarness.h"
#include "test_data.h"

extern "C"
{
#include "tflite_inference.h"
}

TEST_GROUP(TestTFlite)
{
   void setup()
   {
      // This gets run before every test
   }

   void teardown ()
   {
      // This gets run after every test
   }
};

TEST(TestTFlite, Test_create)
{
   S_TFLite* p_tf = NULL;

   p_tf = TFLT_create(NB_SAMPLE);
   CHECK(p_tf != NULL);
   printf("\n=== address struct %p", p_tf);
   printf("\n=============   end Test_create, %p", p_tf);
}
;

TEST(TestTFlite, Test_inference)
{
   S_TFLite* p_tf = NULL;
   int l_s;
   float *input;
   float proba;
   float val_ref = 0.085943;

   p_tf = TFLT_create(NB_SAMPLE);
   input = p_tf->a_3dtraces;

   for (l_s = 0; l_s < NB_SAMPLE; l_s++)
   {
      input[3 * l_s] = trace_1[l_s];
      input[3 * l_s + 1] = trace_2[l_s];
      input[3 * l_s + 2] = trace_3[l_s];
   }

   TFLT_inference(p_tf, &proba);
   DOUBLES_EQUAL(val_ref, proba, 1e-5);
   printf("\n=== proba: %f", proba);

   for (l_s = 0; l_s < NB_SAMPLE; l_s++)
   {
      input[3 * l_s] = 0.0;
      input[3 * l_s + 1] = 0.0;
      input[3 * l_s + 2] = 0.0;
   }
   TFLT_inference(p_tf, &proba);
   printf("\n=== proba: %f", proba);

   for (l_s = 0; l_s < NB_SAMPLE; l_s++)
   {
      input[3 * l_s] = trace_1[l_s];
      input[3 * l_s + 1] = trace_2[l_s];
      input[3 * l_s + 2] = trace_3[l_s];
   }
   TFLT_inference(p_tf, &proba);
   DOUBLES_EQUAL(val_ref, proba, 1e-5);
   printf("\n=== proba: %f", proba);

   printf("\n=============   end Test_inference");
}
;

TEST(TestTFlite, Test_delete)
{
   S_TFLite* p_tf = NULL;

   p_tf = TFLT_create(NB_SAMPLE);
   TFLT_delete(&p_tf);
   CHECK(p_tf == NULL);
   printf("\n=============   end Test_delete");
}
;
