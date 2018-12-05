LIBS = -lfftw3 
CXX = g++ 

all:	detect

detect: detection_main.cpp header/detection_algorithm.cpp header/filt.cpp makefile
	${CXX} detection_main.cpp header/detection_algorithm.cpp header/filt.cpp ${LIBS} -o detect -O3 
clean:
	rm -f core *.o detect
