/**
 * MSI Led enabler for MSI GT60 / 70 can works with other MSIs with the same keyboard.
 * The purpose of this app is to enable the keyboard light leds when using unix based systems.
 * 
 * Christian Panadero Martinez - 2012 - Bakingcode.com - @PaNaVTEC on twitter
 */

#include <stdio.h>
#include <math.h>
#include <wchar.h>
#include "hidapi.h"

// Headers needed for sleeping.
#ifdef _WIN32
	#include <windows.h>
#else
	//#include <unistd.h>
#endif

#include <stdlib.h>
#include <string>

/** Max char params */
#define UCHAR_MAX							0x10

/** Area constants */
#define AREA_LEFT							0x01
#define AREA_MIDDLE							0x02
#define AREA_RIGHT							0x03

/** Color constants */
#define COLOR_BLACK							0x00
#define COLOR_RED							0x01
#define COLOR_ORANGE							0x02
#define COLOR_YELLOW							0x03
#define COLOR_GREEN							0x04
#define COLOR_SKY							0x05
#define COLOR_BLUE							0x06
#define COLOR_PURPLE							0x07
#define COLOR_WHITE							0x08

/** Level constants. High is more intense light */
#define LEVEL_1								0x00
#define LEVEL_2								0x01
#define LEVEL_3								0x02
#define LEVEL_4								0x03

/** Lights modes */
#define MODE_DISABLE							0x00
#define MODE_NORMAL							0x01
#define MODE_GAMING							0x02
#define MODE_BREATHING_STD						0x03
#define MODE_AUDIO							0x04 // not implemented
#define MODE_WAVE_STD  							0x05
#define MODE_DUAL_COLOR							0x06
#define MODE_OFF							0x07 // not implemented, same as MODE_DISABLE ?
#define MODE_BREATHING_IDLE						0x08
#define MODE_WAVE_IDLE							0x09

/** Mode color change period constants */
#define PERIOD_WAVE_STD	   						1.5
#define PERIOD_BREATHING_STD						1
#define PERIOD_DUAL_COLOR  						2
#define PERIOD_WAVE_IDLE						6
#define PERIOD_BREATHING_IDLE						5.5

/** Allowed params */
const char* PARAM_HELP =						"--help";
const char* PARAM_HELP_SHORT=						"-h";
const char* PARAM_VERS =						"--version";
const char* PARAM_VERS_SHORT=						"-v";
const char* PARAM_MODE =						"-mode";
const char* PARAM_COLOR1 =						"-color1";
const char* PARAM_COLOR2 =						"-color2";
const char* PARAM_COLOR3 =						"-color3";
const char* PARAM_LEVEL	=						"-level";
const char* PARAM_IDLE	=						"-idle";

/** Allowed modes values */
const char* VALUE_MODE_DISABLE = 					"disable";
const char* VALUE_MODE_NORMAL = 					"normal";
const char* VALUE_MODE_GAMING = 					"gaming";
const char* VALUE_MODE_BREATHING = 					"breathing";
const char* VALUE_MODE_WAVE = 						"wave";
const char* VALUE_MODE_DUALCOLOR =					"dualcolor";

/** Allowed colors values */
const char* VALUE_COLOR_BLACK =						"black";
const char* VALUE_COLOR_RED	=					"red";
const char* VALUE_COLOR_ORANGE =					"orange";
const char* VALUE_COLOR_YELLOW =					"yellow";
const char* VALUE_COLOR_GREEN =						"green";
const char* VALUE_COLOR_SKY =						"sky";
const char* VALUE_COLOR_BLUE =						"blue";
const char* VALUE_COLOR_PURPLE =					"purple";
const char* VALUE_COLOR_WHITE =						"white";

// enum for array param values positions
enum values {
	kMode,
	kColor1,
	kColor2,
	kColor3,
	kLevel,
	kIdle,
	kSize,
};

// struct for RedGreenBlue color model
struct rgb {
	rgb() : color(COLOR_BLACK), r(0), g(0), b(0) {}
	rgb(unsigned char color, unsigned char r, unsigned char g, unsigned char b) : color(color), r(r), g(g), b(b) {}
	unsigned char color, r, g, b;
	void setRGBvalues(rgb rgbColor) {
		color = rgbColor.color;
		r = rgbColor.r;
		g = rgbColor.g;
		b = rgbColor.b;
	}
};

