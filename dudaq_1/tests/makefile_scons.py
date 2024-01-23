import os
import glob

# PATH to adapt 

PATH_SRC = "../myfiles/"
CPPUTEST_HOME = "/home/grand/install/cpputest/include"
PATH_TF = "/home/grand/install/tf_lite/tensorflow_src"


# sources
src_prj = glob.glob(f"{PATH_SRC}*.c")
print(src_prj)
src_tests = ['test_ring_buffer_eval.cpp', 'run_cpputest.cpp','test_tflite_inference.cpp','test_func_eval.cpp']
src_all = src_tests + src_prj


# compilation option

CPPPATH = [f"{CPPUTEST_HOME}", f"{PATH_SRC}", PATH_TF]
CCFLAGS = "-Wall -Wextra -O0 -fmessage-length=0 -MMD -MP -DCPPUTEST"
CCFLAGS += " -fprofile-arcs -ftest-coverage -g" 
env = Environment(CPPPATH=CPPPATH, CCFLAGS=CCFLAGS)


# compilation and link
env.Program('run_cpputest', src_all, 
	LIBS = ['CppUTest','CppUTestExt',"gcov",'tensorflowlite_c','pthread'],
	LIBPATH = ["/home/grand/install/cpputest/cpputest_build/lib",'/home/grand/install/tf_lite/tflite_build_amd64'])
