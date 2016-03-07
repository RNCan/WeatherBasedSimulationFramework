//************************************************************************
// --- (c) Copyright 1998, Institut National de la Recherche Scientifique (INRS)
// --- TOUS DROITS RÉSERVÉS
//
// --- Ce logiciel est couvert par les lois de copyright. L'utilisation et la copie
// --- du code source à des fins autre que commerciales sont
// --- autorisés sans frais pour autant que la présente notice de copyright
// --- apparaisse sur toutes les copies ainsi que sur la documentation. L'INRS ne
// --- prétend en aucune façon que ce logiciel convient à un emploi quelconque.
// --- Celui-ci est distribué sans aucune garantie implicite ou explicite.
//************************************************************************

//******************************************************************************
// Fichier:      symessag.h
//
// Classe:       SYMessage
//
// Sommaire:     La classe SYMessage permet de créer des messages d'erreur.
//
// Description:  La classe <code>SYMessage</code> représente un message d'erreur
//               ayant un type (OK, AVERTISSEMENT, ERREURR ou FATAL) ainsi
//               qu'un nombre illimité de chaînes de caractères permettant de
//               documenter une erreur ou un événement.<p>
//               Cette classe est une template paramétrisée par un type T,
//               qui doit être une classe de chaînes de caractères supportant
//               un protocole minimum. Ainsi, le type T doit avoir défini: 
//               un constructeur par défaut, un constructeur copie, 
//               les opérateurs = et ==. 
//               <p>
//               Voici un exemple d'utilisation de la classe SYMessage:
//               <pre>
//            #   typedef SYMessage<std::string> ERMsg;
//            #   ERMsg msg = ERMsg::OK;  
//            #   ...
//            #   if (erreur_trouvee)
//            #   {
//            #      // --- La méthode doit retourner une erreur
//            #      msg = ERMsg(ERMsg::ERREUR, "ERR_DESCRIPTION_ERREUR");
//            #      msg.ajoute("commentaire...");
//            #   }
//            #   ... 
//            #   if (!msg)
//            #   {
//            #      switch(msg.reqType())
//            #      {
//            #         case ERMsg::ERREUR:         cout << "ERREUR" << endl; break;
//            #         case ERMsg::AVERTISSEMENT:  cout << "AVERTISSEMENT << endl; break;
//            #         case ERMsg::FATAL:          cout << "FATAL" << endl; break;
//            #         default:                    cout << "ERREUR" << endl; break;
//            #      }
//            #      EntierN dim = msg.dimension();
//            #      for (EntierN i=0; i<dim; i++)
//            #      {
//            #         cout << msg[i] << endl;
//            #      }
//            #   }
//                </pre>
//
// Attributs:     Type type     :  Type du message (OK, AVERTISSEMENT, ERREUR, FATAL)
//                T lstMessages :  Liste des messages
//
// Notes:
//******************************************************************************
// 01-09-98  Yves Secretan, Yves Roy  Version éducationnelle
//******************************************************************************
#ifndef __SYMESSAG_H
#define __SYMESSAG_H

#pragma warning(disable: 4786)

#include "basic/ERMsg/erexcept.h"

#include <vector>
#include <string>



class SYMessage
{
public:
   enum Type { OK, AVERTISSEMENT, ERREUR, FATAL };

                      SYMessage        (Type = OK);
                      SYMessage        (Type, const std::string&);
                      SYMessage        (const SYMessage&);
                      ~SYMessage       ();
   SYMessage&         operator=        (const SYMessage&);

   void               ajoute           (const std::string&);
   void               ajoute           (double);
   void               ajoute           (long);
   void               asgType          (Type);

   unsigned int       dimension        () const;
   Type               reqType          () const;
   const std::string& operator[]       (int) const;
                      operator void*   () const;

private:
   void               verifieInvariant () const;

   Type type;
   std::vector<std::string> lstMessages;
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
inline void SYMessage::verifieInvariant() const
{
}

#endif   // __SYMESSAG_H




