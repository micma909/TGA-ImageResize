#include "targaHandler.h"
// Default constructor
//
// TargaHandler uses MemoryManager to pre-allocate
// memory. The member variable m_runsExpected allows  
// the user to set the pool-size for the allocation.
TargaHandler::TargaHandler() {

	// By setting m_runsExpected to 0 the allocation
	// is then 1:1 - i.e same as using malloc here.
	// (For more info, see class 'MemoryManager')
	m_runsExpected = 0;
}

// Destructor
//
// Because TargaHandler anticipates multiple runs, 
// it also uses a dedicated MemoryManager for each 
// new image-resolution - each needs deletion. 
TargaHandler::~TargaHandler() {

	for (auto const& p : g_MemoryMap) {
		MemoryManager* toDelete = p.second;
		delete toDelete;
		// Note: deleting MemoryManagers launches 
		// MemoryManager::cleanUp() method ensuring
		// all data is freed.
	}
}

// setExpectedRuns
//
// If we expect to run this program more than once given a 
// set of images with same resolution, then this method sets
// the number of pre-allocations needed for the whole set. 
//
// @param runs - number of pre-allocations tbd
void TargaHandler::setExpectedRuns(unsigned int runs) {
	m_runsExpected = runs;
}

// loadTGA
//
// Loads either RLE or Uncompressed 24 or 32 bit targa 
// image files and stores the data in an Image struct. 
bool TargaHandler::loadTGA(const char *filename, Image* img) {
	FILE *filePtr;
	// Open file
	fopen_s(&filePtr, filename, "rb");
	if (filePtr == NULL) {
		printf("Cannot open file specified\n");
		return false;
	}
	// Store header data in member m_header from stream associated to filePtr 
	readHeader(filePtr);
	// only allow for 24, 32 bit
	if ((m_header.width <= 0) || (m_header.height <= 0)
		|| ((m_header.bitCount != 24) && (m_header.bitCount != 32))) {
		printf("Invalid data format\n");
		if (filePtr != NULL) fclose(filePtr);
		return false;
	}

	// pass relevant data to Image struct
	img->width = m_header.width;
	img->height = m_header.height;
	img->bpp = ((short int)m_header.bitCount / 8);
	img->imageSize = (img->bpp * img->width * img->height);

	// determine read method
	bool success = false;
	// Uncompressed 
	if (m_header.datatypecode == 2) { 
		printf("Uncompressed file loading\n");
		success = loadUncompressed(filename, img, filePtr);
		if (!success) printf("Uncompressed file loaded\n");
	} 
	// Compressed
	else if (m_header.datatypecode == 10) { 
		printf("RLE compressed file loading\n");
		success = loadCompressed(filename, img, filePtr);
		if (!success) printf("Failed loading Compressed file\n");
	}
	else {
		printf("File format not supported\n");
	}
	return success;
}

// loadUncompressed
//
// Uncompressed TGA procedure for images of either 24 or 32 bit
// and uses MemoryManager for memory allocation. 
// 
// @param filename - name of input file
// @param img - structure to hold the pixel data
// @param filePtr - pointer to file on disk
//
// @return Returns false if either allocation of memory or 
//         data-read fails. 
bool TargaHandler::loadUncompressed(const char * filename, Image* img, FILE * filePtr) {
	img->data = newMemory<unsigned char>(img->imageSize, m_runsExpected);

	if (img->data == NULL) {
		printf("Could not allocating memory for uncompressed image\n");
		return false;
	}
	// read data
	if (fread(img->data, 1, img->imageSize, filePtr) != img->imageSize) {
		printf("Error reading uncompressed data\n");
		return false;
	}
	fclose(filePtr);

	return true;
}

// loadCompressed
//
// RLE compressed TGA procedure for images of either 24 or 32 
// bit and uses MemoryManager for memory allocation. 
// 
// @param filename - name of input file
// @param img - structure to hold the pixel data
// @param filePtr - pointer to file on disk
//
// @return Returns false if either allocation of memory or 
//         data-read fails. 
bool TargaHandler::loadCompressed(const char * filename, Image* img, FILE * filePtr) {
	// Allocate Memory To Store Image Data
	img->data = newMemory<unsigned char>(img->imageSize, m_runsExpected);

	if (img->data == NULL) {
		printf("Could not allocating memory for compressed image\n");
		return false;
	}

	unsigned int nrOfPixels = img->height * img->width;
	unsigned int pixIdx = 0;
	unsigned int buffIdx = 0;
	unsigned char* pBuff = newMemory<unsigned char>(img->bpp, m_runsExpected);

	while (pixIdx < nrOfPixels) {
		// headerInfo - storage for the ID header (RAW or RLE)
		unsigned char headerInfo = 0; 
		if (fread(&headerInfo, sizeof(unsigned char), 1, filePtr) == 0) {
			// Read in the 1 byte header
			printf("Could not read header\n");
			closeAndFree(img, filePtr);
			return false;
		}

		if (headerInfo < 128) { // header < 128 = RAW, ELSE = RLE
			// next byte, number of consecutive RAW
			headerInfo += 1;
			for (int i = 0; i < headerInfo; i++) {
				// headerInfo - now contains nr of RAW color values -> loop
				// try get each unique pixel
				if (!readPixel(pBuff, img, filePtr)) 
					return false;
				storePixel(img, pBuff, buffIdx);
				buffIdx += img->bpp;
				pixIdx++;

				if (pixIdx > nrOfPixels) {
					printf("Out of bounds when readign pixel data!\n");
					closeAndFree(img, filePtr, pBuff);
					return false;
				}
			}
		}else { // headerInfo > 128, RLE data, next color reapeated maximum 127 times
			// remove high bit (128) to get repeat count
			headerInfo -= 127;
			// Get the next 3 bytes, which should be the values to duplicate.
			if (!readPixel(pBuff, img, filePtr))   
				return false;
			// copy the color into the image data as many times as dictated by headerInfo
			for (int i = 0; i < headerInfo; i++) {	
				storePixel(img, pBuff, buffIdx);
				buffIdx += img->bpp;
				pixIdx++;

				if (pixIdx > nrOfPixels) {
					printf("Out of bounds when readign pixel data!\n");
					closeAndFree(img, filePtr, pBuff);
					return false;
				}
			}
		}
	}

	return true;
}

