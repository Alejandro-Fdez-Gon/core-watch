/*
 * coreWatch.h
 *
 *  Created on: 18 de feb. de 2022
 *      Author: Rodrigo Tamaki Moreno y Alejandro Fernandez Gonzalez
 */

#ifndef COREWATCH_H_
#define COREWATCH_H_

// INCLUDES
#include "systemConfig.h"     
#include "reloj.h"
#include "teclado_TL04.h"
#include <stdio.h>

// ENUMS
enum FSM_ESTADOS_SISTEMA {START, STAND_BY, SET_TIME, SET_CALENDAR}; // Mejora: Nuevo estado para modificar la fecha.
enum FSM_DETECCION_COMANDOS {WAIT_COMMAND};

// FLAGS FSM DEL SISTEMA COREWATCH
#define FLAG_SETUP_DONE 0x04				// Bit 3 para indicar inicio listo del sistema
#define FLAG_RESET 0x08						// Bit 4 para indicar reset del sistema
#define FLAG_SET_CANCEL_NEW_TIME 0x10		// Bit 5 para indicar al sistema el aborto o solicitud del cambio de hora
#define FLAG_NEW_TIME_IS_READY 0x20			// Bit 6 para indicar al sistema que se ha realizado el cambio de hora 
#define FLAG_DIGITO_PULSADO 0x40			// Bit 7 para indicar el uso del teclado
#define FLAG_TIMEOUT 0x80                   // Bit 8 para indicar que ha transcurrido el tiempo para introducir hora
#define FLAG_NEW_CALENDAR_IS_READY 0x100    // Bit 9 para indicar al sistema que se ha realizado el cambio de fecha
#define FLAG_SET_CANCEL_NEW_CALENDAR 0x200  // Bit 10 para indicar al sistema el aborto o solicitud del Scambio de fecha

// DEFINES
#define TECLA_RESET 'F'						// Tecla asociada para resetear el sistema
#define TECLA_SET_CANCEL_TIME 'E'			// Tecla asociada para solicitar o abortar el cambio de hora
#define TECLA_EXIT 'B'						// Tecla asociada para salir del sistema
#define TECLA_DIA_SEMANA 'D'				// Tecla asociada para mostrar día de la semana
#define TECLA_SET_CANCEL_CALENDAR 'C'       // Tecla asociada para solicitar o abortar el cambio de fecha
#define TECLA_SEE_PASSED_TIME 'A'			// Tecla asociada para mostrar el tiempo transcurrido desde el inicio

#define ESPERA_MENSAJE_MS 2000
#define TIMEOUT_MS 10000

// DECLARACIÓN ESTRUCTURAS
typedef struct {							// Estructura del sistema, contiene:
	TipoReloj reloj ;						// Reloj 						     - TipoReloj
	TipoTeclado teclado;					// Teclado						     - TipoTeclado
	int tempTime ;							// Hora auxiliar durante pulsado     - entero
	int digitosGuardados ;					// Cantidad digitos pulsados 	     - entero
	int digitoPulsado ;						// Ultimo digito pulsado 		     - entero
	int lcdId;								// Handle a nustro display 		     - entero
	int tempCalendar;                       // Mejora: Fecha aux durante pulsado - entero
} TipoCoreWatch ;

//------------------------------------------------------
// FUNCIONES PROPIAS
//------------------------------------------------------
int ConfiguraInicializaSistema(TipoCoreWatch* p_sistema);
void DelayUntil(unsigned int next);
int EsNumero(char value);
void ShowDay(TipoCalendario *p_fecha, int handler); // Mejora Mostrar Día
void ShowTimePassed(int handler);					// Majora Mostrar tiempo transcurrido
//------------------------------------------------------
// FUNCIONES DE ENTRADA O DE TRANSICION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
int CompruebaDigitoPulsado(fsm_t *p_this);
int CompruebaNewTimeIsReady(fsm_t *p_this);
int CompruebaNewCalendarIsReady(fsm_t *p_this);    // Mejora SetFecha
int CompruebaReset(fsm_t *p_this);
int CompruebaSetCancelNewTime(fsm_t *p_this);
int CompruebaSetCancelNewCalendar(fsm_t *p_this);  // Mejora SetFecha
int CompruebaSetupDone(fsm_t *p_this);
int CompruebaTeclaPulsada(fsm_t *p_this);
int CompruebaTimeActualizado(fsm_t *p_this);
int CompruebaTimeout(fsm_t *p_this);               // Mejora Timeout
//------------------------------------------------------
// FUNCIONES DE SALIDA O DE ACCION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
void CancelSetNewTime(fsm_t *p_this);
void CancelSetNewCalendar(fsm_t *p_this);          // Mejora SetFecha
void PrepareSetNewTime(fsm_t *p_this);
void PrepareSetNewCalendar(fsm_t *p_this);         // Mejora SetFecha
void ProcesaDigitoTime(fsm_t *p_this);
void ProcesaDigitoCalendar(fsm_t *p_this);         // Mejora SetFecha
void ProcesaTeclaPulsada(fsm_t *p_this);
void Reset(fsm_t *p_this);
void SetNewTime(fsm_t *p_this);
void SetNewCalendar(fsm_t *p_this);                // Mejora SetFecha
void ShowTime(fsm_t *p_this);
void Start(fsm_t *p_this);
//------------------------------------------------------
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES
//------------------------------------------------------
void timeout_isr(union sigval value);              // Mejora Timeout
//------------------------------------------------------
// FUNCIONES LIGADAS A THREADS ADICIONALES
//------------------------------------------------------
#if VERSION == 2
PI_THREAD(ThreadExploraTecladoPC);
#endif

#endif /* COREWATCH_H_ */
