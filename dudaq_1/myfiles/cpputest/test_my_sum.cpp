#include "CppUTest/TestHarness.h"

extern "C" {
#include "my_sum.h"
}

TEST_GROUP(TestMySum) {
	void setup() {
		// This gets run before every test
	}

	void teardown() {
		// This gets run after every test
	}
};

TEST(TestMySum, Test_MySumBasic) {
	LONGS_EQUAL(7, my_sum(3, 4));
}
