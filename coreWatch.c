/*
 * coreWatch.c
 *
 *  Created on: 18 de feb. de 2022
 *      Author: Rodrigo Tamaki Moreno y Alejandro Fernandez Gonzalez
 */

// INCLUDES
#include "coreWatch.h"
#include <stdio.h>

//------------------------------------------------------
// DECLARACION Y DEFINICION VARIABLES GLOBALES
//------------------------------------------------------
// Declaramos las variables globales
TipoCoreWatch g_coreWatch;
static int g_flagsCoreWatch, passed_time;
tmr_t* g_tmr_timeout;        //Mejora: tmr del Timeout

// Definimos maquina de estados del Core Watch
fsm_trans_t fsmTransCoreWatch[] = {
		{START, CompruebaSetupDone, STAND_BY, Start},
		{STAND_BY, CompruebaTimeActualizado, STAND_BY, ShowTime},
		{STAND_BY, CompruebaReset, STAND_BY, Reset},
		{STAND_BY, CompruebaSetCancelNewTime, SET_TIME, PrepareSetNewTime},
		{STAND_BY, CompruebaSetCancelNewCalendar, SET_CALENDAR, PrepareSetNewCalendar},// Mejora SetFecha
		{SET_TIME, CompruebaSetCancelNewTime, STAND_BY, CancelSetNewTime},
		{SET_TIME, CompruebaNewTimeIsReady, STAND_BY, SetNewTime},
		{SET_TIME, CompruebaReset, STAND_BY, Reset},                                   // Mejora: Reset durante SET_TIME
		{SET_TIME, CompruebaDigitoPulsado, SET_TIME, ProcesaDigitoTime},
		{SET_TIME, CompruebaTimeout, STAND_BY, CancelSetNewTime},
		{SET_CALENDAR, CompruebaSetCancelNewCalendar, STAND_BY, CancelSetNewCalendar}, // Mejora SetFecha
		{SET_CALENDAR, CompruebaNewCalendarIsReady, STAND_BY, SetNewCalendar},         // Mejora SetFecha
		{SET_CALENDAR, CompruebaDigitoPulsado, SET_CALENDAR, ProcesaDigitoCalendar},   // Mejora SetFecha
		{SET_CALENDAR, CompruebaTimeout, STAND_BY, CancelSetNewCalendar},              // Mejora SetFecha
		{SET_CALENDAR, CompruebaReset, STAND_BY, Reset},                               // Mejora: Reset durante SET_CALENDAR
		{-1, NULL, -1, NULL}
};

// Definimos maquina de estados control teclado matricial
fsm_trans_t	fsmTransDeteccionComandos[] = {
        {WAIT_COMMAND, CompruebaTeclaPulsada, WAIT_COMMAND, ProcesaTeclaPulsada},
        {-1, NULL, -1, NULL}
};

//------------------------------------------------------
// FUNCIONES DE INICIALIZACION DE LAS VARIABLES
//------------------------------------------------------
// Funcion que inicializa el sistema
int ConfiguraInicializaSistema(TipoCoreWatch *p_sistema) {
	int aux;
	int arrayFilas[NUM_FILAS_TECLADO] = {GPIO_KEYBOARD_ROW_1, GPIO_KEYBOARD_ROW_2, GPIO_KEYBOARD_ROW_3, GPIO_KEYBOARD_ROW_4};
	int arrayColumnas[NUM_COLUMNAS_TECLADO] = {GPIO_KEYBOARD_COL_1, GPIO_KEYBOARD_COL_2, GPIO_KEYBOARD_COL_3, GPIO_KEYBOARD_COL_4};

	g_flagsCoreWatch = 0;
	p_sistema->tempTime = 0;
	p_sistema->digitosGuardados = 0;
	p_sistema->tempCalendar = 0;
	aux = ConfiguraInicializaReloj(&(p_sistema->reloj));

	if (aux != 0) {
		return 1;
	}

	#if VERSION == 2
		int thread = piThreadCreate(ThreadExploraTecladoPC);
		if (thread != 0) {
			return 2;
		}
	#endif

	#if VERSION >= 3
		memcpy(p_sistema->teclado.filas, arrayFilas, sizeof(arrayFilas));
		memcpy(p_sistema->teclado.columnas, arrayColumnas, sizeof(arrayColumnas));
		if (wiringPiSetupGpio()) {
			return 2;
		}
		ConfiguraInicializaTeclado(&(p_sistema->teclado));
	#endif

    #if VERSION >= 4
        p_sistema->lcdId = lcdInit(NUM_ROWS, NUM_COLS, NUM_BITS, GPIO_LCD_RS, GPIO_LCD_EN,
		    GPIO_LCD_D0, GPIO_LCD_D1, GPIO_LCD_D2, GPIO_LCD_D3, GPIO_LCD_D4, GPIO_LCD_D5, GPIO_LCD_D6, GPIO_LCD_D7);

        if (p_sistema->lcdId == -1) {
            return 3;
        }

        piLock(STD_IO_LCD_BUFFER_KEY);
        lcdPuts(p_sistema->lcdId, "INIT SUCCESS");
        piUnlock(STD_IO_LCD_BUFFER_KEY);
    #endif   

	piLock(SYSTEM_KEY);
	g_flagsCoreWatch |= FLAG_SETUP_DONE;
	piUnlock(SYSTEM_KEY);
	return 0;
}

