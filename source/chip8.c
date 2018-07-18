#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>

u16 opcode = 0;
u8 memory[0x10000];
u8 SV[8];
u8 V[16];
u16 I = 0;
u16 pc = 0;
int16 delay_timer = 0;
int16 sound_timer = 0;
u16 stack[16];
u8 sp = 0;
u8 keys[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int8 drawFlag = 0;

int8 paused = 0;
int8 playing = 0;
int8 extendedScreen = 0;
u8 plane;

u8 game_data[65024];
u8 keypad[16];
u8 controlMap[16];

u8 canvas_data[2][8192];

u8 fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
    0x90, 0x90, 0xF0, 0x10, 0x10, //4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
    0xF0, 0x10, 0x20, 0x40, 0x40, //7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
    0xF0, 0x90, 0xF0, 0x90, 0x90, //A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
    0xF0, 0x80, 0x80, 0x80, 0xF0, //C
    0xE0, 0x90, 0x90, 0x90, 0xE0, //D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
    0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};
u16  fontset_ten[80] = {
	0xC67C, 0xDECE, 0xF6D6, 0xC6E6, 0x007C, // 0
	0x3010, 0x30F0, 0x3030, 0x3030, 0x00FC, // 1
	0xCC78, 0x0CCC, 0x3018, 0xCC60, 0x00FC, // 2
	0xCC78, 0x0C0C, 0x0C38, 0xCC0C, 0x0078, // 3
	0x1C0C, 0x6C3C, 0xFECC, 0x0C0C, 0x001E, // 4
	0xC0FC, 0xC0C0, 0x0CF8, 0xCC0C, 0x0078, // 5
	0x6038, 0xC0C0, 0xCCF8, 0xCCCC, 0x0078, // 6
	0xC6FE, 0x06C6, 0x180C, 0x3030, 0x0030, // 7
	0xCC78, 0xECCC, 0xDC78, 0xCCCC, 0x0078, // 8
	0xC67C, 0xC6C6, 0x0C7E, 0x3018, 0x0070, // 9
	0x7830, 0xCCCC, 0xFCCC, 0xCCCC, 0x00CC, // A
	0x66FC, 0x6666, 0x667C, 0x6666, 0x00FC, // B
	0x663C, 0xC0C6, 0xC0C0, 0x66C6, 0x003C, // C
	0x6CF8, 0x6666, 0x6666, 0x6C66, 0x00F8, // D
	0x62FE, 0x6460, 0x647C, 0x6260, 0x00FE, // E
	0x66FE, 0x6462, 0x647C, 0x6060, 0x00F0  // F
};

u8 step;
u16 pixel;
u16 indx;

u8 _y;
u8 _x;

u8 screen_width;
u8 screen_height;

void initialize() {
	
	opcode = I = sp = delay_timer = sound_timer = 0;
	pc = 0x200;
	
	extendedScreen = 0;
	screen_width = 64;
	screen_height = 32;
	plane = 1;

	memset(canvas_data[0], 0, 8192);
	memset(canvas_data[1], 0, 8192);
		
	memset(keys, 0, 16);
	memset(stack, 0, 16);
	memset(V, 0, 16);
	memset(SV, 0, 8);
	memset(memory, 0, 0x10000);
	
	memcpy(memory, fontset, 80);
	memcpy(memory + 80, fontset_ten, 80);
	
	srand(time(NULL));
}

void loadProgram(char *fileName) {
	int i;
	u16 romSize;
	FILE *file;
	
	playing = 1;
	paused = 0;
	
	file = fopen(fileName, "rb");
	fseek(file, 0, SEEK_END);
	romSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	fread(game_data, romSize, 1, file);
	fclose(file);
	
	initialize();
	
	for(i = 0; i < romSize; ++i) {
		memory[i + 512] = (u8)game_data[i];
	}
}



