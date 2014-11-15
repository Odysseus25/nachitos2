#ifndef TABLAPROCESOS_H
#define TABLAPROCESOS_H

#include "synch.h"
#include "bitmap.h"
#include "thread.h"


class tablaIndicadores
{
public:
    tablaIndicadores();
    ~tablaIndicadores();

    int RegisterProcess( Thread* hilo ); // Register the process ID
    void DeRegisterProcess( int pid );      // Unregister the process ID
    bool isRegistered( int pid ) const;     // Checks if a process ID is already registered
    Thread* getThread( int pid ) const;     // returns the Thread pointer to the given process ID
    bool IsIncluded(Thread* hilo);          // Checks if the given Thread's pointer is included in the table
    
    void Print() const;               // Print contents

  private:
    Thread ** runningProcesses;		// A vector with user running processes
    BitMap * runningProcessesMap;	// A bitmap to control our vector
    Lock * processesMutexLock; // A mutex lock to assure that no other process is trying to modify the table at the same time
    bool * usoDeSemaforos;
    bool * usoDeArchivos;
    int usage;	     	// How many threads are using this table
    void cleanTable(); // Elimina todas las asociaciones entre 

};

#endif // TABLAPROCESOS_H
