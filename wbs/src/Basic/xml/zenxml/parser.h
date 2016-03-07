// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef ZEN_XML_PARSER_HEADER_81248670213764583021432
#define ZEN_XML_PARSER_HEADER_81248670213764583021432

#include <cstdio>
#include <cstddef> //ptrdiff_t; req. on Linux
#include "basic/xml/zen/string_traits.h"
#include "basic/xml/zenXml/dom.h"
#include "basic/xml/zenXml/error.h"

namespace zen
{
/**
\file
\brief Convert an XML document object model (class XmlDoc) to and from a byte stream representation.
*/

///Save XML document as a byte stream
/**
\param doc Input XML document
\param lineBreak Line break, default: carriage return + new line
\param indent Indentation, default: four space characters
\return Output byte stream
*/
std::string serialize(const XmlDoc& doc,
                      const std::string& lineBreak = "\r\n",
                      const std::string& indent = "    "); //throw ()

///Exception thrown due to an XML parsing error
struct XmlParsingError : public XmlError
{
    XmlParsingError(size_t rowNo, size_t colNo) : row(rowNo), col(colNo) {}
    ///Input file row where the parsing error occured (zero-based)
    size_t row; //beginning with 0
    ///Input file column where the parsing error occured (zero-based)
    size_t col; //
};


///Load XML document from a byte stream
/**
\param stream Input byte stream
\returns Output XML document
\throw XmlParsingError
*/
XmlDoc parse(const std::string& stream); //throw XmlParsingError




















//---------------------------- implementation ----------------------------
//see: http://www.w3.org/TR/xml/

namespace implementation
{
inline
std::pair<char, char> hexify(unsigned char c)
{
    auto hexifyDigit = [](int num) -> char //input [0, 15], output 0-9, A-F
    {
        assert(0 <= num&&  num <= 15); //guaranteed by design below!
        return static_cast<char>(num <= 9 ? //no signed/unsigned char problem here!
        '0' + num :
        'A' + (num - 10));
    };
    return std::make_pair(hexifyDigit(c / 16), hexifyDigit(c % 16));
}


inline
char unhexify(char high, char low)
{
    auto unhexifyDigit = [](char hex) -> int //input 0-9, a-f, A-F; output range: [0, 15]
    {
        if ('0' <= hex && hex <= '9') //no signed/unsigned char problem here!
            return hex - '0';
        else if ('A' <= hex && hex <= 'F')
            return (hex - 'A') + 10;
        else if ('a' <= hex && hex <= 'f')
            return (hex - 'a') + 10;
        assert(false);
        return 0;
    };
    return static_cast<unsigned char>(16 * unhexifyDigit(high) + unhexifyDigit(low)); //[!] convert to unsigned char first, then to char (which may be signed)
};


template <class Predicate> inline
std::string normalize(const std::string& str, Predicate pred) //pred: unary function taking a char, return true if value shall be encoded as hex
{
    std::string output;
    for (const char c : str)
    {
        if (c == '&')      //
            output += "&amp;";
        else if (c == '<') //normalization mandatory: http://www.w3.org/TR/xml/#syntax
            output += "&lt;";
        else if (c == '>') //
            output += "&gt;";
        else if (pred(c))
        {
            if (c == '\'')
                output += "&apos;";
            else if (c == '\"')
                output += "&quot;";
            else
            {
                output += "&#x";
                const auto hexDigits = hexify(c); //hexify beats "printNumber<std::string>("&#x%02X;", c)" by a nice factor of 3!
                output += hexDigits.first;
                output += hexDigits.second;
                output += ';';
            }
        }
        else
            output += c;
    }
    return output;
}

inline
std::string normalizeName(const std::string& str)
{
    return normalize(str, [](char c) { return isWhiteSpace(c) || c == '=' || c == '/' || c == '\'' || c == '\"'; });
}

inline
std::string normalizeElementValue(const std::string& str)
{
    return normalize(str, [](char c) { return static_cast<unsigned char>(c) < 32; });
}

inline
std::string normalizeAttribValue(const std::string& str)
{
    return normalize(str, [](char c) { return static_cast<unsigned char>(c) < 32 || c == '\'' || c == '\"'; });
}


template <class CharIterator, size_t N> inline
bool checkEntity(CharIterator& first, CharIterator last, const char (&placeholder)[N])
{
    assert(placeholder[N - 1] == 0);
    const ptrdiff_t strLen = N - 1; //don't count null-terminator
    if (last - first >= strLen && std::equal(first, first + strLen, placeholder))
    {
        first += strLen - 1;
        return true;
    }
    return false;
}


namespace
{
std::string denormalize(const std::string& str)
{
    std::string output;
    for (auto it = str.begin(); it != str.end(); ++it)
    {
        const char c = *it;

        if (c == '&')
        {
            if (checkEntity(it, str.end(), "&amp;"))
                output += '&';
            else if (checkEntity(it, str.end(), "&lt;"))
                output += '<';
            else if (checkEntity(it, str.end(), "&gt;"))
                output += '>';
            else if (checkEntity(it, str.end(), "&apos;"))
                output += '\'';
            else if (checkEntity(it, str.end(), "&quot;"))
                output += '\"';
            else if (str.end() - it >= 6 &&
                     it[1] == '#' &&
                     it[2] == 'x' &&
                     it[5] == ';')
            {
                output += unhexify(it[3], it[4]); //unhexify beats "::sscanf(&it[3], "%02X", &tmp)" by a factor of 3000 for ~250000 calls!!!
                it += 5;
            }
            else
                output += c; //unexpected char!
        }
        else if (c == '\r') //map all end-of-line characters to \n http://www.w3.org/TR/xml/#sec-line-ends
        {
            auto itNext = it + 1;
            if (itNext != str.end() && *itNext == '\n')
                ++it;
            output += '\n';
        }
        else
            output += c;
    };
    return output;
}


void serialize(const XmlElement& element, std::string& stream,
               const std::string& lineBreak,
               const std::string& indent,
               size_t indentLevel)
{
    const std::string& nameFmt = normalizeName(element.getNameAs<std::string>());

    for (size_t i = 0; i < indentLevel; ++i)
        stream += indent;

    stream += '<' + nameFmt;

    auto attr = element.getAttributes();
    for (auto it = attr.first; it != attr.second; ++it)
        stream += ' ' + normalizeName(it->first) + "=\"" + normalizeAttribValue(it->second) + '\"';

    //no support for mixed-mode content
    auto iterPair = element.getChildren();
    if (iterPair.first != iterPair.second) //structured element
    {
        stream += '>' + lineBreak;

        std::for_each(iterPair.first, iterPair.second,
        [&](const XmlElement & el) { serialize(el, stream, lineBreak, indent, indentLevel + 1); });

        for (size_t i = 0; i < indentLevel; ++i)
            stream += indent;
        stream += "</" + nameFmt + '>' + lineBreak;
    }
    else
    {
        std::string value;
        element.getValue(value);

        if (!value.empty()) //value element
            stream += '>' + normalizeElementValue(value) + "</" + nameFmt + '>' + lineBreak;
        else //empty element
            stream += "/>" + lineBreak;
    }
}

std::string serialize(const XmlDoc& doc,
                      const std::string& lineBreak,
                      const std::string& indent)
{
    std::string version = doc.getVersionAs<std::string>();
    if (!version.empty())
        version = " version=\"" + normalizeAttribValue(version) + '\"';

    std::string encoding = doc.getEncodingAs<std::string>();
    if (!encoding.empty())
        encoding = " encoding=\"" + normalizeAttribValue(encoding) + '\"';

    std::string standalone = doc.getStandaloneAs<std::string>();
    if (!standalone.empty())
        standalone = " standalone=\"" + normalizeAttribValue(standalone) + '\"';

    std::string output = "<?xml" + version + encoding + standalone + "?>" + lineBreak;
    serialize(doc.root(), output, lineBreak, indent, 0);
    return output;
}
}
}

inline
std::string serialize(const XmlDoc& doc,
                      const std::string& lineBreak,
                      const std::string& indent) { return implementation::serialize(doc, lineBreak, indent); }

/*
Grammar for XML parser
-------------------------------
document-expression:
	<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
    element-expression:

element-expression:
	<string attributes-expression/>
	<string attributes-expression> pm-expression </string>

element-list-expression:
	<empty>
	element-expression element-list-expression

attributes-expression:
	<empty>
	string="string" attributes-expression

pm-expression:
	string
    element-list-expression
*/

namespace implementation
{
struct Token
{
    enum Type
    {
        TK_LESS,
        TK_GREATER,
        TK_LESS_SLASH,
        TK_SLASH_GREATER,
        TK_EQUAL,
        TK_QUOTE,
        TK_DECL_BEGIN,
        TK_DECL_END,
        TK_NAME,
        TK_END
    };

