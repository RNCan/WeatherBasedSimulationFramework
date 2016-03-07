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
// Fichier:      syenvmsg.h
//
// Classe:       ERMsg
//
// Sommaire:     Classe servant � g�rer le compte des r�f�rences aux objets SYMessage.
//
// Description:  La classe <code>ERMsg</code> g�re le compte 
//               des r�f�rences aux objets SYMessage. 
//               <p>
//               Cette classe est une implantation du pattern "Compte de r�f�rences"
//               et de la "Copie sur modification".
//               L'id�e est la suivante: tr�s souvent on copie un objet non pas
//               pour le modifier mais pour le consulter (80% des cas).  Il est
//               donc tr�s peu productif de copier dans ces cas.  A l'aide de cette
//               implantation du message, on donne l'impression de copier mais
//               on ne le fait que si on tente de modifier l'objet.  Cette 
//               implantation rend de plus int�ressant le retour par valeur
//               puisqu'on a qu'un pointeur � copier et non pas tous le message.
//               <p>
//               Puisque la plupart des m�thodes retournent un message,
//               il est beaucoup plus profitable de retourner l'enveloppe
//               du message plut�t que le message lui-m�me (c'est-�-dire
//               l'objet SYMessage).
//               <p>
//               Pour �viter de polluer la classe SYMessage<T> avec les notions
//               de compte de r�f�rences, on ins�re une petite classe interm�diaire
//               pour faire cette fonction.
//
// Attributs:    messageP: Un pointeur sur un compteur de r�f�rences
//                         � des objets SYMessage.
//
// Notes:        Pour plus d'informations, voir:
//               Horstmann, Cay S., Mastering Object-Oriented Design in
//               C++, pp. 258-287.
//************************************************************************
// 01-09-98  Yves Secretan, Yves Roy  Version �ducationnelle
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
// Description: La m�thode priv�e <code>invariant(const char*)</code>
//              permet de tester les invariants de classe.
//              Dans ce cas-ci, pas d'invariant de classe.
//
// Entr�e:      
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




