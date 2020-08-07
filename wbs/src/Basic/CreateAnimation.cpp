#include "StdAfx.h"
#include "CreateAnimation.h"
#include <windows.h>
#include <stdio.h>

#pragma pack(push,gifpacking,1)

typedef struct {
	char cSignature[3]; // Must be 'GIF'
	char cVersion[3];   // Must be '89a'
} GIF_HEADER;

typedef struct { // 7 bytes
	unsigned short iWidth;
	unsigned short iHeight;
	unsigned char iSizeOfGct : 3;
	unsigned char iSortFlag : 1;
	unsigned char iColorResolution : 3;
	unsigned char iGctFlag : 1;
	unsigned char iBackgroundColorIndex;
	unsigned char iPixelAspectRatio;
} GIF_LOGICAL_SCREEN_DESCRIPTOR;

typedef struct { // 6 bytes
	unsigned char iBlockSize;           // Must be 4
	unsigned char iTransparentColorFlag : 1;
	unsigned char iUserInputFlag : 1;
	unsigned char iDisposalMethod : 3;
	unsigned char iReserved : 3;
	unsigned short iDelayTime;
	unsigned char iTransparentColorIndex;
	unsigned char iBlockTerminator;     // Must be 0
} GIF_GRAPHIC_CONTROL_EXTENSION;

typedef struct { // 9 bytes
	unsigned short iLeft;
	unsigned short iTop;
	unsigned short iWidth;
	unsigned short iHeight;
	unsigned char iSizeOfLct : 3;
	unsigned char iReserved : 2;
	unsigned char iSortFlag : 1;
	unsigned char iInterlaceFlag : 1;
	unsigned char iLctFlag : 1;
} GIF_IMAGE_DESCRIPTOR;

#pragma pack(pop,gifpacking)


unsigned short iGctSize[] = { 6,12,24,48,96,192,384,768 };

namespace WBSF
{