    Token(Type t) : type(t) {}
    Token(const std::string& txt) : type(TK_NAME), name(txt) {}

    Type type;
    std::string name; //filled if type == TK_NAME
};

class Scanner
{
public:
    Scanner(const std::string& stream) :
        xmlCommentBegin("<!--"),
        xmlCommentEnd  ("-->"),
        stream_(stream),
        pos(stream_.begin())
    {
        if (zen::startsWith(stream_, BYTE_ORDER_MARK_UTF8))
            pos += strLength(BYTE_ORDER_MARK_UTF8);

        tokens.emplace_back("<?xml", Token::TK_DECL_BEGIN);
        tokens.emplace_back("?>",    Token::TK_DECL_END);
        tokens.emplace_back("</",    Token::TK_LESS_SLASH);
        tokens.emplace_back("/>",    Token::TK_SLASH_GREATER);
        tokens.emplace_back("<" ,    Token::TK_LESS); //evaluate after TK_DECL_BEGIN!
        tokens.emplace_back(">" ,    Token::TK_GREATER);
        tokens.emplace_back("=" ,    Token::TK_EQUAL);
        tokens.emplace_back("\"",    Token::TK_QUOTE);
        tokens.emplace_back("\'",    Token::TK_QUOTE);
    }

    Token nextToken() //throw XmlParsingError
    {
        //skip whitespace
        pos = std::find_if(pos, stream_.end(), [](char c) { return !zen::isWhiteSpace(c); });

        if (pos == stream_.end())
            return Token::TK_END;

        //skip XML comments
        if (startsWith(xmlCommentBegin))
        {
            auto it = std::search(pos + xmlCommentBegin.size(), stream_.end(), xmlCommentEnd.begin(), xmlCommentEnd.end());
            if (it != stream_.end())
            {
                pos = it + xmlCommentEnd.size();
                return nextToken();
            }
        }

        for (auto it = tokens.begin(); it != tokens.end(); ++it)
            if (startsWith(it->first))
            {
                pos += it->first.size();
                return it->second;
            }

        auto nameEnd = std::find_if(pos, stream_.end(), [](char c)
        {
            return c == '<'  ||
                   c == '>'  ||
                   c == '='  ||
                   c == '/'  ||
                   c == '\'' ||
                   c == '\"' ||
                   zen::isWhiteSpace(c);
        });

        if (nameEnd != pos)
        {
            std::string name(&*pos, nameEnd - pos);
            pos = nameEnd;
            return implementation::denormalize(name);
        }

        //unknown token
        throw XmlParsingError(posRow(), posCol());
    }