//------------------------------------------------------
// FUNCIONES PROPIAS
//------------------------------------------------------
// Funcion que realiza la espera activa hasta el timestamp indicado en milisegundos
void DelayUntil(unsigned int next) {
	unsigned int now = millis();
	if (next > now) {
		delay(next - now);
	}
}

// Funcion que comprueba si un valor pasado como char es o no un numero
int EsNumero(char value) {
    if (value >= 48 && value <= 57)
        return 1;
    return 0;
}

// Mejora: Funcion que muestra el día de la semana
void ShowDay(TipoCalendario *p_fecha, int handler) {

	int day = p_fecha->dd;
	int month = p_fecha->MM;
	int year = p_fecha->yyyy;

	int weekDay = (day += month < 3 ? year-- : year - 2, 23*month/9 + day + 4 + year/4 - year/100 + year/400) % 7;

	#if	VERSION < 4
		if (weekDay == 0) {
			printf("Dia de la semana: Domingo\n");
		} else if (weekDay == 1) {
			printf("Dia de la semana: Lunes\n");
		} else if (weekDay == 2) {
			printf("Dia de la semana: Martes\n");
		} else if (weekDay == 3) {
			printf("Dia de la semana: Miércoles\n");
		} else if (weekDay == 4) {
			printf("Dia de la semana: Jueves\n");
		} else if (weekDay == 5) {
			printf("Dia de la semana: Viernes\n");
		} else {
			printf("Dia de la semana: Sábado\n");
		}
	#endif

	#if	VERSION >= 4
		piLock(STD_IO_LCD_BUFFER_KEY);
		lcdPosition(handler, 9, 0);

		switch (weekDay) {
			case 0:
				lcdPuts(handler, "DOM");
				break;
			case 1:
				lcdPuts(handler, "LUN");
				break;
			case 2:
				lcdPuts(handler, "MAR");
				break;
			case 3:
				lcdPuts(handler, "MIE");
				break;
			case 4:
				lcdPuts(handler, "JUE");
				break;
			case 5:
				lcdPuts(handler, "VIE");
				break;
			default:
				lcdPuts(handler, "SAB");
		}
		delay(ESPERA_MENSAJE_MS);
		piUnlock(STD_IO_LCD_BUFFER_KEY);

	#endif
}

// Funcion que muestra el tiempo transcurrido desde el inicio del sistema
void ShowTimePassed(int handler) {
	int tiempo_trans = passed_time;

	#if	VERSION < 4
		printf("Han transcurrido %02d segundos desde que se inició el sistema\n", passed_time);
	#endif

	#if	VERSION >= 4
		piLock(STD_IO_LCD_BUFFER_KEY);
		lcdPosition(handler, 0, 1);
		lcdPrintf(handler, "%02d SEGUNDOS", tiempo_trans);
		delay(ESPERA_MENSAJE_MS);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
}
//------------------------------------------------------
// FUNCIONES DE ENTRADA O DE TRANSICION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
// Funcion que comprueba que el bit 7 de las flags del sistema esta activo
int CompruebaDigitoPulsado(fsm_t *p_this) {
    int resultado = 0;
    piLock(KEYBOARD_KEY);
    resultado = g_flagsCoreWatch & FLAG_DIGITO_PULSADO;
    piUnlock(KEYBOARD_KEY);
    return resultado;
}

