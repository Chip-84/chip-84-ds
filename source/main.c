#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>

#include "chip8.h"

#define REG_DISPCNT_MAIN  (*(vu32*)0x04000000)
#define REG_DISPCNT_SUB   (*(vu32*)0x04001000)
#define MODE_FB0          (0x00020000)
#define VRAM_A            ((u16*)0x6800000)
#define VRAM_A_CR         (*(vu8*)0x04000240)
#define VRAM_ENABLE       (1<<7)
#define SCANLINECOUNTER   *(vu16 *)0x4000006

#define COLOR(r,g,b)  ((r) | (g)<<5 | (b)<<10)
#define OFFSET(r,c,w) ((r)*(w)+(c))

#define SCREENWIDTH  (256)
#define SCREENHEIGHT (192)

void setPixel(int x, int y, uint color);
void setChipPixel(int x, int y, uint color);
void waitForVblank();
void drawGraphics();
void drawMenu();
void update();
u8 isDirectory(char *dir, char *path);
void setKeys();

int selectedFile = 0;
int numFiles = 0;
int startPos = 0;

char fileNames[256][26];
char filePath[258];
char pastPath[258];

touchPosition touch;

int main(void) {
	
	consoleDemoInit();
	fatInitDefault();
	
	REG_DISPCNT_MAIN = MODE_FB0;
	VRAM_A_CR = VRAM_ENABLE;
	
	numFiles = getNumFiles("/");
	
	keysSetRepeat(20, 2);
	drawMenu();
	while(1) {
		scanKeys();
		
		swiWaitForVBlank();
		if(playing) {
			setKeys();
			emulateCycle(10);
			drawGraphics();
		}
		update();
	}
	
	return 0;
}

void update() {
	int held = keysDownRepeat();
	if(!playing) {
		if(held & KEY_DOWN) {
			selectedFile++;
			if(selectedFile > numFiles-1) selectedFile = numFiles-1;
			if(selectedFile == startPos+20) startPos++;
			
			drawMenu();
		}
		if(held & KEY_UP) {
			selectedFile--;
			if(selectedFile < 0) selectedFile = 0;
			if(selectedFile == startPos-1) startPos--;
			
			drawMenu();
		}
		if(held & KEY_A) {
			consoleClear();
			strcpy(pastPath, filePath);
			strcat(filePath, "/");
			strcat(filePath, fileNames[selectedFile]);
			if(isDirectory(pastPath, fileNames[selectedFile]) == 1) {
				numFiles = getNumFiles(filePath);
				selectedFile = 0;
				startPos = 0;
				drawMenu();
			} else {
				loadProgram(filePath);
				strcpy(filePath, pastPath);
				consoleClear();
				iprintf("\x1b[0;0H\n\n\n");
				iprintf("       -----------------\n");
				iprintf("       |   |   |   |   |\n");
				iprintf("       | 1 | 2 | 3 | C |\n");
				iprintf("       |   |   |   |   |\n");
				iprintf("       -----------------\n");
				iprintf("       |   |   |   |   |\n");
				iprintf("       | 4 | 5 | 6 | D |\n");
				iprintf("       |   |   |   |   |\n");
				iprintf("       -----------------\n");
				iprintf("       |   |   |   |   |\n");
				iprintf("       | 7 | 8 | 9 | E |\n");
				iprintf("       |   |   |   |   |\n");
				iprintf("       -----------------\n");
				iprintf("       |   |   |   |   |\n");
				iprintf("       | A | 0 | B | F |\n");
				iprintf("       |   |   |   |   |\n");
				iprintf("       -----------------\n");
			}
		}
	} else {
		if(held & KEY_START) {
			playing = false;
			drawMenu();
		}
	}
}

void drawMenu() {
	int i = 0;
	
	consoleClear();
	iprintf("\x1b[0;0HChoose a ROM:\n\n", filePath);
	//Choose a ROM:
	
	for(i = 0; i < 20; i++) {
		if(i+startPos == selectedFile) {
			iprintf(">  %s\n", fileNames[i+startPos]);
		} else {
			iprintf("   %s\n", fileNames[i+startPos]);
		}
	}
}

