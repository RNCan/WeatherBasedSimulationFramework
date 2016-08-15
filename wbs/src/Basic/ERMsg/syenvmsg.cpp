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
// Fichier: syenvmsg.cpp
// Classe:  ERMsg
//************************************************************************
// 01-09-98  Yves Secretan, Yves Roy  Version �ducationnelle
//************************************************************************
#include "stdafx.h"
#include "basic/ERMsg/syenvmsg.h"

using namespace std;

//************************************************************************
// Sommaire:    Constructeur par d�faut de la classe.
//
// Description: Le constructeur par d�faut initialise les attributs � des valeurs
//              par d�faut.  Ainsi le type du message est OK par d�faut et messageP
//              est initialis� � NUL.
//
// Entr�e:      Type& unType: le type du message.
//
// Sortie:
//
// Notes:
//************************************************************************
ERMsg::ERMsg(Type unType)
: messageP(0)
{
   // --- Si le type du message est autre que OK, un objet
   // --- SYMessage est physiquement cr��.
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
// Description: Le constructeur avec param�tre initialise un message � l'aide
//              des param�tres.
//
// Entr�e:      Type&    type:    le type du message.
//              const T& chaine:  la cha�ne initiale du message.
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
// Description: Le constructeur copie construit l'objet � partir d'un objet du
//              m�me type pass� en param�tre.
//
// Entr�e:      obj: L'objet sur lequel se construire.
//
// Sortie:
//
// Notes:
//************************************************************************
ERMsg::ERMsg(const ERMsg &obj)
: messageP(0)
{
   // --- Appel de la m�thode priv�e qui s'occupe des copies et du compte
   // --- de r�f�rences.
   copie(obj);

   _POSTCONDITION(messageP == obj.messageP);
   _INVARIANTS();
}

//************************************************************************
// Sommaire:    Destructeur de la classe
//
// Description: Ce destructeur g�re le compte de r�f�rences avant de
//              laisser d�truire l'objet
//
// Entr�e:
//
// Sortie:
//
// Notes:
//************************************************************************
ERMsg::~ERMsg()
{
   _INVARIANTS();

   // --- Appel � la m�thode priv�e qui s'occupe de lib�rer les ressources
   // --- et qui g�re le compte de r�f�rences.
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
// Sommaire:     Cette m�thode ajoute une cha�ne de caract�res � la fin du message.
//
// Description:  La m�thode publique <code>ajoute(...)</code> fait le repr�sentant
//               de la m�thode de l'objet auquel on r�f�re.  
//               On passe par le SYCompteReference<T> pour acc�der � l'objet de message.
//               <p>
//               Notons que dans SYCompteReference<T> on a d�finit deux
//               m�thodes reqObjet, une const et l'autre non const.
//               La m�thode non const est utilis�e ici.
//               <p>
//               On doit implanter dans cette m�thode la notion de "Copie sur
//               modification" puisque que cette m�thode modifie l'objet.  Ainsi,
//               lorsque cette m�thode est appel�e on doit "cloner" l'objet
//               r�f�renc� pour avoir un comportement pr�visible.
//               <p>
//               Si le pointeur � l'objet SYCompteReference (messageP)
//               est nul, un tel objet doit �videmment �tre cr��.
//
// Entr�e:       const std::string& chaine: la cha�ne de caract�res � ajouter au message.
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
		// --- On applique la th�orie du "Copie sur Modification"
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
// Sommaire:     Cette m�thode ajoute un r�el � la fin du message.
//
// Description:  La m�thode publique <code>ajoute(...)</code> fait le repr�sentant
//               de la m�thode de l'objet auquel on r�f�re.  
//               On passe par le SYCompteReference<T> pour acc�der � l'objet de message.
//               <p>
//               Notons que dans SYCompteReference<T> on a d�finit deux
//               m�thodes reqObjet, une const et l'autre non const.
//               La m�thode non const est utilis�e ici.
//               <p>
//               On doit implanter dans cette m�thode la notion de "Copie sur
//               modification" puisque que cette m�thode modifie l'objet.  Ainsi,
//               lorsque cette m�thode est appel�e on doit "cloner" l'objet
//               r�f�renc� pour avoir un comportement pr�visible.
//               <p>
//               Si le pointeur � l'objet SYCompteReference (messageP)
//               est nul, un tel objet doit �videmment �tre cr��.
//
// Entr�e:       double val: le r�el � convertir et � ajouter au message.
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
      // --- On applique la th�orie du "Copie sur Modification"
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
// Sommaire:     Cette m�thode ajoute un entier � la fin du message.
//
// Description:  La m�thode publique <code>ajoute(...)</code> fait le repr�sentant
//               de la m�thode de l'objet auquel on r�f�re.  
//               On passe par le SYCompteReference<T> pour acc�der � l'objet de message.
//               <p>
//               Notons que dans SYCompteReference<T> on a d�finit deux
//               m�thodes reqObjet, une const et l'autre non const.
//               La m�thode non const est utilis�e ici.
//               <p>
//               On doit implanter dans cette m�thode la notion de "Copie sur
//               modification" puisque que cette m�thode modifie l'objet.  Ainsi,
//               lorsque cette m�thode est appel�e on doit "cloner" l'objet
//               r�f�renc� pour avoir un comportement pr�visible.
//               <p>
//               Si le pointeur � l'objet SYCompteReference (messageP)
//               est nul, un tel objet doit �videmment �tre cr��.
//
// Entr�e:       long val: l'entier � convertir et � ajouter au message.
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
      // --- On applique la th�orie du "Copie sur Modification"
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
// Sommaire:     Cette m�thode assigne un type au message.
//
// Description:  La m�thode publique <code>asgType(...)</code> r�implante 
//               la m�thode de l'objet auquel on r�f�re.  
//               On passe par le SYCompteReference<T>
//               pour acc�der � l'objet.
//               <p>
//               Notons que dans SYCompteReference<T> on a d�finit deux
//               m�thodes reqObjet(), une const et l'autre non const.
//               La m�thode non const est utilis�e ici.
//               <p>
//               On doit implanter dans cette m�thode la notion de "Copie sur
//               modification" puisque que cette m�thode modifie l'objet.  Ainsi,
//               lorsque cette m�thode est appel�e on doit "cloner" l'objet
//               r�f�renc� pour avoir un comportement pr�visible.
//               <p>
//               Si le pointeur � l'objet SYCompteReference (messageP)
//               est nul, un tel objet doit �videmment �tre cr��.
//
// Entr�e:       Type unType: le type du message.
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
      // --- On applique la th�orie du "Copie sur modification"
      // --- On appelle clone() avant de modifier.
      clone();
      SYMessage::Type unType = static_cast<SYMessage::Type>(type);
      messageP->reqObjet().asgType(unType);
   }
   else
   {
      // --- Un nouveau message de type 'unType' est cr��.
      SYMessage::Type unType = static_cast<SYMessage::Type>(type);
      messageP = new SYCompteReference<SYMessage>(SYMessage (unType));
   }

   _INVARIANTS();
}

