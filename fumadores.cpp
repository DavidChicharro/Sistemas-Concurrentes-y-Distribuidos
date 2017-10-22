// Compilación: g++ -std=c++11 -pthread -I. -o fumadores fumadores.cpp Semaphore.cpp 
// El programa ejecuta un bucle infinito. Pararlo con Ctrl+C o Ctrl+Z

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

/*  El estanquero produce el producto 0 para el fumador 0, el producto 1 para el fumador 1,...
    el producto k para el fumador k
*/

const unsigned num_fumadores = 3;

Semaphore fumador[num_fumadores] = {0,0,0},
			 estanquero = 1;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------
template< int min, int max > int aleatorio() {
	static default_random_engine generador( (random_device())() );
	static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
	return distribucion_uniforme( generador );
}

//----------------------------------------------------------------------
// función auxiliar
void producir( ) {
	// calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_producir( aleatorio<50,200>() );

	int ingrediente = aleatorio<0,2>();
	
   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_producir );

   cout << "Estanquero produce ingrediente " << ingrediente 
		  << " ("<< duracion_producir.count() << " milisegundos)" << endl;

   sem_signal (fumador[ingrediente]);
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero
void funcion_hebra_estanquero( ) {
	while( true ) {
		sem_wait ( estanquero );		
		producir ( );
	}
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra
void fumar( int num_fumador ) {
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<50,200>() );

   // informa de que comienza a fumar
   cout << "Fumador " << num_fumador << " :"
        << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar
    cout << "Fumador " << num_fumador << " : termina de fumar, comienza espera de ingrediente." << endl;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador ) {
   while( true ) {
   	sem_wait(fumador[num_fumador]);
   	sem_signal(estanquero);
   	fumar( num_fumador );
   }
}

//----------------------------------------------------------------------

int main() {
	cout << "------------------------------------" << endl
        << "Problema de los fumadores." << endl
        << "------------------------------------" << endl
        << flush ;
   cout << "El fumador 1 tiene PAPEL y TABACO por lo que necesita CERILLAS" << endl
   	  << "El fumador 2 tiene PAPEL y CERILLAS por lo que necesita TABACO" << endl
  		  << "El fumador 3 tiene TABACO y CERILLAS por lo que neceista PAPEL" << endl << flush;

   // declarar hebras y ponerlas en marcha
   // ......
   thread hebra_estanquero( funcion_hebra_estanquero ),
   		 hebras_fumadores[num_fumadores];

   //hebra_estanquero ( funcion_hebra_estanquero );
  	for( int i = 0 ; i < num_fumadores ; i++ )
   	hebras_fumadores[i] = thread( funcion_hebra_fumador, i );

   for( int i = 0 ; i < num_fumadores ; i++ )
   	hebras_fumadores[i].join();
   hebra_estanquero.join();
}
