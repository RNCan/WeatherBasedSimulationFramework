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
// Fichier: syenvmsg.cpp
// Classe:  ERMsg
//************************************************************************
// 01-09-98  Yves Secretan, Yves Roy  Version éducationnelle
//************************************************************************
#include "stdafx.h"
#include "basic/ERMsg/syenvmsg.h"

using namespace std;

//************************************************************************
// Sommaire:    Constructeur par défaut de la classe.
//
// Description: Le constructeur par défaut initialise les attributs à des valeurs
//              par défaut.  Ainsi le type du message est OK par défaut et messageP
//              est initialisé à NUL.
//
// Entrée:      Type& unType: le type du message.
//
// Sortie:
//
// Notes:
//************************************************************************
ERMsg::ERMsg(Type unType)
: messageP(0)
{
   // --- Si le type du message est autre que OK, un objet
   // --- SYMessage est physiquement créé.
   if (unType != OK)
   {
      messageP = new SYCompteReference<SYMessage>
                                 (SYMessage ((SYMessage::Type)unType));
   }

   _POSTCONDITION(unType != OK || messageP == 0);
   _INVARIANTS();
}

//************************************************************************
// Sommaire:    Constructeur de la classe
//
// Description: Le constructeur avec paramètre initialise un message à l'aide
//              des paramètres.
//
// Entrée:      Type&    type:    le type du message.
//              const T& chaine:  la chaîne initiale du message.
//
// Sortie:
//
// Notes:
//************************************************************************
ERMsg::ERMsg(Type type, const std::string& chaine)
{
   _PRECONDITION(!chaine.empty());

   SYMessage::Type unType = static_cast<SYMessage::Type>(type);
   messageP = new SYCompteReference<SYMessage>(SYMessage(unType, chaine));

   _INVARIANTS();
}

//************************************************************************
// Sommaire:    Constructeur copie de la classe
//
// Description: Le constructeur copie construit l'objet à partir d'un objet du
//              même type passé en paramètre.
//
// Entrée:      obj: L'objet sur lequel se construire.
//
// Sortie:
//
// Notes:
//************************************************************************
ERMsg::ERMsg(const ERMsg &obj)
: messageP(0)
{
   // --- Appel de la méthode privée qui s'occupe des copies et du compte
   // --- de références.
   copie(obj);

   _POSTCONDITION(messageP == obj.messageP);
   _INVARIANTS();
}

//************************************************************************
// Sommaire:    Destructeur de la classe
//
// Description: Ce destructeur gère le compte de références avant de
//              laisser détruire l'objet
//
// Entrée:
//
// Sortie:
//
// Notes:
//************************************************************************
ERMsg::~ERMsg()
{
   _INVARIANTS();

   // --- Appel à la méthode privée qui s'occupe de libérer les ressources
   // --- et qui gère le compte de références.
   if (messageP != 0)
   {
      libere();
   }
}
void ERMsg::ajoute(const char* pChaine)
{
	std::string chaine( pChaine);
	ajoute(chaine);
}
//************************************************************************
// Sommaire:     Cette méthode ajoute une chaîne de caractères à la fin du message.
//
// Description:  La méthode publique <code>ajoute(...)</code> fait le représentant
//               de la méthode de l'objet auquel on réfère.  
//               On passe par le SYCompteReference<T> pour accéder à l'objet de message.
//               <p>
//               Notons que dans SYCompteReference<T> on a définit deux
//               méthodes reqObjet, une const et l'autre non const.
//               La méthode non const est utilisée ici.
//               <p>
//               On doit implanter dans cette méthode la notion de "Copie sur
//               modification" puisque que cette méthode modifie l'objet.  Ainsi,
//               lorsque cette méthode est appelée on doit "cloner" l'objet
//               référencé pour avoir un comportement prévisible.
//               <p>
//               Si le pointeur à l'objet SYCompteReference (messageP)
//               est nul, un tel objet doit évidemment être créé.
//
// Entrée:       const std::string& chaine: la chaîne de caractères à ajouter au message.
//
// Sortie:
//
// Notes:
//************************************************************************
void ERMsg::ajoute(const std::string& chaine)
{
#pragma omp critical(stepIt)
{
	_PRECONDITION(!chaine.empty());
	_INVARIANTS();

	if (messageP != 0)
	{
		// --- On applique la théorie du "Copie sur Modification"
		// --- On appelle clone() avant de modifier.
		clone();
		messageP->reqObjet().ajoute(chaine);
	}
	else
	{
		messageP = new SYCompteReference<SYMessage>
			(SYMessage(SYMessage::ERREUR, chaine));
	}

	_INVARIANTS();
}
}

