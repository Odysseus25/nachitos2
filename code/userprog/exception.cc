// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#define FALSE  0
#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "tablaarchivos.h"
#include "../threads/synch.h"
#include "../machine/machine.h"
#include "../threads/utility.h"
//#include "nachostabla.h"

/**
 * 	Universidad de Costa Rica
 * 	Sistemas Operativos
 * 	Profesor: Francisco Arroyo
 * 	Estudiantes: José Pablo Ureña Gutierrez B16692, Ulises Gonzáles Zúñiga  B12989

**/

    char bufferString[50];						// Buffer que almacena mensajes leidos de registro
    Semaphore * consola = new Semaphore("Sem consola", 1);
    char mensajeLeido[300];
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions
//	are in machine.h.
//----------------------------------------------------------------------

void returnFromSystemCall() {

        int pc, npc;

        pc = machine->ReadRegister( PCReg );
        npc = machine->ReadRegister( NextPCReg );
        machine->WriteRegister( PrevPCReg, pc );        // PrevPC <- PC
        machine->WriteRegister( PCReg, npc );           // PC <- NextPC
        machine->WriteRegister( NextPCReg, npc + 4 );   // NextPC <- NextPC + 4

}       // returnFromSystemCall

/**
 * 	System call 0 encargado de detener el sistema NachOS, finalizar los
 * 	hilos actuales y terminar los procesos.
**/
void Nachos_Halt() {                    // System call 0

    DEBUG('a', "Shutdown, initiated by user program.\n");
    delete tablaArchivos;
    delete consola;
    interrupt->Halt();

}       // Nachos_Halt


/**
 * 	System call 1 encargado de finalizar el hilo actual sin terminar
 * 	todos los procesos del sistema NachOS. Sin embargo, si existe un
 * 	unico proceso en ejecucion finaliza el sistema NachOS.
**/
void Nachos_Exit(int status){

	   currentThread->Finish();
	    returnFromSystemCall();
}

/**
 * 	System call 2 encargado de tomar un ejecutable (archivo)
 * 	y cargarlo en un address space, para proceder a la ejecucion
 * 	e insercion de datos del hilo actual a la tabla de procesos.
**/
void Nachos_Exec(){
	extraerMensajeRegistro(4);  // preguntar metodo a Macho
                                // variable global
    OpenFile *ejecutable = open(bufferString,O_RDWR); //PREGUNTAR A MACHO
    Thread* newThread = new Thread(bufferString);
    newThread->Fork(Exe_CambiaEspacio,(void*)ejecutable);
    returnFromSystemCall();
}

void Exe_CambiaEspacio(void* exe)
{
    SpaceId processID;
    processID = 0;

    OpenFile *nuevoExe = (OpenFile*) exe;
    if(nuevoExe==NULL){
        printf("Error al cargar el ejecutable");
        return -1;
    }
    space = new AddrSpace(nuevoExe);
    currentThread->space = space;
    processID = processTable.RegisterProcess((long) currentThread);
    delete nuevoExe;
    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();
    machine->Run();
    ASSERT(false);
}

/**
 * 	System call 3 encargado de hallar el proceso o hilo con el
 * 	id que recibe por parámetro, y poner en espera al hilo hasta
 * 	que finalice dicho proceso.
 *
**/
void Nachos_Join(){
    SpaceId pid  = machine->ReadRegister(4); 				// identificador del proceso
    Thread * hilera;
    hilera = TablaIndicadores->getThread(pid);
    //DEBUG('z', "Join System call id(%d) \n", pid);
    Semaphore *s = new Semaphore("Hilo", 0); //estado inicial = wait
    hilera->Join(s);
    s->P();
	returnFromSystemCall()
    //currentThread->Finish();
}


