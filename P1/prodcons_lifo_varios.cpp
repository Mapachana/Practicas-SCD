#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

//**********************************************************************
// variables compartidas

const int num_items = 40 ,   // número de items
	       tam_vec   = 10 ;   // tamaño del buffer
unsigned  cont_prod[num_items] = {0}, // contadores de verificación: producidos
          cont_cons[num_items] = {0}; // contadores de verificación: consumidos

unsigned buff[40]={0};
unsigned contador = 0;

unsigned num_hebras_productoras = 5;
unsigned num_hebras_consumidoras = 5;

//unsigned contador_prod_total = 0;
//unsigned contador_cons_total = 0;

Semaphore can_produce(tam_vec);
Semaphore can_read(0);
Semaphore can_modify_contador(1);

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

   cout << "producido: " << contador << endl << flush ;

   cont_prod[contador] ++ ;
   return contador++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   assert( dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "                  consumido: " << dato << endl ;

}


//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." ;
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  if ( cont_prod[i] != 1 )
      {  cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {  cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------

void  funcion_hebra_productora( int num_hebra  )
{
   //for( unsigned i = 0 ; i < num_items ; i++ )
   for( unsigned i = num_hebra ; i < num_items ; i+=num_hebras_productoras )
   {
      int dato;

      /*sem_wait(can_modify_contador);
        if (contador_prod_total >= num_items){
            sem_signal(can_modify_contador);
            break;
        }
        else{
            contador_prod_total++;
            sem_signal(can_modify_contador);
         }*/

      dato = producir_dato() ;
      
      sem_wait(can_produce);
      sem_wait(can_modify_contador);

      buff[contador] = dato;
      contador++;

      sem_signal(can_modify_contador);
      sem_signal(can_read);

   }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora( int num_hebra )
{
   //for( unsigned i = 0 ; i < num_items ; i++ )
   for( unsigned i = num_hebra ; i < num_items ; i+=num_hebras_consumidoras )
   {
      int dato ;

      /*sem_wait(can_modify_contador);
      if (contador_cons_total >= num_items){
            sem_signal(can_modify_contador);
            break;
      }
      else{
         contador_cons_total++;
         sem_signal(can_modify_contador);
      }*/
      
      sem_wait(can_read);
      sem_wait(can_modify_contador);

      contador--;
      dato = buff[contador];

      sem_signal(can_modify_contador);
      sem_signal(can_produce);

      consumir_dato( dato ) ;
    }
}
//----------------------------------------------------------------------

int main()
{
   cout << "--------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (solución LIFO)." << endl
        << "--------------------------------------------------------" << endl
        << flush ;

    thread hebra_productora[num_hebras_productoras];
    thread hebra_consumidora[num_hebras_consumidoras];
   
   //for (int i = 0; i < num_hebras_productoras-1; ++i)
    for (int i = 0; i < num_hebras_productoras; ++i)
        hebra_productora[i] = thread ( funcion_hebra_productora, i );

    for (int i = 0; i < num_hebras_consumidoras; ++i)
        hebra_consumidora[i] = thread ( funcion_hebra_consumidora, i );
   
   //for (int i = 0; i < num_hebras_productoras-1; ++i)
    for (int i = 0; i < num_hebras_productoras; ++i)
        hebra_productora[i].join() ;

    for (int i = 0; i < num_hebras_consumidoras; ++i)
        hebra_consumidora[i].join() ;


   test_contadores();
}
