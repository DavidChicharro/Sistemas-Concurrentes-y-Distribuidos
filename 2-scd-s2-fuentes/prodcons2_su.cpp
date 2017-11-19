#include <iostream>
#include <iomanip>
#include <cassert>
//#include <thread>
//#include <mutex>
#include <random>
#include <HoareMonitor.hpp>

using namespace std;
using namespace HM;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

constexpr int 	num_items = 40,
					num_hebras_prod = 5,
					num_hebras_cons = 10;
mutex mtx;
unsigned cont_prod[num_items],
 			cont_cons[num_items];

unsigned items_producidos[num_hebras_prod];


template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

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
// clase para monitor Productor-Consumidor, version 3(LIFO),  semántica SU

class MProdCons3SU : public HoareMonitor
{
private:
	static const int 	num_celdas_total = 10;
	int 	buffer[num_celdas_total],
			primera_libre;
	CondVar 	ocupadas,
				libres;

public:
	MProdCons3SU( );
	int leer();
	void escribir (int valor );
};

MProdCons3SU::MProdCons3SU( )
{
	primera_libre 	= 0;
	ocupadas 		= newCondVar();
	libres 			= newCondVar();
}

int MProdCons3SU::leer( )
{
	while ( primera_libre == 0 )
		ocupadas.wait();

	assert( 0 < primera_libre );
	primera_libre--;
	const int valor = buffer[primera_libre];

	libres.signal();

	return valor;
}

void MProdCons3SU::escribir( int valor )
{
	while ( primera_libre == num_celdas_total )
		libres.wait();

	assert( primera_libre < num_celdas_total );

	buffer[primera_libre] = valor;
	primera_libre++;

	ocupadas.signal();
}


void funcion_hebra_productora( MRef<MProdCons3SU> monitor, int num_hebra)
{
	for( unsigned i = 0 ; i< (num_items/num_hebras_prod) ; i++)
	{
		int valor = producir_dato(num_hebra);
		monitor->escribir( valor );
	}
}

void funcion_hebra_consumidora( MRef<MProdCons3SU> monitor )
{
	for( unsigned i = 0 ; i < (num_items/num_hebras_cons) ; i++)
	{
		int valor = monitor->leer();
		consumir_dato( valor );
	}
}

int main()
{
	cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (varios prod/cons, Monitor SU, buffer LIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<MProdCons3SU> monitor = Create<MProdCons3SU>();

   thread hebras_productoras[num_hebras_prod],
          hebras_consumidoras[num_hebras_cons];

   for (int i=0 ; i<num_hebras_prod ; i++)
      hebras_productoras[i] = thread( funcion_hebra_productora, monitor, i);

   for (int j=0 ; j<num_hebras_cons ; j++)
      hebras_consumidoras[j] = thread( funcion_hebra_consumidora, monitor);


   for (int i=0 ; i<num_hebras_prod ; i++)
      hebras_productoras[i].join() ;
   for (int j=0 ; j<num_hebras_cons ; j++)
      hebras_consumidoras[j].join() ;

   test_contadores() ;
}