#pragma once
#ifndef TARGAHANDLER_H
#define TARGAHANDLER_H
#include <stdio.h>
#include "MemoryManager.h"

#define sc_uchar(x) static_cast<unsigned char>(x)
#define sc_float(x) static_cast<float>(x)

// Header
//
// Storage container for TGA header info. 
// based on http://www.paulbourke.net/dataformats/tga/
typedef struct {
	char  idlength;
	char  colourmaptype;
	char  datatypecode;
	short int colourmaporigin;
	short int colourmaplength;
	char  colourmapdepth;
	short int x_origin;
	short int y_origin;
	short width;
	short height;
	char  bitCount;
	unsigned char  imagedescriptor;
} Header;

// Image
//
// Holds read data from a TGA image at runtime
typedef struct {
	int width;
	int height;
	int bpp;
	int imageSize;
	unsigned char *data;
} Image;

enum COMPRESSION { UNCOMPRESSED = 0, RLE = 1 };

// class TartaHandler
//
// Reads and stores compressed/ uncompressed TGA data
// and resamples to a new resolution accorting to user 
// specified scale factor(s)
class TargaHandler {
public:
	TargaHandler();
	~TargaHandler();

	bool loadTGA(const char *filename, Image* img);
	bool saveTGA(const char *filename, Image* img, COMPRESSION comp);
	void ResampleBillinear(Image *img, const float scalex, const float scaley);
	void setExpectedRuns(unsigned int runs);

private:
	bool loadCompressed(const char * filename, Image* img, FILE * filePtr);
	bool loadUncompressed(const char * filename, Image* img, FILE * filePtr);
	bool readPixel(unsigned char* p, Image* img, FILE* fPtr);

	bool saveUncompressed(Header* h, const char* filename, Image* img);

	void storePixel(Image* img, unsigned char* pBuff, unsigned int buffIdx);
	unsigned char getPixelVal(Image* img, int x, int y, int i);
	void closeAndFree(Image* img, FILE* fPtr, unsigned char* buff = NULL);
	void writeHeader(Header *header, FILE* filePtr);
	void readHeader(FILE* filePtr);

	inline float lerp(float s1, float s2, float t) { return s1 + (s2 - s1)*t; }
	inline float blerp(float p00, float p10, float p01, float p11, float tx, float ty) {
		return lerp(lerp(p00, p10, tx), lerp(p01, p11, tx), ty);
	}

	Header m_header;

	unsigned int m_runsExpected;
	unsigned char* newData;
};

#endif /* TARGAHANDLER_H */