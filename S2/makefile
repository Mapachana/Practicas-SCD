INCLUDE = ./

all: prodcons_sc_lifo prodcons_sc_fifo prodcons_su_lifo prodcons_su_fifo

# ************ Limpieza ************
clean :
	echo "Limpiando..."
	rm ./prodcons_sc_fifo ./prodcons_sc_lifo ./prodcons_su_lifo ./prodcons_su_fifo

# ************** Compilacion de modulos ********

prodcons_sc_lifo : prodcons_sc_lifo.cpp HoareMonitor.cpp Semaphore.cpp
	g++ -std=c++11 -pthread -I ./ -o prodcons_sc_lifo prodcons_sc_lifo.cpp HoareMonitor.cpp Semaphore.cpp

prodcons_sc_fifo : prodcons_sc_fifo.cpp HoareMonitor.cpp Semaphore.cpp
	g++ -std=c++11 -pthread -I ./ -o prodcons_sc_fifo prodcons_sc_fifo.cpp HoareMonitor.cpp Semaphore.cpp

prodcons_su_lifo : prodcons_su_lifo.cpp HoareMonitor.cpp Semaphore.cpp
	g++ -std=c++11 -pthread -I ./ -o prodcons_su_lifo prodcons_su_lifo.cpp HoareMonitor.cpp Semaphore.cpp

prodcons_su_fifo : prodcons_su_fifo.cpp HoareMonitor.cpp Semaphore.cpp
	g++ -std=c++11 -pthread -I ./ -o prodcons_su_fifo prodcons_su_fifo.cpp HoareMonitor.cpp Semaphore.cpp



