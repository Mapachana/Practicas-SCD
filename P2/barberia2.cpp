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

const int numero_clientes = 3;

mutex mtx; // Asegura que no se solapen los cout

// Ana Buendía Ruiz-Azuaga

// Función para generar números aleatorios
template< int min, int max > int aleatorio(){
    static default_random_engine generador( (random_device())() );
    static uniform_int_distribution<int> distribucion_uniforme(min, max) ;
    return distribucion_uniforme(generador);
}

class MonitorBarberia : public HoareMonitor{
    private:
        CondVar barbero_duerme; // Cola para que duerma el barbero
        CondVar cliente_espera_en_sala; // Cola para que un cliente espere en sala de espera
        CondVar cliente_cortandose; // Cola para que un cliente espere mientras le cortan el pelo

    public:
        MonitorBarberia(); // Constructor
        void cortarPelo(int num_cliente); // Cliente entra a la barberia a cortarse el pelo
        void siguienteCliente(); // Barbero llama a siguiente cliente a cortarse el pelo
        void finCliente(); // Barbero termina de cortar el pelo a cliente
};

// Constructor del MonitorBarberia
MonitorBarberia::MonitorBarberia(){
    barbero_duerme = newCondVar();
    cliente_espera_en_sala = newCondVar();
    cliente_cortandose = newCondVar();
}

// Cliente entra a la barberia a cortarse el pelo
void MonitorBarberia::cortarPelo(int num_cliente){
    mtx.lock();
    cout << "Cliente " << num_cliente << ": entra en la barberia" << endl << flush;
    mtx.unlock();

    // Cliente comprueba si hay alguien más en la sala de espera, y si lo hay despierta al barbero
    if (!barbero_duerme.empty() && !cliente_espera_en_sala.empty()){
        barbero_duerme.signal();
        cliente_espera_en_sala.wait();

        mtx.lock();
        cout << "Cliente " << num_cliente << ": Ya es mi turno" << endl << flush;
        mtx.unlock();
    }
    else if (!cliente_cortandose.empty() || cliente_espera_en_sala.empty()){ // Si está ocupado o no hay nadie esperando espera
        cliente_espera_en_sala.wait();

        mtx.lock();
        cout << "Cliente " << num_cliente << ": Ya es mi turno" << endl << flush;
        mtx.unlock();
    }
    else{
        cliente_espera_en_sala.wait();
        mtx.lock();
        cout << "Cliente " << num_cliente << ": Ya es mi turno" << endl << flush;
        mtx.unlock();
    }

    cliente_cortandose.wait();

    mtx.lock();
    cout << "Cliente " << num_cliente << ": Ya me he cortado el pelo" << endl << flush;
    mtx.unlock();
}

// Barbero llama a siguiente cliente a cortarse el pelo
void MonitorBarberia::siguienteCliente(){
    if (cliente_espera_en_sala.get_nwt() < 2 && cliente_cortandose.empty()){ // Si no hay al menos dos clientes y nadie cortandose duerme 
        mtx.lock();
        cout << "El barbero va a echarse a dormir" << endl << flush;
        mtx.unlock();

        barbero_duerme.wait();

        cliente_espera_en_sala.signal();
        cliente_espera_en_sala.signal();
    }
    else if (cliente_cortandose.empty() && cliente_espera_en_sala.get_nwt() >= 2){ // Si hay al menos dos y nadie cortandose llama a otros dos
        cliente_espera_en_sala.signal();
        cliente_espera_en_sala.signal();
        
        mtx.lock();
        cout << "Siguiente par de clientes" << endl << flush;
        mtx.unlock();
    }
}

// Barbero termina de cortar el pelo a cliente
void MonitorBarberia::finCliente(){
    cliente_cortandose.signal(); // libera a los dos clientes
    cliente_cortandose.signal();
    
    mtx.lock();
    cout << "Gracias por su visita" << endl << flush;
    mtx.unlock();
}

// Cliente espera fuera de la barberia
void esperarFuerabarberia(int num_cliente){
    mtx.lock();
    cout << "Cliente " << num_cliente << ": espera fuera de la barbería" << endl << flush;
    mtx.unlock();

    chrono::milliseconds duracion_fuera(aleatorio<20,200>());
    this_thread::sleep_for(duracion_fuera);
}

// Barbero corta el pelo a un cliente
void cortarPeloACliente(){
    chrono::milliseconds duracion_cortar(aleatorio<20,200>());

    mtx.lock();
    cout << "El barbero se dispone a cortar el pelo" << endl << flush;
    mtx.unlock();

    this_thread::sleep_for(duracion_cortar);

    mtx.lock();
    cout << "El barbero ha terminado de cortar el pelo" << endl << flush;
    mtx.unlock();


}

// Función a ejecutar por la hebra del barbero
void funcionHebraBarbero(MRef<MonitorBarberia> monitor){
    while(true){
        monitor->siguienteCliente();
        cortarPeloACliente();
        monitor->finCliente();
    }
}

// Función a ejecutar por las hebras de los clientes
void funcionHebraCliente(MRef<MonitorBarberia> monitor, int num){
    while(true){
        monitor->cortarPelo(num);
        esperarFuerabarberia(num);
    }
}

int main(){
    MRef<MonitorBarberia> barberia = Create<MonitorBarberia>();

    thread hebra_barbero(funcionHebraBarbero, barberia);
    thread hebras_clientes[numero_clientes];

    for (int i = 0; i < numero_clientes; ++i)
        hebras_clientes[i] = thread(funcionHebraCliente, barberia, i);

    hebra_barbero.join();

    for (int i = 0; i < numero_clientes; ++i)
        hebras_clientes[i].join();
}