/**
    System call 4 encargado de la creacion de archivos
    en el sistema NachOS, utiliza un metodo externo
    extraerMensajeRegistro para obtener elementos del registro 4

**/
void Nachos_Create(){
    int temporal = 4;
    extraerMensajeRegistro(temporal);
    temporal = creat(bufferString, S_IRUSR | S_IWUSR);
    if(temporal < 0 ){            // Se verifica la creacion correcta del archivo
        perror("Error al crear el archivo");
        }else{
        temporal = currentThread->tabla.Open(temporal);
        currentThread->tabla.addThread();
    }
    returnFromSystemCall();
}


/**
 * 	Metodo compartido entre varios metodos de exception.cc
 * 	encargado de leer el registro que obtiene por parametro,
 * 	y sobreescribir en un buffer el mensaje recopilado de memoria.
**/
void extraerMensajeRegistro(int registro){
    int direccionM = machine->ReadRegister(registro);    // Se obtiene la direccion de memoria del parametro
    int letra;                        // Caracter al que se le aplica casting a char para el buffer
    bool continuar = true;
    char temporal = 'a';
    for(int j = 0; continuar; ++j){
        if(temporal != '\0'){
            machine->ReadMem(direccionM+j, 1, &letra);        // Se lee a partir de la direccion de memoria obtenida
            bufferString[j] = (char) letra;                // y se castea para el buffer
        }else{
            continuar = false;
        }
    }
}


/**
 * 	System call 5 encargado de abrir un archivo y agregar su id
 * 	a la tabla de archivos abiertos.
**/
void Nachos_Open() {

    int pFilename = machine->ReadRegister(4);
    char * file = new char[100];
    bool noError = true;
    int i = 0;
    do{
        noError = machine->ReadMem(pFilename+i, 1, (int*)file+i);
        ++i;
    }while(noError && file[i]);

    if(noError){
        pFile = open(filename,O_RDWR);
        if(pFilename != -1) {
            tablaArchivos->addThread();
            tablaArchivos->Open(pFilename);
        }
    }
    machine->WriteRegister(2,pFilename);
    delete [] file;
    returnFromSystemCall();


	//tabla.Open();
	// Read the name from the user memory, see 4 below
	// Use NachosOpenFilesTable class to create a relationship
	// between user file and unix file
	// Verify for errors

}       // Nachos_Open


/**
 * 	System call 6 encargado de la lectura de un archivo cargado en la
 * 	memoria previamente, al hacer uso de los registros 4, 5 y 6.
**/
void Nachos_Read(){
    int direccionEnArchivo = machine->ReadRegister(4); 				// El archivo de donde se va a leer
    int tamano = machine->ReadRegister(5); 					// Tamano de los que se va a leer
    OpenFileId ID = machine->ReadRegister(6); 					// Descriptor del archivo a leer

    int numBytes = 0;
    char * temp;
    temp = new char[tamano];

    //printf("Tamaño del mensaje a leer: %i ID: %i\n", tamano, ID);

    consola->P();
    switch(ID){
        case ConsoleInput:
            numBytes = read(1, temp, tamano);
            machine->WriteRegister(2, numBytes);
            break;
        case ConseOutput:
            machine->WriteRegister(2, -1); //ver la vara xq hay q copiarlo a memoria de nachos xq no lo puede ver
            printf("Leer desde la salida de consola no es posible. Eliminando programa");
            break;
        case ConsoleError:
            printf("Error %d\n", machine->ReadRegister(4));
            break;
        default:
            if(currentThread->tabla.isOpened(ID) != true){ //true es abierto
                machine->WriteRegister(2, -1);
            }
            else{
                int unitHandle = currentThread->tabla.getUnixHandle(ID);
                numBytes = read(unixHandle, temp, tamano);
                //printf("Leyendo desde otro archivo:");
                machine->WriteRegister(2, numBytes);
            }
    }
    consola->V();
  //machine->WriteMem(direccionEnArchivo, tamano, temp); //escribe en memoria
    returnFromSystemCall();
}


