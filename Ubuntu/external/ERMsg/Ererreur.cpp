//************************************************************************
// --- (c) Copyright 1998, Institut National de la Recherche Scientifique (INRS)
// --- TOUS DROITS R�SERV�S
//
// --- Ce logiciel est couvert par les lois de copyright. L'utilisation et la copie
// --- du code source de ce logiciel � des fins autre que commerciales sont
// --- autoris�s sans frais pour autant que la pr�sente notice de copyright
// --- apparaisse sur toutes les copies ainsi que sur la documentation. L'INRS ne
// --- pr�tend en aucune fa�on que ce logiciel convient � un emploi quelconque.
// --- Celui-ci est distribu� sans aucune garantie implicite ou explicite.
//************************************************************************
//*****************************************************************************
// Fichier:  ererreur.cpp
// Classe:   ERErreur
//*****************************************************************************
// 01-09-98  Yves Roy  Version �ducationnelle
//*****************************************************************************
//#include "stdafx.h"
#include "Ererreur.h"

#include <iostream>
#include <fstream>
#include <signal.h>

#include <stdlib.h>

using namespace std;

string ERErreur::NOM_FICHIER_ERREUR = string("erreur.log");

//*****************************************************************************
// Sommaire:    Affiche un message de type ERMsg � l'�cran.
//
// Description: La m�thode afficheMessage permet l'affichage � l'�cran d'un
//              message du type ERMsg.  La m�thode d�termine le titre du message
//              � partir du type contenu dans messageErreur.  La m�thode �crit
//              aussi le message dans le fichier de log d'erreur.
//              <p>
//              Le programme est termin� lorsque le message est du type FATAL
//              seulement.  Le programme va continuer sur les autres type de messages.
//
// Entr�e:      const ERMsg& msg  : Le message
//
// Sortie:
//
// Notes:
//
//*****************************************************************************
void ERErreur::afficheMessage(const ERMsg& msg)
{
    string titre;
    string message;
    traduisMessage(titre, message, msg);

    ecrisMessage(titre + message);

    cout << titre << endl;
    cout << message << endl;

    if (msg.reqType() == ERMsg::FATAL)
    {
        exit(1);
    }
}

//*****************************************************************************
// Sommaire:    Ecrit dans un fichier de log
//
// Description: La m�thode ecrisMessage permet d'�crire dans le fichier de log
//              d'erreur la cha�ne pass�e en param�tre.  La cha�ne est ajout�e
//              � la fin du fichier.
//
// Entr�e:      const std::string& message : Message � ecrire
//
// Sortie:
//
// Notes:
//
//*****************************************************************************
void ERErreur::ecrisMessage(const std::string& message)
{
    ofstream fichier;
    fichier.open(NOM_FICHIER_ERREUR.c_str(),ios::app);
    fichier << message << endl;
    fichier.close();
}

//*****************************************************************************
// Sommaire:    �crit dans un fichier de log un message de type ERMsg.
//
// Description: La m�thode ecrisMessage permet d'�crire dans le fichier de log
//              d'erreur un message de type ERMsg pass� en param�tre.  Le
//              message est ajout� � la fin du fichier.
//
// Entr�e:      const ERMsg& msg : Message � ecrire
//
// Sortie:
//
// Notes:
//
//*****************************************************************************
void ERErreur::ecrisMessage(const ERMsg& msg)
{
    string titre;
    string message;
    traduisMessage(titre, message, msg);

    ofstream fichier;
    fichier.open(NOM_FICHIER_ERREUR.c_str(), ios::app);
    fichier << titre << endl << message;
    fichier.close();

    if (msg.reqType() == ERMsg::FATAL)
    {
        exit(1);
    }
}

//****************************************************************************
// Sommaire:    Assigne un nom au fichier d'erreur.
//
// Description: La m�thode initNomFichierErreur(...) permet d'assigner le nom
//              d�sir� pour logger les erreurs dans le logiciel.
//
// Entr�e:      const std::string& message : Nom complet du nom de fichier d'erreur
//
// Sortie:
//
// Notes:
//
//****************************************************************************
void ERErreur::initNomFichierErreur(const std::string& nomFichier)
{
    NOM_FICHIER_ERREUR = nomFichier;
}

//****************************************************************************
// Sommaire:    Traduis le type du message
//
// Description: La m�thode priv�e <code>traduisType()</code> centralise la traduction du
//              type du message.
//
// Entr�e:      Entier type        : Type � traduire
//
// Sortie:      std::string& type  : Cha�ne contenant le type
//
// Notes:
//
//****************************************************************************
void ERErreur::traduisType(std::string& titre, int type)
{
    switch (type)
    {
    case ERMsg::OK:
        titre = "OK";
        break;
    case ERMsg::AVERTISSEMENT:
        titre = "AVERTISSEMENT";
        break;
    case ERMsg::ERREUR:
        titre = "ERREUR";
        break;
    case ERMsg::FATAL:
        titre = "FATAL";
        break;
    default:
        titre = "INCONNU";
        break;
    }
}

//****************************************************************************
// Sommaire:    Traduis un message.
//
// Description: La m�thode priv�e <code>traduisMessage()</code> centralise la
//              traduction d'un message.
//
// Entr�e:      const ERMsg& msg : Message � traduire
//
// Sortie:      string& type          Chaine contenant le type
//              string& message       Chaine contenant le corps du message
//
// Notes:
//
//****************************************************************************
void ERErreur::traduisMessage(std::string& type,
                              std::string& message,
                              const ERMsg& msg)
{
    traduisType(type, msg.reqType());

    message = "";
    unsigned int nbr = msg.dimension();
    for (unsigned int i=0; i<nbr; i++)
    {
        message += msg[i];
        message += "\n";
    }
}





