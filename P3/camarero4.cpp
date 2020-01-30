// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: filosofos-plantilla.cpp
// Implementación del problema de los filósofos (sin camarero).
// Plantilla para completar.
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
// -----------------------------------------------------------------------------


#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   num_filosofos = 5 ,
   num_procesos  = 2*num_filosofos +1,
   id_camarero = num_procesos-1;

const int tag_sentarse = 0;
const int tag_levantarse = 1;


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

// ---------------------------------------------------------------------

void funcion_filosofos( int id )
{
  int num_elementos = num_filosofos*2;
  int id_ten_izq = (id+1)              % num_elementos, //id. tenedor izq.
      id_ten_der = (id+num_elementos-1) % num_elementos; //id. tenedor der.
   int pet;
   int * vect = NULL;
   int tam_vect = 0;

  while ( true )
  {
     // Creamos el vector de tamaño entre 1 y 5 y lo mandamos
     tam_vect = aleatorio<1,5>();
     vect = new int [tam_vect];
    cout << "Filosofo " << id/2 << " solicita sentarse" << endl;
    cout << "Filosofo " << id/2 << " manda vector tam " << tam_vect << endl;
    MPI_Ssend(&vect, tam_vect, MPI_INT, id_camarero, tag_sentarse, MPI_COMM_WORLD);

    cout <<"Filósofo " <<id/2 << " solicita ten. izq." <<id_ten_izq <<endl;
    MPI_Ssend(&pet, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);

    cout <<"Filósofo " <<id/2 <<" solicita ten. der." <<id_ten_der <<endl;
    MPI_Ssend(&pet, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);

    cout <<"Filósofo " <<id/2 <<" comienza a comer" <<endl ;
    sleep_for( milliseconds( aleatorio<10,100>() ) );

    cout <<"Filósofo " <<id/2 <<" suelta ten. izq. " <<id_ten_izq <<endl;
    MPI_Ssend(&pet, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);

    cout<< "Filósofo " <<id/2 <<" suelta ten. der. " <<id_ten_der <<endl;
    MPI_Ssend(&pet, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);

    cout << "Filosofo " << id/2 << " solicita levantarse" << endl;
    MPI_Ssend(&pet, 1, MPI_INT, id_camarero, tag_levantarse, MPI_COMM_WORLD);

    cout << "Filosofo " << id/2 << " comienza a pensar" << endl;
    sleep_for( milliseconds( aleatorio<10,100>() ) );
    delete[] vect; // Liberamos la memoria del vector
 }
}
// ---------------------------------------------------------------------

void funcion_tenedores( int id )
{
  int valor, id_filosofo ;  // valor recibido, identificador del filósofo
  MPI_Status estado ;       // metadatos de las dos recepciones

  while ( true )
  {
     MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &estado);
     id_filosofo = estado.MPI_SOURCE;
     cout <<"Ten. " <<id <<" ha sido cogido por filo. " <<id_filosofo/2 <<endl;

     MPI_Recv(&valor, 1, MPI_INT, id_filosofo, 0, MPI_COMM_WORLD, &estado);
     cout <<"Ten. "<< id<< " ha sido liberado por filo. " <<id_filosofo/2 <<endl ;
  }
}
// ---------------------------------------------------------------------

void funcion_camarero( int id )
{
  int valor, id_filosofo ;  // valor recibido, identificador del filósofo
  MPI_Status estado ;       // metadatos de las dos recepciones
  int num_sentados = 0;
  int max_sentados = num_filosofos-1;
  int tag_aceptable = MPI_ANY_TAG;
  int tam_vector = 0;
  int * vec = NULL;
  int contador = 0;
  int decena_sig = 10;

  while ( true )
  {
     if (num_sentados < max_sentados)
         tag_aceptable = MPI_ANY_TAG;
      else
         tag_aceptable = tag_levantarse;
      
      // Esperamos a que haya algún mensaje aceptable, sin recibirlo
      MPI_Probe(MPI_ANY_SOURCE, tag_aceptable, MPI_COMM_WORLD, &estado);
      if (estado.MPI_TAG == tag_sentarse){ // Si el mensaje es para sentarse
         MPI_Get_count(&estado, MPI_INT, &tam_vector); // Leo tamaño del vector sin recibirlo
         vec = new int[tam_vector]; // Preparo vector para recibir el mensaje
         MPI_Recv(vec, tam_vector, MPI_INT, estado.MPI_SOURCE, tag_aceptable, MPI_COMM_WORLD, &estado);
         delete[] vec; // Libero la memoria del vector
      }
      else{ // Si la peticion es para levantarse
         MPI_Recv(&valor, 1, MPI_INT, estado.MPI_SOURCE, tag_aceptable, MPI_COMM_WORLD, &estado);
      }
     
     id_filosofo = estado.MPI_SOURCE;
     
     switch (estado.MPI_TAG){
     case tag_sentarse:
        cout << "Filosofo " << id_filosofo/2 << " se sienta a la mesa" << endl;
        num_sentados++;
        contador += tam_vector;
        cout << "Filosofo " << id_filosofo/2 << " ha mandado vector tam " << tam_vector << endl;
        if (contador >= decena_sig){
           cout << "¡¡¡¡He alcanzado ya " << contador << " unidades en total de propina!!!!" << endl;
           decena_sig += 10;
        }
        break;
     
     case tag_levantarse:
        cout << "Filosofo " << id_filosofo/2 << " se levanta de la mesa" << endl;
        num_sentados--;
        break;
     }
  }
}
// ---------------------------------------------------------------------

int main( int argc, char** argv )
{
   int id_propio, num_procesos_actual ;

   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );


   if ( num_procesos == num_procesos_actual )
   {
      // ejecutar la función correspondiente a 'id_propio'
      if (id_propio == id_camarero)
         funcion_camarero(id_propio);
      else if ( id_propio % 2 == 0 )          // si es par
         funcion_filosofos( id_propio ); //   es un filósofo
      else                               // si es impar
         funcion_tenedores( id_propio ); //   es un tenedor
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   MPI_Finalize( );
   return 0;
}

// ---------------------------------------------------------------------
