import os
import glob


# try to build ouside of src directory
#SConscript('makefile_scons', build_dir='../build')
PATH_SRC = "../files/"

# PATH to adapt 
CPPUTEST_HOME = "/home/grand/install/cpputest/include"
PATH_TF = "/home/grand/install/tf_lite/tensorflow_src"

# sources
src_prj = glob.glob(f"{PATH_SRC}*.c")
src_prj.remove('../files/dudaq.c')
src_prj.remove('../files/scope.c')
src_tests = glob.glob("*.cpp")
src_all = src_tests + src_prj

# compilation option
CPPPATH = [CPPUTEST_HOME, PATH_SRC, PATH_TF]
CCFLAGS = "-Wall -Wextra -O0 -fmessage-length=0 -MMD -MP -DCPPUTEST"
#CCFLAGS +=  " -DCPPUTEST_MEM_LEAK_DETECTION_DISABLED"
#CCFLAGS += " -fprofile-arcs -ftest-coverage -g" 
env = Environment(CPPPATH=CPPPATH, CCFLAGS=CCFLAGS)


# compilation and link
env.Program('run_cpputest', src_all, 
	LIBS = ['CppUTest','CppUTestExt',"gcov",'tensorflowlite_c','pthread'],
	LIBPATH = ["/home/grand/install/cpputest/cpputest_build/lib",'/home/grand/install/tf_lite/tflite_build_amd64'])
