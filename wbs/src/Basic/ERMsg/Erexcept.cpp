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
//*****************************************************************************
// Fichier:  erexcept.cpp
// Classe :  ERExceptionContrat et h�ritiers
//*****************************************************************************
// 01-09-98  Yves Roy  Version �ducationnelle
//************************************************************************
#include "stdafx.h"
#include "basic/ERMsg/erexcept.h"
#include <strstream>
#include <iostream>
#include <crtdbg.h>

using namespace std;


//*****************************************************************************
// Sommaire: Contructeur de la classe de base ERExceptionContrat
// 
// Description:
//    Le constructeur public <code>ERExceptionContrat(...)</code>
//    initialise l'objet � partir des valeurs pass�es en argument.
//
// Entr�e:
//    const char*  fichP   : Fichier source dans lequel a eu lieu l'erreur
//    unsigned int ligne   : Ligne � laquelle a eu lieu l'erreur
//    const char*  msgP    : Message d�crivant l'erreur: Exception de precondition...
//    const char*  exprP   : Test logique qui a �chou�
//
// Sortie:
//
// Notes:
//
//*****************************************************************************
ERExceptionContrat::ERExceptionContrat(const char* fichP, unsigned int prmLigne,
                                       const char* exprP, const char* msgP)
: message(msgP), expression(exprP), fichier(fichP), ligne(prmLigne)
{
}

//*****************************************************************************
// Sommaire: Destructeur de la classe de base ERExceptionContrat
//
// Description:
//    Le destructeur <code>~ERExceptionContrat()</code>
//    est appel�e par les h�ritiers au moment de leur destruction.
//
// Entr�e:
//
// Sortie:
//
// Notes:
//
//*****************************************************************************
ERExceptionContrat::~ERExceptionContrat()
{
}

//*****************************************************************************
// Sommaire:    M�thode d'interruption du programme.
//
// Description:
//    La m�thode publique <code>arreteProgramme()</code> sert � interrompre
//    le programme. En mode normal, on cr�e un objet d'erreur de contrat et
//    on appelle imm�diatement cette m�thode. En mode exception, on cr�e un
//    objet erreur de contrat et on lance cet objet comme exception.  Lors du
//    traitement de l'exception, on pourra �ventuellement appeler cette m�thode.
//
// Entr�e:
//
// Sortie:
//
// Notes:
//
//*****************************************************************************
void ERExceptionContrat::arreteProgramme()
{
   // ---  Pr�pare le message
   char buffer[1024];
   ostrstream os(buffer, 1023);
   os << "Fichier : " << fichier << endl;
   os << "Ligne   : " << ligne << endl;
   os << "Test    : " << expression << endl << ends;

   // ---  Affiche
   cout << message << endl << buffer;
   _ASSERTE(false);
   //exit(1);
}

//*****************************************************************************
// Sommaire: Contructeur de la classe ERExceptionAssertion
//
// Description:
//    Le constructeur public <code>ERExceptionAssertion(...)</code> initialise
//    sa classe de base ERExceptionContrat. On n'a pas d'attribut local. Cette
//    classe est int�ressante pour son TYPE lors du traitement des exceptions.
//
// Entr�e:
//    const char*  fichP   : Fichier source dans lequel a eu lieu l'erreur
//    unsigned int ligne   : Ligne � laquelle a eu lieu l'erreur
//    const char*  exprP   : Test logique qui a �chou�
//
// Sortie:
//
// Notes:
//
//*****************************************************************************
ERExceptionAssertion::ERExceptionAssertion(const char* fichP, unsigned int prmLigne,
                                           const char* exprP)
: ERExceptionContrat(fichP, prmLigne, exprP, "ERREUR D'ASSERTION")
{
}

//*****************************************************************************
// Sommaire:    M�thode de lancement de l'exception.
//
// Description:
//    La m�thode publique <code>lanceException(...)</code> sert � lancer une
//    exception. Si le test logique est � true, on lance l'exception. Donc, si
//    l'on utilise cette m�thode dans le cadre des pr�conditions, postconditions...,
//    il faut nier le test � l'appel de la m�thode. L'appel de la m�thode est
//    virtuel pour permettre de lancer le bon type d'exception.
//
// Entr�e:      bool erreur: A true s'il y a une erreur.
//
// Sortie:
//
// Notes:
//
//*****************************************************************************
void ERExceptionAssertion::lanceException(bool erreur)
{
   if (erreur)
   {
      throw (*this);
   }
}

//*****************************************************************************
// Sommaire: Contructeur de la classe ERExceptionPrecondition
//
// Description:
//    Le constructeur public <code>ERExceptionPrecondition(...)</code> initialise
//    sa classe de base ERExceptionContrat. On n'a pas d'attribut local. Cette
//    classe est int�ressante pour son TYPE lors du traitement des exceptions.
//    La classe repr�sente le erreur de pr�condition dans la th�orie du contrat.
//
// Entr�e:
//    const char*  fichP   : Fichier source dans lequel a eu lieu l'erreur
//    unsigned int ligne   : Ligne � laquelle a eu lieu l'erreur
//    const char*  exprP   : Test logique qui a �chou�
//
// Sortie:
//
// Notes:
//
//*****************************************************************************
ERExceptionPrecondition::ERExceptionPrecondition(const char* fichP, unsigned int prmLigne,
                                                 const char* exprP)
