// Ada SerialTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started:
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

/*  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstdio>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(MACOSX) || defined(LINUX)
#include <sys/time.h>
#endif
#include <fcntl.h>
#include <math.h>
#include <errno.h>
//#include "Adafruit_NeoPixel.h"
#include "LED_Helper.h"
#include "Serial_Functions.h"

// function prototypes
void loop__serialSpeedTest(const PORTTYPE &port, int *size);
void loop__ASCIIDummyLoop(const PORTTYPE& port);
void loop__AnimNeuralDrop(const PORTTYPE& port, volatile bool* watchVar);
void loop__AnimColorWheelShift(const PORTTYPE& port, volatile bool* watchVar);
void printActiveCols(volatile  uint32_t* pixBuffer);
void printDetailedBufferStats(uint32_t* pixBuffer);
void helper_adjustCursor(volatile uint32_t* pixBuffer, int& selectedLED, int selectedCol, int adjustAmount);
void loop__InteractiveLEDControl(const PORTTYPE &port);
void loop_InteractiveAnimationDesign(const PORTTYPE& port);



char buffer[100000];
enum operationType { NONE, SERIALTEST_LOOP, LEDBUFFER_LOOP, ASCII_DUMMY_LOOP, LEDBUFFER_NEURAL_LOOP, INTERACTIVECOLUMN_LOOP, INTERACTIVE_ANIMATION_DESIGN, SET_PIMASKING, QUIT};

uint32_t completePixBufferSz;
volatile uint32_t* completePixBuffer;
volatile uint32_t* pixBuffer;
uint32_t bufferSz;
#include "Ada_Central_Cone.h"


int main(int argc, char** argv)
{
	PORTTYPE port;
	int size = 30000;
	
	// Input parsing and opening of ports:
	if (argc < 2) die("Usage: receive_test <comport>\n       receive_test <blocksize> <comport>\n");
	if (argc == 2) {
		port = open_port_and_set_baud_or_die(argv[1], BAUD);
		printf("port %s opened\n", argv[1]);
	}
	else {
		if (sscanf(argv[1], "%d", &size) != 1 ||
			size < 1 || size > sizeof(buffer)) {
			die("Usage: receive_test <blocksize> <comport>\n");
		}
		port = open_port_and_set_baud_or_die(argv[2], BAUD);
		printf("port %s opened\n", argv[2]);
	}

	createMap();

	completePixBufferSz = sizeof(uint32_t) * NUM_LEDStrips * LEDSPerStrip + sizeof(uint32_t) * 32;
	completePixBuffer = (uint32_t*)malloc(completePixBufferSz + sizeof(uint32_t));
	if (completePixBuffer == NULL)
		die("# ERROR: Unable to allocate complete pixel buffer\n# ERROR: Exiting\n");

	// Create a pointer to the actual frame buffer and ensure it's disabled
	pixBuffer = completePixBuffer + 32;
	memset((void*)pixBuffer, 0x00, sizeof(uint32_t) * NUM_LEDStrips * LEDSPerStrip);
	// Write the metadata to the beginning of the buffer
	memcpy((void*)completePixBuffer, TAG_FULLBUFFER_STRING, strlen(TAG_FULLBUFFER_STRING));
	bufferSz = (NUM_LEDStrips * LEDSPerStrip);
	memcpy((uint8_t*)completePixBuffer + strlen(TAG_FULLBUFFER_STRING), &bufferSz, sizeof(uint32_t));

	boolean looping = true;
	char lineBuffer[256];
	int arg1;
	int count;
	while (looping) {
		operationType command = NONE;
		printf("-----------------------------------------------------\n");
		printf(" curPi: %d\n", curPi);
		printf(" Select command:\n");
		printf(" 1 - Execute serial speed test (default)\n");
		printf(" 2 - Enter LED colorwheel animation loop\n");
		printf(" 3 - Send ASCII DUMMY buffer loop\n");
		printf(" 4 - Enter LED neural animation loop\n");
		printf(" 5 - Interactive LED column identification\n");
		printf(" 6 - Interactive animation design\n");
		printf(" 7 - Set Pi number for masking\n");
		printf(" 8 - Quit\n");
		printf(" Choice: ");
		char lineBuffer[256];
		fgets(lineBuffer, sizeof(lineBuffer), stdin);
		int count = sscanf(lineBuffer, "%d", &command);
		if (count == 0)
			command = NONE;
		switch (command) {
		case SERIALTEST_LOOP:
			loop__serialSpeedTest(port, &size);
			break;
		case LEDBUFFER_LOOP:
			printf("Executing LED Loop Buffer\n");
			loop__AnimColorWheelShift(port, NULL);
			break;
		case ASCII_DUMMY_LOOP:
			printf("Executing ASCII Dummy Loop\n");
			loop__ASCIIDummyLoop(port);
			break;
		case LEDBUFFER_NEURAL_LOOP:
			printf("Executing LED Loop Buffer neural animation\n");
			loop__AnimNeuralDrop(port, NULL);
			break;
		case INTERACTIVECOLUMN_LOOP:
			printf("Executing Interactive Column Test Loop\n");
			loop__InteractiveLEDControl(port);
			break;
		case INTERACTIVE_ANIMATION_DESIGN:
			printf("Interactive animation design\n");
			loop_InteractiveAnimationDesign(port);
			break;
		case SET_PIMASKING:
			fgets(lineBuffer, sizeof(lineBuffer), stdin);
			count = sscanf(lineBuffer, "%d", &arg1);
			if (count == 0) {
				printf("# ERROR: No pi found \n");
			}
			else {
				if (arg1 >= 1 && arg1 <= 3) {
					curPi = arg1;
					printf("# INFO: curPi set to %d\n", curPi);
				}
				else 
					printf("# ERROR: No pi found with that number\n");
			}
			break;
		case QUIT:
			looping = false;
		case NONE:
		default:
			break;
		}
	}
	close_port(port);
	return 0;
}


/***********************************************
 * Background Thread for updating frame buffer *
 ***********************************************/