//************************************************************************
// Sommaire:     Cette méthode ajoute un réel à la fin du message.
//
// Description:  La méthode publique <code>ajoute(...)</code> fait le représentant
//               de la méthode de l'objet auquel on réfère.  
//               On passe par le SYCompteReference<T> pour accéder à l'objet de message.
//               <p>
//               Notons que dans SYCompteReference<T> on a définit deux
//               méthodes reqObjet, une const et l'autre non const.
//               La méthode non const est utilisée ici.
//               <p>
//               On doit implanter dans cette méthode la notion de "Copie sur
//               modification" puisque que cette méthode modifie l'objet.  Ainsi,
//               lorsque cette méthode est appelée on doit "cloner" l'objet
//               référencé pour avoir un comportement prévisible.
//               <p>
//               Si le pointeur à l'objet SYCompteReference (messageP)
//               est nul, un tel objet doit évidemment être créé.
//
// Entrée:       double val: le réel à convertir et à ajouter au message.
//
// Sortie:
//
// Notes:
//************************************************************************
void ERMsg::ajoute(double val)
{
   _INVARIANTS();

   if (messageP != 0)
   {
      // --- On applique la théorie du "Copie sur Modification"
      // --- On appelle clone() avant de modifier.
      clone();
      messageP->reqObjet().ajoute(val);
   }
   else
   {
      messageP = new SYCompteReference<SYMessage>
                     (SYMessage (SYMessage::ERREUR));
      messageP->reqObjet().ajoute(val);
   }

   _INVARIANTS();
}

//************************************************************************
// Sommaire:     Cette méthode ajoute un entier à la fin du message.
//
// Description:  La méthode publique <code>ajoute(...)</code> fait le représentant
//               de la méthode de l'objet auquel on réfère.  
//               On passe par le SYCompteReference<T> pour accéder à l'objet de message.
//               <p>
//               Notons que dans SYCompteReference<T> on a définit deux
//               méthodes reqObjet, une const et l'autre non const.
//               La méthode non const est utilisée ici.
//               <p>
//               On doit implanter dans cette méthode la notion de "Copie sur
//               modification" puisque que cette méthode modifie l'objet.  Ainsi,
//               lorsque cette méthode est appelée on doit "cloner" l'objet
//               référencé pour avoir un comportement prévisible.
//               <p>
//               Si le pointeur à l'objet SYCompteReference (messageP)
//               est nul, un tel objet doit évidemment être créé.
//
// Entrée:       long val: l'entier à convertir et à ajouter au message.
//
// Sortie:
//
// Notes:
//************************************************************************
void ERMsg::ajoute(long val)
{
   _INVARIANTS();

   if (messageP != 0)
   {
      // --- On applique la théorie du "Copie sur Modification"
      // --- On appelle clone() avant de modifier.
      clone();
      messageP->reqObjet().ajoute(val);
   }
   else
   {
      messageP = new SYCompteReference<SYMessage>
                     (SYMessage (SYMessage::ERREUR));
      messageP->reqObjet().ajoute(val);
   }

   _INVARIANTS();
}

//************************************************************************
// Sommaire:     Cette méthode assigne un type au message.
//
// Description:  La méthode publique <code>asgType(...)</code> réimplante 
//               la méthode de l'objet auquel on réfère.  
//               On passe par le SYCompteReference<T>
//               pour accéder à l'objet.
//               <p>
//               Notons que dans SYCompteReference<T> on a définit deux
//               méthodes reqObjet(), une const et l'autre non const.
//               La méthode non const est utilisée ici.
//               <p>
//               On doit implanter dans cette méthode la notion de "Copie sur
//               modification" puisque que cette méthode modifie l'objet.  Ainsi,
//               lorsque cette méthode est appelée on doit "cloner" l'objet
//               référencé pour avoir un comportement prévisible.
//               <p>
//               Si le pointeur à l'objet SYCompteReference (messageP)
//               est nul, un tel objet doit évidemment être créé.
//
// Entrée:       Type unType: le type du message.
//
// Sortie:
//
// Notes:
//************************************************************************
void ERMsg::asgType (Type type)
{
   _INVARIANTS();

   if (messageP != 0)
   {
      // --- On applique la théorie du "Copie sur modification"
      // --- On appelle clone() avant de modifier.
      clone();
      SYMessage::Type unType = static_cast<SYMessage::Type>(type);
      messageP->reqObjet().asgType(unType);
   }
   else
   {
      // --- Un nouveau message de type 'unType' est créé.
      SYMessage::Type unType = static_cast<SYMessage::Type>(type);
      messageP = new SYCompteReference<SYMessage>(SYMessage (unType));
   }

   _INVARIANTS();
}

