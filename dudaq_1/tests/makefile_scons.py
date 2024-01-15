import os

# PATH to adapt 

PATH_SRC = "../myfiles/"
CPPUTEST_HOME = "/home/grand/install/cpputest/include"


# Build cpputest_example
if False:
	src_files = Split('test_my_sum.cpp my_sum.c run_cpputest.cpp ')
	env.Program('run_cpputest', src_files, 
		LIBS = ['CppUTest','CppUTestExt'],
		LIBPATH = "/home/grand/install/cpputest/cpputest_build/lib")

# sources

src_name_only = ["ring_buffer_eval.c"]
src_prj = [PATH_SRC+name for name in src_name_only]
src_all = ['test_ring_buffer_eval.cpp', 'run_cpputest.cpp'] + src_prj


# compilation option

CPPPATH = [f"{CPPUTEST_HOME}", f"{PATH_SRC}"]
CCFLAGS = "-Wall -Wextra -O2 -fmessage-length=0 -MMD -MP"
env = Environment(CPPPATH=CPPPATH, CCFLAGS=CCFLAGS)


# compilation and link
env.Program('run_cpputest', src_all, 
	LIBS = ['CppUTest','CppUTestExt'],
	LIBPATH = "/home/grand/install/cpputest/cpputest_build/lib")
