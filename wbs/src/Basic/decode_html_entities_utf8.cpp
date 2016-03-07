/*	Copyright 2012 Christoph Gärtner
Distributed under the Boost Software License, Version 1.0
*/
#include "stdafx.h"
#include "entities.h"


//#include <errno.h>
//#include <stdbool.h>
//#include <stdlib.h>
//#include <string.h>
//#include <windows.h>
#include <string>

#define UNICODE_MAX 0x10FFFFul
struct SEntities
{
	char* p1;
	wchar_t* p2;
};


static const SEntities NAMED_ENTITIES[] = {
	{	"AElig;",	L"Æ"	},
	{	"Aacute;",	L"Á"	},
	{	"Acirc;",	L"Â"	},
	{	"Agrave;",	L"À"	},
	{	"Alpha;",	L"Α"	},
	{	"Aring;",	L"Å"	},
	{	"Atilde;",	L"Ã"	},
	{	"Auml;",	L"Ä"	},
	{	"Beta;",	L"Β"	},
	{	"Ccedil;",	L"Ç"	},
	{	"Chi;",	L"Χ"	},
	{	"Dagger;",	L"‡"	},
	{	"Delta;",	L"Δ"	},
	{	"ETH;",	L"Ð"	},
	{	"Eacute;",	L"É"	},
	{	"Ecirc;",	L"Ê"	},
	{	"Egrave;",	L"È"	},
	{	"Epsilon;",L"Ε"	},
	{	"Eta;",	L"Η"	},
	{	"Euml;",	L"Ë"	},
	{	"Gamma;",	L"Γ"	},
	{	"Iacute;",	L"Í"	},
	{	"Icirc;",	L"Î"	},
	{	"Igrave;",	L"Ì"	},
	{	"Iota;",	L"Ι"	},
	{	"Iuml;",	L"Ï"	},
	{	"Kappa;",	L"Κ"	},
	{	"Lambda;",	L"Λ"	},
	{	"Mu;",		L"Μ"	},
	{	"Ntilde;",	L"Ñ"	},
	{	"Nu;",		L"Ν"	},
	{	"OElig;",	L"Œ"	},
	{	"Oacute;",	L"Ó"	},
	{	"Ocirc;",	L"Ô"	},
	{	"Ograve;",	L"Ò"	},
	{	"Omega;",	L"Ω"	},
	{	"Omicron;",L"Ο"	},
	{	"Oslash;",	L"Ø"	},
	{	"Otilde;",	L"Õ"	},
	{	"Ouml;",	L"Ö"	},
	{	"Phi;",	L"Φ"	},
	{	"Pi;",		L"Π"	},
	{	"Prime;",	L"″"	},
	{	"Psi;",	L"Ψ"	},
	{	"Rho;",	L"Ρ"	},
	{	"Scaron;",	L"Š"	},
	{	"Sigma;",	L"Σ"	},
	{	"THORN;",	L"Þ"	},
	{	"Tau;",	L"Τ"	},
	{	"Theta;",	L"Θ"	},
	{	"Uacute;",	L"Ú"	},
	{	"Ucirc;",	L"Û"	},
	{	"Ugrave;",	L"Ù"	},
	{	"Upsilon;",L"Υ"	},
	{	"Uuml;",	L"Ü"	},
	{	"Xi;",		L"Ξ"	},
	{	"Yacute;",	L"Ý"	},
	{	"Yuml;",	L"Ÿ"	},
	{	"Zeta;",	L"Ζ"	},
	{	"aacute;",	L"á"	},
	{	"acirc;",	L"â"	},
	{	"acute;",	L"´"	},
	{	"aelig;",	L"æ"	},
	{	"agrave;",	L"à"	},
	{	"alefsym;",L"ℵ"	},
	{	"alpha;",	L"α"	},
	{	"amp;",	L"&"	},
	{	"and;",	L"∧"	},
	{	"ang;",	L"∠"	},
	{	"apos;",	L"'"	},
	{	"aring;",	L"å"	},
	{	"asymp;",	L"≈"	},
	{	"atilde;",	L"ã"	},
	{	"auml;",	L"ä"	},
	{	"bdquo;",	L"„"	},
	{	"beta;",	L"β"	},
	{	"brvbar;",	L"¦"	},
	{	"bull;",	L"•"	},
	{	"cap;",	L"∩"	},
	{	"ccedil;",	L"ç"	},
	{	"cedil;",	L"¸"	},
	{	"cent;",	L"¢"	},
	{	"chi;",	L"χ"	},
	{	"circ;",	L"ˆ"	},
	{	"clubs;",	L"♣"	},
	{	"cong;",	L"≅"	},
	{	"copy;",	L"©"	},
	{	"crarr;",	L"↵"	},
	{	"cup;",	L"∪"	},
	{	"curren;",	L"¤"	},
	{	"dArr;",	L"⇓"	},
	{	"dagger;",	L"†"	},
	{	"darr;",	L"↓"	},
	{	"deg;",	L"°"	},
	{	"delta;",	L"δ"	},
	{	"diams;",	L"♦"	},
	{	"divide;",	L"÷"	},
	{	"eacute;",	L"é"	},
	{	"ecirc;",	L"ê"	},
	{	"egrave;",	L"è"	},
	{	"empty;",	L"∅"	},
	{	"emsp;",	L" "	},
	{	"ensp;",	L" "	},
	{	"epsilon;",L"ε"	},
	{	"equiv;",	L"≡"	},
	{	"eta;",	L"η"	},
	{	"eth;",	L"ð"	},
	{	"euml;",	L"ë"	},
	{	"euro;",	L"€"	},
	{	"exist;",	L"∃"	},
	{	"fnof;",	L"ƒ"	},
	{	"forall;",	L"∀"	},
	{	"frac12;",	L"½"	},
	{	"frac14;",	L"¼"	},
	{	"frac34;",	L"¾"	},
	{	"frasl;",	L"⁄"	},
	{	"gamma;",	L"γ"	},
	{	"ge;",		L"≥"	},
	{	"gt;",		L">"	},
	{	"hArr;",	L"⇔"	},
	{	"harr;",	L"↔"	},
	{	"hearts;",	L"♥"	},
	{	"hellip;",	L"…"	},
	{	"iacute;",	L"í"	},
	{	"icirc;",	L"î"	},
	{	"iexcl;",	L"¡"	},
	{	"igrave;",	L"ì"	},
	{	"image;",	L"ℑ"		},
	{	"infin;",	L"∞"	},
	{	"int;",	L"∫"	},
	{	"iota;",	L"ι"	},
	{	"iquest;",	L"¿"	},
	{	"isin;",	L"∈"	},
	{	"iuml;",	L"ï"	},
	{	"kappa;",	L"κ"	},
	{	"lArr;",	L"⇐"	},
	{	"lambda;",	L"λ"	},
	{	"lang;",	L"〈"	},
	{	"laquo;",	L"«"	},
	{	"larr;",	L"←"	},
	{	"lceil;",	L"⌈"	},
	{	"ldquo;",	L"“"	},
	{	"le;",		L"≤"	},
	{	"lfloor;",	L"⌊"	},
	{	"lowast;",	L"∗"	},
	{	"loz;",	L"◊"	},
	{	"lrm;",	L" "	},
	{	"lsaquo;",	L"‹"	},
	{	"lsquo;",	L"‘"	},
	{	"lt;",		L"<"	},
	{	"macr;",	L"¯"	},
	{	"mdash;",	L"—"	},
	{	"micro;",	L"µ"	},
	{	"middot;",	L"·"	},
	{	"minus;",	L"−"	},
	{	"mu;",		L"μ"	},
	{	"nabla;",	L"∇"	},
	{	"nbsp;",	L"	"	},
	{	"ndash;",	L"–"	},
	{	"ne;",		L"≠"	},
	{	"ni;",		L"∋"	},
	{	"not;",	L"¬"	},
	{	"notin;",	L"∉"	},
	{	"nsub;",	L"⊄"	},
	{	"ntilde;",	L"ñ"	},
	{	"nu;",		L"ν"	},
	{	"oacute;",	L"ó"	},
	{	"ocirc;",	L"ô"	},
	{	"oelig;",	L"œ"	},
	{	"ograve;",	L"ò"	},
	{	"oline;",	L"‾"	},
	{	"omega;",	L"ω"	},
	{	"omicron;",L"ο"	},
	{	"oplus;",	L"⊕"	},
	{	"or;",		L"∨"	},
	{	"ordf;",	L"ª"	},
	{	"ordm;",	L"º"	},
	{	"oslash;",	L"ø"	},
	{	"otilde;",	L"õ"	},
	{	"otimes;",	L"⊗"	},
	{	"ouml;",	L"ö"	},
	{	"para;",	L"¶"	},
	{	"part;",	L"∂"	},
	{	"permil;",	L"‰"	},
	{	"perp;",	L"⊥"	},
	{	"phi;",	L"φ"	},
	{	"pi;",		L"π"	},
	{	"piv;",	L"ϖ"	},
	{	"plusmn;",	L"±"	},
	{	"pound;",	L"£"	},
	{	"prime;",	L"′"	},
	{	"prod;",	L"∏"	},
	{	"prop;",	L"∝"	},
	{	"psi;",	L"ψ"	},
	{	"quot;",	L"\""	},
	{	"rArr;",	L"⇒"	},
	{	"radic;",	L"√"	},
	{	"rang;",	L"〉"	},
	{	"raquo;",	L"»"	},
	{	"rarr;",	L"→"	},
	{	"rceil;",	L"⌉"	},
	{	"rdquo;",	L"”"	},
	{	"real;",	L"ℜ"	},
	{	"reg;",	L"®"	},
	{	"rfloor;",	L"⌋"	},
	{	"rho;",	L"ρ"	},
	{	"rlm;",	L" " },
	{	"rsaquo;",	L"›"	},
	{	"rsquo;",	L"’"	},
	{	"sbquo;",	L"‚"	},
	{	"scaron;",	L"š"	},
	{	"sdot;",	L"⋅"	},
	{	"sect;",	L"§"	},
	{	"shy;",	L"­­­­"	},
	{	"sigma;",	L"σ"	},
	{	"sigmaf;",	L"ς"	},
	{	"sim;",	L"∼"	},
	{	"spades;",	L"♠"	},
	{	"sub;",	L"⊂"	},
	{	"sube;",	L"⊆"	},
	{	"sum;",	L"∑"	},
	{	"sup;",	L"⊃"	},
	{	"sup1;",	L"¹"	},
	{	"sup2;",	L"²"	},
	{	"sup3;",	L"³"	},
	{	"supe;",	L"⊇"	},
	{	"szlig;",	L"ß"	},
	{	"tau;",	L"τ"	},
	{	"there4;",	L"∴"	},
	{	"theta;",	L"θ"	},
	{	"thetasym;",L"ϑ"	},
	{	"thinsp;",	L" "	},
	{	"thorn;",	L"þ"	},
	{	"tilde;",	L"˜"	},
	{	"times;",	L"×"	},
	{	"trade;",	L"™"	},
	{	"uArr;",	L"⇑"	},
	{	"uacute;",	L"ú"	},
	{	"uarr;",	L"↑"	},
	{	"ucirc;",	L"û"	},
	{	"ugrave;",	L"ù"	},
	{	"uml;",	L"¨"	},
	{	"upsih;",	L"ϒ"	},
	{	"upsilon;",L"υ"	},
	{	"uuml;",	L"ü"	},
	{	"weierp;",	L"℘"	},
	{	"xi;",		L"ξ"	},
	{	"yacute;",	L"ý"	},
	{	"yen;",	L"¥"	},
	{	"yuml;",	L"ÿ"	},
	{	"zeta;",	L"ζ"	},
	{	"zwj;",	L" "	},
	{	"zwnj;",	L" "	}
};

