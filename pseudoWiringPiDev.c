/* pseudoLCD.c
 *
 *  Created on: 30 de Nov. de 2021
 *      Author: josueportiz
 *      COMPATIBLE v4.0
 */

//------------------------------------------------------
// INCLUDES
#include "pseudoWiringPiDev.h"

//------------------------------------------------------
// DEFINES, ENUMS, CONSTANTES
/* Para mover el cursor por la terminal de Linux usamos secuencias de escape:
 * La secuencia de escape de:
 * - "Flecha arriba (mueve arriba)" es: "\033[A"
 * - "Flecha abajo (mueve abajo)" es: "\033[B"
 * - "Flecha derecha (mueve derecha)" es: "\033[C"
 * - "Flecha izquierda (mueve izquierda)" es: "\033[D"
 *
 * - Si se acompaña de un número, se mueve tantas veces como se indica:
 *  "\033[2A" = mueve arriba dos veces
 *
 *  Esta no es una solución estandarizada y, por lo tanto,
 *  el código no será independiente de la plataforma.
 *  */
#define BACKUP_NEWLINE_LCD "\033[A\033[12D"
#define BLACK_COLOR_CODE 30
#define GREEN_COLOR_CODE 32
#define LCD_COLOR GREEN_COLOR_CODE

#define GREEN_BOLD printf("\033[1;%dm", GREEN_COLOR_CODE)
#define BLACK_SLIM printf("\033[0;%dm", BLACK_COLOR_CODE)

//------------------------------------------------------
// DECLARACIÓN VARIABLES
// De lcd.c:
struct lcdDataStruct *lcds [MAX_LCDS];
static int lcdControl;
// Row offsets
static const int rowOff[4] = {0x00, 0x40, 0x14, 0x54};

// Del LDC:
static char pseudoLCDMatrix[LCD_NUM_ROWS][LCD_NUM_COLS] = {
		{'_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_'},
		{'_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_'},
};

// Color guide: https://www.theurbanpenguin.com/4184-2/
const int pseudoLCDMatrixColor[LCD_NUM_ROWS][LCD_NUM_COLS] = {
		{LCD_COLOR, LCD_COLOR, LCD_COLOR, LCD_COLOR, LCD_COLOR, LCD_COLOR, LCD_COLOR, LCD_COLOR, LCD_COLOR, LCD_COLOR, LCD_COLOR, LCD_COLOR},
		{LCD_COLOR, LCD_COLOR, LCD_COLOR, LCD_COLOR, LCD_COLOR, LCD_COLOR, LCD_COLOR, LCD_COLOR, LCD_COLOR, LCD_COLOR, LCD_COLOR, LCD_COLOR}
};

// Emulación:
static int EMULACION_LCD_STARTED = FALSE;
static int g_display_on_off_control = 0x00;
static int g_cursor_shift = 0x00;

/*
 * strobe:
 *	Toggle the strobe (Really the "E") pin to the device.
 *	According to the docs, data is latched on the falling edge.
 *********************************************************************************
 */
static void strobe (const struct lcdDataStruct *lcd){
	// Note timing changes for new version of delayMicroseconds ()
	// printf("[pseudoWiringPiDev][pseudoLCD][Strobe: set to %d]\n", 1); // josueportiz: remove "digitalWrite"s, keep delays
	delayMicroseconds(50);
	// printf("[pseudoWiringPiDev][pseudoLCD][Strobe: set to %d]\n", 0);
	delayMicroseconds(50);
}

/*
 * sentDataCmd:
 *	Send an data or command byte to the display.
 *********************************************************************************
 */
static void sendDataCmd (const struct lcdDataStruct *lcd, unsigned char data){
	if (lcd->bits == 4)	{
		strobe(lcd);
	}
	strobe(lcd);
}

/*
 * putCommand:
 *	Send a command byte to the display
 *********************************************************************************
 */
static void putCommand (const struct lcdDataStruct *lcd, unsigned char command){
	sendDataCmd(lcd, command);
	delay(2);
}

static void put4Command (const struct lcdDataStruct *lcd, unsigned char command){
	// josueportiz: commands and digital writes have been removed for emulation purposes.
	strobe(lcd);
}
/*
 *********************************************************************************
 * User Callable code below here
 *********************************************************************************
 */
