/******************************************************************************
 *
 * Project:  GDAL
 * Purpose:  Implementation of a set of GDALDerivedPixelFunc(s) to be used
 *           with source raster band of virtual GDAL datasets.
 * Author:   Rémi Saint-Amant from Antonio Valentino code
 *
 ******************************************************************************
 * Copyright (c) 2008-2014 Antonio Valentino
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

#include <math.h>
#include <gdal.h>

enum { B1, B2, B3, B4, B5, B6, B7 }; 

static const double INDEX_MULT = 10000.0;
static const double INDEX_DIV = 1.0;

__int16 LimitToInt16(double value)
{
	return (__int16)fmax(-32767, fmin(32767, value));
}

__int16 LimitToIndex(double value)
{
	return (__int16)fmax(-INDEX_MULT, fmin(INDEX_MULT, value));
}

CPLErr NBR(void **papoSources, int nSources, void *pData,
	int nXSize, int nYSize,
	GDALDataType eSrcType, GDALDataType eBufType,
	int nPixelSpace, int nLineSpace)
{
	int ii, iLine, iCol;
	__int16 pix_val;
	__int16 b[7] = { 0 };

	// ---- Init ----
	if (nSources < 7) return CE_Failure;


	// ---- Set pixels ----
	for (iLine = 0; iLine < nYSize; iLine++)
	{
		for (iCol = 0; iCol < nXSize; iCol++)
		{
			ii = iLine * nXSize + iCol;
			/* Source raster pixels may be obtained with SRCVAL macro */
			//b[B1] = (__int16)(SRCVAL(papoSources[B1], eSrcType, ii));
			//b[B2] = (__int16)(SRCVAL(papoSources[B2], eSrcType, ii));
			//b[B3] = (__int16)(SRCVAL(papoSources[B3], eSrcType, ii));
			b[B4] = (__int16)(SRCVAL(papoSources[B4], eSrcType, ii));
			//b[B5] = (__int16)(SRCVAL(papoSources[B5], eSrcType, ii));
			//b[B6] = (__int16)(SRCVAL(papoSources[B6], eSrcType, ii));
			b[B7] = (__int16)(SRCVAL(papoSources[B7], eSrcType, ii));


			pix_val = -32768;
			//if (b[B4] == 0 && b[B7] == 0)
				//pix_val = 0;
			//else 
			if (b[B4] > -32768 && b[B7]>-32768 )
				pix_val = LimitToIndex(INDEX_MULT * ((double)b[B4] - b[B7]) / fmax(INDEX_DIV, (double)(b[B4] + b[B7])));

			GDALCopyWords(&pix_val, GDT_Int16, 0,
				((GByte *)pData) + nLineSpace * iLine + iCol * nPixelSpace,
				eBufType, nPixelSpace, 1);
		}
	}

	// ---- Return success ----
	return CE_None;
}

CPLErr NBR2(void **papoSources, int nSources, void *pData,
	int nXSize, int nYSize,
	GDALDataType eSrcType, GDALDataType eBufType,
	int nPixelSpace, int nLineSpace)
{
	int ii, iLine, iCol;
	__int16 pix_val;
	__int16 b[7] = { 0 };

	// ---- Init ----
	if (nSources < 7) return CE_Failure;


	// ---- Set pixels ----
	for (iLine = 0; iLine < nYSize; iLine++)
	{
		for (iCol = 0; iCol < nXSize; iCol++)
		{
			ii = iLine * nXSize + iCol;
			/* Source raster pixels may be obtained with SRCVAL macro */
			//b[B1] = (__int16)(SRCVAL(papoSources[B1], eSrcType, ii));
			//b[B2] = (__int16)(SRCVAL(papoSources[B2], eSrcType, ii));
			//b[B3] = (__int16)(SRCVAL(papoSources[B3], eSrcType, ii));
			//b[B4] = (__int16)(SRCVAL(papoSources[B4], eSrcType, ii));
			b[B5] = (__int16)(SRCVAL(papoSources[B5], eSrcType, ii));
			//b[B6] = (__int16)(SRCVAL(papoSources[B6], eSrcType, ii));
			b[B7] = (__int16)(SRCVAL(papoSources[B7], eSrcType, ii));


			pix_val = -32768;
			//if (b[B5] == 0 && b[B7] == 0)
				//pix_val = 0;
			//else 
			if (b[B5] > -32768 && b[B7]>-32768 )
				pix_val = LimitToIndex(INDEX_MULT * ((double)b[B5] - b[B7]) / max(INDEX_DIV, (double)(b[B5] + b[B7])));



			GDALCopyWords(&pix_val, GDT_Int16, 0,
				((GByte *)pData) + nLineSpace * iLine + iCol * nPixelSpace,
				eBufType, nPixelSpace, 1);
		}
	}

	// ---- Return success ----
	return CE_None;
}