static int cmp(const void *key, const void *value)
{
	return strncmp((const char *)key, *(const char **)value,
		strlen(*(const char **)value));
	//return wcsncmp((const wchar_t *)key, *(const wchar_t **)value,
		//wcslen(*(const wchar_t **)value));
}

static const wchar_t *get_named_entity(const char *name)
{
	//int len = strchr(name, ';') - name;
	
	//convert UTF8 to UTF16
	//int len = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, 0);

	//std::wstring w_name; w_name.resize(len);
	//MultiByteToWideChar(CP_UTF8, 0, name, -1, &(w_name[0]), len);

	const wchar_t *const *entity = (const wchar_t *const *)bsearch(name,
		NAMED_ENTITIES, sizeof NAMED_ENTITIES / sizeof *NAMED_ENTITIES,
		sizeof *NAMED_ENTITIES, cmp);

	return entity ? entity[1] : NULL;
}

static size_t putc_utf8(unsigned long cp, char *buffer)
{
	unsigned char *bytes = (unsigned char *)buffer;

	if (cp <= 0x007Ful)
	{
		bytes[0] = (unsigned char)cp;
		return 1;
	}

	if (cp <= 0x07FFul)
	{
		bytes[1] = (unsigned char)((2 << 6) | (cp & 0x3F));
		bytes[0] = (unsigned char)((6 << 5) | (cp >> 6));
		return 2;
	}

	if (cp <= 0xFFFFul)
	{
		bytes[2] = (unsigned char)((2 << 6) | (cp & 0x3F));
		bytes[1] = (unsigned char)((2 << 6) | ((cp >> 6) & 0x3F));
		bytes[0] = (unsigned char)((14 << 4) | (cp >> 12));
		return 3;
	}

	if (cp <= 0x10FFFFul)
	{
		bytes[3] = (unsigned char)((2 << 6) | (cp & 0x3F));
		bytes[2] = (unsigned char)((2 << 6) | ((cp >> 6) & 0x3F));
		bytes[1] = (unsigned char)((2 << 6) | ((cp >> 12) & 0x3F));
		bytes[0] = (unsigned char)((30 << 3) | (cp >> 18));
		return 4;
	}

	return 0;
}