: ERExceptionContrat(fichP, prmLigne, exprP, "ERREUR DE PRECONDITION")
{
}

//*****************************************************************************
// Sommaire:    M�thode de lancement de l'exception.
//
// Description:
//    La m�thode publique <code>lanceException(...)</code> sert � lancer une
//    exception. Si le test logique est � true, on lance l'exception. Donc, si
//    l'on utilise cette m�thode dans le cadre des pr�conditions, postconditions...,
//    il faut nier le test � l'appel de la m�thode. L'appel de la m�thode est
//    virtuel pour permettre de lancer le bon type d'exception.
//
// Entr�e:      bool erreur: A true s'il y a une erreur.
//
// Sortie:
//
// Notes:
//
//*****************************************************************************
void ERExceptionPrecondition::lanceException(bool erreur)
{
   if (erreur)
   {
      throw (*this);
   }
}

//*****************************************************************************
// Sommaire: Contructeur de la classe ERExceptionPostcondition
//
// Description:
//    Le constructeur public <code>ERExceptionPostcondition(...)</code>
//    initialise sa classe de base ERExceptionContrat.  On n'a pas d'attribut
//    local. Cette classe est int�ressante pour son TYPE lors du traitement des
//    exceptions. La classe repr�sente des erreurs de postcondition dans la
//    th�orie du contrat.
//
// Entr�e:
//    const char*  fichP   : Fichier source dans lequel a eu lieu l'erreur
//    unsigned int ligne   : Ligne � laquelle a eu lieu l'erreur
//    const char*  exprP   : Test logique qui a �chou�
//
// Sortie:
//
// Notes:
//
//*****************************************************************************
ERExceptionPostcondition::ERExceptionPostcondition(const char* fichP, unsigned int prmLigne,
                                                   const char* exprP)
: ERExceptionContrat(fichP, prmLigne, exprP, "ERREUR DE POSTCONDITION")
{
}

//*****************************************************************************
// Sommaire:    M�thode de lancement de l'exception.
//
// Description:
//    La m�thode publique <code>lanceException(...)</code> sert � lancer une
//    exception. Si le test logique est � true, on lance l'exception. Donc, si
//    l'on utilise cette m�thode dans le cadre des pr�conditions, postconditions...,
//    il faut nier le test � l'appel de la m�thode. L'appel de la m�thode est
//    virtuel pour permettre de lancer le bon type d'exception.
//
// Entr�e:      bool erreur: A true s'il y a une erreur.
//
// Sortie:
//
// Notes:
//
//*****************************************************************************
void ERExceptionPostcondition::lanceException(bool erreur)
{
   if (erreur)
   {
      throw (*this);
   }
}

//*****************************************************************************
// Sommaire: Contructeur de la classe ERExceptionInvariant
//
// Description:
//    Le constructeur public <code>ERExceptionInvariant(...)</code> initialise
//    sa classe de base ERExceptionContrat. On n'a pas d'attribut local. Cette
//    classe est int�ressante pour son TYPE lors du traitement des exceptions.
//    La classe repr�sente des erreurs d'invariant dans la th�orie du contrat.
//
// Entr�e:
//    const char*  fichP   : Fichier source dans lequel a eu lieu l'erreur
//    unsigned int ligne   : Ligne � laquelle a eu lieu l'erreur
//    const char*  exprP   : Test logique qui a �chou�
//
// Sortie:
//
// Notes:
//    La condition n'est pas tenu en compte pour l'instant.
//
//*****************************************************************************
ERExceptionInvariant::ERExceptionInvariant(const char* fichP, unsigned int prmLigne,
                                           const char* exprP)
: ERExceptionContrat(fichP, prmLigne, exprP, "ERREUR D'INVARIANT")
{
}

//*****************************************************************************
// Sommaire:    M�thode de lancement de l'exception.
//
// Description:
//    La m�thode publique <code>lanceException(...)</code> sert � lancer une
//    exception. Si le test logique est � true, on lance l'exception. Donc, si
//    l'on utilise cette m�thode dans le cadre des pr�conditions, postconditions...,
//    il faut nier le test � l'appel de la m�thode. L'appel de la m�thode est
//    virtuel pour permettre de lancer le bon type d'exception.
//
// Entr�e:
//    bool erreur: A true s'il y a une erreur.
//
// Sortie:
//
// Notes:
//
//*****************************************************************************
void ERExceptionInvariant::lanceException(bool erreur)
{
   if (erreur)
   {
      throw (*this);
   }
}




