/*
 * reloj.h
 *
 *  Created on: 18 de feb. de 2022
 *      Author: Rodrigo Tamaki Moreno y Alejandro Fernandez Gonzalez
 */

#ifndef RELOJ_H_
#define RELOJ_H_

// INCLUDES
#include "systemConfig.h"
#include "util.h"

// ENUMS
enum FSM_ESTADOS_RELOJ {WAIT_TIC};

// FLAGS FSM DEL RELOJ
#define FLAG_ACTUALIZA_RELOJ 0x01			// Bit 1 para actualizar el reloj
#define FLAG_TIME_ACTUALIZADO 0x02			// Bit 2 para confirmar reloj se ha actualizado

// DEFINES
#define MIN_DAY 1							// Menor dia en un mes
#define MIN_MONTH 1							// Menor mes en un año
#define MAX_MONTH 12						// Mayor mes en un año
#define MIN_YEAR 1970						// Menor año del reloj
#define MAX_YEAR 2999						// Mayor año del reloj
#define MAX_MIN 59							// Mayor mins reloj
#define TIME_FORMAT_12_H 12					// Formato 12 horas
#define TIME_FORMAT_24_H 24					// Formato 24 horas
#define MIN_HOUR_12_H 1						// Menor hora en formato 12h
#define MAX_HOUR_12_H 12					// Mayor hora en formato 12h
#define MIN_HOUR_24_H 0						// Menor hora en formato 24h
#define MAX_HOUR_24_H 23					// Mayor hora en formato 24h
#define PRECISION_RELOJ_MS 1000				// Tiempo actualizacion reloj en ms
#define DEFAULT_DAY 28						// Dia por defecto
#define DEFAULT_MONTH 2						// Mes por defecto
#define DEFAULT_YEAR 2020					// Año por defecto
#define DEFAULT_HOUR 0						// Hora por defecto
#define DEFAULT_MIN 0						// Minuto por defecto
#define DEFAULT_SEC 0						// Segundo por defecto

// DECLARACION ESTRUCTURAS
typedef struct {							// Estructura calendario actual, contiene:
	int dd;									//  Dia - entero
	int MM;									// 	Mes - entero
	int yyyy;								// 	Año - entero
} TipoCalendario;

typedef struct {							// Estructura tiempo actual, contiene:
	int hh;									// 	Hora	- entero
	int mm;									// 	Minuto 	- entero
	int ss;									// 	Segundo	- entero
	int formato ;							// 	Formato	- entero
} TipoHora ;

typedef struct {							// Estructura reloj coreWatch, contiene:
	int timestamp ;							// 	Segundos desde inicio			- entero
	TipoHora hora ;							// 	Hora							- TipoHora
	TipoCalendario calendario ;				// 	Calendario						- TipoCalendario
	tmr_t * tmrTic ;						// 	Puntero para actualizar la hora - temporizador (tmr_t)
} TipoReloj ;

typedef struct {							// Estructura comunicar eventos o estado, contiene:
	int flags ;								// Flags - entero
} TipoRelojShared ;

extern fsm_trans_t g_fsmTransReloj [];		// Declaracion tabla de transiciones
extern const int DIAS_MESES[2][MAX_MONTH];	// Declaracion array dias del mes si año bisiesto o no

//------------------------------------------------------
// FUNCIONES DE INICIALIZACION DE LAS VARIABLES
//------------------------------------------------------
int ConfiguraInicializaReloj(TipoReloj *p_reloj);
void ResetReloj(TipoReloj *p_reloj);
//------------------------------------------------------
// FUNCIONES PROPIAS
//------------------------------------------------------
void ActualizaFecha(TipoCalendario *p_fecha);
void ActualizaHora(TipoHora *p_hora);
int CalculaDiasMes(int month, int year);
int Esbisiesto(int year);
TipoRelojShared GetRelojSharedVar();
int SetFecha(int nuevaFecha, TipoCalendario *p_fecha);
int SetFormato(int nuevoFormato, TipoHora *p_hora);
int SetHora(int nuevaHora, TipoHora *p_hora);
void SetRelojSharedVar(TipoRelojShared value);
//------------------------------------------------------
// FUNCIONES DE ENTRADA O DE TRANSICION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
int CompruebaTic(fsm_t *p_this);
//------------------------------------------------------
// FUNCIONES DE SALIDA O DE ACCION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
void ActualizaReloj(fsm_t *p_this);
//------------------------------------------------------
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES
//------------------------------------------------------
void tmr_actualiza_reloj_isr(union sigval value);

#endif /* RELOJ_H_ */
