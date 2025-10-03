/*
 * reloj.c
 *
 *  Created on: 18 de feb. de 2022
 *      Author: Rodrigo Tamaki Moreno y Alejandro Fernandez Gonzalez
 */

// INCLUDES
#include "reloj.h"

//------------------------------------------------------
// DECLARACION Y DEFINICION VARIABLES GLOBALES
//------------------------------------------------------
// Definimos maquina de estados
fsm_trans_t g_fsmTransReloj[] = {
		{WAIT_TIC, CompruebaTic, WAIT_TIC, ActualizaReloj},
		{-1, NULL, -1, NULL}
};

// Definimos DIAS_MESES
const int DIAS_MESES[2][MAX_MONTH] = {{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};

// Declaramos g_relojSharedVars (static -> solo reloj.c)
static TipoRelojShared g_relojSharedVars;

//------------------------------------------------------
// FUNCIONES DE INICIALIZACION DE LAS VARIABLES
//------------------------------------------------------
// Funcion que inicializa el reloj a valores conocidos de fecha y hora
void ResetReloj(TipoReloj *p_reloj){
	TipoCalendario calendario = {DEFAULT_DAY, DEFAULT_MONTH, DEFAULT_YEAR};
	TipoHora hora = {DEFAULT_HOUR, DEFAULT_MIN, DEFAULT_SEC, TIME_FORMAT_24_H};
	p_reloj->calendario = calendario;
	p_reloj->hora = hora;
	p_reloj->timestamp = 0;
	piLock (RELOJ_KEY);
	g_relojSharedVars.flags = 0;
	piUnlock (RELOJ_KEY);
}

// Funcion que inicializa el reloj con ResetReloj, inicializa tmrTic y lanza temporizador modo periodico
int ConfiguraInicializaReloj(TipoReloj *p_reloj){
	ResetReloj(p_reloj);
	p_reloj->tmrTic = tmr_new(tmr_actualiza_reloj_isr);
	tmr_startms_periodic(p_reloj->tmrTic, PRECISION_RELOJ_MS);
	return 0;
}
//------------------------------------------------------
// FUNCIONES PROPIAS
//------------------------------------------------------
// Funcion que actualiza la fecha del reloj
void ActualizaFecha(TipoCalendario *p_fecha){
	int day = 0, month = 0;	
	day = CalculaDiasMes(p_fecha->MM, p_fecha->yyyy);
	day = (p_fecha->dd + 1) % (day+1);
	day = MAX(1, day);
	p_fecha->dd = day;	
	if (day == 1){
		month = (p_fecha->MM + 1) % (MAX_MONTH+1);
		month = MAX(1, month);
		p_fecha->MM = month;
		if (month == 1){
			p_fecha->yyyy = p_fecha->yyyy + 1;
		}
	}	
}

// Funcion que actualiza la hora del reloj
void ActualizaHora(TipoHora *p_hora){	
	p_hora->ss = (p_hora->ss + 1) % 60;
	if (p_hora->ss == 0 ) {
		p_hora->mm = (p_hora->mm + 1) % 60;
		if (p_hora->mm == 0){
			if(p_hora->formato == TIME_FORMAT_12_H){
				p_hora->hh = (p_hora->hh + 1) % (p_hora->formato + 1);
			}else{
				p_hora->hh = (p_hora->hh + 1) % p_hora->formato;
			}
		}
	} 
}

// Funcion que indica el numero de dias del mes
int CalculaDiasMes(int month, int year){
	int y = 0;
	y = Esbisiesto(year);
	return DIAS_MESES[y][month-1];
}

// Funcion que comprueba si un año es o no bisiesto
int Esbisiesto(int year){
	if(year%4 == 0){
		if(year%100 == 0){
			if(year%400 == 0){
				return 1;
			}
		}else{
			return 1;
		}
	}
	return 0;
}

int SetFormato(int nuevoFormato, TipoHora *p_hora) {

	if ((nuevoFormato != TIME_FORMAT_12_H) && (nuevoFormato != TIME_FORMAT_24_H)) {
		return 1;
	}

	p_hora->formato = nuevoFormato;
	return 0;
}

// Funcion que establece la hora haciendo una conversion de los datos recibidos
int SetHora(int horaInt , TipoHora * p_hora ){
    if (horaInt < 0)
    	return 2;

    int num_dig = 0, aux = horaInt, min = 0, hora = 0;
    do {
        aux /= 10;
        num_dig ++;
    } while (aux != 0);

    if(num_dig > 4)
    	return 3;

    switch (num_dig) {
        case 1:
            p_hora->hh = 0;
            p_hora->mm = horaInt;
            break;
        case 2:
			p_hora->hh = 0;
			p_hora->mm = horaInt;
			break;
        case 3:
            p_hora->hh = (int) horaInt/100;
            min = horaInt - ((int) horaInt/100)*100;
            p_hora->mm = min;
            break;
        case 4:
            min = horaInt - ((int) horaInt/100)*100;
            p_hora->mm = min;
            hora = (int) horaInt/100;
            if (p_hora->formato == TIME_FORMAT_24_H) {
                p_hora->hh = hora;
            } else {
                if (hora < 1) {
                    p_hora->hh = MAX_HOUR_12_H;
                } else if (hora > MAX_HOUR_12_H) {
                    p_hora->hh = hora - MAX_HOUR_12_H;
                } else {
                    p_hora->hh = hora;
                }
            }
            break;
    }

    if (p_hora->formato == TIME_FORMAT_24_H) {
        if (p_hora->hh > MAX_HOUR_24_H) {
            p_hora->hh = MAX_HOUR_24_H;
        }
    } else {
        if (p_hora->hh > MAX_HOUR_12_H) {
            p_hora->hh = MAX_HOUR_12_H;
        }
    }
    if (p_hora->mm > MAX_MIN) {
        p_hora->mm = MAX_MIN;
    }
    p_hora->ss = 0;

    return 0;
}

// Mejora: Funcion que establece la fecha haciendo una conversion de los datos recibidos
int SetFecha(int nuevaFecha, TipoCalendario *p_fecha) {
	if (nuevaFecha < 0)
	    	return 2;

	int num_dig = 0, aux = nuevaFecha, dia = 0, mes = 0, ano = 0;

	do {
		aux /= 10;
		num_dig ++;
	} while (aux != 0);

	if(num_dig > 8)
		return 3;

	switch (num_dig) {
		case 1:
			p_fecha->yyyy = 0;
			p_fecha->MM = 0;
			p_fecha->dd = nuevaFecha;
			break;
		case 2:
			p_fecha-> yyyy = 0;
			p_fecha->MM = 0;
			p_fecha->dd = nuevaFecha;
			break;
		case 3: 
			p_fecha->yyyy = 0;
			mes = nuevaFecha - ((int) nuevaFecha/10) * 10;
			p_fecha->MM = mes;
			dia = (int) nuevaFecha/10;
			p_fecha->dd = dia;
			break;
		case 4:
			p_fecha->yyyy = 0;
			mes = nuevaFecha - ((int) nuevaFecha/100) * 100;
			p_fecha->MM = mes;
			dia = (int) nuevaFecha/100;
			p_fecha->dd = dia;
			break;
		case 5:
			ano = nuevaFecha - ((int) nuevaFecha/10) * 10;
			p_fecha->yyyy = ano;
			mes = ((int) (nuevaFecha - ((int) nuevaFecha/1000) * 1000) / 10);
			p_fecha->MM = mes;
			dia = (int) nuevaFecha/1000;
			p_fecha->dd = dia;
			break;
		case 6:
			ano = nuevaFecha - ((int) nuevaFecha/100) * 100;
			p_fecha->yyyy = ano;
			mes = ((int) (nuevaFecha - ((int) nuevaFecha/10000) * 10000) / 100);
			p_fecha->MM = mes;
			dia = (int) nuevaFecha/10000;
			p_fecha->dd = dia;
			break;
		case 7:
			ano = nuevaFecha - ((int) nuevaFecha/1000) * 1000;
			p_fecha->yyyy = ano;
			mes = ((int) (nuevaFecha - ((int) nuevaFecha/100000) * 100000) / 1000);
			p_fecha->MM = mes;
			dia = (int) nuevaFecha/100000;
			p_fecha->dd = dia;
			break;
		case 8:
			ano = nuevaFecha - ((int) nuevaFecha/10000) * 10000;
			p_fecha->yyyy = ano;
			mes = ((int) (nuevaFecha - ((int) nuevaFecha/1000000) * 1000000) / 10000);
			p_fecha->MM = mes;
			dia = (int) nuevaFecha/1000000;
			p_fecha->dd = dia;
			break;
	}

	ano = p_fecha->yyyy;
	if (ano < MIN_YEAR) 
		p_fecha->yyyy = MIN_YEAR;
	if (ano > MAX_YEAR) 
		p_fecha->yyyy = MAX_YEAR;

	dia = CalculaDiasMes(p_fecha->MM, p_fecha->yyyy);
	
	if (dia < p_fecha->dd)
		p_fecha->dd = dia;

	return 0;
}

// GETTER
TipoRelojShared GetRelojSharedVar() {
    TipoRelojShared local;
    piLock(RELOJ_KEY);
    local.flags = 0;
    local.flags |= g_relojSharedVars.flags;
    piUnlock(RELOJ_KEY);
    return local;
}

// SETTER
void SetRelojSharedVar(TipoRelojShared value) {
    piLock(RELOJ_KEY);
    g_relojSharedVars.flags = value.flags;
    piUnlock(RELOJ_KEY);
}
//------------------------------------------------------
// FUNCIONES DE ENTRADA O DE TRANSICION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
// Funcion que comprueba que el bit 1 de las flags del reloj esta activo
int CompruebaTic(fsm_t *p_this) {
	int resultado = 0;
	piLock(RELOJ_KEY);
	resultado = g_relojSharedVars.flags & FLAG_ACTUALIZA_RELOJ;
	piUnlock(RELOJ_KEY);
	return resultado;
}
//------------------------------------------------------
// FUNCIONES DE SALIDA O DE ACCION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
// Funcion que actualiza hora y fecha mediante ActualizaHora y ActualizaFecha
void ActualizaReloj(fsm_t *p_this) {
	TipoReloj *p_miReloj = (TipoReloj*) (p_this->user_data);
	p_miReloj->timestamp++;
	ActualizaHora(&p_miReloj->hora);
	if ((p_miReloj->hora.hh == 0) && (p_miReloj->hora.mm == 0) && (p_miReloj->hora.ss == 0)){
		ActualizaFecha(&p_miReloj->calendario);
	}
	piLock(RELOJ_KEY);
	g_relojSharedVars.flags &= ~FLAG_ACTUALIZA_RELOJ;
	g_relojSharedVars.flags |= FLAG_TIME_ACTUALIZADO;
	piUnlock(RELOJ_KEY);

	#if VERSION == 1
		printf("Son las: %d:%d:%d del %d/%d/%d \n", p_miReloj->hora.hh, p_miReloj->hora.mm, p_miReloj->hora.ss, p_miReloj->calendario.dd, p_miReloj->calendario.MM, p_miReloj->calendario.yyyy);
		fflush(stdout);
	#endif
}
//------------------------------------------------------
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES
//------------------------------------------------------
// Subrutina de atencion a la interrupcion del temporizador periodico
void tmr_actualiza_reloj_isr(union sigval value) {
	piLock(RELOJ_KEY);
	g_relojSharedVars.flags |= FLAG_ACTUALIZA_RELOJ;
	piUnlock(RELOJ_KEY);
}