//************************************************************************
// Sommaire:    Cr�er une copie de l'objet courant.
//
// Description: La m�thode priv�e <code>clone()</code> permet de cr�er 
//              un clone de l'objet couramment r�f�renc�.  
//              On doit donc d�cr�menter notre compte de r�f�rences sur 
//              l'objet courant et ensuite se cloner.  Par contre, 
//              si le compte de r�f�rences est �gal � 1, on n'a
//              pas besoin de cloner puisqu'on est seul � r�f�rer l'objet.
//              <p>
//              Donc pour implanter la "Copie sur modification", on n'a qu'� appeler
//              cette m�thode � l'int�rieur de chacune des m�thodes modifiant
//              l'objet, i.e. les m�thodes non const.
//
// Entr�e:
//
// Sortie:
//
// Notes:
//************************************************************************
void ERMsg::clone()
{
   _PRECONDITION(messageP != 0);
   _INVARIANTS();

   // --- S'il ne reste que nous-m�me � r�f�rer � l'erreur
   // --- on n'a pas besoin de cloner.
   if (!messageP->referenceUnique()) 
   {
      // --- Cloner signifie ne plus r�f�rer � l'ancien message
      // --- mais plut�t prendre une copie de celui-ci et
      // --- partir une nouvelle branche.
      messageP->decrementeCompte();
      messageP = new SYCompteReference<SYMessage>
         (SYMessage(messageP->reqObjet()));
   }

   _POSTCONDITION(messageP->referenceUnique());
   _INVARIANTS();
}

