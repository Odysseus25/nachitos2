// addrspace.cc
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    #ifdef VM
    	valid = false;
    #else
    	valid = true;
    #endif

    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) &&
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n",
					numPages, size);
// first, set up the translation
    coreMap = new TranslationEntry[numPages];		// Contiene las paginas invertidas
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
	pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
	pageTable[i].physicalPage = i;
	pageTable[i].valid = false;	// Se ponen todas las paginas invalidas en pageTable
	pageTable[i].use = false;
	pageTable[i].dirty = false;
	pageTable[i].readOnly = false;  // if the code segment was entirely on
					// a separate page, we could set its
					// pages to be read-only
    }

// zero out the entire address space, to zero the unitialized data segment
// and the stack segment
    bzero(machine->mainMemory, size);

#ifndef VM
// then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",
			noffH.code.virtualAddr, noffH.code.size);
        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n",
			noffH.initData.virtualAddr, noffH.initData.size);
        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }

 #endif

}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState()
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState()
{
    #ifndef VM
             machine->pageTable = pageTable;
             machine->pageTableSize = numPages;
    #endif
}


/**
 * 	Algoritmo encargado de indicar cual es la pagina que puede utilizar
 * 	y libera los demas espacios en el pagetable para otro proceso que recurra
 * 	a la tabla. El metodo indica la pagina que debe ser movida de memoria a disco,
 * 	retornando el indice de la siguiente pagina utilizable en pagetable.
 *
**/

int AddrSpace::indicadorPaginaPageTable(int indicador, int dimension){ //algoritmo de second chance
	int temporal = indicador + 1;
	bool detener = false;
	do{
		while(temporal < dimension && detener == false){
			if(pageTable[temporal].use == false){
				pageTable[temporal].use == true;	// Si el bit esta apagado se enciende y
				detener = true;			       // detiene el ciclo, obteniendo la posicion
			}else{
				pageTable[temporal].use == false;	// Si el bit esta encendido, lo apaga para
				++temporal;				// el proceso que llegue despues y continua
			}						// buscando un bit apagado hasta el final
		}							// de la memoria
		if(detener == false){
			temporal = 0;
		}

	}while(detener == false);					// Ciclo hasta hallar un bit apagado
	return temporal;
}

<<<<<<< HEAD

int indiceTLB = 0;

void AddreSpace::Load(int dirPF){
	NoffHeader noffH;
	unsigned int pagFaltante = (unsigned) dirPF/PageSize; // se obtiene la direccion de la pag que falta
	if(pageTable[pagFaltante].dirty == true && pageTable[pagFaltante].valid == false){ //busca la pag que falta con la direccion que se sutrajo, y verifica si esta sucia. Si lo esta, se debe de traer la pag de nuevo desde el disco
		DEBUG('a', "La pagina esta sucia, se trae desde el disco \n");
		OpenFile * swap = fileSystem->Open("SWAP");
		int encontrar = mybit.->Find();
		swap->ReadAt(&(machine->mainMemory[pagFaltante*PageSize]), PageSize, encontrar*PageSize); //lee la pag que falta del swap y la guarda

	pageTable[pagFaltante].physicalPage = encontrar;	//actualiza el pageTable
	pageTable[pagFaltante].valid = true;
	coreMap[encontar].virtualPage = pagFaltante;
	}
	else{	//la pag esta limpia
		//Aqui se debe de determinar si la pag contiene codigo o datos inicializados
		//Se lee el archivo ejecutable por su nombre y se verifica su encabezado.
		//Se recupera el tamaño de los semegentos de codigo y datos para calcular las
		//paginas que ocupan

		unsigned int pagSegmento;
		OpenFile *ejecutable = fileSystem->Open(currentThread->filename); //se abre el archivo
		ejecutable = ReadAt((char *)&noffH, sizeof(noffH), 0);
		pagSegmento = divRound((noffH.code.size + noffH.iniData.size), PageSize); //calcula el numero de pags que ocupa el segmento
		DEBUG('a', "Num de pags del segmneto: %i pagFaltante: %i \n", pagSemento, pagFaltante);
		//Ahora se debe insertar esta pag en el frame. Se busca si existe uno libre
		int encontrar = mybit.->Find();
		encontrar = analisisDePagina(encontrar); //revisa si la pag q trajo esta sucia o no

		if(pagFaltante <= pagSegmento){ //si la pag faltante es menor a las pags necesarias, se extraen del ejecutable.
			ejecutable->ReadAt(&(machine->mainMemory[find*PageSize]), PageSize, noffH.code.inFileAddr+PageSize*pagFaltante); //se lee del ejecutable
		}
		//se debe crear una pag nueva, en cualquier caso
		pageTable[pagFaltante].physicalPage = encontrar;
		pageTable[pagFaltante].valid = true;
		coreMap[encontrar].virtualPage = pagFaltante;
	}
	//Finalmente se actualiza el TLB
		machine->tlb[indicadorPaginaPageTable(encontrar, PageSize)].virtualPage = pageTable[pagFaltante].virtualPage;
		machine->tlb[indicadorPaginaPageTable(encontrar, PageSize)].physicalPage = pageTable[pagFaltante].physicalPage;
		machine->tlb[indicadorPaginaPageTable(encontrar, PageSize)].valid = true;
		machine->tlb[indicadorPaginaPageTable(encontrar, PageSize)].use = pageTable[pagFaltante].use;
		machine->tlb[indicadorPaginaPageTable(encontrar, PageSize)].dirty = pageTable[pagFaltante].dirty;
		indiceTLB = indicadorPaginaPageTable(indiceTLB, TLBSize);
}
=======
/**
 * 	Metodo encargado de analizar si la pagina obtenida por el indice retornado en
 * 	indicadorPaginaPageTable se encuentra en estado dirty (se realiza un swap) o no.
**/

int AddrSpace::analisisDePagina(int indice){
	int enMemoriaSwap;
	if(pageTable[coreMap[indice].virtualPage].dirty != true){		// Condicional que verifica si la pagina esta sucia o no
		pageTable[coreMap[indice].virtualPage].physicalPage = -1;
		pageTable[coreMap[indice].virtualPage].valid = false;		// Se invalida la pagina
		coreMap[find].virtualPage = -1;

	}else{
		OpenFile *manejaSwap = fileSystem->Open("Realiza Swap");
		enMemoriaSwap = mybitSwap->Find();
		manejaSwap->WriteAt(&(machine->mainMemory[find*PageSize]), PageSize, enMemoriaSwap*PageSize);	// Se escribe en memoria
		pageTable[coreMap[indice].virtualPage].physicalPage = enMemoriaSwap;		// Los valores son actualizados
		pageTable[coreMap[indice].virtualPage].valid = false;				// Se invalida la pagina
		coreMap[indice].virtualPage = -1;
	}
	return indice;
}


>>>>>>> origin/master