typedef struct threadArgs {
	threadArgs() : exitThread(false), port(NULL), animFunc() {}
	bool exitThread;
	const PORTTYPE *port;
	void* animFunc;
}threadArgs;
threadArgs serialThreadArgs;

#ifdef _WIN32
DWORD WINAPI serialThread(LPVOID lpParam)
{
	volatile threadArgs* serialArgs = (threadArgs*)lpParam;
#else
void* serialThread(void* param) {
	volatile threadArgs* serialArgs = (threadArgs*)param;
#endif
	printf("# INFO: Thread started\n");
	while (!serialArgs->exitThread) {
		// Send buffer (pre-amble + frame buffer)
		//printf(" Wrote: %d\n", 
		maskCenterCone();
		transmit_bytes(*(serialArgs->port), (char*)completePixBuffer, completePixBufferSz);
		// recvData(*(serialArgs->port));
		// Transmit at approximately 30FPS:
		delay_ms(25);
	}

	printf("Thread exiting\n");
	return 0;
}

threadArgs animThreadArgs;
#ifdef _WIN32
DWORD WINAPI animationThread(LPVOID lpParam)
{
	volatile threadArgs* animThreadArgs = (threadArgs*)lpParam;
#else
void* animationThread(void* param) {
	volatile threadArgs* animThreadArgs = (threadArgs*)param;
#endif
	printf("# INFO: Thread started\n");
	while (!animThreadArgs->exitThread) {
		if (animThreadArgs->animFunc == NULL) {
			maskCenterCone();
			loop__AnimNeuralDrop(*(animThreadArgs->port), &(animThreadArgs->exitThread));
		}
	}

	printf("Thread exiting\n");
	return 0;
}


/***********************************************
 * Frame Buffer Stats                          *
 ***********************************************/
void printActiveCols(volatile  uint32_t* pixBuffer)
{
	bool active[NUM_LEDStrips] = { false };
	for (int curLED = 0; curLED < LEDSPerStrip; curLED++) {
		for (int curCol = 0; curCol < NUM_LEDStrips; curCol++) {
			if (*(pixBuffer + curLED * NUM_LEDStrips + curCol) > 0)
				active[curCol] = true;
		}
	}
	printf(" Active Columns:\n");
	for (int curCol = 0; curCol < NUM_LEDStrips; curCol++) {
		printf("  %02d - %s\n", curCol, active[curCol] ? "Active" : "Inactive");
	}
}

void printDetailedBufferStats(volatile uint32_t* pixBuffer)
{
	double current[NUM_LEDStrips] = { 0.0 };
	double average[NUM_LEDStrips][3] = { 0.0 };
	int averageVal[NUM_LEDStrips][3] = { 0 };
	int averageCount[NUM_LEDStrips][3] = { 0 };
	uint8_t curR, curG, curB;
	for (int curLED = 0; curLED < LEDSPerStrip; curLED++) {
		for (int curCol = 0; curCol < NUM_LEDStrips; curCol++) {
			int pixelValue = *(pixBuffer + curLED * NUM_LEDStrips + curCol);
			current[curCol] += 2; // Idle current
			if (pixelValue > 0) {
				curB = pixelValue & 0xFF;
				curG = (pixelValue & 0xFF0000) >> 16;
				curR = (pixelValue & 0xFF00) >> 8;
				if (curR > 0) {
					averageVal[curCol][0] += curR;
					averageCount[curCol][0]++;
					current[curCol] += 3 * log(1.0 + curR) / log(1.0 + 255);
				}
				if (curG > 0) {
					averageVal[curCol][1] += curG;
					averageCount[curCol][1]++;
					current[curCol] += 3 * log(1.0 + curG) / log(1.0 + 255);
				}
				if (curB > 0) {
					averageVal[curCol][2] += curB;
					averageCount[curCol][2]++;
					current[curCol] += 3 * log(1.0 + curB) / log(1.0 + 255);
				}
			}
		}
	}
	// Compute actual averages:
	for (int curCol = 0; curCol < NUM_LEDStrips; curCol++) {
		average[curCol][0] = (static_cast<double>(averageVal[curCol][0])) / (1e-10 + averageCount[curCol][0]);
		average[curCol][1] = (static_cast<double>(averageVal[curCol][1])) / (1e-10 + averageCount[curCol][1]);
		average[curCol][2] = (static_cast<double>(averageVal[curCol][2])) / (1e-10 + averageCount[curCol][2]);
	}
	printf(" %10s %10s [%7s %7s %7s]\n", "LED Column", "Current", "AverageR", "AverageG", "AverageB");
	for (int curCol = 0; curCol < NUM_LEDStrips; curCol++) {
		printf("%7d %7.1f mA [%7.2f %7.2f %7.2f]\n", curCol, current[curCol], average[curCol][0], average[curCol][1], average[curCol][2]);
	}
}

void helper_adjustCursor(volatile uint32_t* pixBuffer, int& selectedLED, int selectedCol, int adjustAmount)
{
	*(pixBuffer + selectedLED * NUM_LEDStrips + selectedCol) = 0;
	selectedLED += adjustAmount;
	if (selectedLED > NUM_LEDStrips)
		selectedLED = NUM_LEDStrips;
	if (selectedLED < 0)
		selectedLED = 0;

	*(pixBuffer + selectedLED * NUM_LEDStrips + selectedCol) = 0xFFFFFF;
}


/***********************************************
 * Interactive Mode for Finding Fiber LEDs     *
 ***********************************************/
void CentralConeSetup(volatile uint32_t* pixBuffer)
{
        char lineBuffer[256];
        char command;
        boolean looping = true;
        int selectedCol = 0;
        int selectedLED = 0;
        uint8_t R = 10, G = 20, B = 25;
        int arg1, arg2, arg3;
        uint32_t ledValue;
        while (looping) {
                printf("Current output column: col: %d, LED: %d\n", selectedCol, selectedLED);
                printf(" [I] - Isolate buffer to only active column (Red/green)\n");
                printf(" [E] - Isolate buffer to only active column (all off except active)\n");
                printf(" [S] N - Select new output column\n");
				printf(" [K] N - Set selected LED to N\n");
                printf(" [Y] - Next + 10\n");
                printf(" [U] - Next +  5\n");
                printf(" [H] - Prev - 10\n");
                printf(" [J] - Prev -  5\n");
                printf(" [N] - Next\n");
                printf(" [M] - Keep on, move to next\n");
                printf(" [R] - Print active LEDs in this column\n");
                printf(" [C] R, G, B - Set apply buffer to R, G, B value\n");
                printf(" [F] GGRRBB - Apply specific buffer (HEX)\n");
                printf(" [V] - Paint active column to R, G, B value\n");
                printf("Apply Buffer: R %3d, G %3d, B %3d\n", R, G, B);
                printf(" [Q] - Quit\n");
                fgets(lineBuffer, sizeof(lineBuffer), stdin);
                int count = sscanf(lineBuffer, "%c %d %d %d", &command, &arg1, &arg2, &arg3);
                if (count == 0)
                        command = NULL;
                printf("\n");
		switch (command) {
		case 'f':
		case 'F':
			count = sscanf(lineBuffer, "%c %x", &command, &ledValue);
			if( count != 2 ){
					printf("Specify value in valid hex number\n" );
			}
			else {
                        for (int curLED = 0; curLED < LEDSPerStrip; curLED++) {
                                pixBufferAccess(curLED, selectedCol) = ledValue;
                        }
			}
		break;
		case 'i':
		case 'I':
			printf(" Isolating only to active column: %d\n", selectedCol);
			for (int curLED = 0; curLED < LEDSPerStrip; curLED++) {
				for (int curCol = 0; curCol < NUM_LEDStrips; curCol++) {
					if( selectedCol == curCol )
						pixBufferAccess(curLED, curCol) = 0x100000;
					else
						pixBufferAccess(curLED, curCol) = 0x1000;
				}
			}
			break;
		case 'e':
		case 'E':
			printf(" Isolating only to active column: %d\n", selectedCol);
			for (int curLED = 0; curLED < LEDSPerStrip; curLED++) {
				for (int curCol = 0; curCol < NUM_LEDStrips; curCol++) {
					if (selectedCol == curCol)
						pixBufferAccess(curLED, curCol) = 0x100000;
					else
						pixBufferAccess(curLED, curCol) = 0;
				}
			}
			break;
		case 's':
		case 'S':
			if (count == 2) {
				selectedCol = arg1;
				if (selectedCol > 15 || selectedCol < 0) {
					printf("Columns must be between 0 - 15\n");
					break;
				}
				printf(" Column option: %d\n", selectedCol);
				for (int curLED = 0; curLED < LEDSPerStrip; curLED++) {
					*(pixBuffer + curLED * NUM_LEDStrips + selectedCol) = 0;
				}
				selectedLED = 0;
			}
			else
				printf("No column found\n");
			break;
		case 'k':
		case 'K':
			if (count == 2) {
				helper_adjustCursor(pixBuffer, selectedLED, selectedCol, arg1-selectedLED);
			}
			else
				printf("No LED index found\n");
			break;
		case 'y':
		case 'Y':
			helper_adjustCursor(pixBuffer, selectedLED, selectedCol, 10);
			break;
		case 'h':
		case 'H':
			helper_adjustCursor(pixBuffer, selectedLED, selectedCol, -10);
			break;
		case 'u':
		case 'U':
			helper_adjustCursor(pixBuffer, selectedLED, selectedCol, 5);
			break;
		case 'j':
		case 'J':
			helper_adjustCursor(pixBuffer, selectedLED, selectedCol, -5);
			break;
		case 'n':
		case 'N':
			helper_adjustCursor(pixBuffer, selectedLED, selectedCol, 1);
			break;
		case 'm':
		case 'M':
			selectedLED++;
			if (selectedLED > LEDSPerStrip)
				selectedLED--;
			*(pixBuffer + selectedLED * NUM_LEDStrips + selectedCol) = 0xFFFFFF;
			break;
		case 'r':
		case 'R':
			for (int scanLED = 0; scanLED < LEDSPerStrip; scanLED++) {
				int pixValue = *(pixBuffer + scanLED * NUM_LEDStrips + selectedCol);
				if (pixValue > 0) {
					printf(" %3d, ", scanLED);
				}
			}
			printf("\n");
			break;
		case 'c':
		case 'C':
			if (count == 4) {
				if (arg1 > 255 || arg1 < 0 || arg2 > 255 || arg2 < 0 || arg3 > 255 || arg3 < 0)
					printf(" Values must be between 0 - 255\n");
				else {
					R = arg1;
					G = arg2;
					B = arg3;
				}
			}
			else {
				printf("All colors must be specified\n");
			}
			break;
		case 'v':
		case 'V':
			for (int curLED = 0; curLED < LEDSPerStrip; curLED++) {
				pixBufferAccess(curLED, selectedCol) =
					((((uint32_t)G) << 16) & 0xFF0000) +
					((((uint32_t)R) <<  8) & 0x00FF00) +
					((((uint32_t)B) <<  0) & 0x0000FF);
			}
			break;
		case 'q':
		case 'Q':
			looping = false;
			return;
			break;

		default:
			break;
		}
	}
}

/***********************************************
 * Interactive Mode for Controlling LEDs       *
 ***********************************************/
void loop__InteractiveLEDControl(const PORTTYPE &port){

    pthread_t sendingThread;
	pthread_attr_t attr;

	bool sendThreadRunning = false;
    bool allOutputsDisabled = true;
    uint8_t R = 25, G = 20, B= 25;
	int arg1 = 0, arg2 = 0, arg3 = 0;
	int col = 0;
	int result = 0;

	while (true) {
		printf("------------------------------------------------\n");
		printf(" Serial Thread is [%s]\n", sendThreadRunning ? "RUNNING" : "NOT Running");
		printf("Select option:\n");
		printf("  [Q] - Quit\n");
		printf("  [Z] - %s all outputs\n", allOutputsDisabled ? "Enable" : "Disable");
		printf("  [+] N - Enable Output N [0-15]\n");
		printf("  [-] N - Disable Output N [0-15]\n");
		printf("  [K] - %s center cone color\n", changeCenterCone ? "Enable" : "Disable" );
		printf("  [M] - Apply center cone color: 0x%x\n", centerConeColor );
		printf("  [A] - Print active outptus\n");
		printf("  [S] - Print status of all buffers\n");
		printf("  [T] - %s LED update thread\n", sendThreadRunning ? "Disable" : "Enable");
		printf("  [C] R, G, B - Set apply buffer to R, G, B value\n");
		printf("  [B] N - Set spply buffer brightness to N (0-255)\n");
		printf("  [L] - Configure cone LEDs\n");
		printf("Apply Buffer: R %3d, G %3d, B %3d\n", R, G, B);
		char command, lineBuffer[256];
		fgets(lineBuffer, sizeof(lineBuffer), stdin);
		int count = sscanf(lineBuffer, "%c %d %d %d", &command, &arg1, &arg2, &arg3);
		if (count == 0)
			command = NULL;
		printf("\n");
		switch (command) {
case 'k':
case 'K':
	if( changeCenterCone == true ) 
		changeCenterCone = false;
	else
		changeCenterCone = true;
break;
case 'm':
case 'M':
	centerConeColor = 
						    ((((uint32_t)G) << 16) & 0xFF0000) +
                                                        ((((uint32_t)R) <<  8) & 0x00FF00) +
                                                        ((((uint32_t)B) <<  0) & 0x0000FF);
break;

			case 'q':
			case 'Q':
				if (sendThreadRunning) {
					printf("Closing thread\n");
					sendThreadRunning = false;
					serialThreadArgs.exitThread = true;
				}
				return;
				break;
			case 'z':
			case 'Z':
				if (allOutputsDisabled) {
					printf("Outputs have been enabled\n");
					for (int curLED = 0; curLED < LEDSPerStrip; curLED++) {
						for (int curCol = 0; curCol < NUM_LEDStrips; curCol++) {
							pixBufferAccess(curLED, curCol) =
								((((uint32_t)G) << 16) & 0xFF0000) +
								((((uint32_t)R) <<  8) & 0x00FF00) +
								((((uint32_t)B) <<  0) & 0x0000FF);
						}
					}
					allOutputsDisabled = false;
				}
				else {
					printf("Outputs have been disabled\n");
					memset((void*)pixBuffer, 0x00, sizeof(uint32_t) * NUM_LEDStrips * LEDSPerStrip);
					allOutputsDisabled = true;
				}
				break;
			case '+':
				if (count == 2) {
					col = arg1;
					if (col > 15 || col < 0) {
						printf("Columns must be between 0 - 15\n");
						break;
					}
					printf(" Column option: %d\n", col);
					for (int curLED = 0; curLED < LEDSPerStrip; curLED++) {
						pixBufferAccess(curLED, col) =
							((((uint32_t)G) << 16) & 0xFF0000) +
							((((uint32_t)R) <<  8) & 0x00FF00) +
							((((uint32_t)B) <<  0) & 0x0000FF);
					}
					printActiveCols(pixBuffer);
				}
				else
					printf("No column found\n");
				break;
			case '-':
				if (count == 2) {
					col = arg1;
					if (col > 15 || col < 0) {
						printf("Columns must be between 0 - 15\n");
						break;
					}
					printf(" Column option: %d\n", col);
					for (int curLED = 0; curLED < LEDSPerStrip; curLED++) {
						pixBufferAccess(curLED, col) = 0x00;
					}
					printActiveCols(pixBuffer);
				}
				else
					printf("No column found\n");
				break;
			case 'a':
			case 'A':
				printActiveCols(pixBuffer);
				break;
			case 's':
			case 'S':
				printDetailedBufferStats(pixBuffer);
				break;
			case 't':
			case 'T':
				if (!sendThreadRunning) {
					serialThreadArgs.port = &port;
					serialThreadArgs.exitThread = false;
#if defined(MACOSX) || defined(LINUX)
					result = pthread_attr_init(&attr);
#endif
					printf("# INFO: Thread attr init result: %d\n", result );
					result = pthread_create(&sendingThread, &attr, serialThread, &serialThreadArgs);
					printf("# INFO: Thread create result: %d\n", result );
					sendThreadRunning = true;
				}
				else {
					serialThreadArgs.exitThread = true;
					sendThreadRunning = false;
				}
				break;
			case 'c':
			case 'C':
				if (count == 4) {
					if (arg1 > 255 || arg1 < 0 || arg2 > 255 || arg2 < 0 || arg3 > 255 || arg3 < 0)
						printf(" Values must be between 0 - 255\n");
					else {
						R = arg1;
						G = arg2;
						B = arg3;
					}
				}
				else {
					printf("All colors must be specified\n");
				}
				break;
			case 'b':
			case 'B':
				if (count == 2) {
					if (arg1 > 255 || arg1 < 0 )
						printf(" Values must be between 0 - 255\n");
					else {
						printf(" Applying brightness factor of %5.2f%%\n", arg1 / 255.0 * 100.0);
						R = (uint8_t) (R * (arg1 / 255.0));
						G = (uint8_t) (G * (arg1 / 255.0));
						B = (uint8_t) (B * (arg1 / 255.0));
					}
				}
				else {
					printf("Specify a brightness value [0-255]\n");
				}
				break;
			case 'l':
			case 'L':
				CentralConeSetup(pixBuffer);
				break;

			default:
				break;
		}
		printf("\n\n");

	}
}


/***********************************************
 * Interactive Mode for Designing Animations   *
 ***********************************************/
uint32_t anim_ledValue = 0x5b638b;
uint32_t anim_stepSpeed = 1;
float breathing_time = 3e3;
void loop_InteractiveAnimationDesign(const PORTTYPE& port) {

	pthread_t animThread;
	pthread_attr_t attr;

	bool animationThreadRunning = false;
	bool allOutputsDisabled = true;
	uint8_t R = 25, G = 20, B = 25;
	int arg1 = 0, arg2 = 0, arg3 = 0;
	int col = 0;
	int result = 0;
	float timeConst = 0;

	while (true) {
		printf("------------------------------------------------\n");
		printf(" Animation Thread is [%s]\n", animationThreadRunning ? "RUNNING" : "NOT Running");
		printf("Select option:\n");
		printf("  [Q] - Quit\n");
		printf("  [T] - %s LED update thread\n", animationThreadRunning ? "Disable" : "Enable");
		printf("  [S] - Print status of all buffers\n");
		printf("  [C] R, G, B - Set apply buffer to R, G, B value\n");
		printf("  [B] N - Set spply buffer brightness to N (0-255)\n");
		printf("  [W] - Change background of animation to apply buffer\n");
		printf("  [E] N.0 - Change breathing time constant (default 3000)\n");
		printf("  [R] N - Change animation step (default: 1)\n");
		printf("Apply Buffer: R %3d, G %3d, B %3d\n", R, G, B);
		char command, lineBuffer[256];
		fgets(lineBuffer, sizeof(lineBuffer), stdin);
		int count = sscanf(lineBuffer, "%c %d %d %d", &command, &arg1, &arg2, &arg3);
		if (count == 0)
			command = NULL;
		printf("\n");
		switch (command) {
		case 'q':
		case 'Q':
			if (animationThreadRunning) {
				printf("Closing thread\n");
				animationThreadRunning = false;
				animThreadArgs.exitThread = true;
			}
			return;
			break;
		case 'w':
		case 'W':
			anim_ledValue =
				((((uint32_t)G) << 16) & 0xFF0000) +
				((((uint32_t)R) << 8) & 0x00FF00) +
				((((uint32_t)B) << 0) & 0x0000FF);
			printf("# INFO: Changing background color to 0x%x\n", anim_ledValue);
			break;
		case 'r':
		case 'R':
			if (count == 2) {
				anim_stepSpeed = arg1;
			}
			else
				printf("# ERROR: No count speed found\n");
			break;
		case 'e':
		case 'E':
			if (count == 2) {
				sscanf(lineBuffer, "%c %f", &command, &timeConst);
				breathing_time = timeConst;
			}
			else
				printf("# ERROR: No time constant found\n");
			break;
		case '-':
			break;
		case 'a':
		case 'A':
			printActiveCols(pixBuffer);
			break;
		case 's':
		case 'S':
			printDetailedBufferStats(pixBuffer);
			break;
		case 't':
		case 'T':
			if (!animationThreadRunning) {
				animThreadArgs.port = &port;
				animThreadArgs.exitThread = false;
#if defined(MACOSX) || defined(LINUX)
				result = pthread_attr_init(&attr);
#endif
				printf("# INFO: Thread attr init result: %d\n", result);
				result = pthread_create(&animThread, &attr, animationThread, &animThreadArgs);
				printf("# INFO: Thread create result: %d\n", result);
				animationThreadRunning = true;
			}
			else {
				animThreadArgs.exitThread = true;
				animationThreadRunning = false;
			}
			break;
		case 'c':
		case 'C':
			if (count == 4) {
				if (arg1 > 255 || arg1 < 0 || arg2 > 255 || arg2 < 0 || arg3 > 255 || arg3 < 0)
					printf(" Values must be between 0 - 255\n");
				else {
					R = arg1;
					G = arg2;
					B = arg3;
				}
			}
			else {
				printf("All colors must be specified\n");
			}
			break;
		case 'b':
		case 'B':
			if (count == 2) {
				if (arg1 > 255 || arg1 < 0)
					printf(" Values must be between 0 - 255\n");
				else {
					printf(" Applying brightness factor of %5.2f%%\n", arg1 / 255.0 * 100.0);
					R = (uint8_t)(R * (arg1 / 255.0));
					G = (uint8_t)(G * (arg1 / 255.0));
					B = (uint8_t)(B * (arg1 / 255.0));
				}
			}
			else {
				printf("Specify a brightness value [0-255]\n");
			}
			break;
		default:
			break;
		}
		printf("\n\n");

	}
}

/***********************************************
 * Color wheel style animation                 *
 ***********************************************/
int startTime = 0;
int FPSCount = 0;
void loop__AnimColorWheelShift(const PORTTYPE &port, volatile bool* watchVar)
{
	bool loopVar = true;
	if (watchVar != NULL)
		loopVar = watchVar;
	while (loopVar) {
//		printf("Creating effect\n");
		// Create effect:
		strip0_loop0_eff0();
		// Replicate buffer:
		uint32_t* pixBuffer = (uint32_t*)strip_0.strip.getPixels();
		volatile uint32_t* buffer = completePixBuffer + 32;
		for (int curLED = 0; curLED < LEDSPerStrip; curLED++) {
			for (int curCol = 0; curCol < NUM_LEDStrips; curCol++) {
				*buffer = pixBuffer[curLED];
				buffer = buffer + 1;
			}
		}
//		printf("Copying effect\n");
		// Add preamble [tag][size]:
		memcpy((void*)completePixBuffer, TAG_FULLBUFFER_STRING, strlen(TAG_FULLBUFFER_STRING));
		uint32_t bufferSz = (NUM_LEDStrips * LEDSPerStrip);
		memcpy((uint8_t*)completePixBuffer + strlen(TAG_FULLBUFFER_STRING), &bufferSz, sizeof(uint32_t));

		// Get debug feedback [NOTE: this can delay sending speeds dramatically if the serial port read timeout is long]
//		printf("recv data:\n" );
		// recvData(port);

		// Blocking send buffer (pre-amble + frame buffer)
//		printf("TX data:\n" );
		//printf(" Wrote: %d\n", 
		transmit_bytes(port, (char*)completePixBuffer, completePixBufferSz);

		FPSCount++;
		if( millis()-startTime > 1e3 ) {
			printf("# INFO: FPS %.2f\n", FPSCount/((double)millis()-startTime)*1000.0 );
			startTime = millis();
			FPSCount = 0;
		}

		// Transmit at approximately 30FPS:
//		delay_ms(25);	
	}
}

const uint32_t ASCIIDummyLoopLength = LEDSPerStrip;
void loop__ASCIIDummyLoop(const PORTTYPE& port) {
	while( true ){
		// Paint the entire canvas the 'background' breathing color:
		for (uint32_t curLED = 0; curLED < ASCIIDummyLoopLength; curLED++) {
			for (uint32_t curCol = 0; curCol < NUM_LEDStrips; curCol++) {
				pixBufferAccess(curLED, curCol) = curCol+'a';
				if (curCol == 0) {
					pixBufferAccess(curLED, curCol) = (((curLED / 100) % 10 + '0') << 0) + (((curLED / 10) % 10 + '0') << 8) + (curLED % 10 + '0' << 16);
				}
			}
		}
		// Add preamble [tag][size]:
		memcpy((void*)completePixBuffer, TAG_FULLBUFFER_STRING, strlen(TAG_FULLBUFFER_STRING));
		uint32_t bufferSz = (NUM_LEDStrips * ASCIIDummyLoopLength)*sizeof(uint32_t);
		memcpy((uint8_t*)completePixBuffer + strlen(TAG_FULLBUFFER_STRING), &bufferSz, sizeof(uint32_t));

		// Get debug feedback [NOTE: this can delay sending speeds dramatically if the serial port read timeout is long]
		recvData(port, writeDataSz, 5000);

		// Send buffer (pre-amble + frame buffer)
		transmit_bytes(port, (char*)completePixBuffer, (NUM_LEDStrips * ASCIIDummyLoopLength) * sizeof(uint32_t));

		// Transmit at approximately 30FPS:
		delay_ms(25);
	}
}


void loop__AnimNeuralDrop(const PORTTYPE& port, volatile bool* watchVar)
{
	bool loopVar = true;
	if (watchVar != NULL)
		loopVar = watchVar;
	uint32_t ledValue;
	while (loopVar) {
		// Create effect:
		double ramp[16] = { 0 , 0.2094 , 0.4189 , 0.6283 , 0.8378 , 1.0472 , 1.2566 , 1.4661 , 1.6755 , 1.8850 , 2.0944 , 2.3038 , 2.5133 , 2.7227 , 2.9322 , 3.1416 };
		double counterF = 0.0;
		uint32_t curOutLED;
		for (uint32_t curLED = 0; curLED < LEDSPerStrip; curLED+=anim_stepSpeed) {
			// Generate a 'breathing' animation with a pseudo triangular wave with different decay:
			counterF = exp(sin(millis() / breathing_time * 3.1415926)) / 6.0 + 0.25;
			ledValue = anim_ledValue;
			ledValue = (((uint32_t)((double)((ledValue & 0xFF0000) >> 16) * counterF)) << 16) +
				(((uint32_t)((double)((ledValue & 0x00FF00) >> 8) * counterF)) << 8) +
				(((uint32_t)((double)((ledValue & 0x0000FF) >> 0) * counterF)) << 0);

			// Paint the entire canvas the 'background' breathing color:
			for (uint32_t curLED = 0; curLED < LEDSPerStrip; curLED++) {
				for (uint32_t curCol = 0; curCol < NUM_LEDStrips; curCol++) {
					pixBufferAccess(curLED, curCol) = ledValue;
				}
			}

			// Paint animating section falling down the strip:
			for (curOutLED = curLED; curOutLED < (curLED + 16); curOutLED++) {
				int tempCounter = (int) (128 * pow(sin((double)ramp[curOutLED - curLED]), (double)2.0) + 8);
				ledValue = (tempCounter << 8) + (tempCounter << 0);
				if (curOutLED < LEDSPerStrip) {
					for (int curCol = 0; curCol < NUM_LEDStrips; curCol++) {
						pixBufferAccess(curOutLED, curCol) = ledValue;
					}
				}
			}

			// Add preamble [tag][size]:
			memcpy((void*)completePixBuffer, TAG_FULLBUFFER_STRING, strlen(TAG_FULLBUFFER_STRING));
			uint32_t bufferSz = (NUM_LEDStrips * LEDSPerStrip);
			memcpy((uint8_t*)completePixBuffer + strlen(TAG_FULLBUFFER_STRING), &bufferSz, sizeof(uint32_t));

			// Get debug feedback [NOTE: this can delay sending speeds dramatically if the serial port read timeout is long]
			// recvData(port);

			// Send buffer (pre-amble + frame buffer)
			transmit_bytes(port, (char*)completePixBuffer, completePixBufferSz);

			// Transmit at approximately 30FPS:
			// blocking write will take care of this
			delay_ms(25);
		}
	}
}


/***********************************************
 * Original Serial speed test                  *
 * (no frame buffer sent)                      *
 ***********************************************/
void loop__serialSpeedTest(const PORTTYPE &port, int *size)
{
	struct timeval begin, end;
	int i, n, sent, count = 0;
	double elapsed, speed, sum = 0.0;
	for (i = 0; i < *size; i++) {
		buffer[i] = rand();
	}
	gettimeofday(&begin, NULL);
	for (count = 0; count < 20; count++) {
		for (sent = 0; sent < 50000; sent += *size) {
			n = transmit_bytes(port, (char*)buffer, *size);
			if (n != *size) die("errors transmitting data\n");
		}
		gettimeofday(&end, NULL);
		if (count < 5) {
			printf("ignoring startup buffering...\n");
		}
		else {
			elapsed = (double)((double)end.tv_sec - begin.tv_sec);
			elapsed += (double)((double)end.tv_usec - begin.tv_usec) / 1000000.0;
			speed = (double)sent / elapsed;
			printf("Bytes per second = %.0lf\n", speed);
			sum += speed;
		}
		begin.tv_sec = end.tv_sec;
		begin.tv_usec = end.tv_usec;
	}
	speed = sum / 15.0;
	printf("Average bytes per second = %.0lf\n", speed);
}