// getPixel
//
// Helper method to fascilitate extraction of stored pixels
// in Image struct which are stored 'as-read' in byte i.e BGR(A).
//
// @param img - image pixel container
// @param i - determines which BGRA component to get
// 
// @return returns a Blue, Red, Green, Alpha byte component
unsigned char TargaHandler::getPixelVal(Image* img, int x, int y, int i) {
	return img->data[y*(img->width*img->bpp) + x*(img->bpp) + i];
}

// ResampleBillinear
//
// Bilinear interpolation for image scaling. 
// Given Y- and X- scale parameters this method computes an
// approximate new image-size, requests memory for the new image from
// its dedicated MemoryManager and stores the data in an struct Image.
//
// @param img - image pixel container
// @param scalex - width-scaling, 0.5 => half width
// @param scaley - height-scaling, 0.5 => half height
void TargaHandler::ResampleBillinear(Image *img, float scalex, float scaley) {
	int newWidth  = static_cast<int>(img->width * scalex);
	int newHeight = static_cast<int>(img->height * scaley);
	int x, y, ui, vi;
	float u, v;
	unsigned char p00, p10, p01, p11;

	int newSize = newWidth * newHeight * img->bpp;
	unsigned char* newData = newMemory<unsigned char>(newSize, m_runsExpected);
	int* bgr = newMemory<int>(img->bpp, m_runsExpected);

	for (x = 0; x < newWidth; x++) {
		for (y = 0; y < newHeight; y++) {
			// compute sampling points in pixel-grid of img
			u = x / (float)(newWidth)*(img->width - 1);
			v = y / (float)(newHeight)*(img->height - 1);
			ui = (int)u;
			vi = (int)v;

			for (int i = 0; i < img->bpp; i++) {
				// gather four surrounding pixels component wise
				p00 = getPixelVal(img, ui, vi, i);
				p10 = getPixelVal(img, ui + 1, vi, i);
				p01 = getPixelVal(img, ui, vi + 1, i);
				p11 = getPixelVal(img, ui + 1, vi + 1, i);
				// bilinear interpolation
				// looking for indices, so loosing information 
				// due to int conversion shouldnt cause problems
				bgr[i] = static_cast<int>(blerp(p00, p10, p01, p11, u - ui, v - vi));
			}
			// calculate index in the resized image
			int destIdx = (y * (newWidth * img->bpp)) + (x * img->bpp);

			newData[destIdx + 0] = bgr[0];
			newData[destIdx + 1] = bgr[1];
			newData[destIdx + 2] = bgr[2];
			if (img->bpp == 4) newData[destIdx + 3] = bgr[3];		// if alpha, 32 bpp 
		}
	}
	// Original data will replaced, free
	freeMemory(&img->data[0], img->imageSize);
	freeMemory(&bgr[0], img->bpp);

	// re-point to the new data
	img->data = newData;
	img->width = newWidth;
	img->height = newHeight;
	img->imageSize = newSize;
}
// saveTGA
//
// Determines method of compression for tga image.
// Currently only supports uncompressed writing.
//
// @param filename - output file name
// @param img - image pixel container 
// @param comp - enum determining which compression to use
//
// @return returns the outcome of func. saveUncompressed
bool TargaHandler::saveTGA(const char *filename, Image* img, COMPRESSION comp) {
	// create generic header
	Header gHeader = { 0 };
	gHeader.width = img->width;
	gHeader.height = img->height;
	gHeader.bitCount = img->bpp * 8;
	gHeader.imagedescriptor = 32; // tb read from upper-left corner

	if (comp == UNCOMPRESSED) {
		gHeader.datatypecode = 2; // uncompressed code
								  // write uncompressed file
		return saveUncompressed(&gHeader, filename, img);
	}
	else if (comp == RLE) {
		gHeader.datatypecode = 10; // compressed code
								   // write compressed file
		printf("Saving RLE-images available in next patch.\n");
		return false;
	}
	else {
		printf("Unsupported compression format\n");
		return false;
	}
	return true;
}