//************************************************************************
// Sommaire:    Créer une copie de l'objet courant.
//
// Description: La méthode privée <code>clone()</code> permet de créer 
//              un clone de l'objet couramment référencé.  
//              On doit donc décrémenter notre compte de références sur 
//              l'objet courant et ensuite se cloner.  Par contre, 
//              si le compte de références est égal à 1, on n'a
//              pas besoin de cloner puisqu'on est seul à référer l'objet.
//              <p>
//              Donc pour implanter la "Copie sur modification", on n'a qu'à appeler
//              cette méthode à l'intérieur de chacune des méthodes modifiant
//              l'objet, i.e. les méthodes non const.
//
// Entrée:
//
// Sortie:
//
// Notes:
//************************************************************************
void ERMsg::clone()
{
   _PRECONDITION(messageP != 0);
   _INVARIANTS();

   // --- S'il ne reste que nous-même à référer à l'erreur
   // --- on n'a pas besoin de cloner.
   if (!messageP->referenceUnique()) 
   {
      // --- Cloner signifie ne plus référer à l'ancien message
      // --- mais plutôt prendre une copie de celui-ci et
      // --- partir une nouvelle branche.
      messageP->decrementeCompte();
      messageP = new SYCompteReference<SYMessage>
         (SYMessage(messageP->reqObjet()));
   }

   _POSTCONDITION(messageP->referenceUnique());
   _INVARIANTS();
}

//************************************************************************
// Sommaire:    Gère l'allocation des ressources et le compte de
//              références.
//
// Description: La méthode privée <code>copie(...)</code> gère l'allocation 
//              de ressources et le compte de références sur l'objet. 
//              Toutes les copies doivent passer par ici pour ne pas créer 
//              de trou dans la gestion du compte de références.
//
// Entrée:      const ERMsg& obj : un objet du même type.
//
// Sortie:
//
// Notes:
//************************************************************************
void ERMsg::copie(const ERMsg& obj)
{
   _PRECONDITION(messageP == 0);
   _INVARIANTS();

   messageP = obj.messageP;
   if (messageP != 0)
   {
      messageP->incrementeCompte();
   }

   _POSTCONDITION(messageP == obj.messageP);
   _INVARIANTS();
}

//************************************************************************
// Sommaire:    Gère la désallocation des ressources et le compte de
//              références.
//
// Description: La méthode privée <code>libere()</code> gère la désallocation de ressources
//              et le compte de références sur l'objet.  Toutes les
//              libérations doivent passer par ici pour ne pas créer de
//              trous dans la gestion du compte de références.
//
// Entrée:
//
// Sortie:
//
// Notes:
//************************************************************************
void ERMsg::libere()
{
   _PRECONDITION(messageP != 0);
   _INVARIANTS();

   if (messageP->referenceUnique())
   {
      // --- Il ne reste qu'une seule référence à l'objet,
      // --- on doit alors détruire l'objet.
      delete messageP;
   }
   else
   {
      messageP->decrementeCompte();
   }
   messageP = 0;

   _POSTCONDITION(messageP == 0);
   _INVARIANTS();
}

//************************************************************************
// Sommaire:     Cette méthode retourne le nombre de messages dans la liste.
//
// Description:  La méthode publique <code>dimension()</code> retourne le nombre
//               de chaînes de caractères dans le message.
//               <p>
//               Cette méthode réimplante la méthode de l'objet auquel
//               on référence.  On passe par le SYCompteReference<T>
//               pour accéder à l'objet.
//               <p>
//               Notons que dans SYCompteReference<T> on a défini deux
//               méthodes reqObjet, une const et l'autre non const.
//               La méthode const est utilisée ici.
//
// Entrée:
//
// Sortie:
//
// Notes:
//************************************************************************
unsigned int ERMsg::dimension() const
{
   _INVARIANTS();

   unsigned int n;

   if (messageP != 0)
   {
      n = messageP->reqObjet().dimension();
   }
   else
   {
      n = 0;
   }

   _INVARIANTS();
   return n;
}

//************************************************************************
// Sommaire:     Méthode d'accès au type du message.
//
// Description:  La méthode publique <code>reqType()</code> réimplante 
//               la méthode de l'objet auquel on référence.  
//               On passe par le SYCompteReference<T>
//               pour accéder à l'objet.
//               <p>
//               Notons que dans SYCompteReference<T> on a défini deux
//               méthodes reqObjet, une const et l'autre non const.
//               La méthode const est utilisée ici.
//               <p>
//               Si le pointeur à l'objet SYCompteReference (messageP)
//               est nul, on assume que le type du message est 'OK'.
//
// Entrée:
//
// Sortie:       Retourne le type de l'erreur.
//
// Notes:
//************************************************************************
ERMsg::Type ERMsg::reqType() const
{
   _INVARIANTS();

   ERMsg::Type typeRet;

   if (messageP != 0)
   {
	  SYMessage::Type unType = messageP->reqObjet().reqType();
      typeRet = static_cast<ERMsg::Type>(unType);
   }
   else
   {
      typeRet = ERMsg::OK;
   }

   _INVARIANTS();
   return typeRet;
}

