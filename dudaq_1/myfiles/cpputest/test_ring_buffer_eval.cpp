/*
 * test_buffer_eval_evt.cpp
 *
 *  Created on: Jan 12, 2024
 *      Author: grand
 */

#include "CppUTest/TestHarness.h"

extern "C" {
#include "ring_buffer_eval.h"
}

TEST_GROUP(TestRingBufferEval) {
	void setup() {
		// This gets run before every test
	}

	void teardown() {
		// This gets run after every test
	}
};

TEST(TestRingBufferEval, Test_create) {
	RingBufferEval_struct* p_buf=NULL;

	p_buf = RBE_create (16,2);
	CHECK(p_buf != NULL );
}

TEST(TestRingBufferEval, Test_delete) {
	RingBufferEval_struct* p_buf=NULL;

	p_buf = RBE_create (16,2);
	CHECK(p_buf == NULL );
	RBE_delete(&p_buf);
	CHECK(p_buf == NULL );
}

