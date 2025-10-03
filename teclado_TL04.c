#include "teclado_TL04.h"

const char tecladoTL04[NUM_FILAS_TECLADO][NUM_COLUMNAS_TECLADO] = {
		{'1', '2', '3', 'C'},
		{'4', '5', '6', 'D'},
		{'7', '8', '9', 'E'},
		{'A', '0', 'B', 'F'}
};

// Maquina de estados: lista de transiciones
// {EstadoOrigen, CondicionDeDisparo, EstadoFinal, AccionesSiTransicion }
fsm_trans_t g_fsmTransExcitacionColumnas[] = {
		{ TECLADO_ESPERA_COLUMNA, CompruebaTimeoutColumna, TECLADO_ESPERA_COLUMNA, TecladoExcitaColumna},
		{-1, NULL, -1, NULL },
};

static TipoTecladoShared g_tecladoSharedVars;

//------------------------------------------------------
// FUNCIONES DE INICIALIZACION DE LAS VARIABLES ESPECIFICAS
//------------------------------------------------------
void ConfiguraInicializaTeclado(TipoTeclado *p_teclado) {
	
	piLock(KEYBOARD_KEY);
	
	int i;
	g_tecladoSharedVars.flags = 0;
	for (i=0; i<NUM_FILAS_TECLADO; i++) {
		g_tecladoSharedVars.debounceTime[i] = 0;
	}
	g_tecladoSharedVars.columnaActual = 0;
	g_tecladoSharedVars.teclaDetectada = 'H';

	// Configuracion pines

	// Salidas
	pinMode (GPIO_KEYBOARD_COL_1, OUTPUT) ;
	pinMode (GPIO_KEYBOARD_COL_2, OUTPUT) ;
	pinMode (GPIO_KEYBOARD_COL_3, OUTPUT) ;
	pinMode (GPIO_KEYBOARD_COL_4, OUTPUT) ;

	// Entradas
	pinMode (GPIO_KEYBOARD_ROW_1,INPUT);
	pinMode (GPIO_KEYBOARD_ROW_2,INPUT);
	pinMode (GPIO_KEYBOARD_ROW_3,INPUT);
	pinMode (GPIO_KEYBOARD_ROW_4,INPUT);

	// Asignaci�n de valores de entrada
	pullUpDnControl(GPIO_KEYBOARD_ROW_1, PUD_DOWN);
	pullUpDnControl(GPIO_KEYBOARD_ROW_2, PUD_DOWN);
	pullUpDnControl(GPIO_KEYBOARD_ROW_3, PUD_DOWN);
	pullUpDnControl(GPIO_KEYBOARD_ROW_4, PUD_DOWN);

	// Asignaci�n de valores de salida
	digitalWrite(GPIO_KEYBOARD_COL_1, LOW);
	digitalWrite(GPIO_KEYBOARD_COL_2, LOW);
	digitalWrite(GPIO_KEYBOARD_COL_3, LOW);
	digitalWrite(GPIO_KEYBOARD_COL_4, LOW);

	// Configuraci�n de las interrupciones
	wiringPiISR(GPIO_KEYBOARD_ROW_1, INT_EDGE_RISING, teclado_fila_1_isr);
	wiringPiISR(GPIO_KEYBOARD_ROW_2, INT_EDGE_RISING, teclado_fila_2_isr);
	wiringPiISR(GPIO_KEYBOARD_ROW_3, INT_EDGE_RISING, teclado_fila_3_isr);
	wiringPiISR(GPIO_KEYBOARD_ROW_4, INT_EDGE_RISING, teclado_fila_4_isr);

	// Crear y asignar temporizador de excitacion de columnas
	tmr_t* timerColumna = tmr_new (timer_duracion_columna_isr);

	// Lanzar temporizador
	p_teclado->tmr_duracion_columna = timerColumna;
	tmr_startms(p_teclado->tmr_duracion_columna, TIMEOUT_COLUMNA_TECLADO_MS);

	piUnlock(KEYBOARD_KEY);
}

//------------------------------------------------------
// FUNCIONES PROPIAS
//------------------------------------------------------
/* Getter y setters de variables globales */
TipoTecladoShared GetTecladoSharedVar() {
	TipoTecladoShared result;
	piLock(KEYBOARD_KEY);
	result = g_tecladoSharedVars;
	piUnlock(KEYBOARD_KEY);
	return result;
}

void SetTecladoSharedVar(TipoTecladoShared value) {
	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars = value;
	piUnlock(KEYBOARD_KEY);
}

