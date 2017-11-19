// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// archivo: prodcons_1.cpp
// Ejemplo de un monitor en C++11 con semántica SU, para el problema
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
#include <random>
#include <HoareMonitor.hpp>

using namespace std;
using namespace HM;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

constexpr int  num_items = 40,         // número de ítems a producir/consumir
               num_hebras_prod = 4,    // número de hebras productoras
               num_hebras_cons = 10;   // número de hebras consumidoras
mutex mtx;                    // mutex de escritura en pantalla

unsigned cont_prod[num_items],         // contadores de verificación: producidos
         cont_cons[num_items];         // contadores de verificación: consumidos

unsigned items_producidos[num_hebras_prod];  // contador de ítems producidos por cada hebra productora


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
      cout << " dato == " << dato << ", num_items == " << num_items << endl ;
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
// clase para monitor Productor-Consumidor, version 3(FIFO), semántica SU

class MProdCons3SU : public HoareMonitor
{
 private:
 static const int           // constantes:
   num_celdas_total = 10;   //  núm. de entradas del buffer
   int   buffer[num_celdas_total],//  buffer de tamaño fijo, con los datos
         primera_libre,    //  índice de celda de la próxima inserción
         primera_ocupada,  //  índice de celda de la próxima lectura
         n ;               //  número de celdas ocupadas
   CondVar  ocupadas,         //  cola donde espera el consumidor (n>0)
            libres ;          //  cola donde espera el productor  (n<num_celdas_total)

 public:                    // constructor y métodos públicos
   MProdCons3SU(  );             // constructor
   int  leer();                  // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor );   // insertar un valor (sentencia E) (productor)
} ;
// -----------------------------------------------------------------------------

MProdCons3SU::MProdCons3SU(  )
{
   primera_libre     = 0 ;
   primera_ocupada   = 0;
   n                 = 0;
   ocupadas          = newCondVar();
   libres            = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int MProdCons3SU::leer(  )
{
   // esperar bloqueado hasta que 0 < num_celdas_ocupadas
   while ( n == 0 )
      ocupadas.wait();

   // hacer la operación de lectura, actualizando estado del monitor
   assert( 0 < n );
   const int valor = buffer[primera_ocupada] ;
   primera_ocupada++ ;
   n--;

   if (primera_ocupada >= num_celdas_total)
      primera_ocupada %= num_celdas_total;

   // señalar al productor que hay un hueco libre, por si está esperando
   libres.signal();

   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------

void MProdCons3SU::escribir( int valor )
{
   // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
   while ( n == num_celdas_total )
      libres.wait();

   assert( n < num_celdas_total );

   // hacer la operación de inserción, actualizando estado del monitor
   buffer[primera_libre] = valor ;
   primera_libre++ ;
   n++;

   if ( primera_libre >= num_celdas_total )
      primera_libre %= num_celdas_total;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( MRef<MProdCons3SU> monitor, int num_hebra )
{
   for( unsigned i = 0 ; i < (num_items/num_hebras_prod) ; i++ )
   {
      int valor = producir_dato(num_hebra) ;
      monitor->escribir( valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<MProdCons3SU> monitor )
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
        << "Problema de los productores-consumidores (varios prod/cons, Monitor SU, buffer FIFO). " << endl
        << "-------------------------------------------------------------------------------------" << endl
        << flush ;

   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<MProdCons3SU> monitor = Create<MProdCons3SU>();
   inicializador();

   // crear hebras
   thread hebras_productoras[num_hebras_prod],
          hebras_consumidoras[num_hebras_cons];

   // lanzar hebras
   for (int i=0 ; i<num_hebras_prod ; i++)
      hebras_productoras[i] = thread( funcion_hebra_productora, monitor, i);
   for (int j=0 ; j<num_hebras_cons ; j++)
      hebras_consumidoras[j] = thread( funcion_hebra_consumidora, monitor);

   // esperar a que terminen las hebras (no pasa nunca)
   for (int i=0 ; i<num_hebras_prod ; i++)
      hebras_productoras[i].join() ;
   for (int j=0 ; j<num_hebras_cons ; j++)
      hebras_consumidoras[j].join() ;

   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores() ;
}