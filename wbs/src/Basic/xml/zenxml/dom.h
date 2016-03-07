// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl-3.0        *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef ZEN_XML_DOM_HEADER_82085720723894567204564256
#define ZEN_XML_DOM_HEADER_82085720723894567204564256

#include <string>
#include <map>
#include "basic/xml/zen/fixed_list.h"
#include "basic/xml/zenXml/cvrt_text.h"

namespace zen
{
class XmlDoc;

/// An XML element
class XmlElement
{
public:
    XmlElement() : parent_(nullptr) {}

    //Construct an empty XML element
    template <class String>
    explicit XmlElement(const String& name, XmlElement* parentElement = nullptr) : name_(utfCvrtTo<std::string>(name)), parent_(parentElement) {}

    ///Retrieve the name of this XML element.
    /**
      \tparam String Arbitrary string class: e.g. std::string, std::wstring, wxString, MyStringClass, ...
      \returns Name of the XML element.
    */
    template <class String>
    String getNameAs() const { return utfCvrtTo<String>(name_); }

    ///Get the value of this element as a user type.
    /**
      \tparam T Arbitrary user data type: e.g. any string class, all built-in arithmetic numbers, STL container, ...
      \returns "true" if Xml element was successfully converted to value, cannot fail for string-like types
    */
    template <class T>
    bool getValue(T& value) const { return readStruc(*this, value); }

    ///Set the value of this element.
    /**
      \tparam T Arbitrary user data type: e.g. any string-like type, all built-in arithmetic numbers, STL container, ...
    */
    template <class T>
    void setValue(const T& value) { writeStruc(value, *this); }

    ///Retrieve an attribute by name.
    /**
      \tparam String Arbitrary string-like type: e.g. std::string, wchar_t*, char[], wchar_t, wxString, MyStringClass, ...
      \tparam T String-convertible user data type: e.g. any string class, all built-in arithmetic numbers
      \param name The name of the attribute to retrieve.
      \param value The value of the attribute converted to T.
      \return "true" if value was retrieved successfully.
    */
    template <class String, class T>
    bool getAttribute(const String& name, T& value) const
    {
        auto it = attributes.find(utfCvrtTo<std::string>(name));
        return it == attributes.end() ? false : readText(it->second, value);
    }

    ///Create or update an XML attribute.
    /**
      \tparam String Arbitrary string-like type: e.g. std::string, wchar_t*, char[], wchar_t, wxString, MyStringClass, ...
      \tparam T String-convertible user data type: e.g. any string-like type, all built-in arithmetic numbers
      \param name The name of the attribute to create or update.
      \param value The value to set.
    */
    template <class String, class T>
    void setAttribute(const String& name, const T& value)
    {
        std::string attrValue;
        writeText(value, attrValue);
        attributes[utfCvrtTo<std::string>(name)] = attrValue;
    }

    ///Remove the attribute with the given name.
    /**
      \tparam String Arbitrary string-like type: e.g. std::string, wchar_t*, char[], wchar_t, wxString, MyStringClass, ...
    */
    template <class String>
    void removeAttribute(const String& name) { attributes.erase(utfCvrtTo<std::string>(name)); }

    ///Create a new child element and return a reference to it.
    /**
      \tparam String Arbitrary string-like type: e.g. std::string, wchar_t*, char[], wchar_t, wxString, MyStringClass, ...
      \param name The name of the child element to be created.
    */
    template <class String>
    XmlElement& addChild(const String& name)
    {
        std::string utf8Name = utfCvrtTo<std::string>(name);
        childElements.emplace_back(utf8Name, this);
        XmlElement& newElement = childElements.back();
        childElementsSorted.emplace(utf8Name, &newElement);
        return newElement;
    }

    ///Retrieve a child element with the given name.
    /**
      \tparam String Arbitrary string-like type: e.g. std::string, wchar_t*, char[], wchar_t, wxString, MyStringClass, ...
      \param name The name of the child element to be retrieved.
      \return A pointer to the child element or nullptr if none was found.
    */
    template <class String>
    const XmlElement* getChild(const String& name) const
    {
        auto it = childElementsSorted.find(utfCvrtTo<std::string>(name));
        return it == childElementsSorted.end() ? nullptr : it->second;
    }

    ///\sa getChild
    template <class String>
    XmlElement* getChild(const String& name)
    {
        return const_cast<XmlElement*>(static_cast<const XmlElement*>(this)->getChild(name));
    }

