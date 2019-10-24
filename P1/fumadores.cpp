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
Semaphore mostr_vacio(1); // Vale 1 si el mostrador está vacío y 0 si no.
Semaphore ingrediente_disp[numero_fumadores] = {0,0,0}; //Array de Semaforos, vale 0 si el ingrediente i no está disponible y 1 si sí.
Semaphore escribir_ingrediente(1);
Semaphore leer_ingrediente(0);

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

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
   int ingr;
   
   while(true){
      ingr = aleatorio<0, numero_fumadores-1>();

      sem_wait(mostr_vacio);
      mtx.lock();
      cout << "Puesto ingrediente " << ingr << endl;
      mtx.unlock();

      sem_wait(escribir_ingrediente);
      ingrediente_producido = ingr;
      sem_signal(leer_ingrediente);

      sem_signal(ingrediente_disp[ingr]);
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
   thread hebra_estanquero(funcion_hebra_estanquero);
   thread hebras_fumadores[numero_fumadores];

   for (int i = 0; i < numero_fumadores; ++i)
      hebras_fumadores[i] = thread(funcion_hebra_fumador,i);

   for (int i = 0; i < numero_fumadores; ++i)
      hebras_fumadores[i].join();
   
   hebra_estanquero.join();
}