CPLErr EVI(void **papoSources, int nSources, void *pData,
int nXSize, int nYSize,
GDALDataType eSrcType, GDALDataType eBufType,
int nPixelSpace, int nLineSpace)
{
	int ii, iLine, iCol;
	__int16 pix_val;
	__int16 b[7] = { 0 };

	// ---- Init ----
	if (nSources < 7) return CE_Failure;


	// ---- Set pixels ----
	for (iLine = 0; iLine < nYSize; iLine++)
	{
		for (iCol = 0; iCol < nXSize; iCol++)
		{
			ii = iLine * nXSize + iCol;
			/* Source raster pixels may be obtained with SRCVAL macro */
			b[B1] = (__int16)(SRCVAL(papoSources[B1], eSrcType, ii));
			//b[B2] = (__int16)(SRCVAL(papoSources[B2], eSrcType, ii));
			b[B3] = (__int16)(SRCVAL(papoSources[B3], eSrcType, ii));
			b[B4] = (__int16)(SRCVAL(papoSources[B4], eSrcType, ii));
			//b[B5] = (__int16)(SRCVAL(papoSources[B5], eSrcType, ii));
			//b[B6] = (__int16)(SRCVAL(papoSources[B6], eSrcType, ii));
			//b[B7] = (__int16)(SRCVAL(papoSources[B7], eSrcType, ii));


			pix_val = -32768;
			if (b[B1] > -32768 && b[B3] > -32768 && b[B4]>-32768) 
				pix_val = LimitToIndex(INDEX_MULT * 2.5 * ((double)b[B4] - b[B3]) / max(INDEX_DIV, (b[B4] + 6 * b[B3] - 7.5*b[B1] + 1)));
			


			GDALCopyWords(&pix_val, GDT_Int16, 0,
				((GByte *)pData) + nLineSpace * iLine + iCol * nPixelSpace,
				eBufType, nPixelSpace, 1);
		}
	}

	// ---- Return success ----
	return CE_None;
}


CPLErr SAVI(void **papoSources, int nSources, void *pData,
int nXSize, int nYSize,
GDALDataType eSrcType, GDALDataType eBufType,
int nPixelSpace, int nLineSpace)
{
	int ii, iLine, iCol;
	__int16 pix_val;
	__int16 b[7] = { 0 };

	// ---- Init ----
	if (nSources < 7) return CE_Failure;


	// ---- Set pixels ----
	for (iLine = 0; iLine < nYSize; iLine++)
	{
		for (iCol = 0; iCol < nXSize; iCol++)
		{
			ii = iLine * nXSize + iCol;
			/* Source raster pixels may be obtained with SRCVAL macro */
			//b[B1] = (__int16)(SRCVAL(papoSources[B1], eSrcType, ii));
			//b[B2] = (__int16)(SRCVAL(papoSources[B2], eSrcType, ii));
			b[B3] = (__int16)(SRCVAL(papoSources[B3], eSrcType, ii));
			b[B4] = (__int16)(SRCVAL(papoSources[B4], eSrcType, ii));
			//b[B5] = (__int16)(SRCVAL(papoSources[B5], eSrcType, ii));
			//b[B6] = (__int16)(SRCVAL(papoSources[B6], eSrcType, ii));
			//b[B7] = (__int16)(SRCVAL(papoSources[B7], eSrcType, ii));


			pix_val = -32768;
			if (b[B3] > -32768 && b[B4]>-32768 )//&& (b[B4] + b[B3] + 0.5) != 0
				pix_val = LimitToIndex(INDEX_MULT * 1.5 * ((double)b[B4] - b[B3]) / max(INDEX_DIV, (b[B4] + b[B3] + 0.5)));


			GDALCopyWords(&pix_val, GDT_Int16, 0,
				((GByte *)pData) + nLineSpace * iLine + iCol * nPixelSpace,
				eBufType, nPixelSpace, 1);
		}
	}

	// ---- Return success ----
	return CE_None;
}

CPLErr MSAVI(void **papoSources, int nSources, void *pData,
	int nXSize, int nYSize,
	GDALDataType eSrcType, GDALDataType eBufType,
	int nPixelSpace, int nLineSpace)
{
	int ii, iLine, iCol;
	__int16 pix_val;
	__int16 b[7] = { 0 };

	// ---- Init ----
	if (nSources < 7) return CE_Failure;


	// ---- Set pixels ----
	for (iLine = 0; iLine < nYSize; iLine++)
	{
		for (iCol = 0; iCol < nXSize; iCol++)
		{
			ii = iLine * nXSize + iCol;
			/* Source raster pixels may be obtained with SRCVAL macro */
			//b[B1] = (__int16)(SRCVAL(papoSources[B1], eSrcType, ii));
			//b[B2] = (__int16)(SRCVAL(papoSources[B2], eSrcType, ii));
			b[B3] = (__int16)(SRCVAL(papoSources[B3], eSrcType, ii));
			b[B4] = (__int16)(SRCVAL(papoSources[B4], eSrcType, ii));
			//b[B5] = (__int16)(SRCVAL(papoSources[B5], eSrcType, ii));
			//b[B6] = (__int16)(SRCVAL(papoSources[B6], eSrcType, ii));
			//b[B7] = (__int16)(SRCVAL(papoSources[B7], eSrcType, ii));


			pix_val = -32768;
			if (b[B3] > -32768 && b[B4]>-32768 )
				pix_val = LimitToIndex(INDEX_MULT * (2.0 * b[B4] + 1 - sqrt((2 * b[B4] + 1)*(2 * b[B4] + 1) - 8 * (b[B4] - b[B3]))) / 2);
			

			GDALCopyWords(&pix_val, GDT_Int16, 0,
				((GByte *)pData) + nLineSpace * iLine + iCol * nPixelSpace,
				eBufType, nPixelSpace, 1);
		}
	}

	// ---- Return success ----
	return CE_None;
}