    template < class IterTy,     //underlying iterator type
               class T,            //target object type
               class AccessPolicy > //access policy: see AccessPtrMap
    class PtrIter : public std::iterator<std::input_iterator_tag, T>, private AccessPolicy //get rid of shared_ptr indirection
    {
    public:
        PtrIter(IterTy it) : it_(it) {}
        PtrIter(const PtrIter& other) : it_(other.it_) {}
        PtrIter& operator++() { ++it_; return *this; }
        PtrIter operator++(int) { PtrIter tmp(*this); operator++(); return tmp; }
        inline friend bool operator==(const PtrIter& lhs, const PtrIter& rhs) { return lhs.it_ == rhs.it_; }
        inline friend bool operator!=(const PtrIter& lhs, const PtrIter& rhs) { return !(lhs == rhs); }
        T& operator* () { return  AccessPolicy::template objectRef<T>(it_); }
        T* operator->() { return &AccessPolicy::template objectRef<T>(it_); }
    private:
        IterTy it_;
    };

    struct AccessMapElement
    {
        template <class T, class IterTy>
        T& objectRef(const IterTy& it) { return *(it->second); }
    };

    typedef PtrIter<std::multimap<std::string, XmlElement*>::iterator,             XmlElement, AccessMapElement> ChildIter2;
    typedef PtrIter<std::multimap<std::string, XmlElement*>::const_iterator, const XmlElement, AccessMapElement> ChildIterConst2;

    ///Access all child elements with the given name via STL iterators.
    /**
      \code
      auto iterPair = elem.getChildren("Item");
      std::for_each(iterPair.first, iterPair.second,
    		[](const XmlElement& child) { ... });
      \endcode
      \param name The name of the child elements to be retrieved.
      \return A pair of STL begin/end iterators to access the child elements sequentially.
    */
    template <class String>
    std::pair<ChildIterConst2, ChildIterConst2> getChildren(const String& name) const { return childElementsSorted.equal_range(utfCvrtTo<std::string>(name)); }

    ///\sa getChildren
    template <class String>
    std::pair<ChildIter2, ChildIter2> getChildren(const String& name) { return childElementsSorted.equal_range(utfCvrtTo<std::string>(name)); }

    struct AccessListElement
    {
        template <class T, class IterTy>
        T& objectRef(const IterTy& it) { return *it; }
    };

    typedef PtrIter<FixedList<XmlElement>::iterator,             XmlElement, AccessListElement> ChildIter;
    typedef PtrIter<FixedList<XmlElement>::const_iterator, const XmlElement, AccessListElement> ChildIterConst;

    ///Access all child elements sequentially via STL iterators.
    /**
      \code
      auto iterPair = elem.getChildren();
      std::for_each(iterPair.first, iterPair.second,
    		[](const XmlElement& child) { ... });
      \endcode
      \return A pair of STL begin/end iterators to access all child elements sequentially.
    */
    std::pair<ChildIterConst, ChildIterConst> getChildren() const { return std::make_pair(childElements.begin(), childElements.end()); }

    ///\sa getChildren
    std::pair<ChildIter, ChildIter> getChildren() { return std::make_pair(childElements.begin(), childElements.end()); }

    ///Get parent XML element, may be nullptr for root element
    XmlElement* parent() { return parent_; };
    ///Get parent XML element, may be nullptr for root element
    const XmlElement* parent() const { return parent_; };


    typedef std::map<std::string, std::string>::const_iterator AttrIter;

    /* -> disabled documentation extraction
      \brief Get all attributes associated with the element.
      \code
        auto iterPair = elem.getAttributes();
        for (auto it = iterPair.first; it != iterPair.second; ++it)
           std::cout << "name: " << it->first << " value: " << it->second << "\n";
      \endcode
      \return A pair of STL begin/end iterators to access all attributes sequentially as a list of name/value pairs of std::string.
    */
    std::pair<AttrIter, AttrIter> getAttributes() const { return std::make_pair(attributes.begin(), attributes.end()); }

