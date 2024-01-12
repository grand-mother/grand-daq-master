/*
 * run_cpputest.cpp
 *
 *  Created on: Jan 12, 2024
 *      Author: grand
 */


#include "CppUTest/CommandLineTestRunner.h"

int main(int ac, char** av)
{
   return CommandLineTestRunner::RunAllTests(ac, av);
}