void ActualizaExcitacionTecladoGPIO(int columna) {
	// ATENCION: Evitar que este mas de una columna activa a la vez.

	piLock(KEYBOARD_KEY);

	switch(columna){
		case COLUMNA_1:
			digitalWrite(GPIO_KEYBOARD_COL_1, HIGH);
			digitalWrite(GPIO_KEYBOARD_COL_2, LOW);
			digitalWrite(GPIO_KEYBOARD_COL_3, LOW);
			digitalWrite(GPIO_KEYBOARD_COL_4, LOW);
			break;
		case COLUMNA_2:
			digitalWrite(GPIO_KEYBOARD_COL_1, LOW);
			digitalWrite(GPIO_KEYBOARD_COL_2, HIGH);
			digitalWrite(GPIO_KEYBOARD_COL_3, LOW);
			digitalWrite(GPIO_KEYBOARD_COL_4, LOW);
			break;
		case COLUMNA_3:
			digitalWrite(GPIO_KEYBOARD_COL_1, LOW);
			digitalWrite(GPIO_KEYBOARD_COL_2, LOW);
			digitalWrite(GPIO_KEYBOARD_COL_3, HIGH);
			digitalWrite(GPIO_KEYBOARD_COL_4, LOW);
			break;
		case COLUMNA_4:
			digitalWrite(GPIO_KEYBOARD_COL_1, LOW);
			digitalWrite(GPIO_KEYBOARD_COL_2, LOW);
			digitalWrite(GPIO_KEYBOARD_COL_3, LOW);
			digitalWrite(GPIO_KEYBOARD_COL_4, HIGH);
	}

	piUnlock(KEYBOARD_KEY);
}

//------------------------------------------------------
// FUNCIONES DE ENTRADA O DE TRANSICION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
int CompruebaTimeoutColumna(fsm_t* p_this) {
	int result = 0;
	piLock(KEYBOARD_KEY);
	result = g_tecladoSharedVars.flags & FLAG_TIMEOUT_COLUMNA_TECLADO;
	piUnlock(KEYBOARD_KEY);
	return result;
}

//------------------------------------------------------
// FUNCIONES DE SALIDA O DE ACCION DE LAS MAQUINAS DE ESTADOS
//------------------------------------------------------
void TecladoExcitaColumna(fsm_t* p_this) {
	TipoTeclado *p_teclado = (TipoTeclado*)(p_this->user_data);

	// 1. Actualizo que columna SE VA a excitar
	if (g_tecladoSharedVars.columnaActual == 3) {
		g_tecladoSharedVars.columnaActual = 0;
	} else {
		g_tecladoSharedVars	.columnaActual++;
	};

	// 2. Ha pasado el timer y es hora de excitar la siguiente columna:
	//    (i) Llamada a ActualizaExcitacionTecladoGPIO con columna A ACTIVAR como argumento
	ActualizaExcitacionTecladoGPIO(g_tecladoSharedVars.columnaActual);

	// 3. Actualizar la variable flags
	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars.flags &= ~FLAG_TIMEOUT_COLUMNA_TECLADO;
	piUnlock(KEYBOARD_KEY);

	// 4. Manejar el temporizador para que vuelva a avisarnos
	tmr_startms(p_teclado->tmr_duracion_columna, TIMEOUT_COLUMNA_TECLADO_MS);

}

//------------------------------------------------------
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES
//------------------------------------------------------
void teclado_fila_1_isr(void) {
	int now = millis();
	if (now < g_tecladoSharedVars.debounceTime[FILA_1]) return;

	g_tecladoSharedVars.teclaDetectada = tecladoTL04[FILA_1][g_tecladoSharedVars.columnaActual];

	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars.flags |= FLAG_TECLA_PULSADA;
	piUnlock(KEYBOARD_KEY);

	g_tecladoSharedVars.debounceTime[FILA_1] = now + DEBOUNCE_TIME_MS;
}

void teclado_fila_2_isr(void) {
	int now = millis();
	if (now < g_tecladoSharedVars.debounceTime[FILA_2]) return;

	g_tecladoSharedVars.teclaDetectada = tecladoTL04[FILA_2][g_tecladoSharedVars.columnaActual];

	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars.flags |= FLAG_TECLA_PULSADA;
	piUnlock(KEYBOARD_KEY);

	g_tecladoSharedVars.debounceTime[FILA_2] = now + DEBOUNCE_TIME_MS;


}

void teclado_fila_3_isr(void) {
	int now = millis();
	if (now < g_tecladoSharedVars.debounceTime[FILA_3]) return;

	g_tecladoSharedVars.teclaDetectada = tecladoTL04[FILA_3][g_tecladoSharedVars.columnaActual];

	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars.flags |= FLAG_TECLA_PULSADA;
	piUnlock(KEYBOARD_KEY);

	g_tecladoSharedVars.debounceTime[FILA_3] = now + DEBOUNCE_TIME_MS;
}

void teclado_fila_4_isr (void) {
	int now = millis();
	if (now < g_tecladoSharedVars.debounceTime[FILA_4]) return;

	g_tecladoSharedVars.teclaDetectada = tecladoTL04[FILA_4][g_tecladoSharedVars.columnaActual];

	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars.flags |= FLAG_TECLA_PULSADA;
	piUnlock(KEYBOARD_KEY);

	g_tecladoSharedVars.debounceTime[FILA_4] = now + DEBOUNCE_TIME_MS;
}

void timer_duracion_columna_isr(union sigval value) {
	piLock(KEYBOARD_KEY);
	g_tecladoSharedVars.flags |= FLAG_TIMEOUT_COLUMNA_TECLADO;
	piUnlock(KEYBOARD_KEY);
}
