//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//
//******************************************************************************
#pragma once

#include "basic/zenXml.h"
#include "Basic/UtilStd.h"


namespace zen
{

	template <class T> inline
	void writeStruc2(const std::vector<T>& in, XmlElement& output)
	{
		std::for_each(in.begin(), in.end(), 
			[&](const T & childVal)
			{
				XmlElement& newChild = output.addChild(childVal.GetXMLFlag());
				zen::writeStruc(childVal, newChild);
			}
		);
	}

	template <class T> inline
	bool readStruc2(const XmlElement& input, std::vector<T>& out)
	{
		bool success = true;
		out.clear();

		auto iterPair = input.getChildren(T::GetXMLFlag());
		out.resize(std::distance( iterPair.first, iterPair.second) );
		std::vector<T>::iterator it = out.begin();
		for (auto iter = iterPair.first; iter != iterPair.second; ++iter, it++)
		{
			if (!zen::readStruc(*iter, *it))
				success = false;
		}
		return success;
	}


	template <class T> inline
		void writeStruc3(const std::vector<T>& in, XmlElement& output, const char* XMLFlag)
	{
		std::for_each(in.begin(), in.end(),
			[&](const T & childVal)
		{
			XmlElement& newChild = output.addChild(XMLFlag);
			zen::writeStruc(childVal, newChild);
		}
		);
	}

	template <class T> inline
		bool readStruc3(const XmlElement& input, std::vector<T>& out, const char* XMLFlag)
	{
		bool success = true;
		out.clear();

		auto iterPair = input.getChildren(XMLFlag);
		out.resize(std::distance(iterPair.first, iterPair.second));
		std::vector<T>::iterator it = out.begin();
		for (auto iter = iterPair.first; iter != iterPair.second; ++iter, it++)
		{
			if (!zen::readStruc(*iter, *it))
				success = false;
		}
		return success;
	}
	
	template<class T>
	void ReadXML(const zen::XmlIn& in, T& Class)
	{
		readStruc(*in.get(), Class);
	}

	template<class T>
	ERMsg LoadXML(const std::string& filePath, std::string rootName, std::string version, T& Class)
	{
		ERMsg msg;
		zen::XmlDoc doc; //empty XML document

		try
		{
			std::string filePathU8 = WBSF::ANSI_UTF8(filePath);
			doc = zen::load(filePathU8); //throw XmlFileError, XmlParsingError
			zen::XmlIn in(doc); 
 
			readStruc(doc.root(), Class);
		}
		catch (const zen::XmlFileError& e)
		{
			// handle error
			msg.ajoute(WBSF::GetErrorDescription(e.lastError));
		}
		catch (const zen::XmlParsingError& e)
		{
			// handle error
			msg.ajoute("Error parsing XML file: col=" + WBSF::ToString(e.col) + ", row=" + WBSF::ToString(e.row) );
		}
		catch (const ERMsg& e)
		{
			// handle error
			msg = e;
		}

	   return msg; 
	}

	template<class T>
	ERMsg SaveXML(std::string filePath, std::string rootName, std::string version, T& Class)
	{
		ERMsg msg;

		zen::XmlDoc doc(rootName); //empty XML document
		doc.root().setAttribute("version", version);
		doc.setEncoding("Windows - 1252");

		try
		{
			writeStruc(Class, doc.root());
  
			std::string filePathU8 = WBSF::ANSI_UTF8(filePath);
			zen::save(doc, filePathU8); //throw XmlFileError, XmlParsingError
		}
		catch (const zen::XmlFileError& e) 
		{
			// handle error
			msg.ajoute(WBSF::GetErrorDescription(e.lastError));
		}
		catch (const ERMsg& e)
		{
			// handle error
			msg = e;
		}

	   return msg; 
	}

	template<class T>
	ERMsg from_string(T& Class, const std::string& str)
	{
		ERMsg msg;

		try
		{
			zen::XmlDoc doc = zen::parse(str); 
			zen::readStruc(doc.root(), Class);
		}
		catch (const zen::XmlParsingError& e)
		{
			// handle error
			msg.ajoute("Error parsing XML file: col=" + WBSF::ToString(e.col) + ", row=" + WBSF::ToString(e.row));
		}

		return msg;
	}

	template<class T>
	std::string to_string(T& Class, std::string rootName, std::string version)
	{
		string str;

		try
		{
			zen::XmlDoc doc(rootName);
			doc.setEncoding("Windows - 1252");
			doc.root().setAttribute("version", version);
			zen::writeStruc(Class, doc.root());
			str = zen::serialize(doc);
		}
		catch (const zen::XmlFileError& )
		{
			assert(false);
		}
		catch (const ERMsg& )
		{
			assert(false);
		}

		return str;
	}

}


