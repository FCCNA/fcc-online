/******************************************************************************
*
*	CAEN SpA - Software Division
*	Via Vetraia, 11 - 55049 - Viareggio ITALY
*	+39 0594 388 398 - www.caen.it
*
*******************************************************************************
*
*	Copyright (C) 2020-2023 CAEN SpA
*
*	This file is part of the CAEN Dig2 Library.
*
*	Permission is hereby granted, free of charge, to any person obtaining a
*	copy of this software and associated documentation files (the "Software"),
*	to deal in the Software without restriction, including without limitation
*	the rights to use, copy, modify, merge, publish, distribute, sublicense,
*	and/or sell copies of the Software, and to permit persons to whom the
*	Software is furnished to do so.
*
*	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
*	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*	DEALINGS IN THE SOFTWARE.
*
*	SPDX-License-Identifier: MIT-0
*
***************************************************************************//*!
*
*	\file		main.c
*	\brief		CAEN Open FPGA Digitzers Scope demo
*	\author		Giovanni Cerretani
*
******************************************************************************/

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <CAEN_FELib.h>

// Basic hardcoded configuration
#define COMMAND_TRIGGER			't'
#define COMMAND_STOP			'q'
#define COMMAND_PLOT_WAVE		'w'
#define MAX_NUMBER_OF_SAMPLES	(1U << 10)
#define TIMEOUT_MS				(100)
#define WAVE_FILE_NAME			"Wave.txt"
#define WAVE_FILE_ENABLED		false
#define EVT_FILE_NAME			"EventInfo.txt"
#define EVT_FILE_ENABLED		false
#define DATA_FORMAT " \
	[ \
		{ \"name\" : \"TIMESTAMP\", \"type\" : \"U64\" }, \
		{ \"name\" : \"TRIGGER_ID\", \"type\" : \"U32\" }, \
		{ \"name\" : \"WAVEFORM\", \"type\" : \"U16\", \"dim\" : 2 }, \
		{ \"name\" : \"WAVEFORM_SIZE\", \"type\" : \"SIZE_T\", \"dim\" : 1 }, \
		{ \"name\" : \"EVENT_SIZE\", \"type\" : \"SIZE_T\" } \
	] \
"

// Utilities
#define ARRAY_SIZE(X)		(sizeof(X)/sizeof((X)[0]))

#ifdef _WIN32 // Windows
#include <conio.h>
#define GNUPLOT						"pgnuplot.exe"
#else // Linux
#include <unistd.h>
#include <termios.h>
#define GNUPLOT						"gnuplot"
#define _popen(command, type)		popen(command, type)
#define _pclose(command)			pclose(command)
// Linux replacement for non standard _getch
static int _getch() {
	struct termios oldattr;
	if (tcgetattr(STDIN_FILENO, &oldattr) == -1) perror(NULL);
	struct termios newattr = oldattr;
	newattr.c_lflag &= ~(ICANON | ECHO);
	if (tcsetattr(STDIN_FILENO, TCSANOW, &newattr) == -1) perror(NULL);
	const int ch = getchar();
	if (tcsetattr(STDIN_FILENO, TCSANOW, &oldattr) == -1) perror(NULL);
	return ch;
}
#endif

static unsigned long long value_to_ull(const char* value) {
	char* value_end;
	const unsigned long long ret = strtoull(value, &value_end, 0);
	if (value == value_end || errno == ERANGE)
		fprintf(stderr, "strtoull error\n");
	return ret;
}

static double value_to_d(const char* value) {
	char* value_end;
	const double ret = strtod(value, &value_end);
	if (value == value_end || errno == ERANGE)
		fprintf(stderr, "strtod error\n");
	return ret;
}

static int print_last_error(void) {
	char msg[1024];
	int ec = CAEN_FELib_GetLastError(msg);
	if (ec != CAEN_FELib_Success) {
		fprintf(stderr, "%s failed\n", __func__);
		return ec;
	}
	fprintf(stderr, "last error: %s\n", msg);
	return ec;
}

static int connect_to_digitizer(uint64_t* dev_handle, int argc, char* argv[]) {
	const char* path;
	char local_path[256];
	printf("device path: ");
	if (argc == 2) {
		path = argv[1];
		puts(path);
	} else {
		while (fgets(local_path, sizeof(local_path), stdin) == NULL);
		local_path[strcspn(local_path, "\r\n")] = '\0'; // remove newline added by fgets
		path = local_path;
	}
	return CAEN_FELib_Open(path, dev_handle);
}

static int get_n_channels(uint64_t dev_handle, size_t* n_channels) {
	int ret;
	char value[256];
	ret = CAEN_FELib_GetValue(dev_handle, "/par/NumCh", value);
	if (ret != CAEN_FELib_Success) return ret;
	*n_channels = (size_t)value_to_ull(value);
	return ret;
}

static int print_digitizer_details(uint64_t dev_handle) {
	int ret;
	char value[256];
	ret = CAEN_FELib_GetValue(dev_handle, "/par/ModelName", value);
	if (ret != CAEN_FELib_Success) return ret;
	printf("Model name:\t%s\n", value);
	ret = CAEN_FELib_GetValue(dev_handle, "/par/SerialNum", value);
	if (ret != CAEN_FELib_Success) return ret;
	printf("Serial number:\t%s\n", value);
	ret = CAEN_FELib_GetValue(dev_handle, "/par/ADC_Nbit", value);
	if (ret != CAEN_FELib_Success) return ret;
	printf("ADC bits:\t%llu\n", value_to_ull(value));
	ret = CAEN_FELib_GetValue(dev_handle, "/par/NumCh", value);
	if (ret != CAEN_FELib_Success) return ret;
	printf("Channels:\t%llu\n", value_to_ull(value));
	ret = CAEN_FELib_GetValue(dev_handle, "/par/ADC_SamplRate", value);
	if (ret != CAEN_FELib_Success) return ret;
	printf("ADC rate:\t%f Msps\n", value_to_d(value));
	ret = CAEN_FELib_GetValue(dev_handle, "/par/cupver", value);
	if (ret != CAEN_FELib_Success) return ret;
	printf("CUP version:\t%s\n", value);
	return ret;
}


int main(int argc, char* argv[]) {

	int ret;

	uint64_t dev_handle;

	printf("##########################################\n");
	printf("\tCAEN firmware Scope demo\n");
	printf("##########################################\n");

	if (argc > 2) {
		fputs("invalid arguments", stderr);
		return EXIT_FAILURE;
	}

	// select device
	ret = connect_to_digitizer(&dev_handle, argc, argv);
	if (ret != CAEN_FELib_Success) {
		print_last_error();
		return EXIT_FAILURE;
	}

	ret = print_digitizer_details(dev_handle);
	if (ret != CAEN_FELib_Success) {
		print_last_error();
		return EXIT_FAILURE;
	}

	size_t n_channels;
	ret = get_n_channels(dev_handle, &n_channels);
	if (ret != CAEN_FELib_Success) {
		print_last_error();
		return EXIT_FAILURE;
	}

	printf("Resetting...\t");

	// reset
	ret = CAEN_FELib_SendCommand(dev_handle, "/cmd/reset");
	if (ret != CAEN_FELib_Success) {
		print_last_error();
		return EXIT_FAILURE;
	}

	printf("done.\n");

	ret = CAEN_FELib_Close(dev_handle);
	if (ret != CAEN_FELib_Success) {
		print_last_error();
		return EXIT_FAILURE;
	}

	printf("\nBye!\n");

	return EXIT_SUCCESS;
}
