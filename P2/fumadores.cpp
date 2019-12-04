#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"
#include "HoareMonitor.h"

using namespace std ;
using namespace HM ;


const int numero_fumadores = 3;
//Semaphore mostr_vacio(1); // Vale 1 si el mostrador está vacío y 0 si no.
//Semaphore ingrediente_disp[numero_fumadores] = {0,0,0}; //Array de Semaforos, vale 0 si el ingrediente i no está disponible y 1 si sí.

mutex mtx; //Asegura que no se mezclen los cout


class MonitorEstanco : public HoareMonitor
{
private:
   int ingrediente;
   CondVar mostrador_vacio;
   CondVar espera_ingrediente[numero_fumadores];
   
public:                    // constructor y métodos públicos
   MonitorEstanco(  ) ;           // constructor
   void obtenerIngrediente(int i);
   void ponerIngrediente(int i);
   void esperaRecogidaIngrediente();
} ;

MonitorEstanco::MonitorEstanco(){
   ingrediente = -1;
   mostrador_vacio = newCondVar();

   for (int i = 0; i < numero_fumadores; ++i)
      espera_ingrediente[i] = newCondVar();
}

void MonitorEstanco::obtenerIngrediente(int i){
   if (ingrediente != i)
      espera_ingrediente[i].wait();

   mtx.lock();
   cout << "Retirado ingrediente " << i << endl << flush;
   mtx.unlock();

   ingrediente = -1;

   mostrador_vacio.signal();
}

void MonitorEstanco::ponerIngrediente(int i){
   ingrediente = i;
   mtx.lock();
   cout << "Puesto ingrediente " << i << endl << flush;
   mtx.unlock();

   espera_ingrediente[i].signal();
}

void MonitorEstanco::esperaRecogidaIngrediente(){
   if(ingrediente != -1)
      mostrador_vacio.wait();
}



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

void funcion_hebra_estanquero( MRef<MonitorEstanco> monitor )
{
   int ingr;
   
   while(true){
      ingr = aleatorio<0, numero_fumadores-1>();
      monitor->esperaRecogidaIngrediente();
      monitor->ponerIngrediente(ingr);
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
void  funcion_hebra_fumador( int num_fumador, MRef<MonitorEstanco> monitor )
{
   while( true )
   {
      monitor->obtenerIngrediente(num_fumador);
      fumar(num_fumador);
   }
}

//----------------------------------------------------------------------

int main()
{
   MRef<MonitorEstanco> estanco = Create<MonitorEstanco>();
   thread hebra_estanquero(funcion_hebra_estanquero, estanco);
   thread hebras_fumadores[numero_fumadores];

   for (int i = 0; i < numero_fumadores; ++i)
      hebras_fumadores[i] = thread(funcion_hebra_fumador, i, estanco);

   for (int i = 0; i < numero_fumadores; ++i)
      hebras_fumadores[i].join();
   
   hebra_estanquero.join();
}