void emulateCycle(u8 steps) {
	
	for(step = 0; step < steps; ++step) {
		int i;
		u8 x;
		u8 y;
		opcode = (memory[pc] << 8) | memory[pc+1];
		x = (opcode & 0x0f00) >> 8;
		y = (opcode & 0x00f0) >> 4;
		
		pc += 2;
		
		switch(opcode & 0xf000) {
			case 0x0000: {
				switch(opcode & 0x00f0) {
					case 0x00c0: { //SCD
						u8 n = (opcode & 0x000f);
						u8 layer;
						for(layer = 0; layer < 2; layer++) {
							if((plane & (layer+1)) == 0) continue;
							for(i = screen_height-2; i >= 0; i--) {
								memcpy(canvas_data[layer] + (i+n)*screen_width, canvas_data[layer] + i*screen_width, screen_width);
								memset(canvas_data[layer] + i*screen_width, 0, screen_width);
							}
						}
						
						drawFlag = 1; 
						
						break;
					}
					case 0x00d0: { //SCU
						u8 n = (opcode & 0x000f);
						u8 layer;
						for(layer = 0; layer < 2; layer++) {
							if((plane & (layer+1)) == 0) continue;
							for(i = 0; i < screen_height-2; i--) {
								memcpy(canvas_data[layer] + (i+n)*screen_width, canvas_data[layer] + i*screen_width, screen_width);
								memset(canvas_data[layer] + i*screen_width, 0, screen_width);
							}
						}
						
						drawFlag = 1; 
						
						break;
					}
					break;
				}
				switch(opcode & 0x00ff) {
					case 0x00e0: {
						u8 layer;
						for(layer = 0; layer < 2; layer++) {
							if((plane & (layer+1)) == 0) continue;
							if(extendedScreen)
								memset(canvas_data[layer], 0, 8192);
							else
								memset(canvas_data[layer], 0, 2048);
						}
						drawFlag = 1;
						break;
					}
					case 0x00ee:
						pc = stack[(--sp)&0xf];
						break;
					case 0x00fb: { //SCR
						u8 layer;
						u8 *disp;
						for(layer = 0; layer < 2; layer++) {
							if((plane & (layer+1)) == 0) continue;
							disp = &canvas_data[layer];
							for(i = 0; i < screen_height; i++) {
								memmove(disp + 4, disp, screen_width - 4);
								memset(disp, 0, 4);
								disp += screen_width;
							}
						}
						break;
					}
					case 0x00fc: { //SCL
						u8 layer;
						u8 *disp;
						for(layer = 0; layer < 2; layer++) {
							if((plane & (layer+1)) == 0) continue;
							disp = &canvas_data[layer];
							for(i = 0; i < screen_height; i++) {
								memmove(disp, disp + 4, screen_width - 4);
								memset(disp + screen_width - 4, 0, 4);
								disp += screen_width;
							}
						}
						break;
					}
					case 0x00fd:
						playing = 0;
						//exit
						break;
					case 0x00fe:
						extendedScreen = 0;
						screen_width = 64;
						screen_height = 32;
						break;
					case 0x00ff:
						extendedScreen = 1;
						screen_width = 128;
						screen_height = 64;
						break;
					default:
						pc = (pc & 0x0fff);
						break;
				}
				break;
			}
			case 0x1000: {
				pc = (opcode & 0x0fff);
				break;
			}
			case 0x2000: {
				stack[sp++] = pc;
				pc = (opcode & 0x0fff);
				break;
			}
			case 0x3000: {
				if(V[x] == (opcode & 0x00ff))
					pc += 2;
				break;
			}
			case 0x4000: {
				if(V[x] != (opcode & 0x00ff))
					pc += 2;
				break;
			}
			case 0x5000: {
				u8 n = (opcode & 0x000f);
				if(n != 0) {
					u8 dist = abs(x - y);
					u8 z;
					if(n == 2) {
						if(x < y) {
							for(z = 0; z <= dist; z++)
								memory[I + z] = V[x + z];
						} else {
							for(z = 0; z <= dist; z++)
								memory[I + z] = V[x - z];
						}
					} else if(n == 3) {
						if(x < y) {
							for(z = 0; z <= dist; z++)
								V[x + z] = memory[I + z];
						} else {
							for(z = 0; z <= dist; z++)
								V[x - z] = memory[I + z];
						}
					}
				}
				if(V[x] == V[y])
					pc += 2;
				break;
			}
			case 0x6000: {
				V[x] = (opcode & 0x00ff);
				break;
			}
			case 0x7000: {
				V[x] = (V[x] + (opcode & 0x00ff)) & 0xff;
				break;
			}
			case 0x8000: {
				switch(opcode & 0x000f) {
					case 0x0000: {
						V[x]  = V[y];
						break;
					}
					case 0x0001: {
						V[x] |= V[y];
						break;
					}
					case 0x0002: {
						V[x] &= V[y];
						break;
					}
					case 0x0003: {
						V[x] ^= V[y];
						break;
					}
					case 0x0004: {
						V[0xf] = (V[x] + V[y] > 0xff);
						V[x] += V[y];
						V[x] &= 255;
						break;
					}
					case 0x0005: {
						V[0xf] = V[x] >= V[y];
						V[x] -= V[y];
						break;
					}
					case 0x0006: {
						V[0xf] = V[x] & 1;
						V[x] >>= 1;
						break;
					}
					case 0x0007: {
						V[0xf] = V[y] >= V[x];
						V[x] = V[y] - V[x];
						break;
					}
					case 0x000E: {
						V[0xf] = V[x] >> 7;
						V[x] <<= 1;
						break;
					}
					break;
				}
				break;
			}
			case 0x9000: {
				if(V[x] != V[y])
					pc += 2;
				break;
			}
			case 0xa000: {
				I = (opcode & 0x0fff);
				break;
			}
			case 0xb000: {
				pc = V[0] + (opcode & 0x0fff);
				break;
			}
			case 0xc000: {
				V[x] = (rand() & 0xff) & (opcode & 0x00FF);
				break;
			}
			case 0xd000: {
				u8 xd = V[x];
				u8 yd = V[y];
				u8 height = (opcode & 0x000f);
				u8 layer;
				u8 i = I;
				
				V[0xf] = 0;
				
				for(layer = 0; layer < 2; layer++) {
					u8 source;
					if((plane & (layer+1)) == 0) continue;
					if(extendedScreen) {
						//Extended screen DXY0
						u8 cols = 1;
						if(height == 0) {
							cols = 2;
							height = 16;
						}
						for(_y = 0; _y < height; ++_y) {
							pixel = memory[I + (cols*_y)];
							if(cols == 2) {
								pixel <<= 8;
								pixel |= memory[I + (_y << 1)+1];
							}
							for(_x = 0; _x < (cols << 3); ++_x) {
								if((pixel & (((cols == 2) ? 0x8000 : 0x80) >> _x)) != 0) {
									indx = (((xd + _x) & 0x7f) + (((yd + _y) & 0x3f) << 7));
									source = ((memory[I + (_y * 2)+(_x > 7 ? 1 : 0)] >> (7 - (_x % 8))) & 0x1) != 0;
									V[0xf] |= canvas_data[layer][indx] & 1;
									//if(!source) continue;
									if (canvas_data[layer][indx])
										canvas_data[layer][indx] = 0;
									else
										canvas_data[layer][indx] = 1;
								}
							}
						}
						i += 32;
					} else {
						//Normal screen DXYN
						if(height == 0) height = 16;
						for(_y = 0; _y < height; ++_y) {
							pixel = memory[I + _y];
							for(_x = 0; _x < 8; ++_x) {
								if((pixel & (0x80 >> _x)) != 0) {
									indx = (((xd + _x) & 0x3f) + (((yd + _y) & 0x1f) << 6));
									source = ((memory[I + _y] >> (7 - _x)) & 0x1) != 0;
									V[0xf] |= canvas_data[layer][indx] & 1;
									//if(!source) continue;
									if (canvas_data[layer][indx])
										canvas_data[layer][indx] = 0;
									else
										canvas_data[layer][indx] = 1;
								}
							}
						}
						i += height;
					}
				}
				
				/*u8 i = I;
				u8 layer;
				u8 source;
				u8 target;
				u8 len = memory[pc + 1] & 0x00f;
				u8 a, b;
				for(layer = 0; layer < 2; layer++) {
					if((plane & (layer+1)) == 0) continue;
					if(len == 0) {
						for(a = 0; a < 16; a++) {
							for(b = 0; b < 16; b++) {
								target = ((V[x] + b) % screen_width) + ((V[y] + a) % screen_height) * screen_width;
								source = ((memory[i + (a * 2) + (b > 7 ? 1 : 0)] >> (7 - (b % 8))) & 0x1) != 0;
								if(!source) continue;
								if(canvas_data[layer][target]) {
									canvas_data[layer][target] = 0;
									V[0xf] = 0x1;
								} else {
									canvas_data[layer][target] = 1;
								}
							}
						}
						i += 32;
					} else {
						for(a = 0; a < len; a++) {
							for(b = 0; b < 8; b++) {
								target = ((V[x] + b) % screen_width) + ((V[y] + a) % screen_height) * screen_width;
								source = ((memory[i + a] >> (7 - b)) & 0x1) != 0;
								if(!source) continue;
								if(canvas_data[layer][target]) {
									canvas_data[layer][target] = 0;
									V[0xf] = 0x1;
								} else {
									canvas_data[layer][target] = 1;
								}
							}
						}
						i += len;
					}
				}*/
				
				drawFlag = 1;
				
				break;
			}
			case 0xe000: {
				switch(opcode & 0x00ff) {
					case 0x009e: {
						if(keys[V[x]])
							pc += 2;
						break;
					}
					case 0x00a1: {
						if(!keys[V[x]])
							pc += 2;
						break;
					}
				}
				break;
			}
			case 0xf000: {
				switch(opcode & 0x00ff) {
					case 0x0000: {
						I = opcode & 0xffff;
						pc += 2;
						break;
					}
					case 0x0001: {
						plane = (x & 0x3);
						break;
					}
					case 0x0002: {
						//audio
						break;
					}
					case 0x0007: {
						V[x] = delay_timer;
						break;
					}
					case 0x000A: {
						bool key_pressed = false;
						pc -= 2;
						
						for(i = 0; i < 16; ++i) {
							if(keys[i]) {
								V[x] = i;
								pc += 2;
								key_pressed = true;
								break;
							}
						}
						
						if(!key_pressed)
							return;
					}
					case 0x0015: {
						delay_timer = V[x];
						break;
					}
					case 0x0018: {
						sound_timer = V[x];
						break;
					}
					case 0x001E: {
						I = (I + V[x]) & 0xffff;
						break;
					}
					case 0x0029: {
						I = (V[x] & 0xf) * 5;
						break;
					}
					case 0x0030: {
						I = (V[x] & 0xf) * 10 + 80;
						break;
					}
					case 0x0033: {
						memory[ I ] = V[x] / 100;
						memory[I+1] = (V[x] / 10) % 10;
						memory[I+2] = V[x] % 10;
						break;
					}
					case 0x0055: {
						for(i = 0; i <= x; ++i) {
							memory[I + i] = V[i];
						}
						break;
					}
					case 0x0065: {
						for(i = 0; i <= x; ++i) {
							V[i] = memory[I + i];
						}
						break;
					}
					case 0x0075: {
						if (x > 7) x = 7;
						for(i = 0; i <= x; ++i) {
							SV[i] = V[i];
						}
						break;
					}
					case 0x0085: {
						if (x > 7) x = 7;
						for(i = 0; i <= x; ++i) {
							V[i] = SV[i];
						}
						break;
					}
				}
				break;
			}
			default:
				break;
		}
	}
	if(sound_timer > 0) {
		--sound_timer;
	}
	if(delay_timer > 0) {
		--delay_timer;
	}
}
















