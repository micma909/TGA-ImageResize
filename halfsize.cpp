#include "targaHandler.h"
#define DEFAULT_INPUT "DefaultFiles/testpattern_rle.tga"
#define DEFAULT_OUTPUT "outputUC.tga"
#define DEFAULT_SX 0.5f
#define DEFAULT_SY 0.5f

int main(int argc, char* argv[]){
	// optional / default params
	char* fileToRead  = DEFAULT_INPUT;
	char* fileToWrite = DEFAULT_OUTPUT;
	float scale_x = DEFAULT_SX;
	float scale_y = DEFAULT_SY;

	// Simple if/else for handling user input
	if (argc == 5) {
		// read user specified IO
		fileToRead  = argv[1];
		fileToWrite = argv[2];
		// if user specifies X and Y scaling
		float sx = sc_float(atof(argv[3]));
		float sy = sc_float(atof(argv[4]));
		if (sx > 0 && sy > 0) {
			scale_x = sx;
			scale_y = sy;
		}else printf("scaling req. floating point values > 0, \
					 using default 0.5\n");
	}else if (argc == 4) {
		// read user specified IO
		fileToRead  = argv[1];
		fileToWrite = argv[2];
		// if user specifies only one scaling factor
		float s = sc_float(atof(argv[3]));
		if (s > 0) scale_x = scale_y = s;
		else printf("scaling req. floating point values > 0, \
					 using default 0.5\n");
	}else if (argc == 3) {
		// read user specified IO
		fileToRead = argv[1];
		fileToWrite = argv[2];
	}else if (argc == 2) {
		// if user doesnt specify output file, use default
		fileToRead = argv[1];
		printf("OUT file not specified , using default\n");
	}else {
		// default
		printf("IN/OUT files not specified, using defaults \n");
	}
	printf("Reading: \"%s\"...\n", fileToRead);

	Image image;
	TargaHandler* targaHandler = new TargaHandler();

	//targaHandler->setExpectedRuns(2); 
	//
	// func. commented above has impact on memory managment.
	// The idea is to fascilitate multiple runs of TargaHandler
	// - if for example this was to be run in a loop for multiple 
	// streaming files, then this call pre-allocates the memory 
	// which will be required during the entire loop. 
	// 
	// Note: If user anticipates a set of image-size resolutions
	// with large variation in resolution then this is best left
	// unset, where the allocation defaults to a standard 1:1 
	// allocation scheme.  

	bool success = targaHandler->loadTGA(fileToRead, &image);
	if (success) {
		printf("Done.\n");
		printf("Original size: %ix%i\n", image.width, image.height);

		targaHandler->ResampleBillinear(&image, scale_x, scale_y);

		printf("Resized to: %ix%i\n", image.width, image.height);
		printf("Saving: \"%s\"... \n", fileToWrite);

		success = targaHandler->saveTGA(fileToWrite, &image, UNCOMPRESSED);

		if (success) {
			printf("Done.\n");
		}else {
			printf("Could not save file: \"%s\" \n", fileToWrite);
		}
	}	else {
		printf("%s terminates due to error...\n", argv[0]);
	}

	delete targaHandler;

    return 0;
}