CPLErr NDVI(void **papoSources, int nSources, void *pData,
	int nXSize, int nYSize,
	GDALDataType eSrcType, GDALDataType eBufType,
	int nPixelSpace, int nLineSpace)
{
	int ii, iLine, iCol;
	__int16 pix_val;
	__int16 b[7] = { 0 };

	// ---- Init ----
	if (nSources < 7) return CE_Failure;


	// ---- Set pixels ----
	for (iLine = 0; iLine < nYSize; iLine++)
	{
		for (iCol = 0; iCol < nXSize; iCol++)
		{
			ii = iLine * nXSize + iCol;
			/* Source raster pixels may be obtained with SRCVAL macro */
			//b[B1] = (__int16)(SRCVAL(papoSources[B1], eSrcType, ii));
			//b[B2] = (__int16)(SRCVAL(papoSources[B2], eSrcType, ii));
			b[B3] = (__int16)(SRCVAL(papoSources[B3], eSrcType, ii));
			b[B4] = (__int16)(SRCVAL(papoSources[B4], eSrcType, ii));
			//b[B5] = (__int16)(SRCVAL(papoSources[B5], eSrcType, ii));
			//b[B6] = (__int16)(SRCVAL(papoSources[B6], eSrcType, ii));
			//b[B7] = (__int16)(SRCVAL(papoSources[B7], eSrcType, ii));

			pix_val = -32768;
			if (b[B3] > -32768 && b[B4]>-32768 )//&& (b[B4] + b[B3]) != 0
				pix_val = LimitToIndex(INDEX_MULT * ((double)b[B4] - b[B3]) / max(INDEX_DIV, (double)(b[B4] + b[B3])));


			GDALCopyWords(&pix_val, GDT_Int16, 0,
				((GByte *)pData) + nLineSpace * iLine + iCol * nPixelSpace,
				eBufType, nPixelSpace, 1);
		}
	}

	// ---- Return success ----
	return CE_None;
}

CPLErr NDMI(void **papoSources, int nSources, void *pData,
	int nXSize, int nYSize,
	GDALDataType eSrcType, GDALDataType eBufType,
	int nPixelSpace, int nLineSpace)
{
	int ii, iLine, iCol;
	__int16 pix_val;
	__int16 b[7] = { 0 };

	// ---- Init ----
	if (nSources < 7) return CE_Failure;


	// ---- Set pixels ----
	for (iLine = 0; iLine < nYSize; iLine++)
	{
		for (iCol = 0; iCol < nXSize; iCol++)
		{
			ii = iLine * nXSize + iCol;
			/* Source raster pixels may be obtained with SRCVAL macro */
			//b[B1] = (__int16)(SRCVAL(papoSources[B1], eSrcType, ii));
			//b[B2] = (__int16)(SRCVAL(papoSources[B2], eSrcType, ii));
			//b[B3] = (__int16)(SRCVAL(papoSources[B3], eSrcType, ii));
			b[B4] = (__int16)(SRCVAL(papoSources[B4], eSrcType, ii));
			b[B5] = (__int16)(SRCVAL(papoSources[B5], eSrcType, ii));
			//b[B6] = (__int16)(SRCVAL(papoSources[B6], eSrcType, ii));
			//b[B7] = (__int16)(SRCVAL(papoSources[B7], eSrcType, ii));



			pix_val = -32768;
			if (b[B4] > -32768 && b[B5]>-32768 )//&& (b[B4] + b[B5]) != 0
				pix_val = LimitToIndex(INDEX_MULT * ((double)b[B4] - b[B5]) / max(INDEX_DIV, (double)(b[B4] + b[B5])));

			GDALCopyWords(&pix_val, GDT_Int16, 0,
				((GByte *)pData) + nLineSpace * iLine + iCol * nPixelSpace,
				eBufType, nPixelSpace, 1);
		}
	}

	// ---- Return success ----
	return CE_None;
}