// saveUncompressed
//
// Saves an uncompressed tga image to disk 
//
// @param h - header file based on http://www.paulbourke.net/dataformats/tga/
// @param filename - output file name
// @param img - image pixel container 
bool TargaHandler::saveUncompressed(Header* header, const char* filename, Image* img) {
	FILE *filePtr;
	// Open file
	fopen_s(&filePtr, filename, "wb");
	if (filePtr == NULL) {
		printf("Cannot open file specified\n");
	}
	writeHeader(header, filePtr);
	// write data to file
	fwrite(img->data, sizeof(unsigned char), img->imageSize, filePtr);
	fclose(filePtr);

	bool freed = freeMemory(&img->data[0], img->imageSize);
	if (!freed) return false;

	return true;
}
// -------------------------------------------
//	Below, some some convenience functions put away 
//	to make the code above a bit more readable/compact
// -------------------------------------------

// closeAndFree
// 
// Resp. for freeing buffer and any image data used during loading of tga image
// as well as closing and disassociating the file associated with the stream.
void TargaHandler::closeAndFree(Image* img, FILE * filePtr, unsigned char* pbuff) {
	if (filePtr != NULL)   fclose(filePtr);
	if (pbuff != NULL)     free(pbuff);
	if (img->data != NULL) free(img->data);
}

// readPixel
//
// Reads a single pixel in the file stream during loading of tga image
bool TargaHandler::readPixel(unsigned char* p, Image* img, FILE* filePtr) {
	size_t readBytes = fread(p, 1, img->bpp, filePtr);
	if (readBytes != img->bpp) {
		printf("Could not read image data\n");
		closeAndFree(img, filePtr, p);
		return false;
	}
	return true;
}

// storePixel
//
// Reads pixel from buffer and stores pixels component wise  
// into its index in an Image struct. 
void TargaHandler::storePixel(Image* img, unsigned char* pBuff, unsigned int buffIdx) {
	img->data[buffIdx + 0] = pBuff[0];
	img->data[buffIdx + 1] = pBuff[1];
	img->data[buffIdx + 2] = pBuff[2];
	if (img->bpp == 4) img->data[buffIdx + 3] = pBuff[3];		// if alpha, 32 bpp 
}

// formatHeader
//
// Writes header for uncompressed file. Moved here  
// to make func. saveUncompressed easier to read. 
void TargaHandler::writeHeader(Header *h, FILE* filePtr) {
	unsigned char cHeader[18] = { 0 };
	cHeader[0] = sc_uchar(h->idlength);
	cHeader[1] = sc_uchar(h->colourmaptype);
	cHeader[2] = sc_uchar(h->datatypecode);
	cHeader[3] = sc_uchar(h->colourmaporigin % 256);
	cHeader[4] = sc_uchar(h->colourmaporigin / 256);
	cHeader[5] = sc_uchar(h->colourmaplength % 256);
	cHeader[6] = sc_uchar(h->colourmaplength / 256);
	cHeader[7] = sc_uchar(h->colourmapdepth);
	cHeader[8] = sc_uchar(h->x_origin % 256);
	cHeader[9] = sc_uchar(h->x_origin / 256);
	cHeader[10] = sc_uchar(h->y_origin % 256);
	cHeader[11] = sc_uchar(h->y_origin / 256);
	cHeader[12] = sc_uchar(h->width % 256);
	cHeader[13] = sc_uchar(h->width / 256);
	cHeader[14] = sc_uchar(h->height % 256);
	cHeader[15] = sc_uchar(h->height / 256);
	cHeader[16] = sc_uchar(h->bitCount);
	cHeader[17] = sc_uchar(h->imagedescriptor);
	fwrite(cHeader, sizeof(unsigned char), 18, filePtr);
}
// readHeader
//
// Reads header for input TGA files. Moved here  
// to make func. loadTGA easier to read. 
void TargaHandler::readHeader(FILE* filePtr) {
	fread(&m_header.idlength, sizeof(unsigned char), 1, filePtr);
	fread(&m_header.colourmaptype, sizeof(unsigned char), 1, filePtr);
	fread(&m_header.datatypecode, sizeof(unsigned char), 1, filePtr);
	fread(&m_header.colourmaporigin, sizeof(short int), 1, filePtr);
	fread(&m_header.colourmaplength, sizeof(short int), 1, filePtr);
	fread(&m_header.colourmapdepth, sizeof(unsigned char), 1, filePtr);
	fread(&m_header.x_origin, sizeof(short int), 1, filePtr);
	fread(&m_header.y_origin, sizeof(short int), 1, filePtr);
	fread(&m_header.width, sizeof(short int), 1, filePtr);
	fread(&m_header.height, sizeof(short int), 1, filePtr);
	fread(&m_header.bitCount, sizeof(unsigned char), 1, filePtr);
	fread(&m_header.imagedescriptor, sizeof(unsigned char), 1, filePtr);
}