/**
 * 	System call 7 encargado de encontrar el archivo necesario en el cual
 * 	se sobreescribiran datos, posteriormente se procede a la escritura
 * 	en el archivo por el metodo propio de Unix "write".
**/
void Nachos_Write() {

    int size = machine->ReadRegister( 5 );

    OpenFileId id = machine->ReadRegister( 6 );
    int indiceUnix = tabla->getUnixHandle(id);


    int bufferVirtual = machine->ReadRegister(4);
    int caracterLeido;
    int cantCaracteres = 0;

    char buffer[size];
    for(int i=0; i<size; ++i){
        machine->ReadMem(bufferVirtual, 1, &caracterLeido);
        buffer[i] = (char)caracterLeido;
        ++bufferVirtual;
        ++cantCaracteres;
        caracterLeido = 0;
    }
    switch (id) {

    case  ConsoleInput:
        machine->WriteRegister( 2, -1 );
        break;
    case  ConsoleOutput:
        buffer[ size ] = 0;
        //strcpy(mensajeLeido, buffer);
        stats->numConsoleCharsWritten++;
        printf( "%s \n", bufferString );
        break;
    case ConsoleError:
        printf( "%d\n", machine->ReadRegister( 4 ) );
        break;
    default:
        bool abierto = tabla->isOpened(id);
        if(abierto == true){
            write(indiceUnix, buffer, size);
            machine->WriteRegister(2, cantCaracteres);
            stats->numDiskWrites++;
        }else{
            printf("El archivo no está abierto\n");
            machine->WriteRegister(2, -1);
        }
    }
    returnFromSystemCall();
}


/**
    System call 8 encargado de cerrar o finalizar un archivo
    que se encuentre abierto en el sistema NachOS. Esto lo
    realiza leyendo del registro 6 para encontrar el ID
    del mismo.
**/
void Nachos_Close(){
    int lecturaRegistro = machine->ReadRegister(6);
    int resultado = currentThread->tabla->Close(lecturaRegistro);                // Se obtiene el numero archivo deseado para eliminar
    if(resultado < 0){                                    // Verificacion del cerrado del archivo en la tabla
        perror("Error al proceder con el borrado del archivo");
    }else{
        resultado = close(tabla->getUnixHandle(lecturaRegistro));            // Close de Unix para el hilo actual
        printf("Se ha cerrado el archivo correctamente");
        currentThread->tabla->delThread();
    }
    returnFromSystemCall();
}


/**
 * 	System call 9 encargado de recibir una referencia a un metodo,
 * 	en donde se abre un nuevo hilo hijo del hijo padre para proceder
 * 	con su ejecucion.
**/
void Nachos_Fork() {

	DEBUG( 'u', "Entering Fork System call\n" );
	// We need to create a new kernel thread to execute the user thread
	Thread * newT = new Thread( "child to execute Fork code" );

	// We need to share the Open File Table structure with this new child

	// Child and father will also share the same address space, except for the stack
	// Text, init data and uninit data are shared, a new stack area must be created
	// for the new child
	// We suggest the use of a new constructor in AddrSpace class,
	// This new constructor will copy the shared segments (space variable) from currentThread, passed
	// as a parameter, and create a new stack for the new child
	newT->space = new AddrSpace( currentThread->space );

	// We (kernel)-Fork to a new method to execute the child code
	// Pass the user routine address, now in register 4, as a parameter
	newT->Fork( NachosForkThread, machine->ReadRegister( 4 ) );

	returnFromSystemCall();	// This adjust the PrevPC, PC, and NextPC registers

	DEBUG( 'u', "Exiting Fork System call\n" );
}	// Kernel_Fork

// Pass the user routine address as a parameter for this function
// This function is similar to "StartProcess" in "progtest.cc" file under "userprog"
// Requires a correct AddrSpace setup to work well

