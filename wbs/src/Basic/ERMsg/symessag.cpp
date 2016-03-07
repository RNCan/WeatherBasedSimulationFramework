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
//******************************************************************************
// Fichier: symessag.cpp
// Classe:  SYMessage
//******************************************************************************
// 01-09-98  Yves Secretan, Yves Roy  Version éducationnelle
//******************************************************************************
#include "stdafx.h"
#include "basic/ERMsg/symessag.h"
#include <strstream>
using namespace std;

//******************************************************************************
// Sommaire:    Constructeur par défaut.
//
// Description: Le constructeur par défaut initialise les attributs de la classe
//              avec des valeurs par défaut (la liste de messages est
//              laissée vide et le type (optionnel) est initialisé à prmType).
//              Le type est initialisé par défaut à OK.
//
// Entrée:      Type prmType (optionnel) :   Type du message
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
//               le message avec un type et une première chaîne de caractères.
//               Les paramètres passés sont le message et le type du message.
//               La longueur de la liste de messages égalera donc 1.
//               On considèrera souvent cette première chaîne comme un code d'erreur.
//
// Entrée:       Type typeMessage : le type de message (OK, AVERTISSEMENT, ERREUR, FATAL)
//               const std::string& message : une chaine de caractères
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
//              un objet du même type.
//
// Entrée:      const SYMessage<T>& obj: un objet du même type
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
// Description:  Aucune ressource à libérer.
//
// Entrée:
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
// Sommaire:     Opérateur d'assignation
//
// Description:  L'opérateur d'assignation permet d'assigner des états à l'objet
//               courant à partir d'un objet du même type.  
//               On assume qu'un objet de type T a défini l'opérateur =.
//
// Entrée:       const SYMessage<T>& obj: un objet du même type.
//
// Sortie:       On retourne l'objet par référence pour permettre les appels
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
// Description: La méthode publique <code>asgType(...)</code> permet d'assigner 
//              un type au message.
//
// Entrée:      Type prmType: le type de message
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
// Sommaire:     Cette méthode ajoute un message à la fin de la liste.
//
// Description:  La méthode publique <code>ajoute(...)</code> permet d'ajouter
//               un message à la fin de la liste des messages.  Par exemple, 
//               le premier message de la liste peut être "ERR_FICHIER_INTROUVABLE" 
//               et le second message peut contenir le nom du fichier.
//
// Entrée:       const std::string& message :   le message à ajouter
//
// Sortie:
//
// Notes:
//******************************************************************************
void SYMessage::ajoute(const std::string& message)
{
   _PRECONDITION(!message.empty());
   _INVARIANTS ();

   // ---  Ajoute à la liste
   lstMessages.push_back(message);

   _POSTCONDITION(lstMessages.back() == message);
   _INVARIANTS ();
}

//******************************************************************************
// Sommaire:     Cette méthode convertit et ajoute un réel dans le message.
//
// Description:  La méthode publique <code>ajoute(...)</code> permet d'ajouter
//               un réel convertit en chaîne à la fin de la liste des messages.  
//
// Entrée:       double val : la valeur à convertir et ajouter.
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

   // ---  Ajoute à la liste la chaîne dans le buffer
   lstMessages.push_back(buffer);

   _INVARIANTS ();
}

//******************************************************************************
// Sommaire:     Cette méthode convertit et ajoute un entier dans le message.
//
// Description:  La méthode publique <code>ajoute(...)</code> permet d'ajouter
//               un entier convertit en chaîne à la fin de la liste des messages.  
//
// Entrée:       long val : la valeur à convertir et ajouter.
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

   // ---  Ajoute à la liste la chaîne dans le buffer
   lstMessages.push_back(buffer);

   _INVARIANTS ();
}

//******************************************************************************
// Sommaire:     Retourne le type de message.
//
// Description:  La méthode publique <code>reqType()</code> retourne le type 
//               du message couramment stocké.
//
// Entrée:
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
// Description: La méthode publique <code>dimension()</code> retourne le nombre 
//              de chaînes de caractères contenu dans le message.
//
// Entrée:
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
// Description: La méthode publique <code>operator[](int)</code> permet
//              d'obtenir la ieme chaîne de caractères dans le message.
//              L'indice doit être compris entre 0 et la longueur de la liste - 1.
//
// Entrée:      int indice : Indice de la chaîne à retourner.
//
// Sortie:      Retourne la chaîne à cette position.
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
// Sommaire:     Permet de faire un test logique sur l'état du message.
//
// Description:  La méthode publique <code>operator void*()</code> permet de 
//               faire un test logique sur l'état du message.  
//               Ainsi, si le message est du type OK, le test sera
//               VRAI et si le message est d'un autre type, il sera FAUX.
//               <p>
//               Exemple:
//               <pre>
//            #   typedef SYMessage<std::string> ERMsg;
//            #   ERMsg msg = ERMsg::OK;  
//            #   ...
//            #   msg = une_methode();  
//            #   if (msg) // utilisation de l'opérateur void*
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
// Entrée:
//
// Sortie:
//
// Notes:       Nous avons préféré l'utilisation du cast en void* à celui en
//              int pour éviter l'utilisation du résultat dans des opérations
//              arithmétiques.
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

