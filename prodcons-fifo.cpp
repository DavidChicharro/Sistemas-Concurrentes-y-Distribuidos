// Compilación: g++ -std=c++11 -pthread -I. -o prod-cons-fifo prodcons-fifo.cpp Semaphore.cpp

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

const int num_items = 80 ,   // número de items
	       tam_vec   = 15 ;   // tamaño del buffer
unsigned  cont_prod[num_items] = {0}, // contadores de verificación: producidos
          cont_cons[num_items] = {0}; // contadores de verificación: consumidos
unsigned	 vec[tam_vec],
			 pos_escritura = 0,
			 pos_lectura 	= 0;
Semaphore libres 		= tam_vec,
          ocupadas   = 0;
Semaphore acceder_buffer = 1;          


//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( ( random_device() )() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato() {
   static int contador = 0 ;
   // Realizar esperas aleatoria entre 20 y 100 milisegundos
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "producido: " << contador << endl << flush ;

   cont_prod[contador] ++ ;
   return contador++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato ) {
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
   for( unsigned i = 0 ; i < num_items ; i++ ) {  
   	if ( cont_prod[i] != 1 ) {  
	      cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
	      ok = false ;
    	}
    	if ( cont_cons[i] != 1 ) {
	      cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
	      ok = false ;
    	}
   }
	if (ok)
   	cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------

void  funcion_hebra_productora(  ) {
   for( unsigned i = 0 ; i < num_items ; i++ ) {
      sem_wait( libres );
      sem_wait( acceder_buffer );
      int dato = producir_dato() ;
      vec[pos_escritura] = dato;
      pos_escritura++;      
      cout << "escrito: " << dato << endl;
      if(pos_escritura >= tam_vec)
      	pos_escritura %= tam_vec;
      sem_signal( acceder_buffer );
      sem_signal( ocupadas );
   }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora(  ) {
   for( unsigned i = 0 ; i < num_items ; i++ ) {
   	sem_wait ( ocupadas );
   	sem_wait( acceder_buffer );
      int dato = vec[pos_lectura];
      pos_lectura++;
      cout << "                  leido: " << dato << endl;
      if(pos_lectura >= tam_vec)
      	pos_lectura %= tam_vec;
      sem_signal( acceder_buffer );
      sem_signal ( libres );
      consumir_dato( dato ) ;
    }
}
//----------------------------------------------------------------------

int main()
{
   cout << "--------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (solución FIFO)." << endl
        << "--------------------------------------------------------" << endl
        << flush ;

   thread hebra_productora ( funcion_hebra_productora ),
          hebra_consumidora( funcion_hebra_consumidora );

   hebra_productora.join() ;
   hebra_consumidora.join() ;

   test_contadores();
}
