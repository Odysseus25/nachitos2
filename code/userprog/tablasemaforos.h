#ifndef TABLASEMAFOROS_H
#define TABLASEMAFOROS_H

#include "synch.h"
#include "bitmap.h"

extern Thread *currentThread;

class tablaControladoraSemaforos
{
public:
    tablaControladoraSemaforos();
    ~tablaControladoraSemaforos();

    int nuevoSemaforo( int valorInicial ); // Register the file handle
    int destruirSemaforo( int semaforoID );      // Unregister the file handle
    int semaforoSignal( int semaforoID ) ;
    int semaforoWait( int semaforoID ) ;
    void addThread();		// If a user thread is using this table, add it
    void delThread();		// If a user thread is using this table, delete it

    void Print() const;               // Print contents

  private:
    Semaphore **semaforos;		// A vector with user opened files
    BitMap * semaforosMap;	// A bitmap to control our vector
    Lock * semMutexLocks;  // A mutex lock to assure that no other process is trying to modify the same semaphore at the same time
    int usage;			// How many threads are using this table
    void cleanTable(); // Cierra todos los archivos de la tabla

};

#endif // TABLASEMAFOROS_H
