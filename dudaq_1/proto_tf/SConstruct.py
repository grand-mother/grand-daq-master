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

ARM64 = False

if ARM64:
	CC_val = 'aarch64-linux-gnu-gcc'
	m_LIBPATH='/home/grand/install/tf_lite/tflite_build'
else:
	CC_val = 'gcc'
	m_LIBPATH='/home/grand/install/tf_lite/tflite_build_amd64'


env = Environment(ENV = os.environ, CC=CC_val, CCFLAGS=f'-Wall -O2 -fmessage-length=0 -MMD -MP -I/home/grand/install/tf_lite/tensorflow_src')


# Build application
env.Program('tf_inference', 'main.c',
	LIBS = 'tensorflowlite_c',
	LIBPATH = m_LIBPATH)