// Funcion que comprueba que el bit 6 de las flags del sistema esta activo
int CompruebaNewTimeIsReady(fsm_t *p_this) {
    int resultado = 0;
    piLock(SYSTEM_KEY);
    resultado = g_flagsCoreWatch & FLAG_NEW_TIME_IS_READY;
    piUnlock(SYSTEM_KEY);
    return resultado;
}

// Mejora: Funcion que comprueba que el bit 9 de las flags del sistema esta activo
int CompruebaNewCalendarIsReady(fsm_t *p_this) {
	int resultado = 0;
	piLock(SYSTEM_KEY);
	resultado = g_flagsCoreWatch & FLAG_NEW_CALENDAR_IS_READY;
	piUnlock(SYSTEM_KEY);
	return resultado;
}

// Funcion que comprueba que el bit 4 de las flags del sistema esta activo
int CompruebaReset(fsm_t *p_this) {
     int resultado = 0;
     piLock(SYSTEM_KEY);
     resultado = g_flagsCoreWatch & FLAG_RESET;
     piUnlock(SYSTEM_KEY);
     return resultado;
}

// Funcion que comprueba que el bit 5 de las flags del sistema esta activo
int CompruebaSetCancelNewTime(fsm_t *p_this) {
    int resultado = 0;
    piLock(SYSTEM_KEY);
    resultado = g_flagsCoreWatch & FLAG_SET_CANCEL_NEW_TIME;
    piUnlock(SYSTEM_KEY);
    return resultado;
}

// Mejora: Funcion que comprueba que el bit 10 de las flags del sistema esta activo
int CompruebaSetCancelNewCalendar(fsm_t *p_this) {
	int resultado = 0;
	piLock(SYSTEM_KEY);
	resultado = g_flagsCoreWatch & FLAG_SET_CANCEL_NEW_CALENDAR;
	piUnlock(SYSTEM_KEY);
	return resultado;
}

// Funcion que comprueba que el bit 3 de las flags del sistema esta activo
int CompruebaSetupDone(fsm_t *p_this) {
    int resultado = 0;
    piLock(SYSTEM_KEY);
    resultado = g_flagsCoreWatch & FLAG_SETUP_DONE;
    piUnlock(SYSTEM_KEY);
    return resultado;
}

// Funcion que comprueba que el bit 2 de las flags del teclado esta activo
int CompruebaTeclaPulsada(fsm_t *p_this) {
    TipoTecladoShared teclado; 
    teclado = GetTecladoSharedVar();
    int resultado = 0;
    piLock(KEYBOARD_KEY);
    resultado = teclado.flags & FLAG_TECLA_PULSADA;
    piUnlock(KEYBOARD_KEY);
	return resultado;
}

// Funcion que comprueba que el bit 2 de las flags del reloj esta activo
int CompruebaTimeActualizado(fsm_t *p_this) {
    int resultado = 0;
    TipoRelojShared g_reloj;
    g_reloj = GetRelojSharedVar();
    piLock(RELOJ_KEY);
    resultado = g_reloj.flags & FLAG_TIME_ACTUALIZADO;
    piUnlock(RELOJ_KEY);
    return resultado;
}

// Mejora: Funcion que comprueba que el bit 8 de las flags del sistema esta activo
int CompruebaTimeout(fsm_t *p_this) {
	int resultado = 0;
	piLock(STD_IO_LCD_BUFFER_KEY);
	resultado = g_flagsCoreWatch & FLAG_TIMEOUT;
	piUnlock(STD_IO_LCD_BUFFER_KEY);
	return resultado;
}
//------------------------------------------------------
// FUNCIONES DE SALIDA O DE ACCION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
// Funcion que muestra la hora y fecha al usuario
void ShowTime(fsm_t *p_this) {
    TipoCoreWatch *p_sistema = (TipoCoreWatch*) (p_this->user_data);
    TipoRelojShared g_reloj;
    g_reloj = GetRelojSharedVar();
    piLock(RELOJ_KEY);
    g_reloj.flags &= ~FLAG_TIME_ACTUALIZADO;
    piUnlock(RELOJ_KEY);
    SetRelojSharedVar(g_reloj);

    #if VERSION < 4
        printf("Son las: %02d:%02d:%02d del %02d/%02d/%02d \n", p_sistema->reloj.hora.hh, p_sistema->reloj.hora.mm, p_sistema->reloj.hora.ss,
        		p_sistema->reloj.calendario.dd, p_sistema->reloj.calendario.MM, p_sistema->reloj.calendario.yyyy);
        fflush(stdout);
    #endif

	#if VERSION >= 4
        int handler = p_sistema->lcdId;
        piLock(STD_IO_LCD_BUFFER_KEY);
        lcdClear(handler);
        lcdPosition(handler, 0, 0);
        lcdPrintf(handler, "%02d:%02d:%02d", p_sistema->reloj.hora.hh, p_sistema->reloj.hora.mm, p_sistema->reloj.hora.ss);
        lcdPosition(handler, 0, 1);
		lcdPrintf(handler, "%02d:%02d:%02d", p_sistema->reloj.calendario.dd, p_sistema->reloj.calendario.MM, p_sistema->reloj.calendario.yyyy);
		lcdPosition(handler, 0, 0);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif

}

