#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;


int ingrediente_producido = -1;
const int numero_fumadores = 3;
const int numero_estanqueros = 2;
const int num_productos = 10;
int vector_productos[num_productos] = {0,0,0,0,0,0,0,0,0,0};
Semaphore mostr_vacio(1); // Vale 1 si el mostrador está vacío y 0 si no.
Semaphore ingrediente_disp[numero_fumadores] = {0,0,0}; //Array de Semaforos, vale 0 si el ingrediente i no está disponible y 1 si sí.
Semaphore escribir_ingrediente(1);
Semaphore leer_ingrediente(0);
Semaphore puede_proveer(1);
Semaphore est_puede_actuar(0);

mutex mtx; //Asegura que no se mezclen los cout


//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//---------------------------------------------------------------------
// funcion que ejecuta la hebra proveedora

void funcion_hebra_proveedora( ){
    while(true){
        sem_wait(puede_proveer);
        mtx.lock();
        cout << "Se va a suministrar" << endl;
        mtx.unlock();

        for (int i = 0; i < num_productos; ++i)
            vector_productos[i] = aleatorio<0,numero_fumadores-1>();
        sem_signal(est_puede_actuar);
    }
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero( int num_estanquero )
{
   int ingr;
   
   while(true){
        //ingr = aleatorio<0, numero_fumadores-1>();
        sem_wait(est_puede_actuar);

        for (int i = 0; i < num_productos; ++i){
            ingr = vector_productos[i];
            sem_wait(mostr_vacio);
            mtx.lock();
            cout << "Estanquero " << num_estanquero << " puesto ingrediente " << ingr << endl;
            mtx.unlock();

            sem_wait(escribir_ingrediente);
            ingrediente_producido = ingr;
            sem_signal(leer_ingrediente);

            sem_signal(ingrediente_disp[ingr]);
        }
        sem_signal(puede_proveer);
   }

}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar

   mtx.lock();
    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;
   mtx.unlock();

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

   mtx.lock();
    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
   mtx.unlock();

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
   int ingr;

   while( true )
   {
      sem_wait(ingrediente_disp[num_fumador]);
      
      sem_wait(leer_ingrediente);
      ingr = ingrediente_producido;
      mtx.lock();
      cout << "Fumador " << num_fumador << " coge el ingrediente" << endl;
      mtx.unlock();
      sem_signal(escribir_ingrediente);
      
      sem_signal(mostr_vacio);

      fumar(ingr);
   }
}

//----------------------------------------------------------------------

int main()
{
    thread hebra_proveedora = thread(funcion_hebra_proveedora);

    thread hebras_estanqueros[numero_estanqueros];

    for (int i = 0; i < numero_estanqueros; ++i)
    hebras_estanqueros[i] = thread (funcion_hebra_estanquero, i);
   
   thread hebras_fumadores[numero_fumadores];

   for (int i = 0; i < numero_fumadores; ++i)
      hebras_fumadores[i] = thread(funcion_hebra_fumador,i);

   for (int i = 0; i < numero_fumadores; ++i)
      hebras_fumadores[i].join();
   for (int i = 0; i < numero_estanqueros; ++i)
        hebras_estanqueros[i].join();
    hebra_proveedora.join();
}
