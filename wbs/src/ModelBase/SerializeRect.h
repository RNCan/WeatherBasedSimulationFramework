#pragma once

#include <WTypes.h>

#include "basic/zenXml.h"


namespace WBSF
{

	inline std::string ToString(const tagRECT& rect)
	{
		std::stringstream s;
		s << rect.top << " " << rect.left << " " << rect.bottom << " " << rect.right;

		return s.str();
	}

	inline tagRECT ToRect(const std::string& str)
	{
		tagRECT rect;

		std::stringstream s(str);
		s >> rect.top >> rect.left >> rect.bottom >> rect.right;


		//	
		//sscanf_s( str.c_str(), "%d %d %d %d", &rect.top, &rect.left, &rect.bottom, &rect.right);

		return rect;
	}


}

namespace zen
{
	template <> inline
		void writeStruc(const tagRECT& rect, XmlElement& output)
	{
		std::string str = WBSF::ToString(rect);

		output.setValue( str );
	}

	template <> inline
		bool readStruc(const XmlElement& input, tagRECT& rect)
	{
		std::string str;
		input.getValue(str);

		rect = WBSF::ToRect(str);
		
		return true;
	}

}