// struct for colors defined with RGB values at intensity LEVEL_2
struct colors {
	colors() : black(COLOR_BLACK, 0, 0, 0), red(COLOR_RED, 255, 0, 0),
				orange(COLOR_ORANGE, 187, 112, 0), yellow(COLOR_YELLOW, 238, 238, 0),
				green(COLOR_GREEN, 176, 255, 0), sky(COLOR_SKY, 0, 255, 255),
				blue(COLOR_BLUE, 0, 0, 255), purple(COLOR_PURPLE, 48, 0, 255),
				white(COLOR_WHITE, 176, 255, 176) {}
	const rgb black, red, orange, yellow, green, sky, blue, purple, white;
};

char usage[] =
"Usage [DISABLE MODE]:\n"
"msiledenabler -mode disable\n"
"Usage [NORMAL MODE]:\n"
"msiledenabler -mode normal -color1 <valid_color> [-color2 <valid_color>] [-color3 <valid_color>]\n"
"\t      -level <valid_intensity_level>\n"
"Usage [GAMING MODE]:\n"
"msiledenabler -mode gaming -color1 <valid_color> -level <valid_intensity_level>\n"
"Usage [BEATHING MODE]:\n"
"msiledenabler -mode breathing -color1 <valid_color> -color2 <valid_color> -color3 <valid_color>\n"
"\t     [-idle <valid_idle_value>]\n"
"Usage [WAVE MODE]:\n"
"msiledenabler -mode wave -color1 <valid_color> -color2 <valid_color> -color3 <valid_color>\n"
"\t     [-idle <valid_idle_value>]\n"
"Usage [DUAL_COLOR MODE]:\n"
"msiledenabler -mode dualcolor -color1 <valid_color> -color2 <valid_color>\n\n"
"Valid intensity levels: [0,1,2,3]\n"
"Valid colors: [black|red|orange|yellow|green|sky|blue|purple|white]\n"
"Valid idle value: [1]\n"
"Example usage: ./msiledenabler -mode normal -color1 blue -color2 green -color3 yellow -level 0\n\n";

char version[] =
"MSI Led Enabler v0.5+\n"
"Author: Christian Panadero @ bakingcode.com - Twitter: @PaNaVTEC\n";

/**
 * Sends to the handler the area / color and level selected. NOTE you need to commit for this applies
 */
void
sendActivateArea(hid_device *handle, unsigned char modeValue, unsigned char area, unsigned char color, unsigned char level, unsigned char blue) {

	// Will send a 8 bytes array
	unsigned char data[8];
	memset(&data, 0x00, 8);
	data[0] = 0x01; // Fixed report value
	data[1] = 0x02; // Fixed report value

	data[2] = modeValue; // 43 = set special modes color input / 42 = set color input / 41 = confirm
	data[3] = area; // 1 = left / 2 = middle / 3 = right
	data[4] = color; // see color constants
	data[5] = level; // see level constants
	data[6] = blue; // blue component gain speed for special modes
	data[7] = 0xec; // EOR

	if (hid_send_feature_report(handle, data, 9) < 0) {
		printf("Unable to send a feature report.\n");
	}
	
}

/**
 * Commits the lights with the modes
 */
void
commit(hid_device *handle, unsigned char mode) {

	//CONFIRMATION. This needs to be sent for confirmate all the led operations
	unsigned char data[8];
	data[0] = 0x01;
	data[1] = 0x02;

	data[2] = 0x41; // commit byte
	data[3] = mode; // current mode
	data[4] = 0x00;  
	data[5] = 0x00; 
	data[6] = 0x00;
	data[7] = 0xec;

	if (hid_send_feature_report(handle, data, 9) < 0) {
		printf("Unable to send a feature report.\n");
	}

}

