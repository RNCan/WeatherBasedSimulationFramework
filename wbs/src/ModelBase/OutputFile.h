#pragma once

#include "basic/ERMsg.h"
#include "basic/UtilStd.h"
#include "ModelBase/InputParam.h"


namespace WBSF
{

	class COutputFile : public ofStream
	{
	public:

		COutputFile();
		~COutputFile();

		ERMsg open(const std::string& filePath);

		template <class T>
		COutputFile& operator<< (const T& t)
		{
			((std::ofstream&)*this) << t;
			((std::ofstream&)*this) << m_separator;

			return *this;
		}

		COutputFile& EndLine();
		void Close();

		
		void SetParameter(const CParameterVector& parameters);
		void SetSeparator(const char* separator){ m_separator = separator; }

	private:

		CParameterVector m_parameters;
		std::string m_separator;

	};






}