// Funcion que indica que el sistema se ha iniciado y configurado correctamente
void Start(fsm_t *p_this) {
    piLock(SYSTEM_KEY);
    g_flagsCoreWatch &= ~FLAG_SETUP_DONE;
    piUnlock(SYSTEM_KEY);
}

// Funcion que resetea el reloj del sistema
void Reset(fsm_t *p_this) {
    TipoCoreWatch *p_sistema = (TipoCoreWatch*) (p_this->user_data);
    ResetReloj(&(p_sistema->reloj));
    piLock(SYSTEM_KEY);
    g_flagsCoreWatch &= ~FLAG_RESET;
    piUnlock(SYSTEM_KEY);

    #if VERSION < 4
        printf("[RESET] Hora reiniciada \n");
        fflush(stdout);
    #endif

	#if VERSION >= 4
        int handler = p_sistema->lcdId;
		piLock(STD_IO_LCD_BUFFER_KEY);
		lcdClear(handler);
		lcdPosition(handler, 0, 1);
		lcdPuts(handler, "RESET");
		delay(ESPERA_MENSAJE_MS);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
}

// Funcion que informa al usuario de que debe introducir una hora y formato
void PrepareSetNewTime(fsm_t *p_this) {
    TipoCoreWatch *p_sistema = (TipoCoreWatch*) (p_this->user_data);

    int formato = p_sistema->reloj.hora.formato;
    piLock(KEYBOARD_KEY);
    g_flagsCoreWatch &= ~FLAG_DIGITO_PULSADO;
    piUnlock(KEYBOARD_KEY);
    piLock(SYSTEM_KEY);
    g_flagsCoreWatch &= ~FLAG_SET_CANCEL_NEW_TIME;
    piUnlock(SYSTEM_KEY);

    #if VERSION < 4
        printf("[SET_TIME] Introduzca la nueva hora en formato 0-%d\n", formato);
        fflush(stdout);
    #endif

    #if VERSION >= 4
        piLock(STD_IO_LCD_BUFFER_KEY);
        lcdClear(p_sistema->lcdId);              
        lcdPosition(p_sistema->lcdId, 0, 1);          
        lcdPrintf(p_sistema->lcdId, "FORMAT: 0-%d", formato);
        piUnlock(STD_IO_LCD_BUFFER_KEY);
    #endif
    tmr_startms(g_tmr_timeout, TIMEOUT_MS);
}

// Mejora: Funcion que informa al usuario de que debe introducir una fecha
void PrepareSetNewCalendar(fsm_t *p_this) {
	TipoCoreWatch *p_sistema = (TipoCoreWatch*) (p_this->user_data);

	piLock(KEYBOARD_KEY);
	g_flagsCoreWatch &= ~FLAG_DIGITO_PULSADO;
	piUnlock(KEYBOARD_KEY);
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch &= ~FLAG_SET_CANCEL_NEW_CALENDAR;
	piUnlock(SYSTEM_KEY);

	#if VERSION < 4
		printf("[SET_CALENDAR] Introduzca la nueva fecha");
		fflush(stdout);
	#endif

	#if VERSION >= 4
		piLock(STD_IO_LCD_BUFFER_KEY);
		lcdClear(p_sistema->lcdId);
		lcdPosition(p_sistema->lcdId, 0, 1);
		lcdPuts(p_sistema->lcdId, "NEW DATE");
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif

	tmr_startms(g_tmr_timeout, TIMEOUT_MS);
}