	ERMsg MakeGIF(std::string output_file_path, const std::vector<std::string>& file_list, unsigned short delay, bool bLoop)
	{
		ERMsg msg;

		DWORD dw=0;

		GIF_HEADER gh;
		GIF_LOGICAL_SCREEN_DESCRIPTOR glsd1;
		ZeroMemory(&glsd1, sizeof(GIF_LOGICAL_SCREEN_DESCRIPTOR));

		HANDLE hFileOut = CreateFileA(output_file_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
		if (hFileOut == INVALID_HANDLE_VALUE)
		{
			msg.ajoute("Could not open file " + output_file_path + ". GIF creation aborted.");
			return msg;
		}


		strncpy((char *)&gh, "GIF89a", 6);
		WriteFile(hFileOut, &gh, sizeof(GIF_HEADER), &dw, 0);
		WriteFile(hFileOut, &glsd1, sizeof(GIF_LOGICAL_SCREEN_DESCRIPTOR), &dw, 0);
		if (bLoop) {
			WriteFile(hFileOut, "\41\377\013NETSCAPE2.0\003\001\377\377\0", 19, &dw, 0);
		}
		for (DWORD dwIndex = 0; dwIndex < (unsigned)file_list.size(); dwIndex++) 
		{
			GIF_LOGICAL_SCREEN_DESCRIPTOR glsd;
			GIF_GRAPHIC_CONTROL_EXTENSION ggce;
			GIF_IMAGE_DESCRIPTOR gid;
			ZeroMemory(&glsd, sizeof(GIF_LOGICAL_SCREEN_DESCRIPTOR));
			ZeroMemory(&ggce, sizeof(GIF_GRAPHIC_CONTROL_EXTENSION));
			ZeroMemory(&gid, sizeof(GIF_IMAGE_DESCRIPTOR));
			HANDLE hFileIn = CreateFileA(file_list[dwIndex].c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
			if (hFileIn == INVALID_HANDLE_VALUE) {
				msg.ajoute("Could not open file" + file_list[dwIndex] + ". GIF creation aborted.");
				CloseHandle(hFileOut);
				return msg;
			}
			ReadFile(hFileIn, &gh, sizeof(GIF_HEADER), &dw, 0);
			if (strncmp(gh.cSignature, "GIF", 3) || (strncmp(gh.cVersion, "89a", 3) && strncmp(gh.cVersion, "87a", 3))) {
				msg.ajoute("Not a GIF file, or incorrect version number.");
				msg.ajoute(file_list[dwIndex]);
				CloseHandle(hFileIn);
				CloseHandle(hFileOut);
				return msg;
			}
			ReadFile(hFileIn, &glsd, sizeof(GIF_LOGICAL_SCREEN_DESCRIPTOR), &dw, 0);
			char szColorTable[768];
			if (glsd.iGctFlag) ReadFile(hFileIn, szColorTable, iGctSize[glsd.iSizeOfGct], &dw, 0);
			if (glsd1.iWidth < glsd.iWidth) glsd1.iWidth = glsd.iWidth;
			if (glsd1.iHeight < glsd.iHeight) glsd1.iHeight = glsd.iHeight;
			for (;;) {
				unsigned char c = 0;
				ReadFile(hFileIn, &c, 1, &dw, 0);
				if (dw == 0) {
					msg.ajoute("Premature end of file encountered; no GIF image data present.");
					msg.ajoute(file_list[dwIndex]);
					
					CloseHandle(hFileIn);
					CloseHandle(hFileOut);
					return msg;
				}
				if (c == 0x2C) {
					ReadFile(hFileIn, &gid, sizeof(GIF_IMAGE_DESCRIPTOR), &dw, 0);
					if (gid.iLctFlag) {
						ReadFile(hFileIn, szColorTable, iGctSize[gid.iSizeOfLct], &dw, 0);
					}
					else {
						gid.iLctFlag = 1;
						gid.iSizeOfLct = glsd.iSizeOfGct;
					}
					break;
				}
				else if (c == 0x21) {
					ReadFile(hFileIn, &c, 1, &dw, 0);
					if (c == 0xF9) {
						ReadFile(hFileIn, &ggce, sizeof(GIF_GRAPHIC_CONTROL_EXTENSION), &dw, 0);
					}
					else {
						for (;;) {
							ReadFile(hFileIn, &c, 1, &dw, 0);
							if (!c) break;
							SetFilePointer(hFileIn, c, 0, FILE_CURRENT);
						}
					}
				}
			}
			ggce.iBlockSize = 4;
			ggce.iDelayTime = (unsigned short)delay;
			ggce.iDisposalMethod = 2;
			unsigned char c = (char)0x21;
			WriteFile(hFileOut, &c, 1, &dw, 0);
			c = (char)0xF9;
			WriteFile(hFileOut, &c, 1, &dw, 0);
			WriteFile(hFileOut, &ggce, sizeof(GIF_GRAPHIC_CONTROL_EXTENSION), &dw, 0);
			c = (char)0x2C;
			WriteFile(hFileOut, &c, 1, &dw, 0);
			WriteFile(hFileOut, &gid, sizeof(GIF_IMAGE_DESCRIPTOR), &dw, 0);
			WriteFile(hFileOut, szColorTable, iGctSize[gid.iSizeOfLct], &dw, 0);
			ReadFile(hFileIn, &c, 1, &dw, 0);
			WriteFile(hFileOut, &c, 1, &dw, 0);
			for (;;) {
				ReadFile(hFileIn, &c, 1, &dw, 0);
				WriteFile(hFileOut, &c, 1, &dw, 0);
				if (!c) break;
				ReadFile(hFileIn, szColorTable, c, &dw, 0);
				WriteFile(hFileOut, szColorTable, c, &dw, 0);
			}
			CloseHandle(hFileIn);
		}
		unsigned char c = (char)0x3B;
		WriteFile(hFileOut, &c, 1, &dw, 0);
		SetFilePointer(hFileOut, 6, 0, FILE_BEGIN);
		WriteFile(hFileOut, &glsd1, sizeof(GIF_LOGICAL_SCREEN_DESCRIPTOR), &dw, 0);
		CloseHandle(hFileOut);


		return msg;
	}
	

}