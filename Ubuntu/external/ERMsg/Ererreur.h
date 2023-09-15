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
//************************************************************************
// Fichier:     ererreur.h
//
// Classe:      ERErreur
//
// Sommaire:    Classe statique de m�thodes de gestion des erreurs
//
// Description: Cette classe contient les m�thodes statiques n�cessaires
//              pour g�rer les erreurs, les afficher et/ou les �crire
//              dans un fichier de log.
//
// Notes:
//************************************************************************
// 01-00-98  Yves Roy  Version �ducationnelle
//************************************************************************
#ifndef __ERERREUR_H
#define __ERERREUR_H

#include "external/ERMsg/ERMsg.h"
#include <string>

class ERErreur
{
public:
    static void afficheMessage       (const ERMsg&);
    static void ecrisMessage         (const std::string&);
    static void ecrisMessage         (const ERMsg&);
    static void initNomFichierErreur (const std::string&);

private:
    static void traduisType          (std::string&, int);
    static void traduisMessage       (std::string&, std::string&, const ERMsg&);

    static std::string NOM_FICHIER_ERREUR;
};

#endif  // ifndef __ERERREUR_H