// Funcion que cancela la lectura de una nueva hora que se esta introduciendo por el usuario
void CancelSetNewTime(fsm_t *p_this) {
    TipoCoreWatch *p_sistema = (TipoCoreWatch*) (p_this->user_data);

    p_sistema->tempTime = 0;
    p_sistema->digitosGuardados = 0;

    piLock(SYSTEM_KEY);
    g_flagsCoreWatch &= ~FLAG_SET_CANCEL_NEW_TIME;
    g_flagsCoreWatch &= ~FLAG_TIMEOUT;
    piUnlock(SYSTEM_KEY);

    #if VERSION < 4
        printf("[SET_TIME] Operacion cancelada\n");
        fflush(stdout);
    #endif

    #if VERSION >= 4
        piLock(STD_IO_LCD_BUFFER_KEY);
        lcdClear(p_sistema->lcdId);            
        lcdPosition(p_sistema->lcdId, 0, 1);   
        lcdPuts(p_sistema->lcdId, "CANCELADO");  
        delay(ESPERA_MENSAJE_MS);                
        lcdClear(p_sistema->lcdId);
        piUnlock(STD_IO_LCD_BUFFER_KEY);
    #endif
}

// Mejora: Funcion que cancela la lectura de una nueva fecha que se esta introduciendo por el usuario
void CancelSetNewCalendar(fsm_t *p_this) {
	TipoCoreWatch *p_sistema = (TipoCoreWatch*) (p_this->user_data);

	p_sistema->tempCalendar = 0;
	p_sistema->digitosGuardados = 0;
	
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch &= ~FLAG_TIMEOUT;
	g_flagsCoreWatch &= ~FLAG_SET_CANCEL_NEW_CALENDAR;
	piUnlock(SYSTEM_KEY);

	#if VERSION < 4
		printf("[SET_CALENDAR] Operacion cancelada\n");
		fflush(stdout);
	#endif

	#if VERSION >= 4
		piLock(STD_IO_LCD_BUFFER_KEY);
		lcdClear(p_sistema->lcdId);
		lcdPosition(p_sistema->lcdId, 0, 1);     
		lcdPuts(p_sistema->lcdId, "CANCELADO");  
		delay(ESPERA_MENSAJE_MS);                
		lcdClear(p_sistema->lcdId);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif
}

// Funcion que implementa la logica de recoger los digitos marcados para acomodarlos a un entero para asignar hora
void ProcesaDigitoTime(fsm_t *p_this) {
    TipoCoreWatch *p_sistema = (TipoCoreWatch*) (p_this->user_data);
    int ultimoTiempo = p_sistema->tempTime;
    int ultimoGuardados = p_sistema->digitosGuardados;
    int ultimoDigito = p_sistema->digitoPulsado;

    piLock(KEYBOARD_KEY);
    g_flagsCoreWatch &= ~FLAG_DIGITO_PULSADO;
    piUnlock(KEYBOARD_KEY);

    switch(ultimoGuardados) {
        case 0:
            if (p_sistema->reloj.hora.formato == 12) {
                ultimoDigito = MIN(1, ultimoDigito);
            } else {
                ultimoDigito = MIN(2, ultimoDigito);
            }
            ultimoTiempo = ultimoTiempo * 10 + ultimoDigito;
            ultimoGuardados++;
            break;
        case 1:
            if (p_sistema->reloj.hora.formato == 12) {
                if (ultimoTiempo == 0) {
                    ultimoDigito = MAX(1, ultimoDigito);
                } else {
                    ultimoDigito = MIN(2, ultimoDigito);
                }
            } else {
                if (ultimoTiempo == 2) {
                    ultimoDigito = MIN(3, ultimoDigito);
                }
            }
            ultimoTiempo = ultimoTiempo * 10 + ultimoDigito;
            ultimoGuardados++;
            break;
        case 2:
            ultimoTiempo = ultimoTiempo * 10 + MIN(5, ultimoDigito);
            ultimoGuardados++;
            break;
        default:
            ultimoTiempo = ultimoTiempo * 10 + ultimoDigito;
            piLock(KEYBOARD_KEY);
            g_flagsCoreWatch &= ~FLAG_DIGITO_PULSADO;
            piUnlock(KEYBOARD_KEY);
            piLock(SYSTEM_KEY);
            g_flagsCoreWatch |= FLAG_NEW_TIME_IS_READY;
            piUnlock(SYSTEM_KEY);
    }

    if (ultimoGuardados < 3) {
        if (ultimoTiempo > 2359) {
            ultimoTiempo %= 10000;
            ultimoTiempo = 100* MIN((int) (ultimoTiempo/100), 23) + MIN(ultimoTiempo%100, 59);
        }
    }

    #if VERSION < 4
        printf("[SET_TIME] Nueva hora temporal %d\n", ultimoTiempo);
        fflush(stdout);
    #endif

    #if VERSION >= 4
        piLock(STD_IO_LCD_BUFFER_KEY);
        lcdPosition(p_sistema->lcdId, 0, 0);
        lcdClear(p_sistema->lcdId);
        lcdPrintf(p_sistema->lcdId, "SET: %d", ultimoTiempo);
        piUnlock(STD_IO_LCD_BUFFER_KEY);
    #endif

    p_sistema->tempTime = ultimoTiempo;
    p_sistema->digitosGuardados = ultimoGuardados;
}