CPLErr TCB(void **papoSources, int nSources, void *pData,
	int nXSize, int nYSize,
	GDALDataType eSrcType, GDALDataType eBufType,
	int nPixelSpace, int nLineSpace)
{
	int ii, iLine, iCol;
	__int16 pix_val;
	__int16 b[7] = { 0 };

	// ---- Init ----
	if (nSources < 7) return CE_Failure;


	// ---- Set pixels ----
	for (iLine = 0; iLine < nYSize; iLine++)
	{
		for (iCol = 0; iCol < nXSize; iCol++)
		{
			ii = iLine * nXSize + iCol;
			/* Source raster pixels may be obtained with SRCVAL macro */
			b[B1] = (__int16)(SRCVAL(papoSources[B1], eSrcType, ii));
			b[B2] = (__int16)(SRCVAL(papoSources[B2], eSrcType, ii));
			b[B3] = (__int16)(SRCVAL(papoSources[B3], eSrcType, ii));
			b[B4] = (__int16)(SRCVAL(papoSources[B4], eSrcType, ii));
			b[B5] = (__int16)(SRCVAL(papoSources[B5], eSrcType, ii));
			//			b[B6] = (__int16)(SRCVAL(papoSources[B6], eSrcType, ii));
			b[B7] = (__int16)(SRCVAL(papoSources[B7], eSrcType, ii));


			pix_val = -32768;
			if (b[B1] > -32768 && b[B2]>-32768 && b[B3]>-32768 && b[B4] > -32768 && b[B5] > -32768 && b[B7] > -32768)
			{
				static const double F[7] = { 0.2043, 0.4158, 0.5524, 0.5741, 0.3124, 0.0000, 0.2303 };
				pix_val = LimitToInt16(F[B1] * b[B1] + F[B2] * b[B2] + F[B3] * b[B3] + F[B4] * b[B4] + F[B5] * b[B5] + F[B7] * b[B7]);
			}

			GDALCopyWords(&pix_val, GDT_Int16, 0,
				((GByte *)pData) + nLineSpace * iLine + iCol * nPixelSpace,
				eBufType, nPixelSpace, 1);
		}
	}

	// ---- Return success ----
	return CE_None;
}



CPLErr TCG(void **papoSources, int nSources, void *pData,
	int nXSize, int nYSize,
	GDALDataType eSrcType, GDALDataType eBufType,
	int nPixelSpace, int nLineSpace)
{
	int ii, iLine, iCol;
	__int16 pix_val;
	__int16 b[7] = { 0 };

	// ---- Init ----
	if (nSources < 7) return CE_Failure;


	// ---- Set pixels ----
	for (iLine = 0; iLine < nYSize; iLine++)
	{
		for (iCol = 0; iCol < nXSize; iCol++)
		{
			ii = iLine * nXSize + iCol;
			/* Source raster pixels may be obtained with SRCVAL macro */
			b[B1] = (__int16)(SRCVAL(papoSources[B1], eSrcType, ii));
			b[B2] = (__int16)(SRCVAL(papoSources[B2], eSrcType, ii));
			b[B3] = (__int16)(SRCVAL(papoSources[B3], eSrcType, ii));
			b[B4] = (__int16)(SRCVAL(papoSources[B4], eSrcType, ii));
			b[B5] = (__int16)(SRCVAL(papoSources[B5], eSrcType, ii));
			//			b[B6] = (__int16)(SRCVAL(papoSources[B6], eSrcType, ii));
			b[B7] = (__int16)(SRCVAL(papoSources[B7], eSrcType, ii));


			pix_val = -32768;
			if (b[B1] > -32768 && b[B2]>-32768 && b[B3]>-32768 && b[B4] > -32768 && b[B5] > -32768 && b[B7] > -32768)
			{

				static const double F[7] = { -0.1603, -0.2819, -0.4934, 0.7940, 0.0002, 0.000, -0.1446 };
				pix_val = LimitToInt16(F[B1] * b[B1] + F[B2] * b[B2] + F[B3] * b[B3] + F[B4] * b[B4] + F[B5] * b[B5] + F[B7] * b[B7]);
			}

			GDALCopyWords(&pix_val, GDT_Int16, 0,
				((GByte *)pData) + nLineSpace * iLine + iCol * nPixelSpace,
				eBufType, nPixelSpace, 1);
		}
	}

	// ---- Return success ----
	return CE_None;
}


CPLErr TCW(void **papoSources, int nSources, void *pData,
	int nXSize, int nYSize,
	GDALDataType eSrcType, GDALDataType eBufType,
	int nPixelSpace, int nLineSpace)
{
	int ii, iLine, iCol;
	__int16 pix_val;
	__int16 b[7] = { 0 };

	// ---- Init ----
	if (nSources < 7) return CE_Failure;


	// ---- Set pixels ----
	for (iLine = 0; iLine < nYSize; iLine++)
	{
		for (iCol = 0; iCol < nXSize; iCol++)
		{
			ii = iLine * nXSize + iCol;
			/* Source raster pixels may be obtained with SRCVAL macro */
			b[B1] = (__int16)(SRCVAL(papoSources[B1], eSrcType, ii));
			b[B2] = (__int16)(SRCVAL(papoSources[B2], eSrcType, ii));
			b[B3] = (__int16)(SRCVAL(papoSources[B3], eSrcType, ii));
			b[B4] = (__int16)(SRCVAL(papoSources[B4], eSrcType, ii));
			b[B5] = (__int16)(SRCVAL(papoSources[B5], eSrcType, ii));
			//			b[B6] = (__int16)(SRCVAL(papoSources[B6], eSrcType, ii));
			b[B7] = (__int16)(SRCVAL(papoSources[B7], eSrcType, ii));


			pix_val = -32768;
			if (b[B1] > -32768 && b[B2]>-32768 && b[B3]>-32768 && b[B4] > -32768 && b[B5] > -32768 && b[B7] > -32768)
			{
				static const double F[7] = { 0.0315, 0.2021, 0.3102, 0.1594, 0.6806, 0.000, -0.6109 };
				pix_val = LimitToInt16(F[B1] * b[B1] + F[B2] * b[B2] + F[B3] * b[B3] + F[B4] * b[B4] + F[B5] * b[B5] + F[B7] * b[B7]);
			}

			GDALCopyWords(&pix_val, GDT_Int16, 0,
				((GByte *)pData) + nLineSpace * iLine + iCol * nPixelSpace,
				eBufType, nPixelSpace, 1);
		}
	}

	// ---- Return success ----
	return CE_None;
}