    //swap two elements while keeping references to parent.  -> disabled documentation extraction
    void swapSubtree(XmlElement& other)
    {
        name_     .swap(other.name_);
        value_    .swap(other.value_);
        attributes.swap(other.attributes);
        childElements.swap(other.childElements);
        childElementsSorted.swap(other.childElementsSorted);
        //std::swap(parent_, other.parent_); -> parent is physical location; update children's parent reference instead:
        for (XmlElement& child : childElements)
            child.parent_ = this;
        for (XmlElement& child : other.childElements)
            child.parent_ = &other;
    }

private:
    XmlElement           (const XmlElement&) = delete;
    XmlElement& operator=(const XmlElement&) = delete;

    std::string name_;
    std::string value_;
    std::map<std::string, std::string> attributes;
    FixedList<XmlElement>                   childElements;       //all child elements in order of creation
    std::multimap<std::string, XmlElement*> childElementsSorted; //alternate key: sorted by element name
    XmlElement* parent_;
};


//XmlElement::setValue<T>() calls zen::writeStruc() which calls XmlElement::setValue() ... => these two specializations end the circle
template <> inline
void XmlElement::setValue(const std::string& value) { value_ = value; }

template <> inline
bool XmlElement::getValue(std::string& value) const { value = value_; return true; }


///The complete XML document
class XmlDoc
{
public:
    ///Default constructor setting up an empty XML document with a standard declaration: <?xml version="1.0" encoding="UTF-8" ?>
    XmlDoc() : version_("1.0"), encoding_("UTF-8"), rootElement("Root") {}

    XmlDoc(XmlDoc&& tmp) { swap(tmp); }
    XmlDoc& operator=(XmlDoc&& tmp) { swap(tmp); return *this; }

    //Setup an empty XML document
    /**
      \tparam String Arbitrary string-like type: e.g. std::string, wchar_t*, char[], wchar_t, wxString, MyStringClass, ...
      \param rootName The name of the XML document's root element.
    */
    template <class String>
    XmlDoc(String rootName) : version_("1.0"), encoding_("UTF-8"), rootElement(rootName) {}

    ///Get a const reference to the document's root element.
    const XmlElement& root() const { return rootElement; }
    ///Get a reference to the document's root element.
    XmlElement& root() { return rootElement; }

    ///Get the version used in the XML declaration.
    /**
      \tparam String Arbitrary string class: e.g. std::string, std::wstring, wxString, MyStringClass, ...
    */
    template <class String>
    String getVersionAs() const { return utfCvrtTo<String>(version_); }

    ///Set the version used in the XML declaration.
    /**
      \tparam String Arbitrary string-like type: e.g. std::string, wchar_t*, char[], wchar_t, wxString, MyStringClass, ...
    */
    template <class String>
    void setVersion(const String& version) { version_ = utfCvrtTo<std::string>(version); }

    ///Get the encoding used in the XML declaration.
    /**
      \tparam String Arbitrary string class: e.g. std::string, std::wstring, wxString, MyStringClass, ...
    */
    template <class String>
    String getEncodingAs() const { return utfCvrtTo<String>(encoding_); }

    ///Set the encoding used in the XML declaration.
    /**
      \tparam String Arbitrary string-like type: e.g. std::string, wchar_t*, char[], wchar_t, wxString, MyStringClass, ...
    */
    template <class String>
    void setEncoding(const String& encoding) { encoding_ = utfCvrtTo<std::string>(encoding); }

    ///Get the standalone string used in the XML declaration.
    /**
      \tparam String Arbitrary string class: e.g. std::string, std::wstring, wxString, MyStringClass, ...
    */
    template <class String>
    String getStandaloneAs() const { return utfCvrtTo<String>(standalone_); }

    ///Set the standalone string used in the XML declaration.
    /**
      \tparam String Arbitrary string-like type: e.g. std::string, wchar_t*, char[], wchar_t, wxString, MyStringClass, ...
    */
    template <class String>
    void setStandalone(const String& standalone) { standalone_ = utfCvrtTo<std::string>(standalone); }

    //Transactionally swap two elements.  -> disabled documentation extraction
    void swap(XmlDoc& other)
    {
        version_   .swap(other.version_);
        encoding_  .swap(other.encoding_);
        standalone_.swap(other.standalone_);
        rootElement.swapSubtree(other.rootElement);
    }

private:
    XmlDoc           (const XmlDoc&) = delete; //not implemented, thanks to XmlElement::parent_
    XmlDoc& operator=(const XmlDoc&) = delete;

    std::string version_;
    std::string encoding_;
    std::string standalone_;

    XmlElement rootElement;
};

}

#endif //ZEN_XML_DOM_HEADER_82085720723894567204564256
