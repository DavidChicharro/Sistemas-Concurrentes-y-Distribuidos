// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// archivo: prodcons_1.cpp
// Ejemplo de un monitor en C++11 con semántica SC, para el problema
// del productor/consumidor, con múltiples productores y consumidores.
// Opcion FIFO (queue)
//
// Historial:
// Creado en Noviembre de 2017
//
// Autor: David Carrasco Chicharro
// -----------------------------------------------------------------------------


#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>

using namespace std ;

constexpr int num_items  = 40 ,     // número de items a producir/consumir
              num_hebras_prod = 4,
              num_hebras_cons = 4;
mutex mtx ;                 // mutex de escritura en pantalla

unsigned cont_prod[num_items], // contadores de verificación: producidos
         cont_cons[num_items]; // contadores de verificación: consumidos

unsigned items_producidos[num_hebras_prod];
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

int producir_dato(int hebra_prod)
{
   int item = hebra_prod*(num_items/num_hebras_prod) + items_producidos[hebra_prod];

   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "La hebra " << hebra_prod << " ha producido: " << item << endl << flush ;
   mtx.unlock();
   cont_prod[item] ++ ;
   items_producidos[hebra_prod]++;

   return item++ ;
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
   cout << "                                    consumido: " << dato << endl ;
   mtx.unlock();
}
//----------------------------------------------------------------------

void inicializador()
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  cont_prod[i] = 0 ;
      cont_cons[i] = 0 ;
   }

   for (unsigned i = 0 ; i < num_hebras_prod ; i++)
   {
      items_producidos[i] = 0;
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

class ProdCons1SC
{
 private:
 static const int           // constantes:
   num_celdas_total = 10;   //  núm. de entradas del buffer
   int   buffer[num_celdas_total],//  buffer de tamaño fijo, con los datos
         primera_libre,    //  índice de celda de la próxima inserción
         primera_ocupada,  //  índice de celda de la próxima lectura
         n ;               //  número de celdas ocupadas
   mutex cerrojo_monitor ;    // cerrojo del monitor
   condition_variable   ocupadas,                //  cola donde espera el consumidor (n>0)
                        libres ;                 //  cola donde espera el productor  (n<num_celdas_total)

 public:                    // constructor y métodos públicos
   ProdCons1SC(  ) ;           // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor ); // insertar un valor (sentencia E) (productor)
} ;
// -----------------------------------------------------------------------------

ProdCons1SC::ProdCons1SC(  )
{
   primera_libre = 0 ;
   primera_ocupada = 0;
   n = 0;
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdCons1SC::leer(  )
{
   // ganar la exclusión mutua del monitor con una guarda
   unique_lock<mutex> guarda( cerrojo_monitor );

   // esperar bloqueado hasta que 0 < num_celdas_ocupadas
   while ( n == 0 )
      ocupadas.wait( guarda );

   // hacer la operación de lectura, actualizando estado del monitor
   assert( 0 < n );
   const int valor = buffer[primera_ocupada] ;
   primera_ocupada++ ;
   n--;

   if (primera_ocupada >= num_celdas_total)
      primera_ocupada %= num_celdas_total;

   // señalar al productor que hay un hueco libre, por si está esperando
   libres.notify_one();

   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------

void ProdCons1SC::escribir( int valor )
{
   // ganar la exclusión mutua del monitor con una guarda
   unique_lock<mutex> guarda( cerrojo_monitor );

   // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
   while ( n == num_celdas_total )
      libres.wait( guarda );

   assert( n < num_celdas_total );

   // hacer la operación de inserción, actualizando estado del monitor
   buffer[primera_libre] = valor ;
   primera_libre++ ;
   n++;

   if ( primera_libre >= num_celdas_total )
      primera_libre %= num_celdas_total;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.notify_one();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( ProdCons1SC * monitor, int np )
{
   for( unsigned i = 0 ; i < (num_items/num_hebras_prod) ; i++ )
   {
      int valor = producir_dato(np) ;
      monitor->escribir( valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( ProdCons1SC * monitor)
{
   for( unsigned i = 0 ; i < (num_items/num_hebras_cons) ; i++ )
   {
      int valor = monitor->leer();
      consumir_dato( valor ) ;
   }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "-------------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (varios prod/cons, Monitor SC, buffer FIFO). " << endl
        << "-------------------------------------------------------------------------------------" << endl
        << flush ;

   ProdCons1SC monitor ;
   inicializador();

   thread hebras_productoras[num_hebras_prod],
          hebras_consumidoras[num_hebras_cons];

   for (int i=0 ; i<num_hebras_prod ; i++)
      hebras_productoras[i] = thread( funcion_hebra_productora, &monitor, i);

   for (int j=0 ; j<num_hebras_cons ; j++)
      hebras_consumidoras[j] = thread( funcion_hebra_consumidora, &monitor);


   for (int i=0 ; i<num_hebras_prod ; i++)
      hebras_productoras[i].join() ;
   for (int j=0 ; j<num_hebras_cons ; j++)
      hebras_consumidoras[j].join() ;

   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores() ;
}