/*
 * lcdHome: lcdClear:
 *	Home the cursor or clear the screen.
 *********************************************************************************
 */
void lcdHome (const int fd){
	struct lcdDataStruct *lcd = lcds[fd] ;

	putCommand(lcd, LCD_HOME);
	lcd->cx = lcd->cy = 0;
	lcdPosition(fd, 0, 0);
	delay(5);
}

void lcdClear(const int fd) {
	struct lcdDataStruct *lcd = lcds[fd];

	// Emulación de limpiar. Sustituye al comando original:
	putCommand(lcd, LCD_CLEAR);
	int i, j = 0;
	for (i = 0; i< lcd->rows; i++){
		for (j = 0; j< lcd->cols; j++){
			pseudoLCDMatrix[i][j] = '_';
		}
	}
	lcdPutchar(fd, ' '); // Fuerza que imprima la pseudoLCDMatrix

	if (!EMULACION_LCD_STARTED){
		piLock(STD_IO_LCD_BUFFER_KEY);
		printf("[pseudoWiringPiDev][pseudoLCD][Clear]\n");
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	}
	lcdHome(fd);
}

/*
 * lcdDisplay: lcdCursor: lcdCursorBlink:
 *	Turn the display, cursor, cursor blinking on/off
 *********************************************************************************
 */
void lcdDisplay(const int fd, int state){
	struct lcdDataStruct *lcd = lcds[fd];

	if (state){
		lcdControl |=  LCD_DISPLAY_CTRL;
		if (!EMULACION_LCD_STARTED) {
			piLock(STD_IO_LCD_BUFFER_KEY);
			printf("[pseudoWiringPiDev][pseudoLCD][Display on/off control: 0x%x][on/off: ON]\n", lcdControl);
			piUnlock(STD_IO_LCD_BUFFER_KEY);
		}

	} else {
		lcdControl &= ~LCD_DISPLAY_CTRL;
		if (!EMULACION_LCD_STARTED) {
			piLock(STD_IO_LCD_BUFFER_KEY);
			printf("[pseudoWiringPiDev][pseudoLCD][Display on/off control: 0x%x][on/off: OFF]\n", lcdControl);
			piUnlock(STD_IO_LCD_BUFFER_KEY);
		}
	}

	putCommand(lcd, LCD_CTRL | lcdControl);
	g_display_on_off_control |= (LCD_CTRL | lcdControl);
	if (!EMULACION_LCD_STARTED) {
		piLock(STD_IO_LCD_BUFFER_KEY);
		printf("[pseudoWiringPiDev][pseudoLCD][Display on/off control: 0x%x]\n", g_display_on_off_control);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	}
}

void lcdCursor (const int fd, int state){
	struct lcdDataStruct *lcd = lcds [fd];

	if (state){
		lcdControl |=  LCD_CURSOR_CTRL;
		if (!EMULACION_LCD_STARTED) {
			piLock(STD_IO_LCD_BUFFER_KEY);
			printf("[pseudoWiringPiDev][pseudoLCD][Display on/off control: 0x%x][control: ON]\n", lcdControl);
			piUnlock(STD_IO_LCD_BUFFER_KEY);
		}
	} else {
		lcdControl &= ~LCD_CURSOR_CTRL;
		if (!EMULACION_LCD_STARTED) {
			piLock(STD_IO_LCD_BUFFER_KEY);
			printf("[pseudoWiringPiDev][pseudoLCD][Display on/off control: 0x%x][control: OFF]\n", lcdControl);
			piUnlock(STD_IO_LCD_BUFFER_KEY);
		}
	}

	putCommand (lcd, LCD_CTRL | lcdControl);
	g_display_on_off_control |= (LCD_CTRL | lcdControl);
	if (!EMULACION_LCD_STARTED) {
		piLock(STD_IO_LCD_BUFFER_KEY);
		printf("[pseudoWiringPiDev][pseudoLCD][Display on/off control: 0x%x]\n", g_display_on_off_control);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	}
}

