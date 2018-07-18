#include <nds.h>
#include <stdio.h>
#include <stdlib.h>

extern u16 opcode;
extern u8 memory[0x10000];
extern u8 SV[8];
extern u8 V[16];
extern u16 I;
extern u16 pc;
extern int16 delay_timer;
extern int16 sound_timer;
extern u16 stack[16];
extern u8 sp;
extern u8 keys[16];
extern int8 drawFlag;

extern int8 paused;
extern int8 playing;
extern int8 extendedScreen;

extern u8 screen_width;
extern u8 screen_height;

extern u8 canvas_data[2][8192];

extern u8 game_data[65024];
extern u8 keypad[16];
extern u8 controlMap[16];

void loadFontset(void);
void initialize(void);
void loadProgram(char *fileName);
void loadHardProgram();
void emulateCycle(u8 steps);