    std::string extractElementValue()
    {
        auto it = std::find_if(pos, stream_.end(), [](char c)
        {
            return c == '<'  ||
                   c == '>';
        });
        std::string output(pos, it);
        pos = it;
        return implementation::denormalize(output);
    }

    std::string extractAttributeValue()
    {
        auto it = std::find_if(pos, stream_.end(), [](char c)
        {
            return c == '<'  ||
                   c == '>'  ||
                   c == '\'' ||
                   c == '\"';
        });
        std::string output(pos, it);
        pos = it;
        return implementation::denormalize(output);
    }

    size_t posRow() const //current row beginning with 0
    {
        const size_t crSum = std::count(stream_.begin(), pos, '\r'); //carriage returns
        const size_t nlSum = std::count(stream_.begin(), pos, '\n'); //new lines
        assert(crSum == 0 || nlSum == 0 || crSum == nlSum);
        return std::max(crSum, nlSum); //be compatible with Linux/Mac/Win
    }

    size_t posCol() const //current col beginning with 0
    {
        //seek beginning of line
        for (auto it = pos; it != stream_.begin(); )
        {
            --it;
            if (*it == '\r' || *it == '\n')
                return pos - it - 1;
        }
        return pos - stream_.begin();
    }

private:
    Scanner           (const Scanner&) = delete;
    Scanner& operator=(const Scanner&) = delete;

