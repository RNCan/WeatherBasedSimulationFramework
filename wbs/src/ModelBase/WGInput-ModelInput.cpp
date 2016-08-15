//****************************************************************************
// File:	WGInput-ModelInput.cpp
//****************************************************************************
// 15/03/2015	Rémi Saint-Amant	Created from old file
//****************************************************************************

#include "stdafx.h"
#include "ModelBase/WGInput-ModelInput.h"
#include "Basic/UtilMath.h"

namespace WBSF
{

	void WGInput2ModelInput(const CWGInput& WGInput, CModelInput& modelInput)
	{
		zen::XmlDoc input;
		zen::writeStruc(WGInput, input.root());


		zen::XmlDoc output(CModelInput::GetXMLFlag());
		for (auto it = input.root().getChildren().first; it != input.root().getChildren().second; it++)
		{
			std::string value;
			it->getValue(value);
			CModelInputParam in(it->getNameAs<std::string>(), value);
			zen::writeStruc(in, output.root().addChild(CModelInput::XML_FLAG_PARAM));
		}

		zen::readStruc(output.root(), modelInput);
	}

	void ModelInput2WGInput(const CModelInput& modelInput, CWGInput& WGInput)
	{
		zen::XmlDoc input;
		zen::writeStruc(modelInput, input.root());

		zen::XmlDoc output(CWGInput::GetXMLFlag());
		for (auto it = input.root().getChildren().first; it != input.root().getChildren().second; it++)
		{
			std::string value;
			it->getValue(value);
			std::string name;
			it->getAttribute("Name", name);
			output.root().addChild(name).setValue(value);
		}

		zen::readStruc(output.root(), WGInput);
	}


}