#include <GL/glew.h>

#include "GLInterface.h"
#include "GLImage.h"
#include "SexyAppBase.h"
#include "misc/AutoCrit.h"
#include "misc/CritSect.h"
#include "Graphics.h"
#include "MemoryImage.h"

#include <SDL.h>
#include <GL/glext.h>

using namespace Sexy;

static int gMinTextureWidth;
static int gMinTextureHeight;
static int gMaxTextureWidth;
static int gMaxTextureHeight;
static int gMaxTextureAspectRatio;
static int gSupportedPixelFormats;
static bool gTextureSizeMustBePow2;
static const int MAX_TEXTURE_SIZE = 1024;
static bool gLinearFilter = false;


static void CopyImageToTexture8888(MemoryImage *theImage, int offx, int offy, int theWidth, int theHeight, bool rightPad, bool bottomPad, bool create)
{
	uint32_t *aDest = new uint32_t[theWidth * theHeight*2];

	if (theImage->mColorTable == NULL)
	{
		long unsigned int *srcRow = theImage->GetBits() + offy * theImage->GetWidth() + offx;
		uint32_t *dstRow = aDest;

		for(int y=0; y<theHeight; y++)
		{
			long unsigned int *src = srcRow;
			uint32_t *dst = dstRow;
			for(int x=0; x<theWidth; x++)
				*dst++ = *src++;

			if (rightPad)
				*dst = *(dst-1);

			srcRow += theImage->GetWidth();
			dstRow += theWidth;
		}
	}
	else // palette
	{
		uint8_t *srcRow = (uint8_t*)theImage->mColorIndices + offy * theImage->GetWidth() + offx;
		uint32_t *dstRow = aDest;
		long unsigned int *palette = theImage->mColorTable;

		for(int y=0; y<theHeight; y++)
		{
			uint8_t *src = srcRow;
			uint32_t *dst = dstRow;
			for(int x=0; x<theWidth; x++)
				*dst++ = palette[*src++];

			if (rightPad)
				*dst = *(dst-1);

			srcRow += theImage->GetWidth();
			dstRow += theWidth;
		}
	}

	if (bottomPad)
	{
		uint8_t *dstrow = ((uint8_t*)aDest)+(theWidth*4)*theHeight;
		memcpy(dstrow, dstrow-(theWidth*4), (theWidth*4));
	}

	if (create)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, theWidth, theHeight, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, aDest);
	else
		glTexSubImage2D(GL_TEXTURE_2D, 0, offx, offy, theWidth, theHeight, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, aDest);

	delete[] aDest;
}

static void CopyImageToTexture4444(MemoryImage *theImage, int offx, int offy, int theWidth, int theHeight, bool rightPad, bool bottomPad, bool create)
{
	printf("4444 %d %d %d %d\n", offx, offy, theWidth, theHeight);
	fflush(stdout);

	uint16_t *aDest = new uint16_t[theWidth * theHeight*2];

	if (theImage->mColorTable == NULL)
	{
		long unsigned int *srcRow = theImage->GetBits() + offy * theImage->GetWidth() + offx;
		char *dstRow = (char*)aDest;

		for(int y=0; y<theHeight; y++)
		{
			long unsigned int *src = srcRow;
			uint16_t *dst = (uint16_t*)dstRow;
			for(int x=0; x<theWidth; x++)
			{
				long unsigned int aPixel = *src++;
				*dst++ = ((aPixel>>16)&0xF000) | ((aPixel>>12)&0x0F00) | ((aPixel>>8)&0x00F0) | ((aPixel>>4)&0x000F);
			}

			if (rightPad) 
				*dst = *(dst-1);

			srcRow += theImage->GetWidth();
			dstRow += theWidth;
		}
	}
	else // palette
	{
		uint8_t *srcRow = (uint8_t*)theImage->mColorIndices + offy * theImage->GetWidth() + offx;
		uint8_t *dstRow = (uint8_t*)aDest;
		long unsigned int *palette = theImage->mColorTable;

		for(int y=0; y<theHeight; y++)
		{
			uint8_t *src = srcRow;
			uint16_t *dst = (uint16_t*)dstRow;
			for(int x=0; x<theWidth; x++)
			{
				long unsigned int aPixel = palette[*src++];
				*dst++ = ((aPixel>>16)&0xF000) | ((aPixel>>12)&0x0F00) | ((aPixel>>8)&0x00F0) | ((aPixel>>4)&0x000F);
			}

			if (rightPad) 
				*dst = *(dst-1);

			srcRow += theImage->GetWidth();
			dstRow += theWidth;
		}
	}

	if (create)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, theWidth, theHeight, 0, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV, aDest);
	else
		glTexSubImage2D(GL_TEXTURE_2D, 0, offx, offy, theWidth, theHeight, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV, aDest);

	delete[] aDest;
}

static void CopyImageToTexture565(MemoryImage *theImage, int offx, int offy, int theWidth, int theHeight, bool rightPad, bool bottomPad, bool create)
{
	uint16_t *aDest = new uint16_t[theWidth * theHeight*2];

	if (theImage->mColorTable == NULL)
	{
		long unsigned int *srcRow = theImage->GetBits() + offy * theImage->GetWidth() + offx;
		uint16_t *dstRow = aDest;

		for(int y=0; y<theHeight; y++)
		{
			long unsigned int *src = srcRow;
			uint16_t *dst = (uint16_t*)dstRow;
			for(int x=0; x<theWidth; x++)
			{
				long unsigned int aPixel = *src++;
				*dst++ = ((aPixel>>8)&0xF800) | ((aPixel>>5)&0x07E0) | ((aPixel>>3)&0x001F);
			}

			if (rightPad) 
				*dst = *(dst-1);

			srcRow += theImage->GetWidth();
			dstRow += theWidth;
		}
	}
	else
	{
		printf("565 PALETTE %d %d %d %d\n", offx, offy, theWidth, theHeight);
		fflush(stdout);
	}

	if (create)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, theWidth, theHeight, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, aDest);
	else
		glTexSubImage2D(GL_TEXTURE_2D, 0, offx, offy, theWidth, theHeight, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, aDest);

	delete[] aDest;
}