CPLErr SR(void **papoSources, int nSources, void *pData,
	int nXSize, int nYSize,
	GDALDataType eSrcType, GDALDataType eBufType,
	int nPixelSpace, int nLineSpace)
{
	int ii, iLine, iCol;
	__int16 pix_val;
	__int16 b[7] = { 0 };

	// ---- Init ----
	if (nSources < 7) return CE_Failure;


	// ---- Set pixels ----
	for (iLine = 0; iLine < nYSize; iLine++)
	{
		for (iCol = 0; iCol < nXSize; iCol++)
		{
			ii = iLine * nXSize + iCol;
			/* Source raster pixels may be obtained with SRCVAL macro */
			//b[B1] = (__int16)(SRCVAL(papoSources[B1], eSrcType, ii));
			//b[B2] = (__int16)(SRCVAL(papoSources[B2], eSrcType, ii));
			b[B3] = (__int16)(SRCVAL(papoSources[B3], eSrcType, ii));
			b[B4] = (__int16)(SRCVAL(papoSources[B4], eSrcType, ii));
			//b[B5] = (__int16)(SRCVAL(papoSources[B5], eSrcType, ii));
			//b[B6] = (__int16)(SRCVAL(papoSources[B6], eSrcType, ii));
			//b[B7] = (__int16)(SRCVAL(papoSources[B7], eSrcType, ii));

			pix_val = -32768;
			if (b[B3] > -32768 && b[B4]>-32768 )
			{
				pix_val = LimitToIndex(INDEX_MULT * ((double)b[B4] / max(INDEX_DIV, (double)b[B3])));
			}

			GDALCopyWords(&pix_val, GDT_Int16, 0,
				((GByte *)pData) + nLineSpace * iLine + iCol * nPixelSpace,
				eBufType, nPixelSpace, 1);
		}
	}

	// ---- Return success ----
	return CE_None;
}




CPLErr CL(void **papoSources, int nSources, void *pData,
	int nXSize, int nYSize,
	GDALDataType eSrcType, GDALDataType eBufType,
	int nPixelSpace, int nLineSpace)
{
	int ii, iLine, iCol;
	__int16 pix_val;
	__int16 b[7] = { 0 };

	// ---- Init ----
	if (nSources < 7) return CE_Failure;


	// ---- Set pixels ----
	for (iLine = 0; iLine < nYSize; iLine++)
	{
		for (iCol = 0; iCol < nXSize; iCol++)
		{
			ii = iLine * nXSize + iCol;
			/* Source raster pixels may be obtained with SRCVAL macro */
			b[B1] = (__int16)(SRCVAL(papoSources[B1], eSrcType, ii));
			//b[B2] = (__int16)(SRCVAL(papoSources[B2], eSrcType, ii));
			//b[B3] = (__int16)(SRCVAL(papoSources[B3], eSrcType, ii));
			//b[B4] = (__int16)(SRCVAL(papoSources[B4], eSrcType, ii));
			//b[B5] = (__int16)(SRCVAL(papoSources[B5], eSrcType, ii));
			b[B6] = (__int16)(SRCVAL(papoSources[B6], eSrcType, ii));
			//				 b[B7] = (__int16)(SRCVAL(papoSources[B7], eSrcType, ii));


			pix_val = -32768;
			if (b[B1] > -32768 && b[B6]>-32768 )
			{
				pix_val = LimitToIndex(INDEX_MULT * ((double)b[B1] / max(INDEX_DIV, (double)b[B6])));
			}

			GDALCopyWords(&pix_val, GDT_Int16, 0,
				((GByte *)pData) + nLineSpace * iLine + iCol * nPixelSpace,
				eBufType, nPixelSpace, 1);
		}
	}

	// ---- Return success ----
	return CE_None;
}


CPLErr HZ(void **papoSources, int nSources, void *pData,
	int nXSize, int nYSize,
	GDALDataType eSrcType, GDALDataType eBufType,
	int nPixelSpace, int nLineSpace)
{
	int ii, iLine, iCol;
	__int16 pix_val;
	__int16 b[7] = { 0 };

	// ---- Init ----
	if (nSources < 7) return CE_Failure;


	// ---- Set pixels ----
	for (iLine = 0; iLine < nYSize; iLine++)
	{
		for (iCol = 0; iCol < nXSize; iCol++)
		{
			ii = iLine * nXSize + iCol;
			/* Source raster pixels may be obtained with SRCVAL macro */
			b[B1] = (__int16)(SRCVAL(papoSources[B1], eSrcType, ii));
			//				 b[B2] = (__int16)(SRCVAL(papoSources[B2], eSrcType, ii));
			b[B3] = (__int16)(SRCVAL(papoSources[B3], eSrcType, ii));
			//b[B4] = (__int16)(SRCVAL(papoSources[B4], eSrcType, ii));
			//b[B5] = (__int16)(SRCVAL(papoSources[B5], eSrcType, ii));
			//b[B6] = (__int16)(SRCVAL(papoSources[B6], eSrcType, ii));
			//b[B7] = (__int16)(SRCVAL(papoSources[B7], eSrcType, ii));

			pix_val = -32768;
			if (b[B1] > -32768 && b[B3]>-32768 )
			{
				pix_val = LimitToIndex(INDEX_MULT * ((double)b[B1] / max(INDEX_DIV, (double)b[B3])));
			}

			GDALCopyWords(&pix_val, GDT_Int16, 0,
				((GByte *)pData) + nLineSpace * iLine + iCol * nPixelSpace,
				eBufType, nPixelSpace, 1);
		}
	}

	// ---- Return success ----
	return CE_None;
}

