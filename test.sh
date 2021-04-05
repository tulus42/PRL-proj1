#!/bin/bash

#pocet cisel bud zadam nebo 16 :)

numbers=16;
proc_num=5

#preklad cpp zdrojaku
mpic++ -o pms pms.cpp


#vyrobeni souboru s random cisly
dd if=/dev/random bs=1 count=$numbers of=numbers

#spusteni
mpirun --oversubscribe -np $proc_num pms 

#uklid
rm -f pms numbers log.out