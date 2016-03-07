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
//*****************************************************************************
// Fichier:  erexcept.cpp
// Classe :  ERExceptionContrat et héritiers
//*****************************************************************************
// 01-09-98  Yves Roy  Version éducationnelle
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
//    initialise l'objet à partir des valeurs passées en argument.
//
// Entrée:
//    const char*  fichP   : Fichier source dans lequel a eu lieu l'erreur
//    unsigned int ligne   : Ligne à laquelle a eu lieu l'erreur
//    const char*  msgP    : Message décrivant l'erreur: Exception de precondition...
//    const char*  exprP   : Test logique qui a échoué
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
//    est appelée par les héritiers au moment de leur destruction.
//
// Entrée:
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
// Sommaire:    Méthode d'interruption du programme.
//
// Description:
//    La méthode publique <code>arreteProgramme()</code> sert à interrompre
//    le programme. En mode normal, on crée un objet d'erreur de contrat et
//    on appelle immédiatement cette méthode. En mode exception, on crée un
//    objet erreur de contrat et on lance cet objet comme exception.  Lors du
//    traitement de l'exception, on pourra éventuellement appeler cette méthode.
//
// Entrée:
//
// Sortie:
//
// Notes:
//
//*****************************************************************************
void ERExceptionContrat::arreteProgramme()
{
   // ---  Prépare le message
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
//    classe est intéressante pour son TYPE lors du traitement des exceptions.
//
// Entrée:
//    const char*  fichP   : Fichier source dans lequel a eu lieu l'erreur
//    unsigned int ligne   : Ligne à laquelle a eu lieu l'erreur
//    const char*  exprP   : Test logique qui a échoué
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
// Sommaire:    Méthode de lancement de l'exception.
//
// Description:
//    La méthode publique <code>lanceException(...)</code> sert à lancer une
//    exception. Si le test logique est à true, on lance l'exception. Donc, si
//    l'on utilise cette méthode dans le cadre des préconditions, postconditions...,
//    il faut nier le test à l'appel de la méthode. L'appel de la méthode est
//    virtuel pour permettre de lancer le bon type d'exception.
//
// Entrée:      bool erreur: A true s'il y a une erreur.
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
//    classe est intéressante pour son TYPE lors du traitement des exceptions.
//    La classe représente le erreur de précondition dans la théorie du contrat.
//
// Entrée:
//    const char*  fichP   : Fichier source dans lequel a eu lieu l'erreur
//    unsigned int ligne   : Ligne à laquelle a eu lieu l'erreur
//    const char*  exprP   : Test logique qui a échoué
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
// Sommaire:    Méthode de lancement de l'exception.
//
// Description:
//    La méthode publique <code>lanceException(...)</code> sert à lancer une
//    exception. Si le test logique est à true, on lance l'exception. Donc, si
//    l'on utilise cette méthode dans le cadre des préconditions, postconditions...,
//    il faut nier le test à l'appel de la méthode. L'appel de la méthode est
//    virtuel pour permettre de lancer le bon type d'exception.
//
// Entrée:      bool erreur: A true s'il y a une erreur.
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
//    local. Cette classe est intéressante pour son TYPE lors du traitement des
//    exceptions. La classe représente des erreurs de postcondition dans la
//    théorie du contrat.
//
// Entrée:
//    const char*  fichP   : Fichier source dans lequel a eu lieu l'erreur
//    unsigned int ligne   : Ligne à laquelle a eu lieu l'erreur
//    const char*  exprP   : Test logique qui a échoué
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
// Sommaire:    Méthode de lancement de l'exception.
//
// Description:
//    La méthode publique <code>lanceException(...)</code> sert à lancer une
//    exception. Si le test logique est à true, on lance l'exception. Donc, si
//    l'on utilise cette méthode dans le cadre des préconditions, postconditions...,
//    il faut nier le test à l'appel de la méthode. L'appel de la méthode est
//    virtuel pour permettre de lancer le bon type d'exception.
//
// Entrée:      bool erreur: A true s'il y a une erreur.
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
//    classe est intéressante pour son TYPE lors du traitement des exceptions.
//    La classe représente des erreurs d'invariant dans la théorie du contrat.
//
// Entrée:
//    const char*  fichP   : Fichier source dans lequel a eu lieu l'erreur
//    unsigned int ligne   : Ligne à laquelle a eu lieu l'erreur
//    const char*  exprP   : Test logique qui a échoué
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
// Sommaire:    Méthode de lancement de l'exception.
//
// Description:
//    La méthode publique <code>lanceException(...)</code> sert à lancer une
//    exception. Si le test logique est à true, on lance l'exception. Donc, si
//    l'on utilise cette méthode dans le cadre des préconditions, postconditions...,
//    il faut nier le test à l'appel de la méthode. L'appel de la méthode est
//    virtuel pour permettre de lancer le bon type d'exception.
//
// Entrée:
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