CPLErr red_natural(void **papoSources, int nSources, void *pData,
	int nXSize, int nYSize,
	GDALDataType eSrcType, GDALDataType eBufType,
	int nPixelSpace, int nLineSpace)
{
	int ii, iLine, iCol;
	__int16 pix_val;
	__int16 b[7] = { 0 };

	// ---- Init ----
	if (nSources < 7) return CE_Failure;


	// ---- Set pixels ----
	for (iLine = 0; iLine < nYSize; iLine++)
	{
		for (iCol = 0; iCol < nXSize; iCol++)
		{
			ii = iLine * nXSize + iCol;
			/* Source raster pixels may be obtained with SRCVAL macro */
			b[B1] = (__int16)(SRCVAL(papoSources[B1], eSrcType, ii));
			b[B2] = (__int16)(SRCVAL(papoSources[B2], eSrcType, ii));
			b[B3] = (__int16)(SRCVAL(papoSources[B3], eSrcType, ii));
			//b[B4] = (__int16)(SRCVAL(papoSources[B4], eSrcType, ii));
			//b[B5] = (__int16)(SRCVAL(papoSources[B5], eSrcType, ii));
			//b[B6] = (__int16)(SRCVAL(papoSources[B6], eSrcType, ii));
			//b[B7] = (__int16)(SRCVAL(papoSources[B7], eSrcType, ii));

			pix_val = -32768;
			if (b[B1] > -32768 && b[B2] > -32768 && b[B3] > -32768)
			{
				pix_val = (__int16)(max(0.0, min(254.0, ((b[B3]-90)/ (1000.0-90)) * 254.0)));
			}

			GDALCopyWords(&pix_val, GDT_Int16, 0,
				((GByte *)pData) + nLineSpace * iLine + iCol * nPixelSpace,
				eBufType, nPixelSpace, 1);
		}
	}

	// ---- Return success ----
	return CE_None;
}

CPLErr green_natural(void **papoSources, int nSources, void *pData,
	int nXSize, int nYSize,
	GDALDataType eSrcType, GDALDataType eBufType,
	int nPixelSpace, int nLineSpace)
{
	int ii, iLine, iCol;
	__int16 pix_val;
	__int16 b[7] = { 0 };

	// ---- Init ----
	if (nSources < 7) return CE_Failure;


	// ---- Set pixels ----
	for (iLine = 0; iLine < nYSize; iLine++)
	{
		for (iCol = 0; iCol < nXSize; iCol++)
		{
			ii = iLine * nXSize + iCol;
			/* Source raster pixels may be obtained with SRCVAL macro */
			b[B1] = (__int16)(SRCVAL(papoSources[B1], eSrcType, ii));
			b[B2] = (__int16)(SRCVAL(papoSources[B2], eSrcType, ii));
			b[B3] = (__int16)(SRCVAL(papoSources[B3], eSrcType, ii));
			//b[B4] = (__int16)(SRCVAL(papoSources[B4], eSrcType, ii));
			//b[B5] = (__int16)(SRCVAL(papoSources[B5], eSrcType, ii));
			//b[B6] = (__int16)(SRCVAL(papoSources[B6], eSrcType, ii));
			//b[B7] = (__int16)(SRCVAL(papoSources[B7], eSrcType, ii));

			pix_val = -32768;
			if (b[B1] > -32768 && b[B2] > -32768 && b[B3] > -32768)
			{
				pix_val = (__int16)(max(0.0, min(254.0, ((b[B2]-170) / (1050.0-170)) * 254.0)));
			}

			GDALCopyWords(&pix_val, GDT_Int16, 0,
				((GByte *)pData) + nLineSpace * iLine + iCol * nPixelSpace,
				eBufType, nPixelSpace, 1);
		}
	}

	// ---- Return success ----
	return CE_None;
}

