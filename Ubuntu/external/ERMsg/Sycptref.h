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
// Fichier:      sycptref.h
//
// Classe:       SYCompteReference<T>
//
// Sommaire:     Compte de r�f�rences sur un objet quelconque.
//
// Description:  La classe utilitaire <code>SYCompteReference</code> est
//               le compte de r�f�rences sur un objet quelconque.
//               Cette classe conserve simplement le compte de r�f�rences
//               sur l'objet qu'elle poss�de.  Le compte de r�f�rences est
//               pilot� par l'enveloppe de l'objet.
//               <p>
//               Cette classe ne devrait donc pas �tre utilis�e seule, elle
//               est con�ue pour �tre utilis�e � l'int�rieur de n'importe quelle
//               enveloppe implantant le compte de r�f�rences sur un objet.
//               <p>
//               Cette classe offre ou impose une interface pour faire le
//               compte de r�f�rences sur n'importe quel objet.  L'objet lui-
//               m�me fonctionne normalement.
//
// Attributs:    unObjet:         l'instance de l'objet lui-m�me
//               compteReference: le compte de r�f�rence sur cet objet
//
// Notes:
//
//************************************************************************
// 01-09-98  Yves Secretan, Yves Roy  Version �ducationnelle
//************************************************************************
#ifndef __SYCPTREF_H
#define __SYCPTREF_H

#include "external/ERMsg/Erexcept.h"

template <class T>
class SYCompteReference
{
public:
    SYCompteReference  (const T& obj);
    ~SYCompteReference ();
    T&            reqObjet           ();
    const T&      reqObjet           () const;
    unsigned int  reqCompte          () const;
    bool          referenceUnique    () const;
    unsigned int  incrementeCompte   ();
    unsigned int  decrementeCompte   ();

protected:
    void          verifieInvariant   () const;

private:
    // --- Ne pas utiliser constructeur copie et op�rateur d'assignation
    SYCompteReference (const SYCompteReference<T>&);
    SYCompteReference<T>& operator=         (const SYCompteReference<T> &obj);

    T    unObjet;
    int  compteReference;
};

#include "Sycptref.hpp"

//**************************************************************
// Sommaire:    Invariants de classe
//
// Description: Cette classe ne devrait jamais exister si le
//              compte de r�f�rences est inf�rieur � z�ro.  Donc l'invariant est
//              que le compte de r�f�rences est toujours plus grand ou
//              �gal � z�ro.  Le compte de z�ro est permis lors de l'�v�nement
//              transitoire entre le d�cr�ment du compte et le delete de
//              cette classe.
//
// Entr�e:
//
// Sortie:
//
// Notes:
//
//**************************************************************
template<class T>
inline void SYCompteReference<T>::verifieInvariant() const
{
    _INVARIANT(compteReference >= 0);
}

#endif // ifndef __SYCPTREF_H



