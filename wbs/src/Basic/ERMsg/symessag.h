//************************************************************************
// --- (c) Copyright 1998, Institut National de la Recherche Scientifique (INRS)
// --- TOUS DROITS R�SERV�S
//
// --- Ce logiciel est couvert par les lois de copyright. L'utilisation et la copie
// --- du code source � des fins autre que commerciales sont
// --- autoris�s sans frais pour autant que la pr�sente notice de copyright
// --- apparaisse sur toutes les copies ainsi que sur la documentation. L'INRS ne
// --- pr�tend en aucune fa�on que ce logiciel convient � un emploi quelconque.
// --- Celui-ci est distribu� sans aucune garantie implicite ou explicite.
//************************************************************************

//******************************************************************************
// Fichier:      symessag.h
//
// Classe:       SYMessage
//
// Sommaire:     La classe SYMessage permet de cr�er des messages d'erreur.
//
// Description:  La classe <code>SYMessage</code> repr�sente un message d'erreur
//               ayant un type (OK, AVERTISSEMENT, ERREURR ou FATAL) ainsi
//               qu'un nombre illimit� de cha�nes de caract�res permettant de
//               documenter une erreur ou un �v�nement.<p>
//               Cette classe est une template param�tris�e par un type T,
//               qui doit �tre une classe de cha�nes de caract�res supportant
//               un protocole minimum. Ainsi, le type T doit avoir d�fini: 
//               un constructeur par d�faut, un constructeur copie, 
//               les op�rateurs = et ==. 
//               <p>
//               Voici un exemple d'utilisation de la classe SYMessage:
//               <pre>
//            #   typedef SYMessage<std::string> ERMsg;
//            #   ERMsg msg = ERMsg::OK;  
//            #   ...
//            #   if (erreur_trouvee)
//            #   {
//            #      // --- La m�thode doit retourner une erreur
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
// 01-09-98  Yves Secretan, Yves Roy  Version �ducationnelle
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
inline void SYMessage::verifieInvariant() const
{
}

#endif   // __SYMESSAG_H