    bool startsWith(const std::string& prefix) const
    {
        if (stream_.end() - pos < static_cast<ptrdiff_t>(prefix.size()))
            return false;
        return std::equal(prefix.begin(), prefix.end(), pos);
    }

    typedef std::vector<std::pair<std::string, Token::Type> > TokenList;
    TokenList tokens;

    const std::string xmlCommentBegin;
    const std::string xmlCommentEnd;

    const std::string stream_;
    std::string::const_iterator pos;
};


class XmlParser
{
public:
    XmlParser(const std::string& stream) :
        scn(stream),
        tk(scn.nextToken()) {}

    XmlDoc parse() //throw XmlParsingError
    {
        XmlDoc doc;

        //declaration (optional)
        if (token().type == Token::TK_DECL_BEGIN)
        {
            nextToken();

            while (token().type == Token::TK_NAME)
            {
                std::string attribName = token().name;
                nextToken();

                consumeToken(Token::TK_EQUAL);
                expectToken(Token::TK_QUOTE);
                std::string attribValue = scn.extractAttributeValue();
                nextToken();

                consumeToken(Token::TK_QUOTE);

                if (attribName == "version")
                    doc.setVersion(attribValue);
                else if (attribName == "encoding")
                    doc.setEncoding(attribValue);
                else if (attribName == "standalone")
                    doc.setStandalone(attribValue);
            }
            consumeToken(Token::TK_DECL_END);
        }

        XmlElement dummy;
        parseChildElements(dummy);

        auto iterPair = dummy.getChildren();
        if (iterPair.first != iterPair.second)
            doc.root().swapSubtree(*iterPair.first);

        expectToken(Token::TK_END);
        return doc;
    };

private:
    XmlParser           (const XmlParser&) = delete;
    XmlParser& operator=(const XmlParser&) = delete;

    void parseChildElements(XmlElement& parent)
    {
        while (token().type == Token::TK_LESS)
        {
            nextToken();

            expectToken(Token::TK_NAME);
            std::string elementName = token().name;
            nextToken();

            XmlElement& newElement = parent.addChild(elementName);

            parseAttributes(newElement);

            if (token().type == Token::TK_SLASH_GREATER) //empty element
            {
                nextToken();
                continue;
            }

            expectToken(Token::TK_GREATER);
            std::string elementValue = scn.extractElementValue();
            nextToken();

            //no support for mixed-mode content
            if (token().type == Token::TK_LESS) //structured element
                parseChildElements(newElement);
            else //value element
                newElement.setValue(elementValue);

            consumeToken(Token::TK_LESS_SLASH);

            if (token().type != Token::TK_NAME ||
                elementName != token().name)
                throw XmlParsingError(scn.posRow(), scn.posCol());
            nextToken();

            consumeToken(Token::TK_GREATER);
        }
    };

    void parseAttributes(XmlElement& element)
    {
        while (token().type == Token::TK_NAME)
        {
            std::string attribName = token().name;
            nextToken();

            consumeToken(Token::TK_EQUAL);
            expectToken(Token::TK_QUOTE);
            std::string attribValue = scn.extractAttributeValue();
            nextToken();

            consumeToken(Token::TK_QUOTE);
            element.setAttribute(attribName, attribValue);
        }
    }

    const Token& token() const { return tk; }
    void nextToken() { tk = scn.nextToken(); }

    void consumeToken(Token::Type t) //throw XmlParsingError
    {
        expectToken(t); //throw XmlParsingError
        nextToken();
    }

    void expectToken(Token::Type t) //throw XmlParsingError
    {
        if (token().type != t)
            throw XmlParsingError(scn.posRow(), scn.posCol());
    }

    Scanner scn;
    Token tk;
};
}

inline
XmlDoc parse(const std::string& stream) //throw XmlParsingError
{
    return implementation::XmlParser(stream).parse();  //throw XmlParsingError
}
}

#endif //ZEN_XML_PARSER_HEADER_81248670213764583021432
