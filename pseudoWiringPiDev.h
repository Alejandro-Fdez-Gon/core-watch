/*
 * Adaptation: josueportiz
 * On 01 Dec. 2021
 *
 * lcd.h:
 *	Text-based LCD driver.
 *	This is designed to drive the parallel interface LCD drivers
 *	based in the Hitachi HD44780U controller and compatables.
 *
 * Copyright (c) 2012 Gordon Henderson.
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */
#ifndef _PSEUDOLCD_H_
#define _PSEUDOLCD_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

// Incluímos la configuración de GPIOs de la entrenadora:
#include "ent2004cfConfig.h"
#include "pseudoWiringPi.h"



#define	MAX_LCDS	8

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_NUM_ROWS 2
#define LCD_NUM_COLS 12
#define LCD_ROW_1 1
#define LCD_ROW_2 2

// HD44780U Commands
#define	LCD_CLEAR	0x01
#define	LCD_HOME	0x02
#define	LCD_ENTRY	0x04
#define	LCD_CTRL	0x08
#define	LCD_CDSHIFT	0x10
#define	LCD_FUNC	0x20
#define	LCD_CGRAM	0x40
#define	LCD_DGRAM	0x80

// Bits in the entry register
#define	LCD_ENTRY_SH		0x01
#define	LCD_ENTRY_ID		0x02

// Bits in the control register
#define	LCD_BLINK_CTRL		0x01
#define	LCD_CURSOR_CTRL		0x02
#define	LCD_DISPLAY_CTRL	0x04

// Bits in the function register
#define	LCD_FUNC_F	0x04
#define	LCD_FUNC_N	0x08
#define	LCD_FUNC_DL	0x10

#define	LCD_CDSHIFT_RL	0x04

struct lcdDataStruct {
	int bits, rows, cols;
	int rsPin, strbPin;
	int dataPins [8];
	int cx, cy;
};


extern void lcdHome        (const int fd);
extern void lcdClear       (const int fd);
extern void lcdDisplay     (const int fd, int state);
extern void lcdCursor      (const int fd, int state);
extern void lcdCursorBlink (const int fd, int state);
extern void lcdSendCommand (const int fd, unsigned char command);
extern void lcdPosition    (const int fd, int x, int y);
extern void lcdCharDef     (const int fd, int index, unsigned char data [8]);
extern void lcdPutchar     (const int fd, unsigned char data);
extern void lcdPuts        (const int fd, const char *string);
extern void lcdPrintf      (const int fd, const char *message, ...);

extern int  lcdInit (const int rows, const int cols, const int bits,
		const int rs, const int strb,
		const int d0, const int d1, const int d2, const int d3, const int d4,
		const int d5, const int d6, const int d7);

#ifdef __cplusplus
}
#endif
#endif /* _PSEUDOLCD_H_ */
