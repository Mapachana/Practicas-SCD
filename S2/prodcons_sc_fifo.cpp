// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// archivo: prodcons_1.cpp
// Ejemplo de un monitor en C++11 con semántica SC, para el problema
// del productor/consumidor, con un único productor y un único consumidor.
// Opcion LIFO (stack)
//
// Historial:
// Creado en Julio de 2017
// -----------------------------------------------------------------------------


#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>

using namespace std ;

constexpr int
   num_items  = 40 ;     // número de items a producir/consumir

mutex
   mtx ;                 // mutex de escritura en pantalla
unsigned
   cont_prod[num_items], // contadores de verificación: producidos
   cont_cons[num_items]; // contadores de verificación: consumidos
int num_productores = 5;
int num_consumidores = 5;

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

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato()
{
   static int contador = 0 ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "producido: " << contador << endl << flush ;
   mtx.unlock();
   cont_prod[contador] ++ ;
   return contador++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   if ( num_items <= dato )
   {
      cout << " dato === " << dato << ", num_items == " << num_items << endl ;
      assert( dato < num_items );
   }
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "                  consumido: " << dato << endl ;
   mtx.unlock();
}
//----------------------------------------------------------------------

void ini_contadores()
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  cont_prod[i] = 0 ;
      cont_cons[i] = 0 ;
   }
}

//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << flush ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SC, un prod. y un cons.

class ProdConsSC
{
 private:
 static const int num_celdas_total = 10; // constantes: núm. de entradas del buffe
 int buffer[num_celdas_total]; //  buffer de tamaño fijo, con los datos
 int primera_libre = 0; // indice de celda de la próxima inserción
 int primera_ocupada = 0; //indice de celda de la proxima lectura
 int num = 0;
 mutex cerrojo_monitor; // cerrojo del monitor
 condition_variable cola_cons; // cola donde espera el consumidor (n>0)
 condition_variable cola_prod; // cola donde espera el productor  (n<num_celdas_total)
 mutex interno_prod;
 mutex interno_cons;
 mutex interno_n;

 public:                    // constructor y métodos públicos
   ProdConsSC(  ) ;           // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor ); // insertar un valor (sentencia E) (productor)
} ;
// -----------------------------------------------------------------------------

ProdConsSC::ProdConsSC(  )
{
   primera_libre = 0 ;
   primera_ocupada = 0;
   num = 0;
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdConsSC::leer(  )
{
   // ganar la exclusión mutua del monitor con una guarda
   unique_lock<mutex> guarda( cerrojo_monitor );

   // esperar bloqueado hasta que 0 < num_celdas_ocupadas
   interno_n.lock();
   if (num <= 0){
      interno_n.unlock();
      cola_cons.wait(guarda);
   }
   else{
      interno_n.unlock();
   }
   

   // hacer la operación de lectura, actualizando estado del monitor
   //assert( 0 < primera_libre  );
   interno_cons.lock();
    //primera_ocupada = (primera_ocupada + 1) % num_celdas_total;
   const int valor = buffer[primera_ocupada] ;
   primera_ocupada = (primera_ocupada + 1) % num_celdas_total;

   interno_cons.unlock();

   interno_n.lock();
   num--;
   interno_n.unlock();

   // señalar al productor que hay un hueco libre, por si está esperando
   cola_prod.notify_one();

   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------

void ProdConsSC::escribir( int valor )
{
   // ganar la exclusión mutua del monitor con una guarda
   unique_lock<mutex> guarda( cerrojo_monitor );

   // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
    interno_n.lock();
    if(num >= num_celdas_total){
       interno_n.unlock();
       cola_prod.wait(guarda);
    }
    else{
       interno_n.unlock();
    }

   //cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;
   assert( primera_libre < num_celdas_total );

   // hacer la operación de inserción, actualizando estado del monitor
   interno_prod.lock();
   buffer[primera_libre] = valor ;
   primera_libre = (primera_libre + 1) % num_celdas_total ;
   interno_prod.unlock();

   interno_n.lock();
   num++;
   interno_n.unlock();

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   cola_cons.notify_one();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( ProdConsSC * monitor, int num )
{
   for( unsigned i = 0 ; i < num_items ; i+= num )
   {
      int valor = producir_dato() ;
      monitor->escribir( valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( ProdConsSC * monitor, int num )
{
   for( unsigned i = 0 ; i < num_items ; i+= num )
   {
      int valor = monitor->leer();
      consumir_dato( valor ) ;
   }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (Varios prod/cons, Monitor SC, buffer LIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   ProdConsSC monitor ;
          
   thread hebras_productoras[num_productores];
   thread hebras_consumidoras[num_consumidores];

   for (int i = 0; i < num_productores; ++i)
      hebras_productoras[i] = thread(funcion_hebra_productora, &monitor, num_productores);
   for (int i = 0; i < num_consumidores; ++i)
      hebras_consumidoras[i] = thread(funcion_hebra_consumidora, &monitor, num_consumidores);

   for (int i = 0; i < num_productores; ++i)
      hebras_productoras[i].join();
   for (int i = 0; i < num_consumidores; ++i)
      hebras_consumidoras[i].join();

   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores() ;
}
