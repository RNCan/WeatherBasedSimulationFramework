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
//************************************************************************
// Fichier:      syenvmsg.h
//
// Classe:       ERMsg
//
// Sommaire:     Classe servant à gérer le compte des références aux objets SYMessage.
//
// Description:  La classe <code>ERMsg</code> gère le compte 
//               des références aux objets SYMessage. 
//               <p>
//               Cette classe est une implantation du pattern "Compte de références"
//               et de la "Copie sur modification".
//               L'idée est la suivante: très souvent on copie un objet non pas
//               pour le modifier mais pour le consulter (80% des cas).  Il est
//               donc très peu productif de copier dans ces cas.  A l'aide de cette
//               implantation du message, on donne l'impression de copier mais
//               on ne le fait que si on tente de modifier l'objet.  Cette 
//               implantation rend de plus intéressant le retour par valeur
//               puisqu'on a qu'un pointeur à copier et non pas tous le message.
//               <p>
//               Puisque la plupart des méthodes retournent un message,
//               il est beaucoup plus profitable de retourner l'enveloppe
//               du message plutôt que le message lui-même (c'est-à-dire
//               l'objet SYMessage).
//               <p>
//               Pour éviter de polluer la classe SYMessage<T> avec les notions
//               de compte de références, on insère une petite classe intermédiaire
//               pour faire cette fonction.
//
// Attributs:    messageP: Un pointeur sur un compteur de références
//                         à des objets SYMessage.
//
// Notes:        Pour plus d'informations, voir:
//               Horstmann, Cay S., Mastering Object-Oriented Design in
//               C++, pp. 258-287.
//************************************************************************
// 01-09-98  Yves Secretan, Yves Roy  Version éducationnelle
//************************************************************************
#ifndef __SYENVMSG_H
#define __SYENVMSG_H


#include "basic/ERMsg/erexcept.h"

#include "basic/ERMsg/sycptref.h"
#include "basic/ERMsg/symessag.h"

#pragma warning( disable : 4786)

class ERMsg
{
public:
   enum Type { OK            = SYMessage::OK,
               AVERTISSEMENT = SYMessage::AVERTISSEMENT,
               ERREUR        = SYMessage::ERREUR,
               FATAL         = SYMessage::FATAL };

                        ERMsg  (Type = OK);
                        ERMsg  (Type, const std::string&);
                        ERMsg  (const ERMsg&);
                        ~ERMsg ();
   ERMsg&  operator=           (const ERMsg&);
   ERMsg&  operator+=          (const ERMsg&);


   void                 ajoute              (const std::string&);
   void                 ajoute              (const char* pChaine);
   void                 ajoute              (double);
   void                 ajoute              (long);
   void                 ajoute              (const ERMsg&);
   void                 asgType             (Type);
   unsigned int         dimension           () const;
   Type                 reqType             () const;
   const std::string&   operator []         (int) const;
                        operator void*      () const;

private:
   void                 clone               ();
   void                 copie               (const ERMsg&);
   void                 verifieInvariant    () const;
   void                 libere              ();

   SYCompteReference<SYMessage> *messageP;
};

//**************************************************************
// Sommaire:    Invariants de classe
//
// Description: La méthode privée <code>invariant(const char*)</code>
//              permet de tester les invariants de classe.
//              Dans ce cas-ci, pas d'invariant de classe.
//
// Entrée:      
//
// Sortie:
//
// Notes:
//
//**************************************************************
inline void ERMsg::verifieInvariant() const
{
}

#endif // ifndef __SYENVMSG_H