// Mejora: Funcion que implementa la logica de recoger los digitos marcados para acomodarlos a un entero para asignar fecha
void ProcesaDigitoCalendar(fsm_t *p_this) {
	TipoCoreWatch *p_sistema = (TipoCoreWatch*) (p_this->user_data);
	int ultimoCalendario = p_sistema->tempCalendar;
	int ultimoGuardados = p_sistema->digitosGuardados;
	int ultimoDigito = p_sistema->digitoPulsado;

	piLock(KEYBOARD_KEY);
	g_flagsCoreWatch &= ~FLAG_DIGITO_PULSADO;
	piUnlock(KEYBOARD_KEY);

	switch(ultimoGuardados) {
		case 0:
			ultimoDigito = MIN(3, ultimoDigito);
			ultimoCalendario = ultimoCalendario * 10 + ultimoDigito;
			ultimoGuardados++;
			break;
		case 1:
			if (ultimoCalendario == 3) {
				ultimoDigito = MIN(1, ultimoDigito);
			}
			ultimoCalendario = ultimoCalendario * 10 + ultimoDigito;
			ultimoGuardados++;
			break;
		case 2:
			ultimoDigito = MIN(1, ultimoDigito);
			ultimoCalendario = ultimoCalendario * 10 + ultimoDigito;
			ultimoGuardados++;
			break;
		case 3:
			if ((ultimoCalendario - ((int) ultimoCalendario/10) * 10) == 1) {
				ultimoDigito = MIN(2, ultimoDigito);
			}
			ultimoCalendario = ultimoCalendario * 10 + ultimoDigito;
			ultimoGuardados++;
			break;
		case 4:
			ultimoCalendario = ultimoCalendario * 10 + ultimoDigito;
			ultimoGuardados++;
			break;
		case 5:
			ultimoCalendario = ultimoCalendario * 10 + ultimoDigito;
			ultimoGuardados++;
			break;
		case 6:
			ultimoCalendario = ultimoCalendario * 10 + ultimoDigito;
			ultimoGuardados++;
			break;
		default:
			ultimoCalendario = ultimoCalendario * 10 + ultimoDigito;
			piLock(KEYBOARD_KEY);
			g_flagsCoreWatch &= ~FLAG_DIGITO_PULSADO;
			piUnlock(KEYBOARD_KEY);
			piLock(SYSTEM_KEY);
			g_flagsCoreWatch |= FLAG_NEW_CALENDAR_IS_READY;
			piUnlock(SYSTEM_KEY);
	}

	#if VERSION < 4
		printf("[SET_CALENDAR] Nuevo calendario temporal %d\n", ultimoCalendario);
		fflush(stdout);
	#endif

	#if VERSION >= 4
		piLock(STD_IO_LCD_BUFFER_KEY);
		lcdPosition(p_sistema->lcdId, 0, 0);
		lcdClear(p_sistema->lcdId);
		lcdPrintf(p_sistema->lcdId, "SET: %d", ultimoCalendario);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	#endif

	p_sistema->tempCalendar = ultimoCalendario;
	p_sistema->digitosGuardados = ultimoGuardados;
}

// Funcion que guarda en el reloj del sistema la hora que ha introducido el usuario
void SetNewTime(fsm_t * p_this) {
    TipoCoreWatch *p_sistema = (TipoCoreWatch*) (p_this -> user_data);
    piLock(SYSTEM_KEY);
    g_flagsCoreWatch &= ~FLAG_NEW_TIME_IS_READY;
    piUnlock(SYSTEM_KEY);
    SetHora(p_sistema->tempTime, &(p_sistema->reloj.hora));
    p_sistema->tempTime = 0;
    p_sistema->digitosGuardados = 0;
}