CPLErr blue_natural(void **papoSources, int nSources, void *pData,
	int nXSize, int nYSize,
	GDALDataType eSrcType, GDALDataType eBufType,
	int nPixelSpace, int nLineSpace)
{
	int ii, iLine, iCol;
	__int16 pix_val;
	__int16 b[7] = { 0 };

	// ---- Init ----
	if (nSources < 7) return CE_Failure;


	// ---- Set pixels ----
	for (iLine = 0; iLine < nYSize; iLine++)
	{
		for (iCol = 0; iCol < nXSize; iCol++)
		{
			ii = iLine * nXSize + iCol;
			/* Source raster pixels may be obtained with SRCVAL macro */
			b[B1] = (__int16)(SRCVAL(papoSources[B1], eSrcType, ii));
			b[B2] = (__int16)(SRCVAL(papoSources[B2], eSrcType, ii));
			b[B3] = (__int16)(SRCVAL(papoSources[B3], eSrcType, ii));
			//b[B4] = (__int16)(SRCVAL(papoSources[B4], eSrcType, ii));
			//b[B5] = (__int16)(SRCVAL(papoSources[B5], eSrcType, ii));
			//b[B6] = (__int16)(SRCVAL(papoSources[B6], eSrcType, ii));
			//b[B7] = (__int16)(SRCVAL(papoSources[B7], eSrcType, ii));

			pix_val = -32768;
			if (b[B1] > -32768 && b[B2] > -32768 && b[B3] > -32768)
			{
				pix_val = (__int16)(max(0.0, min(254.0, ((b[B1]-130)/ (780.0-130)) * 254.0)));
			}

			GDALCopyWords(&pix_val, GDT_Int16, 0,
				((GByte *)pData) + nLineSpace * iLine + iCol * nPixelSpace,
				eBufType, nPixelSpace, 1);
		}
	}

	// ---- Return success ----
	return CE_None;
}


CPLErr red_LandWater(void **papoSources, int nSources, void *pData,
	int nXSize, int nYSize,
	GDALDataType eSrcType, GDALDataType eBufType,
	int nPixelSpace, int nLineSpace)
{
	int ii, iLine, iCol;
	__int16 pix_val;
	__int16 b[7] = { 0 };

	// ---- Init ----
	if (nSources < 7) return CE_Failure;


	// ---- Set pixels ----
	for (iLine = 0; iLine < nYSize; iLine++)
	{
		for (iCol = 0; iCol < nXSize; iCol++)
		{
			ii = iLine * nXSize + iCol;
			/* Source raster pixels may be obtained with SRCVAL macro */
			//b[B1] = (__int16)(SRCVAL(papoSources[B1], eSrcType, ii));
			//				 b[B2] = (__int16)(SRCVAL(papoSources[B2], eSrcType, ii));
			b[B3] = (__int16)(SRCVAL(papoSources[B3], eSrcType, ii));
			b[B4] = (__int16)(SRCVAL(papoSources[B4], eSrcType, ii));
			b[B5] = (__int16)(SRCVAL(papoSources[B5], eSrcType, ii));
			//b[B6] = (__int16)(SRCVAL(papoSources[B6], eSrcType, ii));
			//b[B7] = (__int16)(SRCVAL(papoSources[B7], eSrcType, ii));

			pix_val = -32768;
			if (b[B3] > -32768 && b[B4] > -32768 && b[B5] > -32768)
			{
				pix_val = (__int16)(max(0.0, min(254.0, ((b[B4] + 150.0) / 6150.0) * 254.0)));
			}

			GDALCopyWords(&pix_val, GDT_Int16, 0,
				((GByte *)pData) + nLineSpace * iLine + iCol * nPixelSpace,
				eBufType, nPixelSpace, 1);
		}
	}

	// ---- Return success ----
	return CE_None;
}

CPLErr green_LandWater(void **papoSources, int nSources, void *pData,
	int nXSize, int nYSize,
	GDALDataType eSrcType, GDALDataType eBufType,
	int nPixelSpace, int nLineSpace)
{
	int ii, iLine, iCol;
	__int16 pix_val;
	__int16 b[7] = { 0 };

	// ---- Init ----
	if (nSources < 7) return CE_Failure;


	// ---- Set pixels ----
	for (iLine = 0; iLine < nYSize; iLine++)
	{
		for (iCol = 0; iCol < nXSize; iCol++)
		{
			ii = iLine * nXSize + iCol;
			/* Source raster pixels may be obtained with SRCVAL macro */
			//b[B1] = (__int16)(SRCVAL(papoSources[B1], eSrcType, ii));
			//b[B2] = (__int16)(SRCVAL(papoSources[B2], eSrcType, ii));
			b[B3] = (__int16)(SRCVAL(papoSources[B3], eSrcType, ii));
			b[B4] = (__int16)(SRCVAL(papoSources[B4], eSrcType, ii));
			b[B5] = (__int16)(SRCVAL(papoSources[B5], eSrcType, ii));
			//b[B6] = (__int16)(SRCVAL(papoSources[B6], eSrcType, ii));
			//b[B7] = (__int16)(SRCVAL(papoSources[B7], eSrcType, ii));

			pix_val = -32768;
			if (b[B3] > -32768 && b[B4] > -32768 && b[B5] > -32768)
			{
				pix_val = (__int16)(max(0.0, min(254.0, ((b[B5] + 190.0) / 5190.0) * 254.0)));
			}

			GDALCopyWords(&pix_val, GDT_Int16, 0,
				((GByte *)pData) + nLineSpace * iLine + iCol * nPixelSpace,
				eBufType, nPixelSpace, 1);
		}
	}

	// ---- Return success ----
	return CE_None;
}