void NachosForkThread( int p ) {

    AddrSpace *space;

    space = currentThread->space;
    space->InitRegisters();             // set the initial register values
    space->RestoreState();              // load page table register

// Set the return address for this thread to the same as the main thread
// This will lead this thread to call the exit system call and finish
    machine->WriteRegister( RetAddrReg, 4 );

    machine->WriteRegister( PCReg, p );
    machine->WriteRegister( NextPCReg, p + 4 );

    machine->Run();                     // jump to the user progam
    ASSERT(FALSE);
}


/**
    System call 10 encargado de colocar el hilo actual en espera
    y permitir al CPU que ejecute otros hilos en espera.
**/
void Nachos_Yield(){
    currentThread->Yield();
    returnFromSystemCall;
}


/**
 * 	System call 11 encargado de crear un nuevo semaforo
 * 	para el hilo de ejecucion actual.
**/
void Nachos_SemCreate(){
	int valorIni = machine->ReadRegister(4);
    int id = tablasSemaforos->nuevoSemaforo(valorIni);
    machine->WriteRegister( 2, id );
    returnFromSystemCall();
}


/**
 * 	System call 12 encargado de eliminar el semaforo
 * 	para el hilo de ejecucion actual.
**/
void Nachos_SemDestroy(){
	int id = machine->ReadRegister(4);
    tablasSemaforos->destruirSemaforo(id);
    returnFromSystemCall();
}


/**
 * 	System call 13 encargado de generar una señal proveniente
 * 	del hilo actual que es recibida por semaforos en espera
 * 	ejecutandose de forma paralela.
**/
void Nachos_SemSignal(){
	 int id = machine->ReadRegister(4);
    tablasSemaforos->semaforoSignal(id);
    returnFromSystemCall();
}


/**
 * 	System call 14 encargado de generar una señal de espera
 *      proveniente del hilo actual, el cual espera hasta recibir
 * 	una señal proveniente de otro semaforo de hilo en ejecucion.
**/
void Nachos_SemWait(){
	 int id = machine->ReadRegister(4);
    tablasSemaforos->semaforoWait(id);
    returnFromSystemCall();
}


/**
 * 	Metodo principal de exception.cc, encargado de obtener por
 * 	parametro el system call solicitado, en donde por medio de
 * 	un switch y variables definidas previamente, se realizan los
 * 	llamados a metodos especificos que ejecutan un determinado
 * 	system call.
**/
void ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    switch ( which ) {

       case SyscallException:
          switch ( type ) {
             case SC_Halt:
                Nachos_Halt();              // System call # 0
                break;
	     case SC_Exit:
		Nachos_Exit();                      // System call #1
		 break;
	     case SC_Exec:
		Nachos_Exec();                      // System call #2
		break;
	     case SC_Join:
		Nachos_Join();                      // System call #3
		break;
	     case SC_Create:
		Nachos_Create();                    // System call #4
		break;
             case SC_Open:
                Nachos_Open();             // System call # 5
                break;
	     case SC_Read:
		Nachos_Read();                      // System call #6
		break;
             case SC_Write:
                Nachos_Write();             // System call #7
                break;
	     case SC_Close:
		Nachos_Close();                     // System call #8
		break;
             case SC_Fork:
                Nachos_Fork();              // System call #9
                break;
	     case SC_Yield:
		        Nachos_Yield();                     // System call #10
		        break;
	     case SC_SemCreate:
		Nachos_SemCreate();                 // System call #11
		break;
	     case SC_SemDestroy:
		Nachos_SemDestroy();                // System call #12
		break;
	     case SC_SemSignal:
		Nachos_SemSignal();                 // System call #13
		break;
	     case SC_SemWait:
		Nachos_SemWait();                   // System call #14
		break;
             default:
                printf("Unexpected syscall exception %d\n", type );
                ASSERT(FALSE);
                break;
          }
       break;
       case PageFaultEception:
            DEBUG('p', "Ocurrio un page fault eception\n");
            break;
       default:
          printf( "Unexpected exception %d\n", which );
          ASSERT(FALSE);
          break;
    }
}
