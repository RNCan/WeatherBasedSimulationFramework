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
// Fichier:     erexcept.h
//
// Classes:     ERExceptionContrat, ERExceptionAssertion, ERExceptionPrecondition,
//              ERExceptionPostcondition, ERExceptionInvariant
//
// Sommaire:    Hi�rarchie pour la gestion des erreurs de th�orie du contrat
//
// Description: Ces classes constituent la hi�rarchie pour la gestion de la
//              th�orie du contrat.  Elle maintient les donn�es n�cessaires �
//              la sauvegarde des renseignements de l'erreur.  Cette classe et sa
//              hi�rarchie sont int�ressantes lors de l'utilisation des
//              exceptions.
//              <pre>
//              ERExceptionContrat:       Classe de base de la hi�rarchie.
//              ERExceptionAssertion:     Classe de gestion des erreurs d'assertion.
//              ERExceptionPrecondition:  Classe de gestion des erreurs de pr�condition.
//              ERExceptionPostcondition: Classe de gestion des erreurs de postcondition.
//              ERExceptionInvariant:     Classe de gestion des erreurs d'invariant.
//              </pre>
//              <p>
//              On note que la hi�rarchie est virtuelle et la classe de base
//              abstraite.  Ceci est n�cessaire pour supporter le lancement
//              d'exceptions � partir de la m�thode lanceException().
//              Chacune des classes d�riv�es doivent donc d�finir cette m�thode.
//
// Attributs:   const char*  messageP:    Le message pouvant servir de titre.
//              const char*  expressionP: Le test logique qui a �chou�.
//              const char*  fichierP:    Le nom du fichier.
//              unsigned int ligne:       Le num�ro de ligne
//
// Notes:  Cette hi�rarchie de classe est inspir� du livre de Horstmann:
//         Mastering object oriented design in C++, 1995, Ref:12-2007257.
//         Voir la classe ChiError sur la disquette fournie avec le livre.
//
//************************************************************************
// 01-09-98  Yves Roy  Version �ducationnelle
//************************************************************************
#ifndef __EREXCEPT_H
#define __EREXCEPT_H

#pragma warning( disable : 4786 )

#include <string>


class ERExceptionContrat
{
public:
                 ERExceptionContrat  (const char*, unsigned int, const char*, const char*);
   virtual       ~ERExceptionContrat ();
   void          arreteProgramme     ();
   virtual void  lanceException      (bool) = 0;

private:
   std::string message;
   std::string expression;
   std::string fichier;
   unsigned int ligne;
};

class ERExceptionAssertion : public ERExceptionContrat
{
public:
                ERExceptionAssertion(const char*, unsigned int, const char*);
   virtual void lanceException(bool);
};

class ERExceptionPrecondition : public ERExceptionContrat
{
public:
                ERExceptionPrecondition(const char*, unsigned int, const char*);
   virtual void lanceException(bool);
};

class ERExceptionPostcondition : public ERExceptionContrat
{
public:
                ERExceptionPostcondition(const char*, unsigned int, const char*);
   virtual void lanceException(bool);
};

class ERExceptionInvariant : public ERExceptionContrat
{
public:
                ERExceptionInvariant(const char*, unsigned int, const char*);
   virtual void lanceException(bool);
};


// --- D�finition des marcros de contr�le de la th�orie du contrat
//     suivant les diff�rents modes d'utilisation

// --- LE MODE TEST UNITAIRE
#if defined(MODE_TEST)

#  ifndef NDEBUG
#     define NDEBUG
#  endif

#  define _INVARIANTS() \
      verifieInvariant()

#  define _ASSERTION(f)     \
      ERExceptionAssertion(__FILE__,__LINE__, #f).lanceException(!(f))
#  define _PRECONDITION(f)  \
      ERExceptionPrecondition(__FILE__, __LINE__, #f).lanceException(!(f))
#  define _POSTCONDITION(f) \
      ERExceptionPostcondition(__FILE__, __LINE__, #f).lanceException(!(f))
#  define _INVARIANT(f)   \
      ERExceptionInvariant(__FILE__,__LINE__, #f).lanceException(!(f))

// --- LE MODE DEBUG STANDARD
#elif !defined(NDEBUG)

#  define _INVARIANTS() \
      verifieInvariant()

#  define _ASSERTION(f)     \
      if (!(f)) ERExceptionAssertion(__FILE__,__LINE__, #f).arreteProgramme()
#  define _PRECONDITION(f)  \
      if (!(f)) ERExceptionPrecondition(__FILE__, __LINE__, #f).arreteProgramme()
#  define _POSTCONDITION(f) \
      if (!(f)) ERExceptionPostcondition(__FILE__, __LINE__, #f).arreteProgramme()
#  define _INVARIANT(f)   \
      if (!(f)) ERExceptionInvariant(__FILE__,__LINE__, #f).arreteProgramme()

// --- LE MODE RELEASE
#else

#  define _PRECONDITION(f);
#  define _POSTCONDITION(f);
#  define _INVARIANTS();
#  define _INVARIANT(f);
#  define _ASSERTION(f);

#endif  // --- if defined (NDEBUG)


#endif  // --- ifndef __EREXCEPT_H



