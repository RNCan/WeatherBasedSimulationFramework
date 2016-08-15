#include "stdafx.h"
#include "ModelBaseError.h"

static short gLanguage=ENGLISH;
short GetLanguage(){ return gLanguage; }
void SetLanguage(short language){gLanguage=language;}

ERMsg GetErrorMessage(int errorID, short language)
{
	ERMsg msg;

	if( language==UNKNOWN_LANGUAGE)
		language=gLanguage;
	    
    if(language==FRENCH)
    {
        switch(errorID)
        {
        case ERROR_UNABLE_OPEN_IDS: msg.ajoute("Erreur du modèle : incapable d'ouvrir le fichier d'entrée.");break;
        case ERROR_UNABLE_OPEN_TGO: msg.ajoute("Erreur du modèle : incapable d'ouvrir le fichier de température.");break;
        case ERROR_UNABLE_OPEN_IDO: msg.ajoute("Erreur du modèle : incapable d'ouvrir le fichier de sortie.");break;
        case ERROR_NOT_ENOUGHT_MEMORY: msg.ajoute("Erreur du modèle : mémoire insuffisante.");break;
        case ERROR_NO_PRECIPITATION: msg.ajoute("Erreur du modèle : le modèle a besoin de précipitation.");break;
		case ERROR_NO_PRECIPITATION_FOR_RAD: msg.ajoute("Erreur du modèle : le modèle a besoin de précipitation pour calculer les variables supplémentaires.");break;
        case ERROR_NO_RADIATION: msg.ajoute("Erreur du modèle : le modèle a besoin de radiation.");break;
        case ERROR_BAD_NUMBER_PARAMETER:msg.ajoute("Erreur du modèle : le nombre de paramètre d'entré est incorrecte. Vérifier les paramètres d'intrants dans la définition du modèle (.mdl)");break;
        case ERROR_BAD_DAY_NUMBER: msg.ajoute("Erreur du modèle : le modèle doit recommencer au jour 1 à chaque année");break;
		case ERROR_BAD_IDS_FORMAT: msg.ajoute("Erreur du modèle : les modèles utilisant CBioSIMModelBase doivent aussi utiliser le nouveau format de tranfert");break;
		case ERROR_REF_NOT_SUPPORT: msg.ajoute("Erreur du modèle : Le modèle ne supporte pas ce type de référence"); break;
		case ERROR_BAD_STREAM_FORMAT: msg.ajoute("Erreur du modèle : erreur dans la lecture du flux d'entré. Vérifier les versions BioSIM/modèle."); break;
		case ERROR_HXGRID_NOT_SUPPORTED: msg.ajoute("Erreur du modèle : Ce modèle ne peux être utilisé avec HxGrid"); break;
		case ERROR_DATA_NOT_FOUND: msg.ajoute("Erreur du modèle : Les données d'intrants sont introuvables"); break;
		case ERROR_INVALID_LIST_INDEX: msg.ajoute("Erreur du modèle : L'index de la liste est en dehors des limites possibles"); break;
		default: msg.ajoute("Erreur du modèle : erreur inconnue");
        }
    }
    else if( language==ENGLISH)
    {
        switch(errorID)
        {
        case ERROR_UNABLE_OPEN_IDS: msg.ajoute("Model error: unable to open input file.");break;
        case ERROR_UNABLE_OPEN_TGO: msg.ajoute("Model error: unable to open weather file.");break;
        case ERROR_UNABLE_OPEN_IDO: msg.ajoute("Model error: unable to open output file.");break;
        case ERROR_NOT_ENOUGHT_MEMORY: msg.ajoute("Model error: not enough memory.");break;
        case ERROR_NO_PRECIPITATION: msg.ajoute("Model error: model needs prepipitation.");break;
		case ERROR_NO_PRECIPITATION_FOR_RAD: msg.ajoute("Model error: model needs prepipitation to compute additional variables.");break;
        case ERROR_NO_RADIATION: msg.ajoute("Model error: model need radiation.");break;
        case ERROR_BAD_NUMBER_PARAMETER: msg.ajoute("Model error: bad number of input parameters. Verify inputs parameters in the model defenition (.mdl)");break;
        case ERROR_BAD_DAY_NUMBER: msg.ajoute("Model error: model must begin with day 1 each year");break;
		case ERROR_BAD_IDS_FORMAT: msg.ajoute("Model error: CBioSIMModelBase based model needs to use new transfer file format");break;
		case ERROR_REF_NOT_SUPPORT: msg.ajoute("Model error: this model does'nt support this type of reference" ); break;
		case ERROR_BAD_STREAM_FORMAT: msg.ajoute("Model error: error reading input stream. Verify BioSIM/model versions."); break;
		case ERROR_HXGRID_NOT_SUPPORTED: msg.ajoute("Model error: This model can't be used with HxGrid"); break;
		case ERROR_DATA_NOT_FOUND: msg.ajoute("Model error: input data are unavailable"); break;
		case ERROR_INVALID_LIST_INDEX: msg.ajoute("Model error: input list index are out of range"); break;
		default: msg.ajoute("Model error: Unknown error");
        }
    }

/*
	ERMsg msg;
	if( pGlobalModel )
	{
		msg = pGlobalModel->GetErrorMessage(errorID);
	}
	else
	{
		msg.asgType( ERMsg::ERREUR );
	}
*/
	return msg;
}
