#pragma once


#include "Basic/Location.h"

namespace WBSF
{

	class CEnvCanStationMap : public std::map < std::string, CLocation >
	{
	public:

		ERMsg Load(const std::string& filePath);
		ERMsg Save(const std::string& filePath);

	};

}

namespace zen
{
	template <> inline
		void writeStruc(const WBSF::CEnvCanStationMap& in, XmlElement& output)
	{
		for (WBSF::CEnvCanStationMap::const_iterator it = in.begin(); it != in.end(); it++)
			writeStruc(it->second, output.addChild("Location"));
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CEnvCanStationMap& out)
	{
		auto test = input.getChildren("Location");

		for (XmlElement::ChildIterConst2 it = test.first; it != test.second; it++)
		{
			WBSF::CLocation loc;
			readStruc(*it, loc);
			out[loc.m_ID] = loc;
			//out[loc.GetSSI("InternalID")] = loc;
			//WBSF::ToValue<__int64>(
		}

		return true;
	}
}

