//************************************************************************
// --- (c) Copyright 1998, Institut National de la Recherche Scientifique (INRS)
// --- TOUS DROITS RÉSERVÉS
//
// --- Ce logiciel est couvert par les lois de copyright. L'utilisation et la copie
// --- du code source de ce logiciel à des fins autre que commerciales sont
// --- autorisés sans frais pour autant que la présente notice de copyright
// --- apparaisse sur toutes les copies ainsi que sur la documentation. L'INRS ne
// --- prétend en aucune façon que ce logiciel convient à un emploi quelconque.
// --- Celui-ci est distribué sans aucune garantie implicite ou explicite.
//************************************************************************
//*****************************************************************************
// Fichier:  ererreur.cpp
// Classe:   ERErreur
//*****************************************************************************
// 01-09-98  Yves Roy  Version éducationnelle
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
// Sommaire:    Affiche un message de type ERMsg à l'écran.
//
// Description: La méthode afficheMessage permet l'affichage à l'écran d'un
//              message du type ERMsg.  La méthode détermine le titre du message
//              à partir du type contenu dans messageErreur.  La méthode écrit
//              aussi le message dans le fichier de log d'erreur.
//              <p>
//              Le programme est terminé lorsque le message est du type FATAL
//              seulement.  Le programme va continuer sur les autres type de messages.
//
// Entrée:      const ERMsg& msg  : Le message
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
// Description: La méthode ecrisMessage permet d'écrire dans le fichier de log
//              d'erreur la chaîne passée en paramètre.  La chaîne est ajoutée
//              à la fin du fichier.
//
// Entrée:      const std::string& message : Message à ecrire
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
// Sommaire:    Écrit dans un fichier de log un message de type ERMsg.
//
// Description: La méthode ecrisMessage permet d'écrire dans le fichier de log
//              d'erreur un message de type ERMsg passé en paramêtre.  Le
//              message est ajouté à la fin du fichier.
//
// Entrée:      const ERMsg& msg : Message à ecrire
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
// Description: La méthode initNomFichierErreur(...) permet d'assigner le nom
//              désiré pour logger les erreurs dans le logiciel.
//
// Entrée:      const std::string& message : Nom complet du nom de fichier d'erreur
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
// Description: La méthode privée <code>traduisType()</code> centralise la traduction du
//              type du message.
//
// Entrée:      Entier type        : Type à traduire
//
// Sortie:      std::string& type  : Chaîne contenant le type
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
// Description: La méthode privée <code>traduisMessage()</code> centralise la
//              traduction d'un message.
//
// Entrée:      const ERMsg& msg : Message à traduire
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





