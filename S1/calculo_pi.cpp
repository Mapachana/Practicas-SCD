// -----------------------------------------------------------------------------
// Sistemas concurrentes y Distribuidos.
// Seminario 1. Programación Multihebra y Semáforos.
//
// Ejemplo 9 (ejemplo9.cpp)
// Calculo concurrente de una integral. Plantilla para completar.
//
// Historial:
// Creado en Abril de 2017
// -----------------------------------------------------------------------------

// Ana Buendía Ruiz-Azuaga

#include <iostream>
#include <iomanip>
#include <chrono>  // incluye now, time\_point, duration
#include <future>
#include <vector>
#include <cmath>

using namespace std ;
using namespace std::chrono;

const long m  = 1024l*1024l*1024l, // Intervalos
           n  = 4  ; // Número de hebras


// -----------------------------------------------------------------------------
// evalua la función $f$ a integrar ($f(x)=4/(1+x^2)$)
double f( double x )
{
  return 4.0/(1.0+x*x) ;
}
// -----------------------------------------------------------------------------
// calcula la integral de forma secuencial, devuelve resultado:
double calcular_integral_secuencial(  )
{
   double suma = 0.0 ;                        // inicializar suma
   for( long i = 0 ; i < m ; i++ )            // para cada $i$ entre $0$ y $m-1$:
      suma += f( (i+double(0.5)) / m );         //   $~$ añadir $f(x_i)$ a la suma actual
   return suma/m ;                            // devolver valor promedio de $f$
}

// ------------------------------------------------------------------------------
// Funciones para asignar las hebras

// Funcion que ejecuta cada hebra de forma contigua
double funcion_hebra_contigua( long i ){
   long chunk = m/n;
   long lim_inf = i*chunk;
   long lim_sup = (i+1)*chunk;

   if (i == (n-1))
      lim_sup = m;

   double suma = 0.0;
   for (long j = lim_inf; j < lim_sup; ++j)
      suma += f( (j+double(0.5)) / m );
   
   return suma;
}

// Funcion  que ejecuta cada hebra de forma ciclica
double funcion_hebra_ciclica( long i ){
   double suma = 0.0;
   for (long j = i; j < m; j+=n)
      suma += f( (j+double(0.5)) / m );

   return suma;
}

// -----------------------------------------------------------------------------
// calculo de la integral de forma concurrente
double calcular_integral_concurrente_contigua();
double calcular_integral_concurrente_ciclica();

// Función para calcular la intergal concurrentemente de forma: Contigua, si el argumento es 0, o Ciclica, si el argumento no es 0
double calcular_integral_concurrente(int num )
{
   double resultado= 0.0;
   if (num == 0)
      resultado = calcular_integral_concurrente_contigua();
   else
      resultado = calcular_integral_concurrente_ciclica();
   
   return resultado;
}

// -----------------------------------------------------------------------------
// Calculo de la integral de forma concurrente segun la asignacion de hebras

// Calculo de la integral de forma concurrente segun hebras contiguas
double calcular_integral_concurrente_contigua(){
   future<double> futuros[n];

   for (long i = 0; i < n; ++i)
      futuros[i] = async(launch::async, funcion_hebra_contigua, i);

   double suma = 0.0;
   for (long i = 0; i < n; ++i)
      suma += futuros[i].get();
   
   return suma/m;
}

// Calculo de la integral de forma concurrente segun hebras ciclicas
double calcular_integral_concurrente_ciclica(){
   future<double> futuros[n];

   for (long i = 0; i < n; ++i)
      futuros[i] = async(launch::async, funcion_hebra_ciclica, i);

   double suma = 0.0;
   for (long i = 0; i < n; ++i)
      suma += futuros[i].get();

   return suma/m;
}


// -----------------------------------------------------------------------------

int main()
{
   time_point<steady_clock> inicio_sec  = steady_clock::now() ;
   const double             result_sec  = calcular_integral_secuencial(  );
   time_point<steady_clock> fin_sec     = steady_clock::now() ;
   double x = sin(0.4567);
   time_point<steady_clock> inicio_conc = steady_clock::now() ;
   const double             result_conc = calcular_integral_concurrente( 0 );
   time_point<steady_clock> fin_conc    = steady_clock::now() ;
   duration<float,milli>    tiempo_sec  = fin_sec  - inicio_sec ,
                              tiempo_conc = fin_conc - inicio_conc ;                           
   const float              porc        = 100.0*tiempo_conc.count()/tiempo_sec.count() ;


   constexpr double pi = 3.14159265358979323846l ;

   cout << "Número de muestras (m)   : " << m << endl
         << "Número de hebras (n)     : " << n << endl
         << setprecision(18)
         << "Valor de PI              : " << pi << endl
         << "Resultado secuencial     : " << result_sec  << endl
         << "Resultado concurrente    : " << result_conc << endl
         << setprecision(5)
         << "Tiempo secuencial        : " << tiempo_sec.count()  << " milisegundos. " << endl
         << "Tiempo concurrente       : " << tiempo_conc.count() << " milisegundos. " << endl
         << setprecision(4)
         << "Porcentaje t.conc/t.sec. : " << porc << "%" << endl;

}
