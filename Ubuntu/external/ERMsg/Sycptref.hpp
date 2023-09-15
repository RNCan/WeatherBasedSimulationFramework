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
// Fichier: sycptref.hpp
// Classe:  SYCompteReference
//************************************************************************
// 01-09-98  Yves Secretan, Yves Roy  Version éducationnelle
//************************************************************************

//************************************************************************
// Sommaire:    Constructeur de la classe
//
// Description: Ce constructeur accepte un objet en parametre.
//              On place l'objet dans la variable interne et on initialise
//              le compte de référence à 1.
//
// Entrée:      Un objet par référence.
//
// Sortie:
//
// Notes:
//
//************************************************************************
template<class T>
inline SYCompteReference<T>::SYCompteReference(const T& obj)
    : unObjet(obj), compteReference(1)
{
    _POSTCONDITION(compteReference == 1);
    _INVARIANTS();
}

//************************************************************************
// Sommaire:    Destructeur de la classe
//
// Description: Ce destructeur ne fait rien de particulier sauf vérifier
//              ses invariants de classe.
//
// Entrée:
//
// Sortie:
//
// Notes:
//
//************************************************************************
template<class T>
inline SYCompteReference<T>::~SYCompteReference()
{
    _INVARIANTS();
}

//************************************************************************
// Sommaire:    Retourne une référence à l'objet contenu dans la classe
//
// Description: La méthode publique <code>reqObjet()</code> permet d'obtenir
//              une référence à l'objet à l'intérieur.
//              L'objet peut être modifié.
//
// Entrée:
//
// Sortie:      La référence à l'objet (non constante)
//
// Notes:
//
//************************************************************************
template<class T>
inline T& SYCompteReference<T>::reqObjet()
{
    _INVARIANTS();
    return unObjet;
}

//************************************************************************
// Sommaire:    Retourne une reference constante à l'objet contenu
//              dans la classe.
//
// Description: La méthode publique <code>reqObjet()</code> retourne une
//              référence à l'objet à l'intérieur.
//              L'objet ne peut être modifié, la référence est constante.
//
// Entrée:
//
// Sortie:      La référence à l'objet constante
//
// Notes:
//
//************************************************************************
template<class T>
inline const T& SYCompteReference<T>::reqObjet() const
{
    _INVARIANTS();
    return unObjet;
}

//************************************************************************
// Sommaire:    Retourne le compte de références.
//
// Description: La méthode publique <code>reqCompte()</code> retourne simplement
//              le compte de références en retournant un entier non signé.
//              La méthode est constante.
//
// Entrée:
//
// Sortie:      Le compteReference.
//
// Notes:
//
//************************************************************************
template<class T>
inline unsigned int SYCompteReference<T>::reqCompte() const
{
    _INVARIANTS();
    return (unsigned int)compteReference;
}

//************************************************************************
// Sommaire:    Retourne un booléen indiquant si l'objet est référé une
//              seule fois.
//
// Description: La méthode publique <code>referenceUnique()</code> permet
//              de déterminer si le compte de référence est à 1.
//              Le but de la méthode est d'offrir une interface descriptive
//              et ainsi mieux documenter le code.
//              <p>
//              Cette fonction teste donc si le compteReference est à un.
//
// Entrée:
//
// Sortie:      Un booleen à true si compteReference==1
//
// Notes:
//
//************************************************************************
template<class T>
inline bool SYCompteReference<T>::referenceUnique() const
{
    _INVARIANTS();
    return (compteReference == 1);
}

//************************************************************************
// Sommaire:    Incrémente le compte de références.
//
// Description: La méthode publique <code>incrementeCompte()</code>
//              incrémente le compte de références simplement
//              en ajoutant une unité à l'attribut compteReference.
//
// Entrée:
//
// Sortie:      Le nouveau compteReference.
//
// Notes:
//
//************************************************************************
template<class T>
inline unsigned int SYCompteReference<T>::incrementeCompte()
{
    _INVARIANTS();
    return (unsigned int)compteReference++;
}

//************************************************************************
// Sommaire:    Décrémente le compte de références.
//
// Description: La méthode publique <code>decrementeCompte</code> décrémente
//              le compte de références simplement
//              en enlevant une unité à l'attribut compteReference.
//
// Entrée:
//
// Sortie:      Le nouveau compteReference.
//
// Notes:
//
//************************************************************************
template<class T>
inline unsigned int SYCompteReference<T>::decrementeCompte()
{
    _INVARIANTS();
    return (unsigned int)compteReference--;
}