//************************************************************************
// Sommaire:    G�re l'allocation des ressources et le compte de
//              r�f�rences.
//
// Description: La m�thode priv�e <code>copie(...)</code> g�re l'allocation 
//              de ressources et le compte de r�f�rences sur l'objet. 
//              Toutes les copies doivent passer par ici pour ne pas cr�er 
//              de trou dans la gestion du compte de r�f�rences.
//
// Entr�e:      const ERMsg& obj : un objet du m�me type.
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
// Sommaire:    G�re la d�sallocation des ressources et le compte de
//              r�f�rences.
//
// Description: La m�thode priv�e <code>libere()</code> g�re la d�sallocation de ressources
//              et le compte de r�f�rences sur l'objet.  Toutes les
//              lib�rations doivent passer par ici pour ne pas cr�er de
//              trous dans la gestion du compte de r�f�rences.
//
// Entr�e:
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
      // --- Il ne reste qu'une seule r�f�rence � l'objet,
      // --- on doit alors d�truire l'objet.
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
// Sommaire:     Cette m�thode retourne le nombre de messages dans la liste.
//
// Description:  La m�thode publique <code>dimension()</code> retourne le nombre
//               de cha�nes de caract�res dans le message.
//               <p>
//               Cette m�thode r�implante la m�thode de l'objet auquel
//               on r�f�rence.  On passe par le SYCompteReference<T>
//               pour acc�der � l'objet.
//               <p>
//               Notons que dans SYCompteReference<T> on a d�fini deux
//               m�thodes reqObjet, une const et l'autre non const.
//               La m�thode const est utilis�e ici.
//
// Entr�e:
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
// Sommaire:     M�thode d'acc�s au type du message.
//
// Description:  La m�thode publique <code>reqType()</code> r�implante 
//               la m�thode de l'objet auquel on r�f�rence.  
//               On passe par le SYCompteReference<T>
//               pour acc�der � l'objet.
//               <p>
//               Notons que dans SYCompteReference<T> on a d�fini deux
//               m�thodes reqObjet, une const et l'autre non const.
//               La m�thode const est utilis�e ici.
//               <p>
//               Si le pointeur � l'objet SYCompteReference (messageP)
//               est nul, on assume que le type du message est 'OK'.
//
// Entr�e:
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
// Sommaire:    Op�rateur d'assignation de la classe.
//
// Description: L'op�rateur d'assignation permet d'utiliser
//              l'assignation de fa�on coh�rente et sans danger pour
//              l'int�grit� du syst�me de compte de r�f�rence.
//              <p>
//              Il faut d'abord lib�rer les ressources qu'on poss�de et g�rer
//              le compte de r�f�rences sur l'objet.  Ensuite on doit assigner
//              la nouvelle r�f�rence et g�rer le compte de r�f�rences.
//
// Entr�e:      const ERMsg<T>& obj : un objet du m�me type.
//
// Sortie:      Un r�f�rence � l'objet lui-m�me pour permettre les
//              appel en cascade.
//
// Notes:
//************************************************************************
ERMsg&
ERMsg::operator=(const ERMsg& obj)
{
   _INVARIANTS();

   if (this != &obj) // --- V�rification de l'auto-assignation.
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
// Sommaire:    Op�rateur de concat�nation de la classe.
//
// Description: L'op�rateur de contat�nation permet d'utiliser
//              l'assignation de fa�on coh�rente et sans danger pour
//              l'int�grit� du syst�me de compte de r�f�rence.
//              <p>
//              Il faut d'abord lib�rer les ressources qu'on poss�de et g�rer
//              le compte de r�f�rences sur l'objet.  Ensuite on doit assigner
//              la nouvelle r�f�rence et g�rer le compte de r�f�rences.
//
// Entr�e:      const ERMsg<T>& obj : un objet du m�me type.
//
// Sortie:      Un r�f�rence � l'objet lui-m�me pour permettre les
//              appel en cascade.
//
// Notes:
//************************************************************************
ERMsg&
ERMsg::operator+=(const ERMsg& obj)
{
   _INVARIANTS();

   if (this != &obj) // --- V�rification de l'auto-assignation.
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
// Sommaire:     Permet d'ajouter un message � un autre message.
//
// Description:  La m�thode publique <code>ajoute(const ERMsg<T>&)</code>
//               permet d'ajouter un message en entier � la fin du message
//               existant.
//
// Entr�e:       const ERMsg<T>& obj : un objet du m�me type.
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
      // --- Pas de message ou message � OK, on assigne simplement
      // --- le message pass� en param�tre.
      *this = obj;
   }
   else
   {
      // --- Ajout � la fin du message des cha�nes de caract�res.
      unsigned int nbrMsg = obj.dimension();
      for (unsigned int i=0; i<nbrMsg; i++)
      {
         ajoute(obj[i]);
      }
   }

   _INVARIANTS();
}

//************************************************************************
// Sommaire:     Retourne la ieme cha�ne de caract�res du message.
//
// Description:  La m�thode publique <code>operator[]</code> permet d'obtenir
//               la ieme cha�ne de caract�res du message.
//
// Entr�e:       int indice : indice de la cha�ne de caract�res.
//
// Sortie:       Retourne une r�f�rence constante � la cha�ne.
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