static void CopyImageToTexturePalette8(int theTexWidth, MemoryImage *theImage, int offx, int offy, int theWidth, int theHeight, bool create)
{
	printf("PALETTE %d %d %d %d - %d\n", offx, offy, theWidth, theHeight, theTexWidth);
	fflush(stdout);

	uint8_t *rgbaData = new uint8_t[theWidth * theHeight * 4];

	delete[] rgbaData;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void CopyImageToTexture(GLuint& theTexture, MemoryImage *theImage, int offx, int offy, int texWidth, int texHeight, PixelFormat theFormat, bool create)
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (gLinearFilter) ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (gLinearFilter) ? GL_LINEAR : GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	int aWidth = std::min(texWidth,(theImage->GetWidth()-offx));
	int aHeight = std::min(texHeight,(theImage->GetHeight()-offy));

	bool rightPad = aWidth<texWidth;
	bool bottomPad = aHeight<texHeight;

	if(aWidth>0 && aHeight>0)
	{
		switch (theFormat)
		{
			case PixelFormat_A8R8G8B8:	CopyImageToTexture8888(theImage, offx, offy, aWidth, aHeight, rightPad, bottomPad, create); break;
			case PixelFormat_A4R4G4B4:	CopyImageToTexture4444(theImage, offx, offy, aWidth, aHeight, rightPad, bottomPad, create); break;
			case PixelFormat_R5G6B5:	CopyImageToTexture565(theImage, offx, offy, aWidth, aHeight, rightPad, bottomPad, create); break;
			case PixelFormat_Palette8:	CopyImageToTexturePalette8(texWidth, theImage, offx, offy, aWidth, aHeight, create); break;
			case PixelFormat_Unknown: break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static int GetClosestPowerOf2Above(int theNum)
{
	int aPower2 = 1;
	while (aPower2 < theNum)
		aPower2<<=1;
	return aPower2;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static bool IsPowerOf2(int theNum)
{
	int aNumBits = 0;
	while (theNum>0)
	{
		aNumBits += theNum&1;
		theNum >>= 1;
	}

	return aNumBits==1;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void GetBestTextureDimensions(int &theWidth, int &theHeight, bool isEdge, bool usePow2, long unsigned int theImageFlags)
{
//	theImageFlags = D3DImageFlag_MinimizeNumSubdivisions;
	if (theImageFlags & D3DImageFlag_Use64By64Subdivisions)
	{
		theWidth = theHeight = 64;
		return;
	}

	static int aGoodTextureSize[MAX_TEXTURE_SIZE];
	static bool haveInited = false;
	if (!haveInited)
	{
		haveInited = true;
		int i;
		int aPow2 = 1;
		for (i=0; i<MAX_TEXTURE_SIZE; i++)
		{
			if (i > aPow2)
				aPow2 <<= 1;

			int aGoodValue = aPow2;
			if ((aGoodValue - i ) > 64)
			{
				aGoodValue >>= 1;
				while (true)
				{
					int aLeftOver = i % aGoodValue;
					if (aLeftOver<64 || IsPowerOf2(aLeftOver))
						break;

					aGoodValue >>= 1;
				}
			}
			aGoodTextureSize[i] = aGoodValue;
		}
	}

	int aWidth = theWidth;
	int aHeight = theHeight;

	if (usePow2)
	{
		if (isEdge || (theImageFlags & D3DImageFlag_MinimizeNumSubdivisions))
		{
			aWidth = aWidth >= gMaxTextureWidth ? gMaxTextureWidth : GetClosestPowerOf2Above(aWidth);
			aHeight = aHeight >= gMaxTextureHeight ? gMaxTextureHeight : GetClosestPowerOf2Above(aHeight);
		}
		else
		{
			aWidth = aWidth >= gMaxTextureWidth ? gMaxTextureWidth : aGoodTextureSize[aWidth];
			aHeight = aHeight >= gMaxTextureHeight ? gMaxTextureHeight : aGoodTextureSize[aHeight];
		}
	}

	if (aWidth < gMinTextureWidth)
		aWidth = gMinTextureWidth;

	if (aHeight < gMinTextureHeight)
		aHeight = gMinTextureHeight;

	/*
	if (aWidth > aHeight)
	{
		while (aWidth > gMaxTextureAspectRatio*aHeight)
			aHeight <<= 1;
	}
	else if (aHeight > aWidth)
	{
		while (aHeight > gMaxTextureAspectRatio*aWidth)
			aWidth <<= 1;
	}
	*/

	theWidth = aWidth;
	theHeight = aHeight;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
TextureData::TextureData()
{
	mWidth = 0;
	mHeight = 0;
	mTexVecWidth = 0;
	mTexVecHeight = 0;
	mBitsChangedCount = 0;
	mTexMemSize = 0;
	mTexPieceWidth = 64;
	mTexPieceHeight = 64;

	//mPalette = NULL;
	mPixelFormat = PixelFormat_Unknown;
	mImageFlags = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
TextureData::~TextureData()
{
	ReleaseTextures();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void TextureData::ReleaseTextures()
{
	for(int i=0; i<(int)mTextures.size(); i++)
	{
		GLuint* aSurface = &mTextures[i].mTexture;
		glDeleteTextures(1, aSurface);
	}

	mTextures.clear();

	mTexMemSize = 0;

	/*
	if (mPalette!=NULL)
	{
		mPalette->Release();
		mPalette = NULL;
	}
	*/
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void TextureData::CreateTextureDimensions(MemoryImage *theImage)
{
	int aWidth = theImage->GetWidth();
	int aHeight = theImage->GetHeight();
	unsigned int i;

	// Calculate inner piece sizes
	mTexPieceWidth = aWidth;
	mTexPieceHeight = aHeight;
	bool usePow2 = true; //gTextureSizeMustBePow2 || mPixelFormat==PixelFormat_Palette8;
	GetBestTextureDimensions(mTexPieceWidth, mTexPieceHeight, false, usePow2, mImageFlags);

	// Calculate right boundary piece sizes
	int aRightWidth = aWidth % mTexPieceWidth;
	int aRightHeight = mTexPieceHeight;
	if (aRightWidth > 0)
		GetBestTextureDimensions(aRightWidth, aRightHeight, true, usePow2, mImageFlags);
	else
		aRightWidth = mTexPieceWidth;

	// Calculate bottom boundary piece sizes
	int aBottomWidth = mTexPieceWidth;
	int aBottomHeight = aHeight % mTexPieceHeight;
	if (aBottomHeight > 0)
		GetBestTextureDimensions(aBottomWidth, aBottomHeight, true, usePow2, mImageFlags);
	else
		aBottomHeight = mTexPieceHeight;

	// Calculate corner piece size
	int aCornerWidth = aRightWidth;
	int aCornerHeight = aBottomHeight;
	GetBestTextureDimensions(aCornerWidth, aCornerHeight, true, usePow2, mImageFlags);

	// Allocate texture array
	mTexVecWidth = (aWidth + mTexPieceWidth - 1) / mTexPieceWidth;
	mTexVecHeight = (aHeight + mTexPieceHeight - 1) / mTexPieceHeight;
	mTextures.resize(mTexVecWidth * mTexVecHeight);

	// Assign inner pieces
	for (i = 0; i < mTextures.size(); i++)
	{
		TextureDataPiece& aPiece = mTextures[i];
		aPiece.mTexture = 0;
		aPiece.mWidth = mTexPieceWidth;
		aPiece.mHeight = mTexPieceHeight;
	}

	// Assign right pieces
	for (i = mTexVecWidth - 1; i < mTextures.size(); i += mTexVecWidth)
	{
		TextureDataPiece& aPiece = mTextures[i];
		aPiece.mWidth = aRightWidth;
		aPiece.mHeight = aRightHeight;
	}

	// Assign bottom pieces
	for (i = mTexVecWidth * (mTexVecHeight - 1); i < mTextures.size(); i++)
	{
		TextureDataPiece& aPiece = mTextures[i];
		aPiece.mWidth = aBottomWidth;
		aPiece.mHeight = aBottomHeight;
	}

	// Assign corner piece
	mTextures.back().mWidth = aCornerWidth;
	mTextures.back().mHeight = aCornerHeight;
	/**/

	mMaxTotalU = aWidth / (float)mTexPieceWidth;
	mMaxTotalV = aHeight / (float)mTexPieceHeight;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void TextureData::CreateTextures(MemoryImage *theImage)
{
	theImage->DeleteSWBuffers(); // don't need these buffers for 3d drawing

	// Choose appropriate pixel format
	PixelFormat aFormat = PixelFormat_A8R8G8B8;
	//theImage->mD3DFlags = D3DImageFlag_UseA4R4G4B4;

	theImage->CommitBits();
	if (!theImage->mHasAlpha && !theImage->mHasTrans && (gSupportedPixelFormats & PixelFormat_R5G6B5))
	{
		if (!(theImage->mD3DFlags & D3DImageFlag_UseA8R8G8B8))
			aFormat = PixelFormat_R5G6B5;
	}

	if (theImage->mColorIndices != NULL && (gSupportedPixelFormats & PixelFormat_Palette8))
	{
		/*
		palEntry aPalette[256];
		for (int i=0; i<256; i++)
		{
			uint32_t aPixel = theImage->mColorTable[i];
			*(uint32_t*)(aPalette+i) = (aPixel&0xFF00FF00) | ((aPixel>>16)&0xFF) | ((aPixel<<16)&0xFF0000);
		}
		HRESULT aResult = theDraw->CreatePalette(DDPCAPS_8BIT | DDPCAPS_ALPHA | DDPCAPS_ALLOW256,aPalette, &aDDPalette, NULL);
		if (SUCCEEDED(aResult))
			aFormat = PixelFormat_Palette8;
		else
		{
			std::string anError = GetDirectXErrorString(aResult);
			gSupportedPixelFormats &= ~PixelFormat_Palette8;
		}
		*/
	}

	if ((theImage->mD3DFlags & D3DImageFlag_UseA4R4G4B4) && aFormat==PixelFormat_A8R8G8B8 && (gSupportedPixelFormats & PixelFormat_A4R4G4B4))
		aFormat = PixelFormat_A4R4G4B4;

	if (aFormat==PixelFormat_A8R8G8B8 && !(gSupportedPixelFormats & PixelFormat_A8R8G8B8))
		aFormat = PixelFormat_A4R4G4B4;

	// Release texture if image size has changed
	bool createTextures = false;
	if (mWidth!=theImage->mWidth || mHeight!=theImage->mHeight || aFormat!=mPixelFormat || theImage->mD3DFlags!=mImageFlags)
	{
		ReleaseTextures();

		mPixelFormat = aFormat;
		mImageFlags = theImage->mD3DFlags;
		CreateTextureDimensions(theImage);
		createTextures = true;
	}

	int i,x,y;

	int aHeight = theImage->GetHeight();
	int aWidth = theImage->GetWidth();

	int aFormatSize = 4;
	if (aFormat==PixelFormat_R5G6B5)
		aFormatSize = 2;
	else if (aFormat==PixelFormat_A4R4G4B4)
		aFormatSize = 2;

	i=0;
	for(y=0; y<aHeight; y+=mTexPieceHeight)
	{
		for(x=0; x<aWidth; x+=mTexPieceWidth, i++)
		{
			TextureDataPiece &aPiece = mTextures[i];
			if (createTextures)
			{
				glGenTextures(1, &aPiece.mTexture);
				glBindTexture(GL_TEXTURE_2D, aPiece.mTexture);
				mTexMemSize += aPiece.mWidth*aPiece.mHeight*aFormatSize;
			}
			else if (aPiece.mTexture)
				glBindTexture(GL_TEXTURE_2D, aPiece.mTexture);

			CopyImageToTexture(aPiece.mTexture,theImage,x,y,aPiece.mWidth,aPiece.mHeight,aFormat, createTextures);
		}
	}

	mWidth = theImage->mWidth;
	mHeight = theImage->mHeight;
	mBitsChangedCount = theImage->mBitsChangedCount;
	mPixelFormat = aFormat;
}
	
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void TextureData::CheckCreateTextures(MemoryImage *theImage)
{
	if(mPixelFormat==PixelFormat_Unknown || theImage->mWidth != mWidth || theImage->mHeight != mHeight || theImage->mBitsChangedCount != mBitsChangedCount || theImage->mD3DFlags != mImageFlags)
		CreateTextures(theImage);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
GLuint TextureData::GetTexture(int x, int y, int &width, int &height, float &u1, float &v1, float &u2, float &v2)
{
	int tx = x/mTexPieceWidth;
	int ty = y/mTexPieceHeight;

	TextureDataPiece &aPiece = mTextures[ty*mTexVecWidth + tx];

	int left = x%mTexPieceWidth;
	int top = y%mTexPieceHeight;
	int right = left+width;
	int bottom = top+height;

	if(right > aPiece.mWidth)
		right = aPiece.mWidth;

	if(bottom > aPiece.mHeight)
		bottom = aPiece.mHeight;

	width = right-left;
	height = bottom-top;

	u1 = (float)left/aPiece.mWidth;
	v1 = (float)top/aPiece.mHeight;
	u2 = (float)right/aPiece.mWidth;
	v2 = (float)bottom/aPiece.mHeight;

	return aPiece.mTexture;
}

static void SetLinearFilter(bool linear)
{
	if (gLinearFilter != linear)
	{
		int aFilter = (linear) ? GL_LINEAR : GL_NEAREST;

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, aFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, aFilter);

		gLinearFilter = linear;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
GLuint TextureData::GetTextureF(float x, float y, float &width, float &height, float &u1, float &v1, float &u2, float &v2)
{
	int tx = x/mTexPieceWidth;
	int ty = y/mTexPieceHeight;

	TextureDataPiece &aPiece = mTextures[ty*mTexVecWidth + tx];

	float left = x - tx*mTexPieceWidth;
	float top = y - ty*mTexPieceHeight;
	float right = left+width;
	float bottom = top+height;

	if(right > aPiece.mWidth)
		right = aPiece.mWidth;

	if(bottom > aPiece.mHeight)
		bottom = aPiece.mHeight;

	width = right-left;
	height = bottom-top;

	u1 = (float)left/aPiece.mWidth;
	v1 = (float)top/aPiece.mHeight;
	u2 = (float)right/aPiece.mWidth;
	v2 = (float)bottom/aPiece.mHeight;

	return aPiece.mTexture;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void TextureData::Blt(float theX, float theY, const Rect& theSrcRect, const Color& theColor)
{
	int srcLeft = theSrcRect.mX;
	int srcTop = theSrcRect.mY;
	int srcRight = srcLeft + theSrcRect.mWidth;
	int srcBottom = srcTop + theSrcRect.mHeight;
	int srcX, srcY;
	float dstX, dstY;
	int aWidth,aHeight;
	float u1,v1,u2,v2;

	srcY = srcTop;
	dstY = theY;

	//long unsigned int aColor = RGBA_MAKE(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);

	if ((srcLeft >= srcRight) || (srcTop >= srcBottom))
		return;

	glEnable(GL_TEXTURE_2D);

	while(srcY < srcBottom)
	{
		srcX = srcLeft;
		dstX = theX;
		while(srcX < srcRight)
		{
			aWidth = srcRight-srcX;
			aHeight = srcBottom-srcY;
			GLuint aTexture = GetTexture(srcX, srcY, aWidth, aHeight, u1, v1, u2, v2);

			float x = dstX - 0.5f;
			float y = dstY - 0.5f;

			glBindTexture(GL_TEXTURE_2D, aTexture);

			glBegin(GL_TRIANGLE_STRIP);
				glColor4ub(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
				glTexCoord2f(u1, v1);
				glVertex2f(x, y);

				glColor4ub(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
				glTexCoord2f(u1, v2);
				glVertex2f(x, y+aHeight);

				glColor4ub(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
				glTexCoord2f(u2, v1);
				glVertex2f(x+aWidth, y);

				glColor4ub(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
				glTexCoord2f(u2, v2);
				glVertex2f(x+aWidth, y+aHeight);
			glEnd();

			srcX += aWidth;
			dstX += aWidth;
		}

		srcY += aHeight;
		dstY += aHeight;
	}
}

void TextureData::BltTransformed(const SexyMatrix3 &theTrans, const Rect& theSrcRect, const Color& theColor, const Rect *theClipRect, float theX, float theY, bool center)
{
	int srcLeft = theSrcRect.mX;
	int srcTop = theSrcRect.mY;
	int srcRight = srcLeft + theSrcRect.mWidth;
	int srcBottom = srcTop + theSrcRect.mHeight;
	int srcX, srcY;
	float dstX, dstY;
	int aWidth;
	int aHeight;
	float u1,v1,u2,v2;
	float startx = 0, starty = 0;
	float pixelcorrect = 0.5f;

	if (center)
	{
		startx = -theSrcRect.mWidth/2.0f;
		starty = -theSrcRect.mHeight/2.0f;
		pixelcorrect = 0.0f;
	}			

	srcY = srcTop;
	dstY = starty;

	//long unsigned int aColor = RGBA_MAKE(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);			

	if ((srcLeft >= srcRight) || (srcTop >= srcBottom))
		return;

	glEnable(GL_TEXTURE_2D);

	while(srcY < srcBottom)
	{
		srcX = srcLeft;
		dstX = startx;
		while(srcX < srcRight)
		{
			aWidth = srcRight-srcX;
			aHeight = srcBottom-srcY;
			GLuint aTexture = GetTexture(srcX, srcY, aWidth, aHeight, u1, v1, u2, v2);

			float x = dstX; // - 0.5f;
			float y = dstY; // - 0.5f;

			SexyVector2 p[4] = { SexyVector2(x, y), SexyVector2(x,y+aHeight), SexyVector2(x+aWidth, y) , SexyVector2(x+aWidth, y+aHeight) };
			SexyVector2 tp[4];

			int i;
			for (i=0; i<4; i++)
			{
				tp[i] = theTrans*p[i];
				tp[i].x -= pixelcorrect - theX;
				tp[i].y -= pixelcorrect - theY;
			}

			bool clipped = false;
			if (theClipRect != NULL)
			{
				int left = theClipRect->mX;
				int right = left + theClipRect->mWidth;
				int top = theClipRect->mY;
				int bottom = top + theClipRect->mHeight;
				for (i=0; i<4; i++)
				{
					if (tp[i].x<left || tp[i].x>=right || tp[i].y<top || tp[i].y>=bottom)
					{
						clipped = true;
						break;
					}
				}
			}

			glBindTexture(GL_TEXTURE_2D, aTexture);

			if (!clipped)
			{
				glBegin(GL_TRIANGLE_STRIP);
					glColor4ub(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
					glTexCoord2f(u1, v1);
					glVertex2f(tp[0].x, tp[0].y);

					glColor4ub(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
					glTexCoord2f(u1, v2);
					glVertex2f(tp[1].x, tp[1].y);

					glColor4ub(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
					glTexCoord2f(u2, v1);
					glVertex2f(tp[2].x, tp[2].y);

					glColor4ub(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
					glTexCoord2f(u2, v2);
					glVertex2f(tp[3].x, tp[3].y);
				glEnd();
			}
			else
			{
				/*
				VertexList aList;
				aList.push_back(aVertex[0]);
				aList.push_back(aVertex[1]);
				aList.push_back(aVertex[3]);
				aList.push_back(aVertex[2]);

				DrawPolyClipped(theClipRect, aList);
				*/
//				DrawPolyClipped(theDevice, theClipRect, aVertex+1, 3);
			}

//			D3DInterface::CheckDXError(theDevice->SetTexture(0, NULL),"SetTexture NULL");

			srcX += aWidth;
			dstX += aWidth;
		}

		srcY += aHeight;
		dstY += aHeight;
	}
}

#define GetColorFromTriVertex(theVertex, theColor) (theVertex.color?theVertex.color:theColor)

void TextureData::BltTriangles(const TriVertex theVertices[][3], int theNumTriangles, unsigned int theColor, float tx, float ty)
{
	if ((mMaxTotalU <= 1.0) && (mMaxTotalV <= 1.0))
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, mTextures[0].mTexture);

		glBegin(GL_TRIANGLES);

		for (int aTriangleNum = 0; aTriangleNum < theNumTriangles; aTriangleNum++)
		{
			TriVertex* aTriVerts = (TriVertex*) theVertices[aTriangleNum];

			for (int i = 0; i < 3; i++)
			{
				unsigned int aColor = GetColorFromTriVertex(aTriVerts[i], theColor);
				uint8_t r = (aColor >> 0) & 0xff;
				uint8_t g = (aColor >> 8) & 0xff;
				uint8_t b = (aColor >> 16) & 0xff;
				//uint8_t a = (aColor >> 24) & 0xff;

				glColor3ub(r, g, b);
				glTexCoord2f(aTriVerts[i].u * mMaxTotalU, aTriVerts[i].v * mMaxTotalV);
				glVertex2f(aTriVerts[i].x + tx, aTriVerts[i].y + ty);
			}
		}

		glEnd();
	}
	else
	{
		printf("bruh %d\n", theNumTriangles);
		for (int aTriangleNum = 0; aTriangleNum < theNumTriangles; aTriangleNum++)
		{
			TriVertex* aTriVerts = (TriVertex*) theVertices[aTriangleNum];
		}
	}
}


GLInterface::GLInterface(SexyAppBase* theApp)
{
	mApp = theApp;
	mWidth = mApp->mWidth;
	mHeight = mApp->mHeight;
	mDisplayWidth = mWidth;
	mDisplayHeight = mHeight;

	mPresentationRect = Rect( 0, 0, mWidth, mHeight );

	SDL_DisplayMode aMode;
	SDL_GetCurrentDisplayMode(SDL_GetWindowDisplayIndex(mApp->mSDLWindow), &aMode);
	mRefreshRate = aMode.refresh_rate;
	if (!mRefreshRate) mRefreshRate = 60;
	mMillisecondsPerFrame = 1000/mRefreshRate;

	mScreenImage = 0;

	mNextCursorX = 0;
	mNextCursorY = 0;
	mCursorX = 0;
	mCursorY = 0;
}

GLInterface::~GLInterface()
{
	Flush();

	ImageSet::iterator anItr;
	for(anItr = mImageSet.begin(); anItr != mImageSet.end(); ++anItr)
	{
		MemoryImage *anImage = *anItr;
		delete (TextureData*)anImage->mD3DData;
		anImage->mD3DData = NULL;
	}
}

void GLInterface::SetDrawMode(int theDrawMode)
{
	if (theDrawMode == Graphics::DRAWMODE_NORMAL)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else // Additive
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	}
}

void GLInterface::AddGLImage(GLImage* theGLImage)
{
	AutoCrit anAutoCrit(mCritSect);

	mGLImageSet.insert(theGLImage);
}

void GLInterface::RemoveGLImage(GLImage* theGLImage)
{
	AutoCrit anAutoCrit(mCritSect);

	GLImageSet::iterator anItr = mGLImageSet.find(theGLImage);
	if (anItr != mGLImageSet.end())
		mGLImageSet.erase(anItr);
}

void GLInterface::Remove3DData(MemoryImage* theImage)
{
	if (theImage->mD3DData != NULL)
	{
		delete (TextureData*)theImage->mD3DData;
		theImage->mD3DData = NULL;

		AutoCrit aCrit(mCritSect); // Make images thread safe
		mImageSet.erase(theImage);
	}
}

GLImage* GLInterface::GetScreenImage()
{
	return mScreenImage;
}

static void GLAPIENTRY MessageCallback(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
		type, severity, message );
	fflush(stderr);
}

void GLInterface::UpdateViewport()
{
	// Restrict to 4:3
	// https://bumbershootsoft.wordpress.com/2018/11/29/forcing-an-aspect-ratio-in-3d-with-opengl/

	int width, viewport_width;
    int height, viewport_height;
    int viewport_x = 0;
    int viewport_y = 0;

    SDL_GL_GetDrawableSize(mApp->mSDLWindow, &width, &height);

    glClear(GL_COLOR_BUFFER_BIT);
	Flush();

    viewport_width = width;
    viewport_height = height;
    if (width * 3 > height * 4)
	{
        viewport_width = height * 4 / 3;
        viewport_x = (width - viewport_width) / 2;
    }
	else if (width * 3 < height * 4)
	{
        viewport_height = width * 3 / 4;
        viewport_y = (height - viewport_height) / 2;
    }
    glViewport(viewport_x, viewport_y, viewport_width, viewport_height);

	glClear(GL_COLOR_BUFFER_BIT);
	Flush();
}

int GLInterface::Init(bool IsWindowed)
{
	int aMaxSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &aMaxSize);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	gTextureSizeMustBePow2 = false;
	gMinTextureWidth = 8;
	gMinTextureHeight = 8;
	gMaxTextureWidth = aMaxSize;
	gMaxTextureHeight = aMaxSize;
	gMaxTextureAspectRatio = 1;
	gSupportedPixelFormats = PixelFormat_A8R8G8B8 | PixelFormat_A4R4G4B4 | PixelFormat_R5G6B5 | PixelFormat_Palette8;
	gLinearFilter = false;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, mWidth, mHeight, 0, -10, 10);

	glEnable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glDisable(GL_DITHER);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);

	mRedBits = 8;
	mGreenBits = 8;
	mBlueBits = 8;
	
	mRedShift = 0;
	mGreenShift = 8;
	mBlueShift = 16;

	mRedMask = (0xFFU << mRedShift);
	mGreenMask = (0xFFU << mGreenShift);
	mBlueMask = (0xFFU << mBlueShift);

	SetVideoOnlyDraw(false);

	return 1;
}

bool GLInterface::Redraw(Rect* theClipRect)
{
	Flush();
	return true;
}

void GLInterface::SetVideoOnlyDraw(bool videoOnly)
{
	if (mScreenImage) delete mScreenImage;
	mScreenImage = new GLImage(this);
	//mScreenImage->SetSurface(useSecondary ? mSecondarySurface : mDrawSurface);		
	//mScreenImage->mNoLock = mVideoOnlyDraw;
	//mScreenImage->mVideoMemory = mVideoOnlyDraw;
	mScreenImage->mWidth = mWidth;
	mScreenImage->mHeight = mHeight;
	mScreenImage->SetImageMode(false, false);
}

void GLInterface::SetCursorPos(int theCursorX, int theCursorY)
{
	mNextCursorX = theCursorX;
	mNextCursorY = theCursorY;
}

bool GLInterface::PreDraw()
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	return true;
}

void GLInterface::Flush()
{
	SDL_GL_SwapWindow(mApp->mSDLWindow);
}

bool GLInterface::CreateImageTexture(MemoryImage *theImage)
{
	bool wantPurge = false;

	if(theImage->mD3DData==NULL)
	{
		theImage->mD3DData = new TextureData();
		
		// The actual purging was deferred
		wantPurge = theImage->mPurgeBits;

		AutoCrit aCrit(mCritSect); // Make images thread safe
		mImageSet.insert(theImage);
	}

	TextureData *aData = (TextureData*)theImage->mD3DData;
	aData->CheckCreateTextures(theImage);
	
	if (wantPurge)
		theImage->PurgeBits();

	return aData->mPixelFormat != PixelFormat_Unknown;
}

bool GLInterface::RecoverBits(MemoryImage* theImage)
{
	if (theImage->mD3DData == NULL)
		return false;

	TextureData* aData = (TextureData*) theImage->mD3DData;
	if (aData->mBitsChangedCount != theImage->mBitsChangedCount) // bits have changed since texture was created
		return false;

	printf("recover\n");
	fflush(stdout);
	for (int aPieceRow = 0; aPieceRow < aData->mTexVecHeight; aPieceRow++)
	{
		for (int aPieceCol = 0; aPieceCol < aData->mTexVecWidth; aPieceCol++)
		{
			TextureDataPiece* aPiece = &aData->mTextures[aPieceRow*aData->mTexVecWidth + aPieceCol];

			int offx = aPieceCol*aData->mTexPieceWidth;
			int offy = aPieceRow*aData->mTexPieceHeight;
			int aWidth = std::min(theImage->mWidth-offx, aPiece->mWidth);
			int aHeight = std::min(theImage->mHeight-offy, aPiece->mHeight);

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, aPiece->mTexture);

			glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, theImage->GetBits());

			/*
			switch (aData->mPixelFormat)
			{
			case PixelFormat_A8R8G8B8:	CopyTexture8888ToImage(aDesc.lpSurface, aDesc.lPitch, theImage, offx, offy, aWidth, aHeight); break;
			case PixelFormat_A4R4G4B4:	CopyTexture4444ToImage(aDesc.lpSurface, aDesc.lPitch, theImage, offx, offy, aWidth, aHeight); break;
			case PixelFormat_R5G6B5: CopyTexture565ToImage(aDesc.lpSurface, aDesc.lPitch, theImage, offx, offy, aWidth, aHeight); break;
			case PixelFormat_Palette8:	CopyTexturePalette8ToImage(aDesc.lpSurface, aDesc.lPitch, theImage, offx, offy, aWidth, aHeight, aData->mPalette); break;
			case PixelFormat_Unknown: break;
			}

			D3DInterface::CheckDXError(aPiece->mTexture->Unlock(NULL),"Texture Unlock");
			*/
		}
	}

	return true;
}

void GLInterface::PushTransform(const SexyMatrix3 &theTransform, bool concatenate)
{
	if (mTransformStack.empty() || !concatenate)
		mTransformStack.push_back(theTransform);
	else
	{
		SexyMatrix3 &aTrans = mTransformStack.back();
		mTransformStack.push_back(theTransform*aTrans);
	}
}

void GLInterface::PopTransform()
{
	if (!mTransformStack.empty())
		mTransformStack.pop_back();
}

void GLInterface::Blt(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode, bool linearFilter)
{
	if (!mTransformStack.empty())
	{
		BltClipF(theImage,theX,theY,theSrcRect,NULL,theColor,theDrawMode);
		return;
	}

	if (!PreDraw())
		return;

	MemoryImage* aSrcMemoryImage = (MemoryImage*) theImage;

	if (!CreateImageTexture(aSrcMemoryImage))
		return;

	SetDrawMode(theDrawMode);

	TextureData *aData = (TextureData*)aSrcMemoryImage->mD3DData;

	SetLinearFilter(linearFilter);
	aData->Blt(theX,theY,theSrcRect,theColor);
}

void GLInterface::BltClipF(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Rect *theClipRect, const Color& theColor, int theDrawMode)
{
	SexyTransform2D aTransform;
	aTransform.Translate(theX, theY);

	BltTransformed(theImage,theClipRect,theColor,theDrawMode,theSrcRect,aTransform,true);
}

void GLInterface::BltMirror(Image* theImage, float theX, float theY, const Rect& theSrcRect, const Color& theColor, int theDrawMode, bool linearFilter)
{
	SexyTransform2D aTransform;		

	aTransform.Translate(-theSrcRect.mWidth,0);
	aTransform.Scale(-1, 1);
	aTransform.Translate(theX, theY);

	BltTransformed(theImage,NULL,theColor,theDrawMode,theSrcRect,aTransform,linearFilter);
}

void GLInterface::StretchBlt(Image* theImage,  const Rect& theDestRect, const Rect& theSrcRect, const Rect* theClipRect, const Color &theColor, int theDrawMode, bool fastStretch, bool mirror)
{
	float xScale = (float)theDestRect.mWidth / theSrcRect.mWidth;
	float yScale = (float)theDestRect.mHeight / theSrcRect.mHeight;

	SexyTransform2D aTransform;
	if (mirror)
	{
		aTransform.Translate(-theSrcRect.mWidth,0);
		aTransform.Scale(-xScale, yScale);
	}
	else
		aTransform.Scale(xScale, yScale);

	aTransform.Translate(theDestRect.mX, theDestRect.mY);
	BltTransformed(theImage,theClipRect,theColor,theDrawMode,theSrcRect,aTransform,!fastStretch);
}

void GLInterface::BltRotated(Image* theImage, float theX, float theY, const Rect* theClipRect, const Color& theColor, int theDrawMode, double theRot, float theRotCenterX, float theRotCenterY, const Rect& theSrcRect)
{
	SexyTransform2D aTransform;

	aTransform.Translate(-theRotCenterX, -theRotCenterY);
	aTransform.RotateRad(theRot);
	aTransform.Translate(theX+theRotCenterX,theY+theRotCenterY);

	BltTransformed(theImage,theClipRect,theColor,theDrawMode,theSrcRect,aTransform,true);
}

void GLInterface::BltTransformed(Image* theImage, const Rect* theClipRect, const Color& theColor, int theDrawMode, const Rect &theSrcRect, const SexyMatrix3 &theTransform, bool linearFilter, float theX, float theY, bool center)
{
	if (!PreDraw())
		return;

	MemoryImage* aSrcMemoryImage = (MemoryImage*) theImage;

	if (!CreateImageTexture(aSrcMemoryImage))
		return;

	SetDrawMode(theDrawMode);

	TextureData *aData = (TextureData*)aSrcMemoryImage->mD3DData;

	if (!mTransformStack.empty())
	{
		SetLinearFilter(true); // force linear filtering in the case of a global transform
		if (theX!=0 || theY!=0)
		{
			SexyTransform2D aTransform;
			if (center)
				aTransform.Translate(-theSrcRect.mWidth/2.0f,-theSrcRect.mHeight/2.0f);

			aTransform = theTransform * aTransform;
			aTransform.Translate(theX,theY);
			aTransform = mTransformStack.back() * aTransform;

			aData->BltTransformed(aTransform, theSrcRect, theColor, theClipRect);
		}
		else
		{
			SexyTransform2D aTransform = mTransformStack.back()*theTransform;
			aData->BltTransformed(aTransform, theSrcRect, theColor, theClipRect, theX, theY, center);
		}
	}
	else
	{
		SetLinearFilter(linearFilter);
		aData->BltTransformed(theTransform, theSrcRect, theColor, theClipRect, theX, theY, center);
	}
}

void GLInterface::DrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color& theColor, int theDrawMode)
{
	if (!PreDraw())
		return;

	SetDrawMode(theDrawMode);

	float x1, y1, x2, y2;

	if (!mTransformStack.empty())
	{
		SexyVector2 p1(theStartX,theStartY);
		SexyVector2 p2(theEndX,theEndY);
		p1 = mTransformStack.back()*p1;
		p2 = mTransformStack.back()*p2;

		x1 = p1.x;
		y1 = p1.y;
		x2 = p2.x;
		y2 = p2.y;
	}
	else
	{
		x1 = theStartX;
		y1 = theStartY;
		x2 = theEndX;
		y2 = theEndY;
	}

	glDisable(GL_TEXTURE_2D);

	glBegin(GL_LINE_STRIP);
		glColor4ub(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
		glVertex2f(x1, y1);
		glColor4ub(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
		glVertex2f(x2, y2);
		glColor4ub(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
		glVertex2f(x2+0.5f, y2+0.5f);
	glEnd();
}

void GLInterface::FillRect(const Rect& theRect, const Color& theColor, int theDrawMode)
{
	if (!PreDraw())
		return;

	SetDrawMode(theDrawMode);

	float x = theRect.mX - 0.5f;
	float y = theRect.mY - 0.5f;
	float aWidth = theRect.mWidth;
	float aHeight = theRect.mHeight;

	float aVertex[4][2] = {
		{x, y},
		{x, y+aHeight},
		{x+aWidth, y},
		{x+aWidth, y+aHeight},
	};

	if (!mTransformStack.empty())
	{
		SexyVector2 p[4] = { SexyVector2(x, y), SexyVector2(x,y+aHeight), SexyVector2(x+aWidth, y) , SexyVector2(x+aWidth, y+aHeight) };

		int i;
		for (i=0; i<4; i++)
		{
			p[i] = mTransformStack.back()*p[i];
			p[i].x -= 0.5f;
			p[i].y -= 0.5f;
			aVertex[i][0] = p[i].x;
			aVertex[i][1] = p[i].y;
		}
	}

	glDisable(GL_TEXTURE_2D);

	glBegin(GL_TRIANGLE_STRIP);
		for (int i=0; i<4; i++)
		{
			glColor4ub(theColor.mRed, theColor.mGreen, theColor.mBlue, theColor.mAlpha);
			glVertex2f(aVertex[i][0], aVertex[i][1]);
		}
	glEnd();
}

void GLInterface::DrawTriangle(const TriVertex &p1, const TriVertex &p2, const TriVertex &p3, const Color &theColor, int theDrawMode)
{
	if (!PreDraw())
		return;

	SetDrawMode(theDrawMode);

	unsigned int aColor = (theColor.mRed << 0) | (theColor.mGreen << 8) | (theColor.mBlue << 16) | (theColor.mAlpha << 24);
	Color col1(GetColorFromTriVertex(p1, aColor));
	Color col2(GetColorFromTriVertex(p1, aColor));
	Color col3(GetColorFromTriVertex(p1, aColor));

	glDisable(GL_TEXTURE_2D);

	glBegin(GL_TRIANGLE_STRIP);
		glColor4ub(col1.mRed, col1.mGreen, col1.mBlue, col1.mAlpha);
		glVertex2f(p1.x, p1.y);

		glColor4ub(col2.mRed, col2.mGreen, col2.mBlue, col2.mAlpha);
		glVertex2f(p2.x, p2.y);

		glColor4ub(col3.mRed, col3.mGreen, col3.mBlue, col3.mAlpha);
		glVertex2f(p3.x, p3.y);
	glEnd();
}

void GLInterface::DrawTriangleTex(const TriVertex &p1, const TriVertex &p2, const TriVertex &p3, const Color &theColor, int theDrawMode, Image *theTexture, bool blend)
{
	TriVertex aVertices[1][3] = {{p1, p2, p3}};
	DrawTrianglesTex(aVertices,1,theColor,theDrawMode,theTexture,blend);
}

void GLInterface::DrawTrianglesTex(const TriVertex theVertices[][3], int theNumTriangles, const Color &theColor, int theDrawMode, Image *theTexture, float tx, float ty, bool blend)
{
	if (!PreDraw()) return;

	MemoryImage* aSrcMemoryImage = (MemoryImage*)theTexture;

	if (!CreateImageTexture(aSrcMemoryImage))
		return;

	SetDrawMode(theDrawMode);

	TextureData *aData = (TextureData*)aSrcMemoryImage->mD3DData;

	SetLinearFilter(blend);

	unsigned int aColor = (theColor.mRed << 0) | (theColor.mGreen << 8) | (theColor.mBlue << 16) | (theColor.mAlpha << 24);
	aData->BltTriangles(theVertices, theNumTriangles, aColor, tx, ty);
}

void GLInterface::DrawTrianglesTexStrip(const TriVertex theVertices[], int theNumTriangles, const Color &theColor, int theDrawMode, Image *theTexture, float tx, float ty, bool blend)
{
	TriVertex aList[100][3];
	int aTriNum = 0;
	while (aTriNum < theNumTriangles)
	{
		int aMaxTriangles = std::min(100,theNumTriangles - aTriNum);
		for (int i=0; i<aMaxTriangles; i++)
		{
			aList[i][0] = theVertices[aTriNum];
			aList[i][1] = theVertices[aTriNum+1];
			aList[i][2] = theVertices[aTriNum+2];
			aTriNum++;
		}
		DrawTrianglesTex(aList,aMaxTriangles,theColor,theDrawMode,theTexture, tx, ty, blend);
	}
}

void GLInterface::FillPoly(const Point theVertices[], int theNumVertices, const Rect *theClipRect, const Color &theColor, int theDrawMode, int tx, int ty)
{
	
}