// Mejora: Funcion que guarda en el reloj del sistema la fecha que ha introducido el usuario
void SetNewCalendar(fsm_t *p_this) {
	TipoCoreWatch *p_sistema = (TipoCoreWatch*) (p_this -> user_data);
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch &= ~FLAG_NEW_CALENDAR_IS_READY;
	piUnlock(SYSTEM_KEY);
	SetFecha(p_sistema->tempCalendar, &(p_sistema->reloj.calendario));
	p_sistema->tempCalendar = 0;
	p_sistema->digitosGuardados = 0;
}

// Funcion que lee las pulsaciones del teclado matricial y las interpreta activando las flags del sistema
void ProcesaTeclaPulsada (fsm_t * p_this) {
    TipoCoreWatch *p_sistema = (TipoCoreWatch*) (p_this -> user_data);
    TipoTecladoShared teclado; 
    teclado = GetTecladoSharedVar();

    piLock(KEYBOARD_KEY);
    teclado.flags &= ~FLAG_TECLA_PULSADA;
    piUnlock(KEYBOARD_KEY);

    SetTecladoSharedVar(teclado);
    char teclaPulsada = teclado.teclaDetectada;

    if (teclaPulsada == TECLA_RESET) {
        piLock(SYSTEM_KEY);
        g_flagsCoreWatch |= FLAG_RESET;
        piUnlock(SYSTEM_KEY);
    } else if (teclaPulsada == TECLA_SET_CANCEL_TIME) {
        piLock(SYSTEM_KEY);
        g_flagsCoreWatch |= FLAG_SET_CANCEL_NEW_TIME;
        piUnlock(SYSTEM_KEY);
    } else if (teclaPulsada == TECLA_DIA_SEMANA) {
        ShowDay(&g_coreWatch.reloj.calendario, p_sistema->lcdId);
    } else if (teclaPulsada == TECLA_SET_CANCEL_CALENDAR) {
    	piLock(SYSTEM_KEY);
		g_flagsCoreWatch |= FLAG_SET_CANCEL_NEW_CALENDAR;
		piUnlock(SYSTEM_KEY);
    } else if (teclaPulsada == TECLA_SEE_PASSED_TIME) {
        	ShowTimePassed(p_sistema->lcdId);
    } else if ((teclaPulsada == '\n') || (teclaPulsada == '\r') || (teclaPulsada == 10)){
        // do nothing
    } else if (EsNumero(teclaPulsada)) {
        g_coreWatch.digitoPulsado = teclaPulsada - 48;
        piLock(KEYBOARD_KEY);
        g_flagsCoreWatch |= FLAG_DIGITO_PULSADO;
        piUnlock(KEYBOARD_KEY);
    } else if (teclaPulsada == TECLA_EXIT) {
        printf("Saliendo del sistema\n");
        fflush(stdout);
        piLock(STD_IO_LCD_BUFFER_KEY);
        lcdPosition(p_sistema->lcdId, 0, 1);
        lcdClear(p_sistema->lcdId);
        lcdPuts(p_sistema->lcdId, "EXIT");
        piUnlock(STD_IO_LCD_BUFFER_KEY);
        exit(0);
    } else {
        printf("Tecla desconocida\n");
        fflush(stdout);
    }
    
}
//------------------------------------------------------
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES
//------------------------------------------------------
// Mejora: Subrutina de atencion a la interrupcion del timeout
void timeout_isr(union sigval value) {
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch |= FLAG_TIMEOUT;
	piUnlock(SYSTEM_KEY);
}

// Subrutina de atencion a la interrupcion del temporizador que indica el tiempo transcurrido
void tmr_passed_time_isr(union sigval value) {
	passed_time++;
}