unsigned char
parseColor(char* color) {

	if (strcmp(color, VALUE_COLOR_BLACK) == 0) {
		return COLOR_BLACK;
	} else if (strcmp(color, VALUE_COLOR_RED) == 0) {
		return COLOR_RED;
	} else if (strcmp(color, VALUE_COLOR_ORANGE) == 0) {
		return COLOR_ORANGE;
	} else if (strcmp(color, VALUE_COLOR_YELLOW) == 0) {
		return COLOR_YELLOW;
	} else if (strcmp(color, VALUE_COLOR_ORANGE) == 0) {
		return COLOR_ORANGE;
	} else if (strcmp(color, VALUE_COLOR_GREEN) == 0) {
		return COLOR_GREEN;
	} else if (strcmp(color, VALUE_COLOR_SKY) == 0) {
		return COLOR_SKY;
	} else if (strcmp(color, VALUE_COLOR_BLUE) == 0) {
		return COLOR_BLUE;
	} else if (strcmp(color, VALUE_COLOR_PURPLE) == 0) {
		return COLOR_PURPLE;
	} else if (strcmp(color, VALUE_COLOR_WHITE) == 0) {
		return COLOR_WHITE;
	}

	return UCHAR_MAX;
}

unsigned char
filterLevel(unsigned char color, unsigned char level)
{
	if (color == COLOR_BLACK || color == COLOR_WHITE) {
		return 0;
	}

	return level;
}

unsigned char
convertLevel(char* level) {

    if (strlen(level) == 1) {
		char clevel = *level-'0';
		if (clevel == LEVEL_1) {
			return LEVEL_4;
		} else if (clevel == LEVEL_2) {
			return LEVEL_3;
		} else if (clevel == LEVEL_3) {
			return LEVEL_2;
		} else if (clevel == LEVEL_4) {
			return LEVEL_1;
		}
	}

	return UCHAR_MAX;
}

unsigned char
convertIdle(char* idle) {

    if (strlen(idle) == 1) {
		char cidle = *idle-'0';
		if (cidle == 1) {			
			return 1; // idle
		}
	}

	return 0; // std
}

rgb
identifyRGBcolor(colors allowedColors, unsigned char colorN) {

	if (colorN == allowedColors.red.color) {
		return allowedColors.red;
	} else if (colorN == allowedColors.orange.color) {
		return allowedColors.orange;
	} else if (colorN == allowedColors.yellow.color) {
		return allowedColors.yellow;
	} else if (colorN == allowedColors.green.color) {
		return allowedColors.green;
	} else if (colorN == allowedColors.sky.color) {
		return allowedColors.sky;
	} else if (colorN == allowedColors.blue.color) {
		return allowedColors.blue;
	} else if (colorN == allowedColors.purple.color) {
		return allowedColors.purple;
	} else if (colorN == allowedColors.white.color) {
		return allowedColors.white;
	}

	return allowedColors.black;
}

unsigned char
computeRampSpeed(double leftColor, double rightColor, double period) {

	if (leftColor - rightColor == 0) {
		return 0;
	}

	return ceil((period * 250) / (abs(leftColor - rightColor)));
}