CPLErr blue_LandWater(void **papoSources, int nSources, void *pData,
	int nXSize, int nYSize,
	GDALDataType eSrcType, GDALDataType eBufType,
	int nPixelSpace, int nLineSpace)
{
	int ii, iLine, iCol;
	__int16 pix_val;
	__int16 b[7] = { 0 };

	// ---- Init ----
	if (nSources < 7) return CE_Failure;


	// ---- Set pixels ----
	for (iLine = 0; iLine < nYSize; iLine++)
	{
		for (iCol = 0; iCol < nXSize; iCol++)
		{
			ii = iLine * nXSize + iCol;
			/* Source raster pixels may be obtained with SRCVAL macro */
			//b[B1] = (__int16)(SRCVAL(papoSources[B1], eSrcType, ii));
			//				 b[B2] = (__int16)(SRCVAL(papoSources[B2], eSrcType, ii));
			b[B3] = (__int16)(SRCVAL(papoSources[B3], eSrcType, ii));
			b[B4] = (__int16)(SRCVAL(papoSources[B4], eSrcType, ii));
			b[B5] = (__int16)(SRCVAL(papoSources[B5], eSrcType, ii));
			//b[B6] = (__int16)(SRCVAL(papoSources[B6], eSrcType, ii));
			//b[B7] = (__int16)(SRCVAL(papoSources[B7], eSrcType, ii));

			pix_val = -32768;
			if (b[B3] > -32768 && b[B4] > -32768 && b[B5] > -32768)
			{
				pix_val = (__int16)(max(0.0, min(254.0, ((b[B3] + 200.0) / 2700.0) * 254.0)));
			}

			GDALCopyWords(&pix_val, GDT_Int16, 0,
				((GByte *)pData) + nLineSpace * iLine + iCol * nPixelSpace,
				eBufType, nPixelSpace, 1);
		}
	}

	// ---- Return success ----
	return CE_None;
}


/************************************************************************/
/*                     GDALRegisterDefaultPixelFunc()                   */
/************************************************************************/

/**
 * This adds a default set of pixel functions to the global list of
 * available pixel functions for derived bands:
 *
 * - "NBR": Normalized Burn Ratio. (B4-B7)/(B4+B7)
 * - "NBR2": Normalized Burn Ratio 2. (B5-B7)/(B5+B7)
 * - "EVI": Enhanced Vegetation Index.  2.5 * ((B4–B3)/(B4 + 6*B3 – 7.5*B1 + 1))
 * - "SAVI": Soil Adjusted Vegetation Index. 
 * - "MSAVI": Modified Soil Adjusted Vegetation Index
 * - "NDVI": Normalized Difference Vegetation Index. (B4-B3)/(B4+B3)
 * - "NDMI": Normalized Difference Moisture Index. (B4-B5)/(B4+B5)
 * - "TCB": Tassel Cap Brightness.
 * - "TCG": Tassel Cap Greenness.
 * - "TCW": Tassel Cap Wetness.
 * - "SR": B4/B3
 * - "CL": B1/B6
 * - "HZ": B1/B3
 *
 * @see GDALAddDerivedBandPixelFunc
 *
 * @return CE_None, invalid (NULL) parameters are currently ignored.
 */
CPLErr CPL_STDCALL GDALRegisterLandsatIndices()
{

	GDALAddDerivedBandPixelFunc("Landsat.NBR", NBR);
	GDALAddDerivedBandPixelFunc("Landsat.NBR2", NBR2);
	GDALAddDerivedBandPixelFunc("Landsat.EVI", EVI);
	GDALAddDerivedBandPixelFunc("Landsat.SAVI", SAVI);
	GDALAddDerivedBandPixelFunc("Landsat.MSAVI", MSAVI);
	GDALAddDerivedBandPixelFunc("Landsat.NDVI", NDVI);
	GDALAddDerivedBandPixelFunc("Landsat.NDMI", NDMI);
	GDALAddDerivedBandPixelFunc("Landsat.TCB", TCB);
	GDALAddDerivedBandPixelFunc("Landsat.TCG", TCG);
	GDALAddDerivedBandPixelFunc("Landsat.TCW", TCW);
	GDALAddDerivedBandPixelFunc("Landsat.SR", SR);
	GDALAddDerivedBandPixelFunc("Landsat.CL", CL);
	GDALAddDerivedBandPixelFunc("Landsat.HZ", HZ);
	GDALAddDerivedBandPixelFunc("Landsat.red(natural)", red_natural);
	GDALAddDerivedBandPixelFunc("Landsat.green(natural)", green_natural);
	GDALAddDerivedBandPixelFunc("Landsat.blue(natural)", blue_natural);
	GDALAddDerivedBandPixelFunc("Landsat.red(LandWater)", red_LandWater);
	GDALAddDerivedBandPixelFunc("Landsat.green(LandWater)", green_LandWater);
	GDALAddDerivedBandPixelFunc("Landsat.blue(LandWater)", blue_LandWater);

	return CE_None;
}


///landsat 8 color
//Natural Color	4 3 2
//False Color(urban)	7 6 4
//Color Infrared(vegetation)	5 4 3
//Agriculture	6 5 2
//Atmospheric Penetration	7 6 5
//Healthy Vegetation	5 6 2
//Land / Water	5 6 4
//Natural With Atmospheric Removal	7 5 3
//Shortwave Infrared	7 5 4
//Vegetation Analysis	6 5 4