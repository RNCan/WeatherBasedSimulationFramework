#include "stdafx.h"
#include "EnvCanLocationMap.h"
#include "Basic/Utilzen.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{

	//*********************************************************************
	ERMsg CEnvCanStationMap::Load(const std::string& filePath)
	{
		//zen::XmlDoc doc("LocationsList");
		
		return zen::LoadXML(filePath, "LocationsList", "1", *this );
		//if (msg)
			//readStruc(doc.root(), *this);

		//return msg;
	}

	ERMsg CEnvCanStationMap::Save(const std::string& filePath)
	{
		//ERMsg msg;

		//zen::XmlDoc doc("LocationsList");
		//doc.setEncoding("Windows-1252");
		//zen::writeStruc(*this, doc.root());
		//doc.root().setAttribute("version", 1);

		//msg = save(doc, filePath);

		//return msg;
		return zen::SaveXML(filePath, "LocationsList", "1", *this);
	}


}