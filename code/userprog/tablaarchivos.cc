#include "tablaarchivos.h"
#include <iostream>

#define MAX_FILES 120
#include <unistd.h>

#define ConsoleInput 0
#define ConsoleOutput 1
#define ConsoleError 2

tablaControladoraArchivos::tablaControladoraArchivos():
usage(0)
{
    openFiles = new int[MAX_FILES];
    openFilesMap = new BitMap(MAX_FILES);
    filesMutexLocks = new Lock("FILE_MUTEX");
    openFiles[ConsoleOutput] = ConsoleOutput;
    openFiles[ConsoleInput] = ConsoleInput;
    openFiles[ConsoleError] = ConsoleError;
    openFilesMap->Mark(ConsoleError);
    openFilesMap->Mark(ConsoleOutput);
    openFilesMap->Mark(ConsoleInput);
}

tablaControladoraArchivos::~tablaControladoraArchivos()
{
    for(int i = 3; i < MAX_FILES; ++i){
        if(openFilesMap->Test(i)){
            close(openFiles[i]);
        }
    }
    delete openFilesMap;
    delete [] openFiles;
}

int tablaControladoraArchivos::Open(int UnixHandle)
{
    filesMutexLocks->Acquire();

    bool encontrado = false;
    int indiceVacio = -1;
    int i = 0;
    for(i = 3; i < MAX_FILES && !encontrado; ++i){
        if(openFilesMap->Test(i) && openFiles[i] == UnixHandle)
        {
            return i;
        }
        if( !openFilesMap->Test(i) ){
            indiceVacio = i;
        }
    }
    if(indiceVacio == -1 )
        return indiceVacio;

    openFiles[i] = UnixHandle;
    openFilesMap->Mark(i);
    return i;

    filesMutexLocks->Release();
}

int tablaControladoraArchivos::Close(int NachosHandle)
{
    filesMutexLocks->Acquire();

    if(NachosHandle >= MAX_FILES)
        return -1;

    if( openFilesMap->Test(NachosHandle) ){
        close(NachosHandle);
        openFilesMap->Clear(NachosHandle);
    }

    filesMutexLocks->Release();
    return 0;
}

bool tablaControladoraArchivos::isOpened(int NachosHandle) const
{
    filesMutexLocks->Acquire();

    if(NachosHandle >= MAX_FILES)
        return false;

    filesMutexLocks->Release();

    return openFilesMap->Test(NachosHandle);
}

int tablaControladoraArchivos::getUnixHandle(int NachosHandle) const
{
    filesMutexLocks->Acquire();

    if(NachosHandle >= MAX_FILES || !openFilesMap->Test(NachosHandle) )
        return -1;

    filesMutexLocks->Release();

    return openFiles[NachosHandle];
}

void tablaControladoraArchivos::addThread()
{
    filesMutexLocks->Acquire();

    ++usage;

    filesMutexLocks->Release();
}

void tablaControladoraArchivos::delThread()
{
    filesMutexLocks->Acquire();

    --usage;
    if(!usage)
        for(int i = 3; i < MAX_FILES; ++i){
            if(openFilesMap->Test(i)){
                close(openFiles[i]);
            }
        }

    filesMutexLocks->Release();
}

void tablaControladoraArchivos::Print() const
{
    for(int i = 0; i < MAX_FILES; ++i){
        if(openFilesMap->Test(i))
            std::cout << i << " : " << openFiles[i] << std::endl;
        else
            std::cout << i << " : " << "no está en uso" << std::endl;
    }
}