int 
main(int argc, char* argv[]) {

  	/** set default values to std */
	unsigned char cMODE_BREATHING = MODE_BREATHING_STD;
	unsigned char cMODE_WAVE      = MODE_WAVE_STD;
	double dPERIOD_WAVE	      = PERIOD_WAVE_STD;
	double dPERIOD_BREATHING      = PERIOD_BREATHING_STD;

	unsigned char arguments[kSize]; arguments[kIdle] = 0x00;
	colors allowedColors;
	rgb color1, color2, color3, speedColor1, speedColor2, speedColor3; 
	hid_device *handle;

#ifdef WIN32
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
#endif

	memset(&arguments, UCHAR_MAX, kSize);

	if (argc == 2 && (strcmp(argv[1], PARAM_HELP_SHORT) == 0 || strcmp(argv[1], PARAM_HELP) == 0)) {

		printf("%s", usage);
		return 1;
	} else if (argc == 2 && (strcmp(argv[1], PARAM_VERS_SHORT) == 0 || strcmp(argv[1], PARAM_VERS) == 0)) {

		printf("%s", version);
		return 1;
	} else if (argc < 3) {

		printf("%s", usage);
		return 1;
	} else {

		// Get arguments for program
		for (int x = 1; x < argc; x++) {

			// The params needs to start with "-"
			if (argv[x][0] == '-') {

				if (!argv[x + 1]) {

					printf("Invalid parameter(s). Use --help for more information\n\n");
					return 1;

				} else if (strcmp(argv[x], PARAM_MODE) == 0) {

					if (strcmp(argv[x + 1], VALUE_MODE_DISABLE) == 0) {

						arguments[kMode] = MODE_DISABLE;

					} else if (strcmp(argv[x + 1], VALUE_MODE_NORMAL) == 0) {

						arguments[kMode] = MODE_NORMAL;

					} else if (strcmp(argv[x + 1], VALUE_MODE_GAMING) == 0) {

						arguments[kMode] = MODE_GAMING;

					} else if (strcmp(argv[x + 1], VALUE_MODE_BREATHING) == 0) {

						arguments[kMode] = cMODE_BREATHING;

					} else if (strcmp(argv[x + 1], VALUE_MODE_WAVE) == 0) {

						arguments[kMode] = cMODE_WAVE;
					
					} else if (strcmp(argv[x + 1], VALUE_MODE_DUALCOLOR) == 0) {

						arguments[kMode] = MODE_DUAL_COLOR;
					}

				} else if (strcmp(argv[x], PARAM_COLOR1) == 0) {

					arguments[kColor1] = parseColor(argv[x + 1]);
				
				} else if (strcmp(argv[x], PARAM_COLOR2) == 0) {

					arguments[kColor2] = parseColor(argv[x + 1]);
				
				} else if (strcmp(argv[x], PARAM_COLOR3) == 0) {

					arguments[kColor3] = parseColor(argv[x + 1]);				

				} else if (strcmp(argv[x], PARAM_LEVEL) == 0) {
					
					arguments[kLevel] = convertLevel(argv[x + 1]);

				} else if (strcmp(argv[x], PARAM_IDLE) == 0) {

					arguments[kIdle] = convertIdle(argv[x + 1]);
				}
			}
		}
	}

	// Check required params
	if (arguments[kMode] == UCHAR_MAX) {
		printf("No mode specified. (-mode). Use --help for more information\n\n");
		return 1;
	}

	if ((arguments[kColor1] == UCHAR_MAX && arguments[kMode] != MODE_DISABLE)
		|| (arguments[kColor2] == UCHAR_MAX && arguments[kMode] == MODE_DUAL_COLOR)
		|| ((arguments[kColor2] == UCHAR_MAX || arguments[kColor3] == UCHAR_MAX) && (arguments[kMode] == cMODE_WAVE || arguments[kMode] == cMODE_BREATHING))) {
		printf("No color specified. (-color1). Use --help for more information\n\n");
		return 1;
	}

	if (arguments[kLevel] == UCHAR_MAX && (arguments[kMode] == MODE_NORMAL || arguments[kMode] == MODE_GAMING)) {
		printf("No intensity level specified. (-level). Use --help for more information\n\n");
		return 1;
	}

	// Ready to open lights
	// Open the device using the VID, PID
	handle = hid_open(0x1770, 0xff00, NULL);
	if (!handle) {
		printf("Unable to open MSI Led device.\n");
 		return 1;
	}

	// Check Modes
	if (arguments[kMode] == MODE_DISABLE) {

		// Disable mode = turn off keyboard led
		commit(handle, MODE_DISABLE);

	} else if (arguments[kMode] == MODE_NORMAL) {

		//Gaming mode = full keyboard illumination 
		if (arguments[kColor3] == UCHAR_MAX && arguments[kColor2] == UCHAR_MAX) {

			sendActivateArea(handle, 0x42, AREA_LEFT, arguments[kColor1], arguments[kLevel], 0x00);
			sendActivateArea(handle, 0x42, AREA_MIDDLE, arguments[kColor1], arguments[kLevel], 0x00);
			sendActivateArea(handle, 0x42, AREA_RIGHT, arguments[kColor1], arguments[kLevel], 0x00);

		} else {

			//Normal mode = full keyboard illumination, 3 colors
			sendActivateArea(handle, 0x42, AREA_LEFT, arguments[kColor1], arguments[kLevel], 0x00);
			sendActivateArea(handle, 0x42, AREA_MIDDLE, arguments[kColor2], arguments[kLevel], 0x00);
			sendActivateArea(handle, 0x42, AREA_RIGHT, arguments[kColor3], arguments[kLevel], 0x00);
		}
		commit(handle, MODE_NORMAL);

	} else if (arguments[kMode] == MODE_GAMING) {

		//Gaming mode = only left area on 1 color with a intensity level
		sendActivateArea(handle, 0x42, AREA_LEFT, arguments[kColor1], arguments[kLevel], 0x00);
		commit(handle, MODE_GAMING);

	} else if (arguments[kMode] == cMODE_BREATHING) {

		if (arguments[kIdle] == 1) {
			dPERIOD_BREATHING = PERIOD_BREATHING_IDLE; cMODE_BREATHING = MODE_BREATHING_IDLE;
		}

		//Breathing mode = 3 areas colors blink with a intensity level of 2
		color1.setRGBvalues(identifyRGBcolor(allowedColors, arguments[kColor1])); color2.setRGBvalues(identifyRGBcolor(allowedColors, arguments[kColor2])); color3.setRGBvalues(identifyRGBcolor(allowedColors, arguments[kColor3]));
    		speedColor1.r = computeRampSpeed(color1.r, 0x00, dPERIOD_BREATHING); speedColor1.g = computeRampSpeed(color1.g, 0x00, dPERIOD_BREATHING); speedColor1.b = computeRampSpeed(color1.b, 0x00, dPERIOD_BREATHING);
		speedColor2.r = computeRampSpeed(color2.r, 0x00, dPERIOD_BREATHING); speedColor2.g = computeRampSpeed(color2.g, 0x00, dPERIOD_BREATHING); speedColor2.b = computeRampSpeed(color2.b, 0x00, dPERIOD_BREATHING);
		speedColor3.r = computeRampSpeed(color3.r, 0x00, dPERIOD_BREATHING); speedColor3.g = computeRampSpeed(color3.g, 0x00, dPERIOD_BREATHING); speedColor3.b = computeRampSpeed(color3.b, 0x00, dPERIOD_BREATHING);

		sendActivateArea(handle, 0x43, AREA_LEFT, arguments[kColor1], LEVEL_2, 0x00);
		sendActivateArea(handle, 0x43, AREA_MIDDLE, 0x00, LEVEL_2, 0x00);
		sendActivateArea(handle, 0x43, AREA_RIGHT, speedColor1.r, speedColor1.g, speedColor1.b);
		sendActivateArea(handle, 0x43, AREA_LEFT+3, arguments[kColor2], LEVEL_2, 0x00);
		sendActivateArea(handle, 0x43, AREA_MIDDLE+3, 0x00, LEVEL_2, 0x00);
		sendActivateArea(handle, 0x43, AREA_RIGHT+3, speedColor2.r, speedColor2.g, speedColor2.b);
		sendActivateArea(handle, 0x43, AREA_LEFT+6, arguments[kColor3], LEVEL_2, 0x00);
		sendActivateArea(handle, 0x43, AREA_MIDDLE+6, 0x00, LEVEL_2, 0x00);
		sendActivateArea(handle, 0x43, AREA_RIGHT+6, speedColor3.r, speedColor3.g, speedColor3.b);
		commit(handle, cMODE_BREATHING);

	} else if (arguments[kMode] == cMODE_WAVE) {

		if (arguments[kIdle] == 1) {
			dPERIOD_WAVE = PERIOD_WAVE_IDLE; cMODE_WAVE = MODE_WAVE_IDLE;
		}

		//Wave mode = 3 areas colors blink with a intensity level of 2
		color1.setRGBvalues(identifyRGBcolor(allowedColors, arguments[kColor1])); color2.setRGBvalues(identifyRGBcolor(allowedColors, arguments[kColor2])); color3.setRGBvalues(identifyRGBcolor(allowedColors, arguments[kColor3]));
    		speedColor1.r = computeRampSpeed(color1.r, 0x00, dPERIOD_WAVE); speedColor1.g = computeRampSpeed(color1.g, 0x00, dPERIOD_WAVE); speedColor1.b = computeRampSpeed(color1.b, 0x00, dPERIOD_WAVE);
		speedColor2.r = computeRampSpeed(color2.r, 0x00, dPERIOD_WAVE); speedColor2.g = computeRampSpeed(color2.g, 0x00, dPERIOD_WAVE); speedColor2.b = computeRampSpeed(color2.b, 0x00, dPERIOD_WAVE);
		speedColor3.r = computeRampSpeed(color3.r, 0x00, dPERIOD_WAVE); speedColor3.g = computeRampSpeed(color3.g, 0x00, dPERIOD_WAVE); speedColor3.b = computeRampSpeed(color3.b, 0x00, dPERIOD_WAVE);

		sendActivateArea(handle, 0x43, AREA_LEFT, arguments[kColor1], LEVEL_2, 0x00);
		sendActivateArea(handle, 0x43, AREA_MIDDLE, 0x00, LEVEL_2, 0x00);
		sendActivateArea(handle, 0x43, AREA_RIGHT, speedColor1.r, speedColor1.g, speedColor1.b);
		sendActivateArea(handle, 0x43, AREA_LEFT+3, arguments[kColor2], LEVEL_2, 0x00);
		sendActivateArea(handle, 0x43, AREA_MIDDLE+3, 0x00, LEVEL_2, 0x00);
		sendActivateArea(handle, 0x43, AREA_RIGHT+3, speedColor2.r, speedColor2.g, speedColor2.b);
		sendActivateArea(handle, 0x43, AREA_LEFT+6, arguments[kColor3], LEVEL_2, 0x00);
		sendActivateArea(handle, 0x43, AREA_MIDDLE+6, 0x00, LEVEL_2, 0x00);
		sendActivateArea(handle, 0x43, AREA_RIGHT+6, speedColor3.r, speedColor3.g, speedColor3.b);
		commit(handle, cMODE_WAVE);

	} else if (arguments[kMode] == MODE_DUAL_COLOR) {

		//Dual color mode = 2 areas colors blink with a intensity level of 2
		color1.setRGBvalues(identifyRGBcolor(allowedColors, arguments[kColor1])); color2.setRGBvalues(identifyRGBcolor(allowedColors, arguments[kColor2]));
    		speedColor1.r = computeRampSpeed(color1.r, color2.r, PERIOD_DUAL_COLOR); 
	  	speedColor1.g = computeRampSpeed(color1.g, color2.g, PERIOD_DUAL_COLOR); 
		speedColor1.b = computeRampSpeed(color1.b, color2.b, PERIOD_DUAL_COLOR);

		sendActivateArea(handle, 0x43, AREA_LEFT, arguments[kColor1], filterLevel(arguments[kColor1], LEVEL_2), 0x00);
		sendActivateArea(handle, 0x43, AREA_MIDDLE, arguments[kColor2], filterLevel(arguments[kColor2], LEVEL_2), 0x00);
		sendActivateArea(handle, 0x43, AREA_RIGHT, speedColor1.r, speedColor1.g, speedColor1.b);
		sendActivateArea(handle, 0x43, AREA_LEFT+3, arguments[kColor1], filterLevel(arguments[kColor1], LEVEL_2), 0x00);
		sendActivateArea(handle, 0x43, AREA_MIDDLE+3, arguments[kColor2], filterLevel(arguments[kColor2], LEVEL_2), 0x00);
		sendActivateArea(handle, 0x43, AREA_RIGHT+3, speedColor1.r, speedColor1.g, speedColor1.b);
		sendActivateArea(handle, 0x43, AREA_LEFT+6, arguments[kColor1], filterLevel(arguments[kColor1], LEVEL_2), 0x00);
		sendActivateArea(handle, 0x43, AREA_MIDDLE+6, arguments[kColor2], filterLevel(arguments[kColor2], LEVEL_2), 0x00);
		sendActivateArea(handle, 0x43, AREA_RIGHT+6, speedColor1.r, speedColor1.g, speedColor1.b);
		commit(handle, MODE_DUAL_COLOR);
	}

	// close actual HID handler
	hid_close(handle);

	// Free static HIDAPI objects. 
	hid_exit();

#ifdef WIN32
	system("pause");
#endif

	return 0;
}
