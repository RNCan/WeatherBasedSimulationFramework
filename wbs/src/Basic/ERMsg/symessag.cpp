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
//******************************************************************************
// Fichier: symessag.cpp
// Classe:  SYMessage
//******************************************************************************
// 01-09-98  Yves Secretan, Yves Roy  Version �ducationnelle
//******************************************************************************
#include "stdafx.h"
#include "basic/ERMsg/symessag.h"
#include <strstream>
using namespace std;

//******************************************************************************
// Sommaire:    Constructeur par d�faut.
//
// Description: Le constructeur par d�faut initialise les attributs de la classe
//              avec des valeurs par d�faut (la liste de messages est
//              laiss�e vide et le type (optionnel) est initialis� � prmType).
//              Le type est initialis� par d�faut � OK.
//
// Entr�e:      Type prmType (optionnel) :   Type du message
//
// Sortie:
//
// Notes:
//******************************************************************************
SYMessage::SYMessage(SYMessage::Type prmType)
: type(prmType)
{
   _POSTCONDITION(lstMessages.size() == 0);
   _INVARIANTS ();
}

//******************************************************************************
// Sommaire:     Constructeur avec type et message.
//
// Description:  Le constructeur avec type et message permet d'initialiser
//               le message avec un type et une premi�re cha�ne de caract�res.
//               Les param�tres pass�s sont le message et le type du message.
//               La longueur de la liste de messages �galera donc 1.
//               On consid�rera souvent cette premi�re cha�ne comme un code d'erreur.
//
// Entr�e:       Type typeMessage : le type de message (OK, AVERTISSEMENT, ERREUR, FATAL)
//               const std::string& message : une chaine de caract�res
//
// Sortie:
//
// Notes:
//******************************************************************************
SYMessage::SYMessage (Type typeMessage, const std::string& message)
: type(typeMessage)
{
   _PRECONDITION(!message.empty());

   lstMessages.push_back(message);

   _POSTCONDITION(typeMessage == type);
   _POSTCONDITION(lstMessages.size() == 1);
   _INVARIANTS ();
}

//******************************************************************************
// Sommaire:    Constructeur copie de la classe.
//
// Description: Le constructeur copie permet de construire l'objet courant avec
//              un objet du m�me type.
//
// Entr�e:      const SYMessage<T>& obj: un objet du m�me type
//
// Sortie:
//
// Notes:
//******************************************************************************
SYMessage::SYMessage(const SYMessage& obj)
: type(obj.type), lstMessages(obj.lstMessages)
{
   _INVARIANTS();
}

//******************************************************************************
// Sommaire:     Destructeur.
//
// Description:  Aucune ressource � lib�rer.
//
// Entr�e:
//
// Sortie:
//
// Notes:
//******************************************************************************
SYMessage::~SYMessage()
{
   _INVARIANTS();
}

//******************************************************************************
// Sommaire:     Op�rateur d'assignation
//
// Description:  L'op�rateur d'assignation permet d'assigner des �tats � l'objet
//               courant � partir d'un objet du m�me type.  
//               On assume qu'un objet de type T a d�fini l'op�rateur =.
//
// Entr�e:       const SYMessage<T>& obj: un objet du m�me type.
//
// Sortie:       On retourne l'objet par r�f�rence pour permettre les appels
//               en cascade.
//
// Notes:
//******************************************************************************
SYMessage& SYMessage::operator=(const SYMessage& obj)
{
   _INVARIANTS();

   if (this != &obj) // Test de l'auto-assignation
   {
      type = obj.type;
      lstMessages = obj.lstMessages;
   }

   _INVARIANTS();
   return *this;
}

//******************************************************************************
// Sommaire:    Assigne le type de message
//
// Description: La m�thode publique <code>asgType(...)</code> permet d'assigner 
//              un type au message.
//
// Entr�e:      Type prmType: le type de message
//
// Sortie:
//
// Notes:
//******************************************************************************
void SYMessage::asgType(Type prmType)
{
   _INVARIANTS();

   type = prmType;

   _POSTCONDITION(type == prmType);
   _INVARIANTS();
}

//******************************************************************************
// Sommaire:     Cette m�thode ajoute un message � la fin de la liste.
//
// Description:  La m�thode publique <code>ajoute(...)</code> permet d'ajouter
//               un message � la fin de la liste des messages.  Par exemple, 
//               le premier message de la liste peut �tre "ERR_FICHIER_INTROUVABLE" 
//               et le second message peut contenir le nom du fichier.
//
// Entr�e:       const std::string& message :   le message � ajouter
//
// Sortie:
//
// Notes:
//******************************************************************************
void SYMessage::ajoute(const std::string& message)
{
   _PRECONDITION(!message.empty());
   _INVARIANTS ();

   // ---  Ajoute � la liste
   lstMessages.push_back(message);

   _POSTCONDITION(lstMessages.back() == message);
   _INVARIANTS ();
}

