#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "HoareMonitor.h"

using namespace std ;
using namespace HM ;

mutex output;

const int nFumadores = 3;


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
// funcion que produce un ingrediente aleatorio
int producirIngrediente( )
{
    return aleatorio<0, nFumadores>(); //El ingrediente nFumadores es el especial
}

//----------------------------------------------------------------------
// Declaración e implementación del monitor:

class Estanco : public HoareMonitor{
    private:
        int 
            mostrador;   //Variable que representa el ingrediente que ocupa el mostrador
        bool
            estancoAbierto;
        CondVar 
            estanquero,                       //Variable de condicion del estanquero
            fumadores[nFumadores];          //Variables de condicion para fumadores

    public:
        void ponerIngrediente (int i);      //Método para poner un ingrediente en el mostrador
        void esperarRecogidaIngrediente();  //Método que llama el estanquero para esperar se recoja un ingrediente
        void obtenerIngrediente (int i);    //Método que llama un fumador para obtener su ingrediente
        bool estaAbierto();                 //Método para comprobar si el estanco sigue abierto
        Estanco();                          //Inicializacion
};

//----------------------------------------------------------------------
// Constuctor/Inicializador del monitor

Estanco :: Estanco(){
    mostrador = -1;
    estancoAbierto = true;
    estanquero = newCondVar();
    for (unsigned i = 0; i < nFumadores; i++)
        fumadores[i]   = newCondVar();
}

//----------------------------------------------------------------------
// Función que llama el estanquero para poner un ingrediente generado en
// el mostrador

void Estanco :: ponerIngrediente(int i){
    if( i == nFumadores ){
        estancoAbierto = false;
        mostrador = i;
        for(int j = 0; j<nFumadores; j++){
            fumadores[j].signal();
        }
    }
    else{
        mostrador = i;
        if(!fumadores[i].empty())
            fumadores[i].signal();
    }
}
    

//----------------------------------------------------------------------
// Función que llama el estanquero después de poner un ingrediente generado
// para esperar a que este sea recogido

void Estanco :: esperarRecogidaIngrediente(){
    if(mostrador!=-1)
        estanquero.wait();
}

//----------------------------------------------------------------------
// Función que llaman los fumadores para obtener su ingrediente
// y acabar la espera del estanquero

void Estanco :: obtenerIngrediente (int i){
    if(mostrador != nFumadores){
        if(mostrador!=i)
            fumadores[i].wait();
        mostrador = -1;
        estanquero.signal();
    }   
}

//----------------------------------------------------------------------
// Función que comprueba si el estanco sigue abierto

bool Estanco :: estaAbierto(){
    return estancoAbierto;
}


//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero( MRef<Estanco> monitor )
{
    int ingrediente;
    while (monitor->estaAbierto()){
        ingrediente = producirIngrediente();
        output.lock();
        cout << "Estanquero produce ingrediente " << ingrediente << endl;
        output.unlock();
        monitor->ponerIngrediente(ingrediente);
        if(ingrediente!=nFumadores)
            monitor->esperarRecogidaIngrediente();
    }
    output.lock();
    cout << "El estanco ha cerrado. Estanquero vuelve a casa" << endl;
    output.unlock();

}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar
    output.lock();
    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;
    output.unlock();
   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar
    output.lock();
    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
    output.unlock();
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( MRef<Estanco> monitor, int num_fumador )
{
   while( monitor->estaAbierto() ){
      monitor->obtenerIngrediente(num_fumador);
      if(monitor->estaAbierto()){
        output.lock();
        cout << "\tEl fumador " << num_fumador << " retira su ingrediente" << endl;
        output.unlock();
        fumar(num_fumador);
      }
    }
    output.lock();
    cout << "El estanco ha cerrado. Fumador " << num_fumador << " vuelve a casa" << endl;
    output.unlock();
}

//----------------------------------------------------------------------

int main()
{
    MRef<Estanco> estanco = Create<Estanco>();

    thread  hebraEstanquero, hebrasFumadores[nFumadores];

    hebraEstanquero = thread( funcion_hebra_estanquero, estanco );
    for (int i = 0; i < nFumadores; i++)
        hebrasFumadores[i] = thread( funcion_hebra_fumador, estanco, i );

    hebraEstanquero.join();
    for (int i = 0; i < nFumadores; i++)
        hebrasFumadores[i].join();
}