void lcdCursorBlink (const int fd, int state){
	struct lcdDataStruct *lcd = lcds [fd];

	if (state){
		lcdControl |=  LCD_BLINK_CTRL;
		if (!EMULACION_LCD_STARTED) {
			piLock(STD_IO_LCD_BUFFER_KEY);
			printf("[pseudoWiringPiDev][pseudoLCD][Display on/off control: 0x%x][blink: ON]\n", lcdControl);
			piUnlock(STD_IO_LCD_BUFFER_KEY);
		}
	} else {
		lcdControl &= ~LCD_BLINK_CTRL;
		if (!EMULACION_LCD_STARTED) {
			piLock(STD_IO_LCD_BUFFER_KEY);
			printf("[pseudoWiringPiDev][pseudoLCD][Display on/off control: 0x%x][blink: OFF]\n", lcdControl);
			piUnlock(STD_IO_LCD_BUFFER_KEY);
		}
	}

	putCommand (lcd, LCD_CTRL | lcdControl);
	g_display_on_off_control |= (LCD_CTRL | lcdControl);

	if (!EMULACION_LCD_STARTED) {
		piLock(STD_IO_LCD_BUFFER_KEY);
		printf("[pseudoWiringPiDev][pseudoLCD][Display on/off control: 0x%x]\n", g_display_on_off_control);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	}
}

/*
 * lcdSendCommand:
 *	Send any arbitary command to the display
 *********************************************************************************
 */
void lcdSendCommand (const int fd, unsigned char command){
	struct lcdDataStruct *lcd = lcds[fd];
	putCommand(lcd, command);
}


/*
 * lcdPosition:
 *	Update the position of the cursor on the display.
 *	Ignore invalid locations.
 *********************************************************************************
 */
void lcdPosition (const int fd, int x, int y){
	struct lcdDataStruct *lcd = lcds[fd] ;

	if ((x > lcd->cols) || (x < 0)){
		if (!EMULACION_LCD_STARTED) {
			piLock(STD_IO_LCD_BUFFER_KEY);
			printf("[pseudoWiringPiDev][pseudoLCD][ERROR!!!][Posición X=%d errónea. Debe estar entre %d y %d]\n", x, 0, lcd->cols-1);
			piUnlock(STD_IO_LCD_BUFFER_KEY);
		}
		return;
	}

	if ((y > lcd->rows) || (y < 0)){
		if (!EMULACION_LCD_STARTED) {
			piLock(STD_IO_LCD_BUFFER_KEY);
			printf("[pseudoWiringPiDev][pseudoLCD][ERROR!!!][Posición Y=%d errónea. Debe estar entre %d y %d]\n", y, 0, lcd->rows-1);
			piUnlock(STD_IO_LCD_BUFFER_KEY);
		}
		return;
	}

	putCommand(lcd, x+(LCD_DGRAM | rowOff[y]));

	lcd->cx = x;
	lcd->cy = y;

	if (!EMULACION_LCD_STARTED) {
		piLock(STD_IO_LCD_BUFFER_KEY);
		printf("[pseudoWiringPiDev][pseudoLCD][Posición: [%d, %d]]\n", x, y);
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	}
}


/*
 * lcdCharDef:
 *	Defines a new character in the CGRAM
 *********************************************************************************
 */
void lcdCharDef (const int fd, int index, unsigned char data[8]){
	struct lcdDataStruct *lcd = lcds[fd];
	int i;

	putCommand(lcd, LCD_CGRAM | ((index & 7) << 3));

	//digitalWrite (lcd->rsPin, 1);
	for (i = 0; i < 8; ++i)
		sendDataCmd(lcd, data[i]);
}


/*
 * lcdPutchar:
 *	Send a data byte to be displayed on the display. We implement a very
 *	simple terminal here - with line wrapping, but no scrolling. Yet.
 *********************************************************************************
 */
void lcdPutchar (const int fd, unsigned char data){
	struct lcdDataStruct *lcd = lcds[fd];
	int i, j;

	//digitalWrite(lcd->rsPin, 1);
	sendDataCmd(lcd, data);

	// josueportiz: replace blanks for "underscores"
	if (EMULACION_LCD_STARTED){
		if (data == ' '){
			data = '_';
		}
		pseudoLCDMatrix[lcd->cy][lcd->cx] = data;

		for(i=0; i<lcd->rows; i++){
			printf("%s", BACKUP_NEWLINE_LCD);
		}

		for (i = 0; i<lcd->rows; i++){
			GREEN_BOLD;
			for (j = 0; j< lcd->cols; j++){
				printf("%c", pseudoLCDMatrix[i][j]);
			}
			BLACK_SLIM;
			printf("\n");
		}

		fflush(stdout);

		if (++lcd->cx == lcd->cols)	{
			lcd->cx = 0;
			if (++lcd->cy == lcd->rows)
				lcd->cy = 0;

			putCommand(lcd, lcd->cx + (LCD_DGRAM | rowOff[lcd->cy]));
		}
	}
}


