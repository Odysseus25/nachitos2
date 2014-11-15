#include "tablaindicadores.h"
#include <iostream>
#include <unistd.h>

#define MAX_PROCESS 120

tablaIndicadores::tablaIndicadores()
{
	runningProcesses = new Thread*[MAX_PROCESS];
	runningProcessesMap = new BitMap(MAX_PROCESS);
	processesMutexLocks = new Lock("PROCESS_MUTEX");
	usoDeSemaforos = new bool[MAX_PROCESS];
	usoDeArchivos = new bool[MAX_PROCESS];
	usage = 0;
}

tablaIndicadores::~tablaIndicadores()
{
	delete[] runningProcesses;
	delete runningProcessesMap;
	delete processesMutexLocks;
	delete usoDeSemaforos;
	delete usoDeArchivos;
}

int tablaIndicadores::RegisterProcess(Thread* hilo)
{
	processesMutexLocks->Acquire();
	int k;

	if (!IsIncluded(hilo))
	{
		int i = runningProcessesMap->Find();
		runningProcesses[i] = hilo;
		++usage;

		processesMutexLocks->Release();

		return i;
	}
	else
	{
		for (int j = 0; i < 120; ++i)
		{
			if (runningProcesses[j] == hilo)
		 	{
		 		j = k;
		 		break;
		 	}
		}

		processesMutexLocks->Release();

		return k;
	}
}

int tablaIndicadores::DelRegisterProcess(int pid)
{
	processesMutexLocks->Acquire();

	runningProcessesMap->Clear(pid);
	--usage;

	processesMutexLocks->Release();
}

bool tablaIndicadores::isRegistered(int pid)
{
	return runningProcessesMap->Check(pid);
}

Thread* tablaIndicadores::getThread(int pid)
{
	return runningProcesses[pid];
}

void tablaIndicadores::cleanTable()
{
	for (int i = 0; i < 120; ++i)
	{
		runningProcesses[i] = NULL;
		runningProcessesMap->Clear(i);	
	}
}

bool tablaIndicadores::IsIncluded(Thread* hilo)
{
	for (int i = 0; i < 120; ++i)
	{
		if (runningProcesses[i] == hilo)
		 {
		 	return true;
		 }
	}
	return false;
}






