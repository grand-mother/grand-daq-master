Traces: Traces.c Traces.h
	gcc  Traces.c  -o Traces

Hist: Hist.c Traces.h Makefile
	clang++ Hist.c -O3 \
	-L/usr/local/lib -lfftw3 \
	-I /Users/timmer/Work/GRAND/Program/c_grand_utils -L /Users/timmer/Work/GRAND/Program/c_grand_utils -lgrand_utillib \
	-I /Users/timmer/Work/GRAND/Simu/GRAND/c-antenna-test-master/src \
	-I ../dudaq/files \
	-I${ROOTSYS}/include -L${ROOTSYS}/lib -lCore -lRIO -lhist -lMathCore -lGpad -std=c++14 \
	-o Hist
	
MDanal: MDanal.c Traces.h Makefile
	clang++ MDanal.c  \
	-L/usr/local/lib -lfftw3 \
	-I /Users/timmer/Work/GRAND/Program/c_grand_utils -L /Users/timmer/Work/GRAND/Program/c_grand_utils -lgrand_utillib \
	-I /Users/timmer/Work/GRAND/Simu/GRAND/c-antenna-test-master/src \
	-I ../DU \
	-I${ROOTSYS}/include -L${ROOTSYS}/lib -lCore -lRIO -lhist -lMathCore -lGpad -std=c++14 \
	-o MDanal

