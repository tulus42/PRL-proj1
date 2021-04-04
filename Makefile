CC=mpic++ --prefix /home/adrian/Stiahnuté/openmpi-1.6.5/ompi
CFLAGS=-std=c++11

all:
	$(CC) -g $(CFLAGS) pms.cpp -o pms $(FLAGS) $(LDFLAGS)

.PHONY: clean

clean:
	-rm pms

test:
	bash test.sh

run:
	mpirun -np 4 pms 