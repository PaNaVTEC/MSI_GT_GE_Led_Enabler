/**
 * MSI Led enabler for MSI GT60 / 70 can works with other MSIs with the same keyboard.
 * The purpose of this app is to enable the keyboard light leds when using unix based systems.
 * 
 * Christian Panadero Martinez - 2012 - Bakingcode.com - @PaNaVTEC on twitter
 */

#include <stdio.h>
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

/** Area constants */
#define AREA_LEFT							0x01
#define AREA_MIDDLE							0x02
#define AREA_RIGHT							0x03

/** Color constants */
#define COLOR_BLACK							0x00
#define COLOR_RED							0x01
#define COLOR_ORANGE						0x02
#define COLOR_YELLOW						0x03
#define COLOR_GREEN							0x04
#define COLOR_SKY							0x05
#define COLOR_BLUE							0x06
#define COLOR_PURPLE						0x07
#define COLOR_WHITE							0x08

/** Level constants. High is more intense light */
#define LEVEL_1								0x00
#define LEVEL_2								0x01
#define LEVEL_3								0x02
#define LEVEL_4								0x03

/** Lights modes */
#define MODE_NORMAL							0x01
#define MODE_GAMING							0x02
#define MODE_BREATHING						0x03
#define MODE_WAVE							0x04
#define MODE_DUAL_COLOR						0x05

/** Allowed params */
const char* PARAM_HELP =					"--help";
const char* PARAM_HELP_SHORT=				"-h";
const char* PARAM_VERS =					"--version";
const char* PARAM_VERS_SHORT=				"-v";
const char* PARAM_MODE =					"-mode";
const char* PARAM_COLOR1 =					"-color1";
const char* PARAM_COLOR2 =					"-color2";
const char* PARAM_COLOR3 =					"-color3";
const char* PARAM_LEVEL	=					"-level";

/** Allowed modes values */
const char* VALUE_MODE_NORMAL = 			"normal";
const char* VALUE_MODE_GAMING = 			"gaming";
const char* VALUE_MODE_BREATHING = 			"breathing";
const char* VALUE_MODE_WAVE = 				"wave";
const char* VALUE_MODE_DUALCOLOR =			"dualcolor";

/** Allowed colors values */
const char* VALUE_COLOR_BLACK =				"black";
const char* VALUE_COLOR_RED	=				"red";
const char* VALUE_COLOR_ORANGE =			"orange";
const char* VALUE_COLOR_YELLOW =			"yellow";
const char* VALUE_COLOR_GREEN =				"green";
const char* VALUE_COLOR_SKY =				"sky";
const char* VALUE_COLOR_BLUE =				"blue";
const char* VALUE_COLOR_PURPLE =			"purple";
const char* VALUE_COLOR_WHITE =				"white";

// enum for array param values positions
enum values {
	kMode,
	kColor1,
	kColor2,
	kColor3,
	kLevel,
	kSize
};

char usage[] =
"Usage [NORMAL MODE]:\n"
"msiledenabler -mode normal -color1 <valid_color> [-color2 <valid_color>] [-color3 <valid_color>]\n"
"		       -level <valid_intensity_level>\n"
"Usage [GAMING MODE]:\n"
"msiledenabler -mode gaming -color1 <valid_color> -level <valid_intensity_level>\n\n"
"Valid intensity levels: [0,1,2,3]\n"
"Valid Colors: [black|red|orange|yellow|green|sky|blue|purple|white]\n"
"Example usage: ./msiledenabler -mode normal -color1 blue -color2 green -color3 yellow -level 0\n\n";

char version[] =
"MSI Led Enabler v0.5\n"
"Author: Christian Panadero @ bakingcode.com - Twitter: @PaNaVTEC\n";

/**
 * Sends to the handler the area / color and level selected. NOTE you need to commit for this applies
 */
void
sendActivateArea(hid_device *handle, unsigned char area, unsigned char color, unsigned char level) {

	// Will send a 8 bytes array
	unsigned char data[8];
	memset(&data, 0x00, 8);
	data[0] = 0x01; // Fixed report value.
	data[1] = 0x02; // Fixed report value?

	data[2] = 0x42; // 42 = set color inputs / 41 = confirm
	data[3] = area; // 1 = left / 2 = middle / 3 = right
	data[4] = color; // see color constants
	data[5] = level; // see level constants
	data[6] = 0x00; // empty 
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

int 
main(int argc, char* argv[]) {

	unsigned char arguments[kSize];
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

				if (strcmp(argv[x], PARAM_MODE) == 0) {

					if (strcmp(argv[x + 1], VALUE_MODE_NORMAL) == 0) {

						arguments[kMode] = MODE_NORMAL;

					} else if (strcmp(argv[x + 1], VALUE_MODE_GAMING) == 0) {

						arguments[kMode] = MODE_GAMING;

					}

				} else if (strcmp(argv[x], PARAM_LEVEL) == 0) {

					arguments[kLevel] = (unsigned char)argv[x + 1][0];
				
				} else if (strcmp(argv[x], PARAM_COLOR1) == 0) {

					arguments[kColor1] = parseColor(argv[x + 1]);
				
				} else if (strcmp(argv[x], PARAM_COLOR2) == 0) {

					arguments[kColor2] = parseColor(argv[x + 1]);
				
				} else if (strcmp(argv[x], PARAM_COLOR3) == 0) {

					arguments[kColor3] = parseColor(argv[x + 1]);
				
				}

			}

		}
	}

	// Check required params
	if (arguments[kMode] == UCHAR_MAX) {
		printf("No mode specified. (-mode). Use --help for more information\n\n");
		return 1;
	}

	if (arguments[kColor1] == UCHAR_MAX) {
		printf("No color specified. (-color1). Use --help for more information\n\n");
		return 1;
	}	

	if (arguments[kLevel] == UCHAR_MAX) {
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
	if (arguments[kMode] == MODE_NORMAL) {

		//Gaming mode = full keyboard illumination 
		if (arguments[kColor3] == UCHAR_MAX && arguments[kColor2] == UCHAR_MAX) {

			sendActivateArea(handle, AREA_LEFT, arguments[kColor1], arguments[kLevel]);
			sendActivateArea(handle, AREA_MIDDLE, arguments[kColor1], arguments[kLevel]);
			sendActivateArea(handle, AREA_RIGHT, arguments[kColor1], arguments[kLevel]);

		} else {

			sendActivateArea(handle, AREA_LEFT, arguments[kColor1], arguments[kLevel]);
			sendActivateArea(handle, AREA_MIDDLE, arguments[kColor2], arguments[kLevel]);
			sendActivateArea(handle, AREA_RIGHT, arguments[kColor3], arguments[kLevel]);

		}

		commit(handle, MODE_NORMAL);

	} else if (arguments[kMode] == MODE_GAMING) {

		//Gaming mode = only left area on 1 color with a intensity level
		sendActivateArea(handle, AREA_LEFT, arguments[kColor1], arguments[kLevel]);
		commit(handle, MODE_GAMING);

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