/*
 * lcdPuts:
 *	Send a string to be displayed on the display
 *********************************************************************************
 */
void lcdPuts (const int fd, const char *string){
	while (*string)
		lcdPutchar(fd, *string++) ;
}


/*
 * lcdPrintf:
 *	Printf to an LCD display
 *********************************************************************************
 */
void lcdPrintf (const int fd, const char *message, ...){
	va_list argp;
	char buffer[1024];

	va_start(argp, message);
	vsnprintf (buffer, 1023, message, argp);
	va_end(argp);

	lcdPuts(fd, buffer) ;
}


/*
 * lcdInit:
 *	Take a lot of parameters and initialise the LCD, and return a handle to
 *	that LCD, or -1 if any error.
 *********************************************************************************
 */
int lcdInit (const int rows, const int cols, const int bits,
		const int rs, const int strb,
		const int d0, const int d1, const int d2, const int d3, const int d4,
		const int d5, const int d6, const int d7) {
	static int initialised = 0;

	unsigned char func;
	int i, j;
	int lcdFd = -1;
	struct lcdDataStruct *lcd;

	if (initialised == 0) {
		if (!EMULACION_LCD_STARTED){
			piLock(STD_IO_LCD_BUFFER_KEY);
			printf("\n\n[pseudoWiringPiDev][pseudoLCD][Starting initialization sequence]\n");
			piUnlock(STD_IO_LCD_BUFFER_KEY);
		}
		initialised = 1;
		for (i = 0; i < MAX_LCDS; ++i)
			lcds[i] = NULL;
	}

	// Simple sanity checks
	if (! ((bits == 4) || (bits == 8)))
		return -1;

	if ((rows < 0) || (rows > 20))
		return -1;

	if ((cols < 0) || (cols > 20))
		return -1;

	// Create a new LCD:
	for (i = 0; i < MAX_LCDS; ++i){
		if (lcds[i] == NULL){
			lcdFd = i;
			break;
		}
	}

	if (lcdFd == -1)
		return -1;

	lcd = (struct lcdDataStruct *)malloc (sizeof (struct lcdDataStruct));
	if (lcd == NULL)
		return -1;

	lcd->rsPin = rs;
	lcd->strbPin = strb;
	lcd->bits = bits;
	lcd->rows = rows;
	lcd->cols = cols;
	lcd->cx = 0;
	lcd->cy = 0;

	lcd->dataPins[0] = d0;
	lcd->dataPins[1] = d1;
	lcd->dataPins[2] = d2;
	lcd->dataPins[3] = d3;
	lcd->dataPins[4] = d4;
	lcd->dataPins[5] = d5;
	lcd->dataPins[6] = d6;
	lcd->dataPins[7] = d7;

	lcds[lcdFd] = lcd;

	/* josueportiz: pseudoLCD: Los "digitalWrite"s no son necesarios.
	 * Se dejan comentarios para saber cómo va la configuración. Los "pinMode"
	 * se dejan a modo de comprobación de que se están configurando los pines */
	pinMode(lcd->rsPin, OUTPUT);
	pinMode(lcd->strbPin, OUTPUT);
	for (i = 0; i < bits; ++i){
		pinMode(lcd->dataPins[i], OUTPUT);
	}
	delay(35); // mS


	// 4-bit mode?
	//	OK. This is a PIG and it's not at all obvious from the documentation I had,
	//	so I guess some others have worked through either with better documentation
	//	or more trial and error... Anyway here goes:
	//
	//	It seems that the controller needs to see the FUNC command at least 3 times
	//	consecutively - in 8-bit mode. If you're only using 8-bit mode, then it appears
	//	that you can get away with one func-set, however I'd not rely on it...
	//
	//	So to set 4-bit mode, you need to send the commands one nibble at a time,
	//	the same three times, but send the command to set it into 8-bit mode those
	//	three times, then send a final 4th command to set it into 4-bit mode, and only
	//	then can you flip the switch for the rest of the library to work in 4-bit
	//	mode which sends the commands as 2 x 4-bit values.

	if (bits == 4){
		func = LCD_FUNC | LCD_FUNC_DL;			// Set 8-bit mode 3 times
		if (!EMULACION_LCD_STARTED){
			piLock(STD_IO_LCD_BUFFER_KEY);
			printf("[pseudoWiringPiDev][pseudoLCD][Function set: 0x%x][Data Length: 8 bits]\n", func);
			piUnlock(STD_IO_LCD_BUFFER_KEY);
		}
		int i = 0;
		for (i=0; i<3; i++){
			put4Command(lcd, func >> 4);
			delay(35); // josueportiz: keep the delays. "put4Command"s for information only
		}

		func = LCD_FUNC;					// 4th set: 4-bit mode
		if (!EMULACION_LCD_STARTED){
			piLock(STD_IO_LCD_BUFFER_KEY);
			printf("[pseudoWiringPiDev][pseudoLCD][Function set: 0x%x][Data Length: 4 bits]\n", func);
			piUnlock(STD_IO_LCD_BUFFER_KEY);
		}
		put4Command(lcd, func >> 4);
		delay (35);
		lcd->bits = 4;
	} else {
		func = LCD_FUNC | LCD_FUNC_DL;
		if (!EMULACION_LCD_STARTED){
			piLock(STD_IO_LCD_BUFFER_KEY);
			printf("[pseudoWiringPiDev][pseudoLCD][Function set: 0x%x][Data Length: 8 bits]\n", func);
			piUnlock(STD_IO_LCD_BUFFER_KEY);
		}
		int i = 0;
		for (i=0; i<3; i++){
			putCommand(lcd, func);
			delay(35); // josueportiz: keep the delays. "putCommand"s for information only
		}
	}

	if (lcd->rows > 1)	{
		func |= LCD_FUNC_N;
		if (!EMULACION_LCD_STARTED){
			piLock(STD_IO_LCD_BUFFER_KEY);
			printf("[pseudoWiringPiDev][pseudoLCD][Function set: 0x%x][Number of display lines: 2]\n", func);
			piUnlock(STD_IO_LCD_BUFFER_KEY);
		}
		putCommand(lcd, func);
		delay(35);

	} else {
		if (!EMULACION_LCD_STARTED){
			piLock(STD_IO_LCD_BUFFER_KEY);
			printf("[pseudoWiringPiDev][pseudoLCD][Function set: 0x%x][Number of display lines: 1]\n", func);
			piUnlock(STD_IO_LCD_BUFFER_KEY);
		}
	}

	// Rest of the initialisation sequence
	lcdDisplay(lcdFd, TRUE);
	lcdCursor(lcdFd, FALSE);
	lcdCursorBlink(lcdFd, FALSE);
	lcdClear(lcdFd);

	if (!EMULACION_LCD_STARTED){
		piLock(STD_IO_LCD_BUFFER_KEY);
		printf("[pseudoWiringPiDev][pseudoLCD][Entry mode set][Accompanies display shift]\n");
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	}
	putCommand(lcd, LCD_ENTRY | LCD_ENTRY_ID);

	if (!EMULACION_LCD_STARTED){
		piLock(STD_IO_LCD_BUFFER_KEY);
		printf("[pseudoWiringPiDev][pseudoLCD][Cursor or display shift][Shift to the right]\n");
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	}
	putCommand(lcd, LCD_CDSHIFT | LCD_CDSHIFT_RL);

	g_cursor_shift |= (LCD_CDSHIFT | LCD_CDSHIFT_RL);

	if (!EMULACION_LCD_STARTED){
		piLock(STD_IO_LCD_BUFFER_KEY);

		printf("\n\n[PSEUDO LCD]\n");

		GREEN_BOLD;
		for (i = 0; i< lcd->rows; i++){
			for (j = 0; j< lcd->cols; j++){
				printf("%c", pseudoLCDMatrix[i][j]);
			}
			printf("\n");
		}
		BLACK_SLIM; // Pone negro sin negrita
		piUnlock(STD_IO_LCD_BUFFER_KEY);
	}
	EMULACION_LCD_STARTED = TRUE;
	return lcdFd;
}
