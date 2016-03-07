//************************************************************************
// $Header: r:/rcs/codecpp/erreur/include/ermsg.h 1.9 1998/09/01 11:20:43 royyv Exp $
// $Date: 1998/09/01 11:20:43 $
// $Author: royyv $
// $Locker:  $
//************************************************************************
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
// Fichier:     ermsg.h
//
// Classe:      ERMsg
//
// Sommaire:    Interface de d�finition de la classe de messages d'erreur
//
// Description: Cette interface permet d'instancier la classe template
//              ERMsg<T> sur le type de cha�ne de caract�res
//              d�sir�.  On fait un typedef pour d�finir le type ERMsg.
//
// Attributs:
//
// Note:        On l'instancie sur une CLChaine.  Mais il est pr�vu de la faire
//              sur la string du standard dans un proche avenir.
//
//************************************************************************
// 08-06-98  Yves Roy       Version initiale
// 12-06-98  Yves Roy       Commentaires
// 15-07-98  Yves Secretan  Change std:: pour std::
// 01-09-98  Yves Roy       Ajout notice de copyright
//************************************************************************
#ifndef __ERMSG_H
#define __ERMSG_H
#pragma once

#include "basic/ERMsg/syenvmsg.h"
typedef ERMsg ERMsg;

#endif
