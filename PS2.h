#ifndef _PS2_H_
#define _PS2_H_

/* Input mappings. These are all the predefined mappings. */
/** The necessary defines. These point to the bits that make up bytes 4 and 5 of the PS2 communication (LSB first). */
/** The values work off of bitshifting. "NC" as 16 will shove the data right off. */
#define SELECT     0
#define L3         1
#define R3         2
#define START      3
#define DPAD_UP    4
#define DPAD_RIGHT 5
#define DPAD_DOWN  6
#define DPAD_LEFT  7
#define L2         8
#define R2         9
#define L1         10
#define R1         11
#define TRIANGLE   12
#define CIRCLE     13
#define CROSS      14
#define SQUARE     15
#define NC         16

void PS2_Init(void);
void PS2_LoadData(void);
void PS2_Acknowledge(void);

#endif