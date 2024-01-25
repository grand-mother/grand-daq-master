/*
 * test_buffer_eval_evt.cpp
 *
 *  Created on: Jan 12, 2024
 *      Author: grand
 */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "ring_buffer_eval.h"
}

TEST_GROUP(TestRingBufferEval)
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

TEST(TestRingBufferEval, Test_create)
{
   S_RingBufferEval* p_rbe = NULL;

   p_rbe = RBE_create(16, 2);
   CHECK(p_rbe != NULL);
   /*
    * TODO
    p_rbe = RBE_create(1600000000000000, 2000000000000);
    CHECK(p_rbe == NULL);
    */
}

TEST(TestRingBufferEval, Test_delete)
{
   S_RingBufferEval* p_rbe = NULL;

   p_rbe = RBE_create(16, 2);
   CHECK(p_rbe != NULL);
   RBE_delete(&p_rbe);
   CHECK(p_rbe == NULL);
}

TEST(TestRingBufferEval, Test_inc_modulo)
{
   uint16_t index;

   index = 10;
   RBE_inc_modulo(&index, 16);
   CHECK(index == 11);

   index = 16;
   RBE_inc_modulo(&index, 16);
   CHECK(index == 0);

   /*
    * TODO
    index = 16;
    RBE_inc_modulo(&index, -1);
    printf("\n%d", index);
    CHECK(index == 0);
    */
}

TEST(TestRingBufferEval, Test_after_xxx)
{
   S_RingBufferEval* p_rbe = NULL;
   int nb_buf = 2;

   p_rbe = RBE_create(16, nb_buf);
   CHECK(p_rbe->nb_write == nb_buf);
   CHECK(p_rbe->inext_write == 0);
   /* write */
   RBE_after_write(p_rbe);
   CHECK(p_rbe->nb_write == (nb_buf - 1));
   CHECK(p_rbe->inext_write == 1);
   /* write */
   RBE_after_write(p_rbe);
   CHECK(p_rbe->nb_write == (nb_buf - 2));
   CHECK(p_rbe->nb_eval == 2);
   CHECK(p_rbe->inext_write == 0);
   /* eval */
   RBE_after_eval(p_rbe);
   CHECK(p_rbe->nb_eval == 1);
   CHECK(p_rbe->nb_trig == 1);
   CHECK(p_rbe->inext_eval == 1);
   /* trigger */
   RBE_after_trigger(p_rbe);
   CHECK(p_rbe->nb_trig == 0);
   CHECK(p_rbe->inext_trig == 1);
   /* eval */
   RBE_after_eval(p_rbe);
   /* trigger */
   RBE_after_trigger(p_rbe);
   printf("\n%d", p_rbe->nb_write);
   CHECK(p_rbe->nb_write == nb_buf);
   CHECK(p_rbe->nb_eval == 0);
   CHECK(p_rbe->nb_trig == 0);
   CHECK(p_rbe->inext_write == 0);
   CHECK(p_rbe->inext_eval == 0);
   CHECK(p_rbe->inext_eval == 0);
}
