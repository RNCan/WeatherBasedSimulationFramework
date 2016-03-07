#pragma once

#include "basic/ERMsg.h"

#define ERROR_USER_BASE 0x00001000
enum TError {   ERROR_NOINPUT_FILE = 0x00000021,  //no input file
                ERROR_UNABLE_OPEN_IDS,            //unable to open ids file
                ERROR_UNABLE_OPEN_TGO,            //unable to open tgo file
                ERROR_UNABLE_OPEN_IDO,           //unable to open ido file to write
                ERROR_NOT_ENOUGHT_MEMORY,         //not enough memory
                ERROR_NO_PRECIPITATION,           //Weather file does not contain precipitation
				ERROR_NO_PRECIPITATION_FOR_RAD,   //Weather file does not contain precipitation to compute radiation
                ERROR_NO_RADIATION,               //  Weather file does not contain radiation
                ERROR_BAD_NUMBER_PARAMETER,
                ERROR_BAD_DAY_NUMBER,
				ERROR_BAD_IDS_FORMAT,			//BioSImModelBase needs new file transfer 
				ERROR_REF_NOT_SUPPORT,
				ERROR_BAD_STREAM_FORMAT,
				ERROR_HXGRID_NOT_SUPPORTED,
				ERROR_DATA_NOT_FOUND,
				ERROR_INVALID_LIST_INDEX
            };


enum TLanguage { UNKNOWN_LANGUAGE=-1, FRENCH, ENGLISH };

short GetLanguage();
void SetLanguage(short language);



ERMsg GetErrorMessage(int errorID, short language=UNKNOWN_LANGUAGE);