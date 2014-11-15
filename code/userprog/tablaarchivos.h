#ifndef TABLAARCHIVOS_H
#define TABLAARCHIVOS_H

#include "synch.h"
#include "bitmap.h"

class tablaControladoraArchivos
{
public:
    tablaControladoraArchivos();
    ~tablaControladoraArchivos();

    int Open( int UnixHandle ); // Register the file handle
    int Close( int NachosHandle );      // Unregister the file handle
    bool isOpened( int NachosHandle ) const;
    int getUnixHandle( int NachosHandle ) const;
    void addThread();		// If a user thread is using this table, add it
    void delThread();		// If a user thread is using this table, delete it

    void Print() const;               // Print contents

  private:
    int * openFiles;		// A vector with user opened files
    BitMap * openFilesMap;	// A bitmap to control our vector
    Lock * filesMutexLocks; // A mutex lock to assure that no other process is trying to modify the same file at the same time
    int usage;	     	// How many threads are using this table
    void cleanTable(); // Cierra todos los archivos de la tabla

};

#endif // TABLAARCHIVOS_H
