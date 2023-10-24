import os
import subprocess

cmd = "git rev-parse --abbrev-ref HEAD"
branch = subprocess.getoutput(cmd)
cmd = "git rev-parse --short  HEAD"
sha_git = subprocess.getoutput(cmd)


# it's necessary to have le path of aarch64-linux-gnu-gcc compilator
# with my installation it's here
# . /home/grand/install/xilinx/SDK/2018.3/settings64.sh
# so

# ################################
#            WARNING
# Before compilation add aarch64-linux-gnu-gcc in your path, 
# aarch64-linux-gnu-gcc is installed with Xilink SDK
#
# to build, just 
# $scons
# 
# to clean, just
# $scons -c
#
# ################################


#https://stackoverflow.com/questions/61164904/compiler-not-found-when-building-with-scons
# => ENV = os.environ need to retrieve the path of aarch64-linux-gnu-gcc
CC_val = 'aarch64-linux-gnu-gcc'
CC_val = 'gcc'
env = Environment(ENV = os.environ, CC=CC_val, CCFLAGS=f'-Wall -O2 -fmessage-length=0 -MMD -MP -DVERSION={branch} -DGIT_SHA={sha_git} ')


env.Program('dudaq_jmc', ['ad_shm.c','buffer.c','dudaq.c','monitor.c','scope.c'])

