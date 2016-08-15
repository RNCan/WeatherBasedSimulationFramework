//******************************************************************************
//  project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#ifndef __CSVFILE
#define __CSVFILE

#pragma once

#include <sstream>
#include "basic/ERMsg.h"


#include "Basic/UtilStd.h"


namespace WBSF
{

	class CSVRow : public StringVector
	{
	public:

		CSVRow(const char* pD, bool bDQ) :m_pD(pD), m_bDQ(bDQ){}


		std::string const& operator[](std::size_t index) const
		{
			if (index > size())
				return EMPTY_STRING;

			return at(index);
		}

		
		void clear()
		{
			StringVector::clear();
			m_line.clear();
		}

		void readNextRow(std::istream& str)
		{
			static const char REPLACEMENT[14] = { 'Ò', 'Ó', 'Õ', 'Ö', 'Ý', 'Ÿ', 'Å', 'ò', 'ó', 'õ', 'ö', 'ý', 'ÿ', 'å' };

			std::string         copy;
			clear();
			//bool bReplacmentOccurd = false;
			std::getline(str, m_line);
			m_line.erase(std::remove(m_line.begin(), m_line.end(), '\r'), m_line.end());


			if (m_bDQ)
			{
				ASSERT(strlen(m_pD) < 14);

				//replace separator inside double cote by temparary separator
				bool bOpen = false;
				for (std::string::iterator it = m_line.begin(); it != m_line.end(); ++it)
				{
					if (*it == '\"')
					{
						bOpen = !bOpen;
					}
					else if (bOpen)
					{
						char const* q = m_pD;
						while (*q != NULL)
						{
							if (*it == *q)
							{
								if (copy.empty())
								{
									copy = m_line;
								}

								ASSERT((q - m_pD) >= 0 && (q - m_pD) < 14);
								*it = REPLACEMENT[q - m_pD];
								//bReplacmentOccurd = true;
								break;
							}
							q++;
						}
					}
				}
			}

			std::stringstream   lineStream(m_line);
			std::string         cell;



			if (strlen(m_pD) == 1)
			{
				while (std::getline(lineStream, cell, m_pD[0]))
				{
					push_back(cell);
				}
			}
			else
			{
				Tokenize(m_line, m_pD, m_bDQ);
				//TokenizeMulti(m_line, m_pD, *this);
			}

			if (m_bDQ)
			{
				if (!copy.empty())
				{
					m_line = copy;
					for (std::vector<std::string>::iterator it2 = begin(); it2 != end(); ++it2)
					{
						bool bOpen = false;
						for (std::string::iterator it = it2->begin(); it != it2->end(); ++it)
						{
							if (*it == '\"')
							{
								bOpen = !bOpen;
							}
							else if (bOpen)
							{
								char const* q = m_pD;
								while (*q != NULL)
								{
									if (*it == REPLACEMENT[q - m_pD])
									{
										*it = *q;
										break;
									}
									q++;
								}
							}
						}
					}
				}

				//remove "
				for (std::vector<std::string>::iterator it = begin(); it != end(); it++)
					it->erase(std::remove(it->begin(), it->end(), '"'), it->end());
			}

		}

		friend std::istream& operator>>(std::istream& str, CSVRow& data);

		const std::string& GetLastLine()const { return m_line; }
	private:

		const char*			m_pD;
		bool				m_bDQ;
		std::string         m_line;

		static const std::string EMPTY_STRING;
	};


	class CSVIterator
	{
	public:
		typedef std::input_iterator_tag     iterator_category;
		typedef CSVRow                      value_type;
		typedef std::size_t                 difference_type;
		typedef CSVRow*                     pointer;
		typedef CSVRow&                     reference;

		//
		CSVIterator(std::istream& str, const char* pD = ",", bool bHeader = true, bool bDQ = false) :m_str(str.good() ? &str : NULL), m_row(pD, bDQ), m_header(pD, bDQ) { if (bHeader)ReadHeader(); ++(*this); }
		CSVIterator(const char* pD = ",", bool bHeader = true, bool bDQ = false) :m_str(NULL), m_row(pD, bDQ), m_header(pD, bDQ) {}

		CSVIterator& ReadHeader()               { if (m_str) { (*m_str) >> m_header; m_str = m_str->good() ? m_str : NULL; }return *this; }

		// Pre Increment
		CSVIterator& operator++()               { if (m_str) { (*m_str) >> m_row; m_str = m_str->good() ? m_str : NULL; }return *this; }
		// Post increment
		CSVIterator operator++(int)             { CSVIterator tmp(*this); ++(*this); return tmp; }
		CSVRow const& operator*()   const       { return m_row; }
		CSVRow const* operator->()  const       { return &m_row; }

		bool operator==(CSVIterator const& rhs) { return ((this == &rhs) || ((this->m_str == NULL) && (rhs.m_str == NULL))); }
		bool operator!=(CSVIterator const& rhs) { return !((*this) == rhs); }

		CSVRow const& Header()const				{ return m_header; }

	private:


		std::istream*       m_str;
		CSVRow              m_row;
		CSVRow              m_header;
	};


}
#endif