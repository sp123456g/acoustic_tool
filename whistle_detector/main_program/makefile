LIBS = -lfftw3 
CXX = g++ 

all:	detect

detect: detection_main.cpp STFT_header/detection_algorithm.cpp makefile
	${CXX} detection_main.cpp STFT_header/detection_algorithm.cpp ${LIBS} -o detect -O3 
clean:
	rm -f core *.o detect