//******************************************************************************
// Sommaire:     Cette m�thode convertit et ajoute un r�el dans le message.
//
// Description:  La m�thode publique <code>ajoute(...)</code> permet d'ajouter
//               un r�el convertit en cha�ne � la fin de la liste des messages.  
//
// Entr�e:       double val : la valeur � convertir et ajouter.
//
// Sortie:
//
// Notes:
//******************************************************************************
void SYMessage::ajoute(double val)
{
   _INVARIANTS ();

   // --- On convertit d'abord.
   char buffer[256];
   ostrstream os(buffer, 255);
   os << val << ends;

   // ---  Ajoute � la liste la cha�ne dans le buffer
   lstMessages.push_back(buffer);

   _INVARIANTS ();
}

//******************************************************************************
// Sommaire:     Cette m�thode convertit et ajoute un entier dans le message.
//
// Description:  La m�thode publique <code>ajoute(...)</code> permet d'ajouter
//               un entier convertit en cha�ne � la fin de la liste des messages.  
//
// Entr�e:       long val : la valeur � convertir et ajouter.
//
// Sortie:
//
// Notes:
//******************************************************************************
void SYMessage::ajoute(long val)
{
   _INVARIANTS ();

   // --- On convertit d'abord.
   char buffer[256];
   ostrstream os(buffer, 255);
   os << val << ends;

   // ---  Ajoute � la liste la cha�ne dans le buffer
   lstMessages.push_back(buffer);

   _INVARIANTS ();
}

//******************************************************************************
// Sommaire:     Retourne le type de message.
//
// Description:  La m�thode publique <code>reqType()</code> retourne le type 
//               du message couramment stock�.
//
// Entr�e:
//
// Sortie:       SYMessage<T>::Type : le type de message
//
// Notes:
//******************************************************************************
SYMessage::Type SYMessage::reqType() const
{
   _INVARIANTS();
   return type;
}

//******************************************************************************
// Sommaire:    Retourne le nombre de messages dans la liste.
//
// Description: La m�thode publique <code>dimension()</code> retourne le nombre 
//              de cha�nes de caract�res contenu dans le message.
//
// Entr�e:
//
// Sortie:      Retourne le nombre.
//
// Notes:
//******************************************************************************
unsigned int SYMessage::dimension() const
{
   _INVARIANTS();
   return (unsigned int)lstMessages.size();
}

//******************************************************************************
// Sommaire:    Retourne le ieme message
//
// Description: La m�thode publique <code>operator[](int)</code> permet
//              d'obtenir la ieme cha�ne de caract�res dans le message.
//              L'indice doit �tre compris entre 0 et la longueur de la liste - 1.
//
// Entr�e:      int indice : Indice de la cha�ne � retourner.
//
// Sortie:      Retourne la cha�ne � cette position.
//
// Notes:
//******************************************************************************
const std::string& SYMessage::operator [] (int indice) const
{
   _PRECONDITION(indice >= 0 && indice < (int)lstMessages.size());
   _INVARIANTS();
   return lstMessages[indice];
}

//******************************************************************************
// Sommaire:     Permet de faire un test logique sur l'�tat du message.
//
// Description:  La m�thode publique <code>operator void*()</code> permet de 
//               faire un test logique sur l'�tat du message.  
//               Ainsi, si le message est du type OK, le test sera
//               VRAI et si le message est d'un autre type, il sera FAUX.
//               <p>
//               Exemple:
//               <pre>
//            #   typedef SYMessage<std::string> ERMsg;
//            #   ERMsg msg = ERMsg::OK;  
//            #   ...
//            #   msg = une_methode();  
//            #   if (msg) // utilisation de l'op�rateur void*
//            #   {   
//            #      ...
//            #      cout << "Il n'y a pas d'erreur" << endl;
//            #   }
//            #   else
//            #   {
//            #      cout << "Il y a une erreur" << endl;
//            #      // --- On affiche le message...
//            #   }
//              </pre>
//
// Entr�e:
//
// Sortie:
//
// Notes:       Nous avons pr�f�r� l'utilisation du cast en void* � celui en
//              int pour �viter l'utilisation du r�sultat dans des op�rations
//              arithm�tiques.
//******************************************************************************
SYMessage::operator void*() const
{
   _INVARIANTS();

   if (type == SYMessage::OK)
   {
      return (void*)true;
   }
   else
   {
      return (void*)false;
   }
}

