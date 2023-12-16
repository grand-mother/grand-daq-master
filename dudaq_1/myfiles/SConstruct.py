# ################################
#
# "Makefile" of dudaq excutable with Scons builder manager
#            see website of Scons : https://scons.org/
#
# Configuration:
# --------------
#  1) Scons installation xith python3
#     python3 pip -m install scons
# 
#  2) ARM compilation tools chain
#     it's necessary to have le path of aarch64-linux-gnu-gcc compilator
#     with my installation it's here
#     . /home/grand/install/xilinx/SDK/2018.3/settings64.sh
#     now C compilator "aarch64-linux-gnu-gcc" must be available in PATH
# 
#
# Build executable dudaq:
# ----------------------
# in terminal , in the same directory of this file, ie "SConstruct.py"
#    $ scons   
#  
# 
# Clean build:
# ------------
#    $ scons -c
#
# ################################

import os
import subprocess

# retrieve git informations
cmd = "git rev-parse --abbrev-ref HEAD"
branch = subprocess.getoutput(cmd)

cmd = "git rev-parse --short  HEAD"
sha_git = subprocess.getoutput(cmd)


# Define environnement and flag compilation
#https://stackoverflow.com/questions/61164904/compiler-not-found-when-building-with-scons
# => ENV = os.environ need to retrieve the path of aarch64-linux-gnu-gcc

ARM64 = False

if ARM64:
	CC_val = 'aarch64-linux-gnu-gcc'
	m_LIBPATH='/home/grand/install/tf_lite/tflite_build'
	# -mcpu=name  -march=name  -mfpu=name -mtune=name
	#a53 = "-mcpu=cortex-a53 -march=armv8-a -mfpu=neon-vfpv4 -mtune=cortex-a53"
	# can't use -mfpu option ...?
	arch_opt = "-mcpu=cortex-a53 -march=armv8-a+simd -mtune=cortex-a53 "
else:
	CC_val = 'gcc'
	m_LIBPATH='/home/grand/install/tf_lite/tflite_build_amd64'
	arch_opt = "-march=native"

env = Environment(ENV = os.environ, CC=CC_val, CCFLAGS=f'-Wall -O2 -fmessage-length=0 -MMD -MP -DVERSION={branch} -I/home/grand/install/tf_lite/tensorflow_src -DGIT_SHA={sha_git} ')


# Build application.c
env.Program('dudaq_jmc', ['ad_shm.c','buffer.c','dudaq.c','monitor.c','scope.c','ring_buffer_eval.c','tflite_inference.c','func_eval_evt.c'],
	LIBS = ['tensorflowlite_c','pthread'],
	LIBPATH = m_LIBPATH)