int getNumFiles(char *dir) {
	DIR *pdir;
	struct dirent *pent;
	pdir = opendir(dir);
	int num = 0;
	
	if (pdir) {
		u16 i;
		for(i = 0; i < 256; i++) {
			memset(fileNames[i], '\0', sizeof(char)*26);
		}
		while ((pent = readdir(pdir))!=NULL) {
			strcpy(fileNames[num], pent -> d_name);
			num++;
		}
	}
	closedir(pdir);
	return num;
}

u8 isDirectory(char *dir, char *path) {
	DIR *pdir;
	struct dirent *pent;
	pdir = opendir(dir);
	
	if (pdir) {
		while ((pent = readdir(pdir))!=NULL) {
			if((strcmp(path, pent -> d_name) == 0) && pent->d_type == DT_DIR) {
				closedir(pdir);
				return 1;
			}
		}
	}
	closedir(pdir);
	return 0;
}

u8 touchInbetween(int touchx, int touchy, int xmin, int xmax, int ymin, int ymax) {
	if(touchx >= xmin && touchx <= xmax && touchy >= ymin && touchy <= ymax) return 1;
	else return 0;
}

void setKeys() {
	int held = keysHeld();
	
	if(held & KEY_TOUCH) {
		touchRead(&touch);
	
		keys[0x1] = touchInbetween(touch.px, touch.py, 0x3a, 0x5b, 0x1d, 0x3b);
		keys[0x2] = touchInbetween(touch.px, touch.py, 0x5b, 0x7c, 0x1d, 0x3b);
		keys[0x3] = touchInbetween(touch.px, touch.py, 0x7b, 0x9d, 0x1d, 0x3b);
		keys[0xc] = touchInbetween(touch.px, touch.py, 0x9d, 0xbe, 0x1d, 0x3b);
		
		keys[0x4] = touchInbetween(touch.px, touch.py, 0x3a, 0x5b, 0x3c, 0x5b);
		keys[0x5] = touchInbetween(touch.px, touch.py, 0x5b, 0x7c, 0x3c, 0x5b);
		keys[0x6] = touchInbetween(touch.px, touch.py, 0x7b, 0x9d, 0x3c, 0x5b);
		keys[0xd] = touchInbetween(touch.px, touch.py, 0x9d, 0xbe, 0x3c, 0x5b);
		
		keys[0x7] = touchInbetween(touch.px, touch.py, 0x3a, 0x5b, 0x5c, 0x7b);
		keys[0x8] = touchInbetween(touch.px, touch.py, 0x5b, 0x7c, 0x5c, 0x7b);
		keys[0x9] = touchInbetween(touch.px, touch.py, 0x7b, 0x9d, 0x5c, 0x7b);
		keys[0xe] = touchInbetween(touch.px, touch.py, 0x9d, 0xbe, 0x5c, 0x7b);
		
		keys[0xa] = touchInbetween(touch.px, touch.py, 0x3a, 0x5b, 0x7b, 0x9b);
		keys[0x0] = touchInbetween(touch.px, touch.py, 0x5b, 0x7c, 0x7b, 0x9b);
		keys[0xb] = touchInbetween(touch.px, touch.py, 0x7b, 0x9d, 0x7b, 0x9b);
		keys[0xf] = touchInbetween(touch.px, touch.py, 0x9d, 0xbe, 0x7b, 0x9b);
	} else {
		u8 i;
		for(i = 0; i < 16; i++) {
			keys[i] = 0;
		}
	}
}

void drawGraphics() {
	u16 x;
	u8 y;
	uint colors[4] = {0x000000, 0x3fffff, 0x7fffff, 0xffffff};
	for(y = 0; y < screen_height; y++) {
		for(x = 0; x < screen_width; x++) {
			u8 colorIdx = canvas_data[0][y * screen_width + x] + (canvas_data[1][y * screen_width + x] << 1);
			setChipPixel(x, y, colors[colorIdx]);
		}
	}
}

void setChipPixel(int x, int y, uint color) {
	int times = 2*(1-extendedScreen)+2;
	int yp;
	int xp;
	for(yp = 0; yp < times; yp++) {
		for(xp = 0; xp < times; xp++) {
			setPixel(x*times+xp, y*times+yp+32, color);
		}
	}
}

void setPixel(int x, int y, uint color) {
	VRAM_A[OFFSET(y, x, SCREEN_WIDTH)] = color;
}

void waitForVblank() {
    while (SCANLINECOUNTER > SCREENHEIGHT);
    while (SCANLINECOUNTER < SCREENHEIGHT);
}