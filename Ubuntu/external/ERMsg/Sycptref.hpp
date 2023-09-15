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
// Fichier: sycptref.hpp
// Classe:  SYCompteReference
//************************************************************************
// 01-09-98  Yves Secretan, Yves Roy  Version �ducationnelle
//************************************************************************

//************************************************************************
// Sommaire:    Constructeur de la classe
//
// Description: Ce constructeur accepte un objet en parametre.
//              On place l'objet dans la variable interne et on initialise
//              le compte de r�f�rence � 1.
//
// Entr�e:      Un objet par r�f�rence.
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
// Description: Ce destructeur ne fait rien de particulier sauf v�rifier
//              ses invariants de classe.
//
// Entr�e:
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
// Sommaire:    Retourne une r�f�rence � l'objet contenu dans la classe
//
// Description: La m�thode publique <code>reqObjet()</code> permet d'obtenir
//              une r�f�rence � l'objet � l'int�rieur.
//              L'objet peut �tre modifi�.
//
// Entr�e:
//
// Sortie:      La r�f�rence � l'objet (non constante)
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
// Sommaire:    Retourne une reference constante � l'objet contenu
//              dans la classe.
//
// Description: La m�thode publique <code>reqObjet()</code> retourne une
//              r�f�rence � l'objet � l'int�rieur.
//              L'objet ne peut �tre modifi�, la r�f�rence est constante.
//
// Entr�e:
//
// Sortie:      La r�f�rence � l'objet constante
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
// Sommaire:    Retourne le compte de r�f�rences.
//
// Description: La m�thode publique <code>reqCompte()</code> retourne simplement
//              le compte de r�f�rences en retournant un entier non sign�.
//              La m�thode est constante.
//
// Entr�e:
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
// Sommaire:    Retourne un bool�en indiquant si l'objet est r�f�r� une
//              seule fois.
//
// Description: La m�thode publique <code>referenceUnique()</code> permet
//              de d�terminer si le compte de r�f�rence est � 1.
//              Le but de la m�thode est d'offrir une interface descriptive
//              et ainsi mieux documenter le code.
//              <p>
//              Cette fonction teste donc si le compteReference est � un.
//
// Entr�e:
//
// Sortie:      Un booleen � true si compteReference==1
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
// Sommaire:    Incr�mente le compte de r�f�rences.
//
// Description: La m�thode publique <code>incrementeCompte()</code>
//              incr�mente le compte de r�f�rences simplement
//              en ajoutant une unit� � l'attribut compteReference.
//
// Entr�e:
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
// Sommaire:    D�cr�mente le compte de r�f�rences.
//
// Description: La m�thode publique <code>decrementeCompte</code> d�cr�mente
//              le compte de r�f�rences simplement
//              en enlevant une unit� � l'attribut compteReference.
//
// Entr�e:
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





