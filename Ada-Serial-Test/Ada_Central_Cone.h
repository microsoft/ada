// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#pragma once

// Global settings for center cone lighting
#include <iostream>
#include <map>
#include <utility>
#include "LED_Helper.h"
extern volatile uint32_t* pixBuffer;
int curPi = 0;
bool changeCenterCone = false;
uint32_t centerConeColor = 0;
typedef std::pair<int, int> type_col_led;
std::map<type_col_led, int> adapi1;
std::map<type_col_led, int> adapi2;
std::map<type_col_led, int> adapi3;
std::map<int, int> adapi1_colmask;
std::map<int, int> adapi2_colmask;
std::map<int, int> adapi3_colmask;

bool maskOn = false;
// Center cone active elements:
void createMap() {
	adapi3_colmask.insert({ 6, 1 });
	adapi3_colmask.insert({ 8, 1 });
	adapi3_colmask.insert({ 9, 1 });
	adapi3_colmask.insert({ 10, 1 });
	// adapi3 col 6 (length 89)
	// 8 21 33 46 58 71 84
	adapi3.insert({ {6,  8}, 1 });
	adapi3.insert({ {6, 21}, 1 });
	adapi3.insert({ {6, 33}, 1 });
	adapi3.insert({ {6, 46}, 1 });
	adapi3.insert({ {6, 58}, 1 });
	adapi3.insert({ {6, 71}, 1 });
	adapi3.insert({ {6, 84}, 1 });
	// adapi3 col 8 (length 89)
	// 11 25 38 50 62 74 88
	adapi3.insert({ {8, 11}, 1 });
	adapi3.insert({ {8, 25}, 1 });
	adapi3.insert({ {8, 38}, 1 });
	adapi3.insert({ {8, 50}, 1 });
	adapi3.insert({ {8, 62}, 1 });
	adapi3.insert({ {8, 74}, 1 });
	adapi3.insert({ {8, 88}, 1 });
	// adapi3 col 9 (length 89)
	// 10 23 35 48 60 74 86
	adapi3.insert({ {9, 10}, 1 });
	adapi3.insert({ {9, 23}, 1 });
	adapi3.insert({ {9, 35}, 1 });
	adapi3.insert({ {9, 48}, 1 });
	adapi3.insert({ {9, 60}, 1 });
	adapi3.insert({ {9, 74}, 1 });
	adapi3.insert({ {9, 86}, 1 });
	// adapi3 col 10 (only half work) (length 39)
	// 8 21 33
	adapi3.insert({ {10,  8}, 1 });
	adapi3.insert({ {10, 21}, 1 });
	adapi3.insert({ {10, 33}, 1 });


	adapi2_colmask.insert({ 1, 1 });
	adapi2_colmask.insert({ 3, 1 });
	adapi2_colmask.insert({ 5, 1 });
	adapi2_colmask.insert({ 6, 1 });
	adapi2_colmask.insert({ 13, 1 });
	// adapi2 col 1 (length 89)
	// 10 23 35 48 60 73 84
	adapi2.insert({ {1, 10}, 1 });
	adapi2.insert({ {1, 23}, 1 });
	adapi2.insert({ {1, 35}, 1 });
	adapi2.insert({ {1, 48}, 1 });
	adapi2.insert({ {1, 60}, 1 });
	adapi2.insert({ {1, 73}, 1 });
	adapi2.insert({ {1, 84}, 1 });
	// adapi2 col 3 (length 89)
	// 12 24 38 50 62 74 88
	adapi2.insert({ {3, 12}, 1 });
	adapi2.insert({ {3, 24}, 1 });
	adapi2.insert({ {3, 38}, 1 });
	adapi2.insert({ {3, 50}, 1 });
	adapi2.insert({ {3, 62}, 1 });
	adapi2.insert({ {3, 74}, 1 });
	adapi2.insert({ {3, 88}, 1 });
	// adapi2 col 5 (length 89)
	// 14 27 39 52 63 75 88
	adapi2.insert({ {5, 14}, 1 });
	adapi2.insert({ {5, 27}, 1 });
	adapi2.insert({ {5, 39}, 1 });
	adapi2.insert({ {5, 52}, 1 });
	adapi2.insert({ {5, 63}, 1 });
	adapi2.insert({ {5, 75}, 1 });
	adapi2.insert({ {5, 88}, 1 });
	// adapi2 col 6 (length 87)
	// 9 22 33 46 58 72 84
	adapi2.insert({ {6,  9}, 1 });
	adapi2.insert({ {6, 22}, 1 });
	adapi2.insert({ {6, 33}, 1 });
	adapi2.insert({ {6, 46}, 1 });
	adapi2.insert({ {6, 58}, 1 });
	adapi2.insert({ {6, 72}, 1 });
	adapi2.insert({ {6, 84}, 1 });
	// adapi2 col 13 (length 89)
	// 10 23 36 48 61 72 85
	adapi2.insert({ {13, 10}, 1 });
	adapi2.insert({ {13, 23}, 1 });
	adapi2.insert({ {13, 36}, 1 });
	adapi2.insert({ {13, 48}, 1 });
	adapi2.insert({ {13, 61}, 1 });
	adapi2.insert({ {13, 72}, 1 });
	adapi2.insert({ {13, 85}, 1 });

	adapi1_colmask.insert({ 5, 1 });
	adapi1_colmask.insert({ 7, 1 });
	// adapi1 col 5 (length 90)
	// 10 23 35 47 60 72 85
	adapi1.insert({ {5, 10}, 1 });
	adapi1.insert({ {5, 23}, 1 });
	adapi1.insert({ {5, 35}, 1 });
	adapi1.insert({ {5, 47}, 1 });
	adapi1.insert({ {5, 60}, 1 });
	adapi1.insert({ {5, 72}, 1 });
	adapi1.insert({ {5, 85}, 1 });
	// adapi1 col 7 (length 88)
	// 6 18 30 44 56 68 81
	adapi1.insert({ {7,  6}, 1 });
	adapi1.insert({ {7, 18}, 1 });
	adapi1.insert({ {7, 30}, 1 });
	adapi1.insert({ {7, 44}, 1 });
	adapi1.insert({ {7, 56}, 1 });
	adapi1.insert({ {7, 68}, 1 });
	adapi1.insert({ {7, 81}, 1 });
}
void maskCenterCone() {
	std::map<type_col_led, int>* adapi = NULL;
	std::map<int, int>* adapi_colmask = NULL;

	switch (curPi) {
	case 1:
		adapi = &adapi1;
		adapi_colmask = &adapi1_colmask;
		break;
	case 2:
		adapi = &adapi2;
		adapi_colmask = &adapi2_colmask;
		break;
	case 3:
		adapi = &adapi3;
		adapi_colmask = &adapi3_colmask;
		break;
	default:
		break;
	}
	if (adapi == NULL)
		return;
	for (int curCol = 0; curCol < NUM_LEDStrips; curCol++) {
		if (adapi_colmask->find(curCol) != adapi_colmask->end()) {
			for (int curLED = 0; curLED < LEDSPerStrip; curLED++) {
				if (adapi->find({ curCol,curLED }) == adapi->end())
					pixBufferAccess(curLED, curCol) = 0;
				else{
				if( changeCenterCone )
					pixBufferAccess(curLED, curCol) = centerConeColor;
				}
			}
		}
	}

}
