import os


CPPUTEST_HOME = "/home/grand/install/cpputest/include"
env = Environment(CPPPATH = [f"{CPPUTEST_HOME}", ".."])


# Build cpputest_example
if False:
	src_files = Split('test_my_sum.cpp my_sum.c run_cpputest.cpp ')
	env.Program('run_cpputest', src_files, 
		LIBS = ['CppUTest','CppUTestExt'],
		LIBPATH = "/home/grand/install/cpputest/cpputest_build/lib")

src_files = Split('test_ring_buffer_eval.cpp ../ring_buffer_eval.c run_cpputest.cpp ')
env.Program('run_cpputest', src_files, 
	LIBS = ['CppUTest','CppUTestExt'],
	LIBPATH = "/home/grand/install/cpputest/cpputest_build/lib")