//------------------------------------------------------
// FUNCIONES LIGADAS A THREADS ADICIONALES
//------------------------------------------------------
// Funcion que lee las pulsaciones del teclado y las interpreta activando las flags del sistema
#if VERSION == 2
	PI_THREAD(ThreadExploraTecladoPC) {
		int teclaPulsada;
		while (1) {
			delay (10) ; // WiringPi function : pauses program execution for at least 10 ms
			if(kbhit ()) {
				teclaPulsada = kbread () ;	// Logica ( diagrama de flujo ):

				#if VERSION >= 3
					piLock(KEYBOARD_KEY);
					g_flagsCoreWatch &= ~FLAG_TECLA_PULSADA;
					piUnlock(KEYBOARD_KEY);
				#endif

				if (teclaPulsada == TECLA_RESET) {
					piLock(SYSTEM_KEY);
					g_flagsCoreWatch |= FLAG_RESET;
					piUnlock(SYSTEM_KEY);
				} else if (teclaPulsada == TECLA_SET_CANCEL_TIME) {
					piLock(SYSTEM_KEY);
					g_flagsCoreWatch |= FLAG_SET_CANCEL_NEW_TIME;
					piUnlock(SYSTEM_KEY);
				} else if (teclaPulsada == TECLA_DIA_SEMANA) {
							ShowDay(&g_coreWatch.reloj.calendario, 0);
				} else if (teclaPulsada == TECLA_SET_CANCEL_CALENDAR) {
						piLock(SYSTEM_KEY);
						g_flagsCoreWatch |= FLAG_SET_CANCEL_NEW_CALENDAR;
						piUnlock(SYSTEM_KEY);
				} else if (teclaPulsada == TECLA_SEE_PASSED_TIME) {
				        	ShowTimePassed(p_sistema->lcdId);
				} else if ((teclaPulsada == '\n') || (teclaPulsada == '\r') || (teclaPulsada == 10)){
					// do nothing
				} else if (EsNumero(teclaPulsada)) {
					g_coreWatch.digitoPulsado = teclaPulsada - 48;
					piLock(KEYBOARD_KEY);
					g_flagsCoreWatch |= FLAG_DIGITO_PULSADO;
					piUnlock(KEYBOARD_KEY);
				} else if (teclaPulsada == TECLA_EXIT) {
					printf("Saliendo del sistema\n");
					fflush(stdout);
					exit(0);
				} else {
					printf("Tecla desconocida\n");
					fflush(stdout);
				}
			}
		}
	}
#endif
//------------------------------------------------------
// MAIN
//------------------------------------------------------
int main() {
	unsigned int next;

#if VERSION == 1
	TipoReloj relojPrueba;
	ConfiguraInicializaReloj(&(relojPrueba));
	SetHora(1601, &(relojPrueba.hora));
#endif
#if VERSION >= 2
	ConfiguraInicializaSistema(&(g_coreWatch));
#endif

	printf("Hola, esta usando coreWatch. Para poder interactuar, utilice las siguientes teclas:\n E -> Cambiar hora\n F -> Reset\n D -> Mostrar dia de la semana\n B -> Salir\n");

	fsm_t* fsmReloj = fsm_new(WAIT_TIC, g_fsmTransReloj, &(g_coreWatch.reloj));
    fsm_t* fsmCoreWatch = fsm_new(START, fsmTransCoreWatch, &(g_coreWatch));
    fsm_t* fsmDeteccionComandos = fsm_new(WAIT_COMMAND, fsmTransDeteccionComandos, &(g_coreWatch));
    fsm_t* tecladoFSM = fsm_new(TECLADO_ESPERA_COLUMNA, g_fsmTransExcitacionColumnas, &(g_coreWatch.teclado));
    g_tmr_timeout = tmr_new(timeout_isr);
    tmr_t* g_tmr_passed_time = tmr_new(tmr_passed_time_isr); // Timer que almacenará el tiempo transcurrido
    tmr_startms_periodic(g_tmr_passed_time, PRECISION_RELOJ_MS);

    next = millis();
	while (1) {
		fsm_fire(fsmReloj);
        fsm_fire(fsmCoreWatch);
        fsm_fire(fsmDeteccionComandos);
        fsm_fire(tecladoFSM);
		next += CLK_MS;
		DelayUntil(next);
	}

	tmr_destroy(g_coreWatch.reloj.tmrTic);
	fsm_destroy(fsmReloj);
	fsm_destroy(fsmCoreWatch);
    fsm_destroy(fsmDeteccionComandos);
    fsm_destroy(tecladoFSM);
    tmr_destroy(g_coreWatch.teclado.tmr_duracion_columna);
    tmr_destroy(g_tmr_timeout);
    tmr_destroy(g_tmr_passed_time);
}
