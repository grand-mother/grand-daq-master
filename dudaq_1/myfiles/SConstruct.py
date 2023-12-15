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
CC_val = 'aarch64-linux-gnu-gcc'
#CC_val = 'gcc'
env = Environment(ENV = os.environ, CC=CC_val, CCFLAGS=f'-Wall -O2 -fmessage-length=0 -MMD -MP -DVERSION={branch} -lpthread -I/home/grand/install/tf_lite/tensorflow_src -DGIT_SHA={sha_git} ')


# Build application
env.Program('dudaq_jmc', ['ad_shm.c','buffer.c','dudaq.c','monitor.c','scope.c'])