static bool parse_entity(
	const char *current, char **to, const char **from)
{
	const char *end = strchr(current, ';');
	if (!end) return 0;

	if (current[1] == '#')
	{
		char *tail = NULL;
		int errno_save = errno;
		bool hex = current[2] == 'x' || current[2] == 'X';

		errno = 0;
		unsigned long cp = strtoul(
			current + (hex ? 3 : 2), &tail, hex ? 16 : 10);

		bool fail = errno || tail != end || cp > UNICODE_MAX;
		errno = errno_save;
		if (fail) return 0;

		*to += putc_utf8(cp, *to);
		*from = end + 1;

		return 1;
	}
	else
	{
		const wchar_t *w_entity = get_named_entity(&current[1]);
		if (!w_entity) return 0;

		//Convert UTF16 to UTF8
		int newLen = WideCharToMultiByte(CP_UTF8, 0, w_entity, -1, NULL, 0, 0, 0);

		std::string str;
		str.resize(newLen);
		WideCharToMultiByte(CP_UTF8, 0, w_entity, -1, &(str[0]), newLen, 0, 0);
		
		const char *entity = str.data();

		size_t len = strlen(entity);
		memcpy(*to, entity, len);

		*to += len;
		*from = end + 1;

		return 1;
	}
}

size_t decode_html_entities_utf8(char *dest, const char *src)
{
	if (!src) src = dest;

	char *to = dest;
	const char *from = src;

	for (const char *current; (current = strchr(from, '&'));)
	{
		memmove(to, from, (size_t)(current - from));
		to += current - from;

		if (parse_entity(current, &to, &from))
			continue;

		from = current;
		*to++ = *from++;
	}

	size_t remaining = strlen(from);

	memmove(to, from, remaining);
	to += remaining;
	*to = 0;

	return (size_t)(to - dest);
}