//************************************************************************
// Sommaire:    Opérateur d'assignation de la classe.
//
// Description: L'opérateur d'assignation permet d'utiliser
//              l'assignation de façon cohérente et sans danger pour
//              l'intégrité du système de compte de référence.
//              <p>
//              Il faut d'abord libérer les ressources qu'on possède et gérer
//              le compte de références sur l'objet.  Ensuite on doit assigner
//              la nouvelle référence et gérer le compte de références.
//
// Entrée:      const ERMsg<T>& obj : un objet du même type.
//
// Sortie:      Un référence à l'objet lui-même pour permettre les
//              appel en cascade.
//
// Notes:
//************************************************************************
ERMsg&
ERMsg::operator=(const ERMsg& obj)
{
   _INVARIANTS();

   if (this != &obj) // --- Vérification de l'auto-assignation.
   {
      if (messageP != 0)
      {
         libere();
      }
      copie(obj);
   }

   _POSTCONDITION(messageP == obj.messageP);
   _INVARIANTS();
   return *this;
}

//************************************************************************
// Sommaire:    Opérateur de concaténation de la classe.
//
// Description: L'opérateur de contaténation permet d'utiliser
//              l'assignation de façon cohérente et sans danger pour
//              l'intégrité du système de compte de référence.
//              <p>
//              Il faut d'abord libérer les ressources qu'on possède et gérer
//              le compte de références sur l'objet.  Ensuite on doit assigner
//              la nouvelle référence et gérer le compte de références.
//
// Entrée:      const ERMsg<T>& obj : un objet du même type.
//
// Sortie:      Un référence à l'objet lui-même pour permettre les
//              appel en cascade.
//
// Notes:
//************************************************************************
ERMsg&
ERMsg::operator+=(const ERMsg& obj)
{
   _INVARIANTS();

   if (this != &obj) // --- Vérification de l'auto-assignation.
   {
	  if (obj.reqType() != OK)
      {
		 asgType(obj.reqType());
         ajoute( obj);
      }
   }

   _INVARIANTS();
   return *this;
}

//************************************************************************
// Sommaire:     Permet d'ajouter un message à un autre message.
//
// Description:  La méthode publique <code>ajoute(const ERMsg<T>&)</code>
//               permet d'ajouter un message en entier à la fin du message
//               existant.
//
// Entrée:       const ERMsg<T>& obj : un objet du même type.
//
// Sortie:
//
// Notes:
//************************************************************************
void ERMsg::ajoute(const ERMsg& obj)
{
   _INVARIANTS();

   if (messageP == 0 || (void*)messageP->reqObjet())
   {
      // --- Pas de message ou message à OK, on assigne simplement
      // --- le message passé en paramètre.
      *this = obj;
   }
   else
   {
      // --- Ajout à la fin du message des chaînes de caractères.
      unsigned int nbrMsg = obj.dimension();
      for (unsigned int i=0; i<nbrMsg; i++)
      {
         ajoute(obj[i]);
      }
   }

   _INVARIANTS();
}

//************************************************************************
// Sommaire:     Retourne la ieme chaîne de caractères du message.
//
// Description:  La méthode publique <code>operator[]</code> permet d'obtenir
//               la ieme chaîne de caractères du message.
//
// Entrée:       int indice : indice de la chaîne de caractères.
//
// Sortie:       Retourne une référence constante à la chaîne.
//
// Notes:
//************************************************************************
const std::string& ERMsg::operator [] (int indice) const
{
   _PRECONDITION(messageP != 0);
   _PRECONDITION(indice >= 0 && indice < (int)dimension());
   _INVARIANTS();

   const SYMessage& objet = messageP->reqObjet();
   return objet[indice];
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
ERMsg::operator void*() const
{
   _INVARIANTS();

   void* voidP;

   // --- Si le pointeur au message n'est pas NUL, on retourne le statut
   // --- de l'objet (VRAI si le message n'est pas en erreur, FAUX sinon).
   // --- Si le pointeur au message est NUL, on assume que le type celui-ci
   // --- est OK, ainsi on retourne VRAI.
   if (messageP != 0)
   {
      voidP = (void*)messageP->reqObjet();
   }
   else
   {
      voidP = (void*)true;
   }

   _INVARIANTS();
   return voidP;
}

