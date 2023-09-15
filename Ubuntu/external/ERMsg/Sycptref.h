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
// Fichier:      sycptref.h
//
// Classe:       SYCompteReference<T>
//
// Sommaire:     Compte de références sur un objet quelconque.
//
// Description:  La classe utilitaire <code>SYCompteReference</code> est
//               le compte de références sur un objet quelconque.
//               Cette classe conserve simplement le compte de références
//               sur l'objet qu'elle possède.  Le compte de références est
//               piloté par l'enveloppe de l'objet.
//               <p>
//               Cette classe ne devrait donc pas être utilisée seule, elle
//               est conçue pour être utilisée à l'intérieur de n'importe quelle
//               enveloppe implantant le compte de références sur un objet.
//               <p>
//               Cette classe offre ou impose une interface pour faire le
//               compte de références sur n'importe quel objet.  L'objet lui-
//               même fonctionne normalement.
//
// Attributs:    unObjet:         l'instance de l'objet lui-même
//               compteReference: le compte de référence sur cet objet
//
// Notes:
//
//************************************************************************
// 01-09-98  Yves Secretan, Yves Roy  Version éducationnelle
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
    // --- Ne pas utiliser constructeur copie et opérateur d'assignation
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
//              compte de références est inférieur à zéro.  Donc l'invariant est
//              que le compte de références est toujours plus grand ou
//              égal à zéro.  Le compte de zéro est permis lors de l'événement
//              transitoire entre le décrément du compte et le delete de
//              cette classe.
//
// Entrée:
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



