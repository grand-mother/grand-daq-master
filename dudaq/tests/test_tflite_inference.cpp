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
#include <string.h>
#include <math.h>
}

extern float G_quantif;

TEST_GROUP(TestTFlite)
{
   void setup()
   {
      // This gets run before every test
   }

   void teardown()
   {
      // This gets run after every test
   }
};

TEST(TestTFlite, Test_create_delete)
{
   S_TFLite* p_tf = NULL;

   p_tf = TFLT_create(1);
   CHECK(p_tf != NULL);
   printf("\n=== address struct %p", p_tf);
   printf("\n=============   end Test_create_delete, %p", p_tf);
   TFLT_delete(&p_tf);
}
;

TEST(TestTFlite, Test_inference)
{
   S_TFLite* p_tf = NULL;
   int l_s;
   float *input=NULL;
   float proba;
   float val_ref = 0.085943;

   p_tf = TFLT_create(1);
   input = p_tf->a_3dtraces;

   for (l_s = 0; l_s < TFLT_SAMPLE_IN_TRACE; l_s++)
   {
      input[3 * l_s] = trace_1[l_s];
      input[3 * l_s + 1] = trace_2[l_s];
      input[3 * l_s + 2] = trace_3[l_s];
   }

   TFLT_inference(p_tf, &proba);
   DOUBLES_EQUAL(val_ref, proba, 1e-5);
   printf("\n=== proba: %f", proba);

   for (l_s = 0; l_s < TFLT_SAMPLE_IN_TRACE; l_s++)
   {
      input[3 * l_s] = 0.0;
      input[3 * l_s + 1] = 0.0;
      input[3 * l_s + 2] = 0.0;
   }
   TFLT_inference(p_tf, &proba);
   printf("\n=== proba: %f", proba);

   for (l_s = 0; l_s < TFLT_SAMPLE_IN_TRACE; l_s++)
   {
      input[3 * l_s] = trace_1[l_s];
      input[3 * l_s + 1] = trace_2[l_s];
      input[3 * l_s + 2] = trace_3[l_s];
   }
   TFLT_inference(p_tf, &proba);
   DOUBLES_EQUAL(val_ref, proba, 1e-5);
   printf("\n=== proba: %f", proba);
   printf("\n=============   end Test_inference");
   TFLT_delete(&p_tf);
   printf("\n=============   end Test_inference");
}
;

TEST(TestTFlite, Test_preprocessing)
{
   S_TFLite* p_tf = NULL;
   int l_s;
   float *input=NULL;
   float proba;
   float val_ref = 0.085943;

   p_tf = TFLT_create(1);
   input = p_tf->a_3dtraces;
   uint32_t a_input[TFLT_SAMPLE_IN_TRACE*3];

   memset(a_input, 0, TFLT_SAMPLE_IN_TRACE*3*sizeof(uint32_t));

   int idx = 0;
   int16_t val1, val2;
   val1 = -1;
   printf("%d %d %#X\n", sizeof(int16_t), val1, val1&0xffff);
   val2=2;
   printf("%d %d %#X\n", val2, val2<<1, (val2<<1)&0xffff);
   printf("%f\n", G_quantif);
   for (l_s = 0; l_s < TFLT_SAMPLE_IN_TRACE/2; l_s++)
   {
      val1 = (int16_t)(round(trace_1[idx]*G_quantif));
      //printf("val:  %d %d\n", val1, val2);
      a_input[l_s] = (val1&0xffff) | a_input[l_s];
//	  printf("\n%f %X", trace_1[idx], val&0xffff);
//	  printf("\na_input %#10X", a_input[l_s]);
      val2 = (int16_t)(round(trace_1[idx+1]*G_quantif));
//printf("val:  %d %d\n", val1, val2);
      a_input[l_s] = (val2&0xffff)<<16 | a_input[l_s];
//	  printf("\n%f %X", trace_1[idx+1], val&0xffff);
//	  printf("\na_input %X", a_input[l_s]);
//	  printf("\n==========");

      if (l_s < 4)
      {

//		  printf("\n%f ", G_quantif);
//		  printf("%f %X\n", trace_1[idx+1], val);
//		  printf("%X\n", a_input[l_s]);
         printf("%f %f ", trace_1[idx],trace_1[idx+1] );
         printf(" , %d %d %X\n", val1, val2, a_input[l_s]);
      }
//		else {
//			break;
//		}
      idx += 2;
   }
   idx=0;
   for (l_s = 512; l_s < (512+TFLT_SAMPLE_IN_TRACE/2); l_s++)
   {
      val1 = (int16_t)(round(trace_2[idx]*G_quantif));
      //printf("val:  %d %d\n", val1, val2);
      a_input[l_s] = (val1&0xffff) | a_input[l_s];
//	  printf("\n%f %X", trace_1[idx], val&0xffff);
//	  printf("\na_input %#10X", a_input[l_s]);
      val2 = (int16_t)(round(trace_2[idx+1]*G_quantif));
//		printf("val:  %d %d\n", val1, val2);
      a_input[l_s] = (val2&0xffff)<<16 | a_input[l_s];
//	  printf("\n%f %X", trace_1[idx+1], val&0xffff);
//	  printf("\na_input %X", a_input[l_s]);
//	  printf("\n==========");
      idx += 2;
   }
   idx= 0;
   for (l_s = 1024; l_s < (1024+TFLT_SAMPLE_IN_TRACE/2); l_s++)
   {
      val1 = (int16_t)(round(trace_3[idx]*G_quantif));
      //printf("val:  %d %d\n", val1, val2);
      a_input[l_s] = (val1&0xffff) | a_input[l_s];
//	  printf("\n%f %X", trace_1[idx], val&0xffff);
//	  printf("\na_input %#10X", a_input[l_s]);
      val2 = (int16_t)(round(trace_3[idx+1]*G_quantif));
//		printf("val:  %d %d\n", val1, val2);
      a_input[l_s] = (val2&0xffff)<<16 | a_input[l_s];
//	  printf("\n%f %X", trace_1[idx+1], val&0xffff);
//	  printf("\na_input %X", a_input[l_s]);
//	  printf("\n==========");
      idx += 2;
   }

   printf("\narray uint32:\n");
   for (l_s = 0; l_s < 8; l_s++)
   {
      printf("%X ", a_input[l_s]);
   }

   TFLT_preprocessing(p_tf, a_input);
   printf("\nTFLT_preprocessing:\n");
   for (l_s = 0; l_s < 36; l_s++)
   {
      printf("%f ", p_tf-> a_3dtraces[l_s]);
   };
   printf("\nTrue value:\n");
   for (l_s = 0; l_s < 12; l_s++)
   {
      printf("%f ", trace_1[l_s]);
      printf("%f ", trace_2[l_s]);
      printf("%f ", trace_3[l_s]);
   }
   printf("\nCppuTest check:\n");
   for(l_s=0; l_s < TFLT_SAMPLE_IN_TRACE; l_s++)
   {
      DOUBLES_EQUAL(p_tf-> a_3dtraces[3*l_s], trace_1[l_s], 1e-5);
      DOUBLES_EQUAL(p_tf-> a_3dtraces[3*l_s+1], trace_2[l_s], 1e-5);
      DOUBLES_EQUAL(p_tf-> a_3dtraces[3*l_s+2], trace_3[l_s], 1e-5);
      //printf("%d ",l_s);
   }
   TFLT_delete(&p_tf);
   printf("\n=============   end Test_inference_eventv2");
}
;

