//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 14-02-2020	Rémi Saint-Amant	Created
//****************************************************************************
#include "stdafx.h"

#include <locale>
#include <codecvt>
#include <iostream>
#include <fstream>
#include <string>
#include <locale>
#include <iomanip>
#include <codecvt>

#include "QGISPalette.h"
#include "WeatherBasedSimulationString.h"
#include "Basic/Registry.h"


using namespace std;

namespace WBSF
{

	const char* CCreateStyleOptions::RAMP_NAME[] = { "DISCRETE", "INTERPOLATED" };
	bool CCreateStyleOptions::operator == (const CCreateStyleOptions& in)const
	{
		bool bEqual = true;

		if (m_create_style_file != in.m_create_style_file)bEqual = false;
		if (m_palette_name != in.m_palette_name )bEqual = false;
		if (m_reverse_palette != in.m_reverse_palette)bEqual = false;
		if (m_color_ramp_type != in.m_color_ramp_type)bEqual = false;

		if (m_breaks_type != in.m_breaks_type)bEqual = false;
		if (m_nb_classes != in.m_nb_classes)bEqual = false;
		if (fabs(m_class_size - in.m_class_size) > 0.001)bEqual = false;

		if (m_min_max_type != in.m_min_max_type)bEqual = false;
		if (fabs(m_var_factor - in.m_var_factor) > 0.001)bEqual = false;
		if (fabs(m_min - in.m_min) > 0.001)bEqual = false;
		if (fabs(m_max - in.m_max) > 0.001)bEqual = false;
		if (m_number_format != in.m_number_format)bEqual = false;
		if (m_date_format != in.m_date_format)bEqual = false;
		if (m_ordinal_date != in.m_ordinal_date)bEqual = false;
		


		return bEqual;
	}

	void CCreateStyleOptions::SaveToRegistry()
	{
		CRegistry option("PaletteStyle");

		option.SetString("palette_name",m_palette_name);
		option.SetValue("reverse_palette",m_reverse_palette);
		option.SetValue("color_ramp_type",m_color_ramp_type);
		option.SetValue("breaks_type",m_breaks_type);
		option.SetValue("nb_classes",m_nb_classes);
		option.SetValue("classes_size",m_class_size);
		option.SetValue("min_max_type",m_min_max_type);
		option.SetValue("var_factor",m_var_factor);
		option.SetValue("user_min",m_min);
		option.SetValue("user_max",m_max);
		option.SetString("number_format",m_number_format);
		option.SetString("date_format",m_date_format);
		option.SetValue("ordinal_date", m_ordinal_date);
		
	}

	void CCreateStyleOptions::LoadFromRegistry()
	{
		CRegistry option("PaletteStyle");

		m_palette_name = option.GetString("palette_name", m_palette_name);
		m_reverse_palette=option.GetValue<bool>("reverse_palette", m_reverse_palette);
		m_color_ramp_type=option.GetValue<size_t>("color_ramp_type", m_color_ramp_type);
		m_breaks_type=option.GetValue<size_t>("breaks_type", m_breaks_type);
		m_nb_classes=option.GetValue<size_t>("nb_classes", m_nb_classes);
		m_class_size=option.GetValue<double>("classes_size", m_class_size);
		m_min_max_type=option.GetValue<size_t>("min_max_type", m_min_max_type);
		m_var_factor=option.GetValue<double>("var_factor", m_var_factor);
		m_min=option.GetValue<double>("user_min" , m_min);
		m_max=option.GetValue<double>("user_max", m_max);
		m_number_format=option.GetString("number_format", m_number_format);
		m_date_format=option.GetString("date_format", m_date_format);
		m_ordinal_date = option.GetValue<bool>("ordinal_date", m_ordinal_date);
	}

	//*****************************************************************************
	CQGISColorRamp::CQGISColorRamp(const std::string& name, const std::string& type):
		m_name(name),
		m_type(type)
	{
	}

	
	CQGISColorRamp& CQGISColorRamp::operator =(const CQGISColorRamp& in)
	{
		m_name = in.m_name;
		m_type = in.m_type;
		m_prop = in.m_prop;

		return *this;
	}

	//bool operator ==(const CColorRamp& in)const;
	//bool operator !=(const CParameter& in)const { return !operator==(in); }

	std::array<unsigned char, 4> GetRGBA(const std::string& str)
	{
		StringVector a(str, ",");
		ASSERT(a.size() == 4);
		
		std::array<unsigned char, 4> RGBA;
		RGBA.fill(0);

		if (a.size() == 4)
		{
			for (size_t i = 0; i < 4; i++)
				RGBA[i] = static_cast<unsigned char>(atoi(a[i].c_str()));
		}
//			RGBA = { { , atoi(a[1].c_str()), atoi(a[2].c_str()), atoi(a[3].c_str())} };

		return RGBA;
	}

	

	CCQGISColorProfile CQGISColorRamp::GetColorProfile()const
	{
		ASSERT(m_type=="gradient");//only gradient supported now

		std::string color1 = "43,131,186,255";
		std::string color2 = "215,25,28,255";
		StringVector stops("0.25;171,221,164,255:0.5;254,255,247,255:0.75;253,174,97,255", ":");

		if(m_prop.find("color1")!=m_prop.end())
			color1 = m_prop.at("color1");
		
		if (m_prop.find("color2") != m_prop.end())
			color2 = m_prop.at("color2");

		if (m_prop.find("stops") != m_prop.end())
			stops.Tokenize(m_prop.at("stops"), ":");


		CCQGISColorProfile profile;

		
		profile.push_back({0.0, GetRGBA(color1)} );
		for (size_t i = 0; i < stops.size(); i++)
		{
			StringVector p(stops[i], ";");
			ASSERT(p.size()==2);
			if(p.size() == 2)
				profile.push_back({ atof(p[0].c_str()), GetRGBA(p[1]) });
		}

		profile.push_back({ 1.0, GetRGBA(color2) });

		return profile;
	}

	std::string CQGISColorRamp::GetColor(size_t i, size_t nb_class)const
	{
		ASSERT(i < nb_class);
		ASSERT(nb_class >= 2);
		
		
		CCQGISColorProfile profile = GetColorProfile();

		std::string color;
		double f = double(i) / (nb_class - 1);
		for (size_t ii = 0; ii < profile.size() - 1 && color.empty(); ii++)
		{
			CCQGISColorStop& a = profile [ii];
			CCQGISColorStop& b = profile [ii + 1];

			if (f >= a.m_f&&f <= b.m_f)
			{
				double f2 = (f - a.m_f) / (b.m_f - a.m_f);
				double f1 = 1.0 - f2;
				ASSERT(f1 + f2 == 1);

				unsigned char R = unsigned char(f1*a.m_RGBA[0] + f2 * b.m_RGBA[0]);
				unsigned char G = unsigned char(f1*a.m_RGBA[1] + f2 * b.m_RGBA[1]);
				unsigned char B = unsigned char(f1*a.m_RGBA[2] + f2 * b.m_RGBA[2]);

				char tmp[10] = { 0 };
				snprintf(tmp, 10, "#%02x%02x%02x", R, G, B);
				color = tmp;
			}
		}

		//#2b83ba
		return color;
	}



	ERMsg CQGISColorRamp::CreateStyleFile(const std::string& file_path, CCreateStyleOptions& options, CTM TM)
	{
		ASSERT(options.m_color_ramp_type< CCreateStyleOptions::NB_COLORRAMP_TYPE);
		ASSERT(options.m_min< options.m_max);
		ASSERT(options.m_nb_classes <2000);

		string format = "%.2lf";

		ERMsg msg;


		
		if (options.m_breaks_type == CCreateStyleOptions::BY_CLASS_SIZE)
		{
			size_t nb_class = (size_t)max(1.0, min(255.0, (options.m_max - options.m_min) / options.m_class_size));
			options.m_nb_classes = nb_class;
		}

		double classes_size = (options.m_max - options.m_min) / options.m_nb_classes;

		ofStream file;
		std::locale utf8_locale = std::locale(std::locale::classic(), new std::codecvt_utf8<size_t>());


		file.imbue(utf8_locale);
		msg = file.open(file_path);
		if (msg)
		{


			file << "<!DOCTYPE qgis>" << endl;
			file << "<qgis>" << endl;
			file << "  <pipe>" << endl;
			file << "    <rasterrenderer band=\"1\" classificationMax=\"" + to_string(options.m_max) + "\" opacity=\"1\" classificationMin=\"" + to_string(options.m_min) + "\" type=\"singlebandpseudocolor\" alphaBand=\"-1\">" << endl;
			file << "      <rasterTransparency/>" << endl;
			file << "      <minMaxOrigin>" << endl;
			file << "        <limits>None</limits>" << endl;
			file << "        <extent>WholeRaster</extent>" << endl;
			file << "        <statAccuracy>Estimated</statAccuracy>" << endl;
			file << "        <cumulativeCutLower>0.02</cumulativeCutLower>" << endl;
			file << "        <cumulativeCutUpper>0.98</cumulativeCutUpper>" << endl;
			file << "        <stdDevFactor>2</stdDevFactor>" << endl;
			file << "      </minMaxOrigin>" << endl;
			file << "      <rastershader>" << endl;


			string ramp_name = CCreateStyleOptions::RAMP_NAME[options.m_color_ramp_type];
			file << "        <colorrampshader clip=\"0\" colorRampType=\""+ ramp_name+"\" classificationMode=\"1\">" << endl;
			file << "          <colorramp name=\"[source]\" type=\"gradient\">" << endl;
			for(auto it= m_prop.begin(); it != m_prop.end(); it++)
				file << "            <prop v=\"" << it->second << "\" k=\"" << it->first <<"\"/>" << endl;
			file << "          </colorramp>" << endl;

			for (size_t i = 0; i < options.m_nb_classes; i++)
			{
				size_t ii = options.m_reverse_palette? options.m_nb_classes - i - 1 : i;
				string color = GetColor(ii, options.m_nb_classes);
				string value = (i < options.m_nb_classes - 1) ? ToString(options.m_min + (i + 1) * classes_size) : "inf";
				string lable1 = WBSF::FormatA(options.m_number_format.c_str(), float(options.m_min + i*classes_size));
				string lable2 = WBSF::FormatA(options.m_number_format.c_str(), float(options.m_min + (i + 1) * classes_size));

				if (options.m_ordinal_date || (TM.IsInit()&&TM.Type()!=CTM::ATEMPORAL) )
				{
					CTRef TRef1;
					CTRef TRef2;

					if (options.m_ordinal_date)
					{
						TRef1 = CJDayRef(options.m_min + int(i) * classes_size + 1);
						TRef2 = CJDayRef(options.m_min + int(i + 1) * classes_size);
					}
					else
					{
						TRef1.SetRef(options.m_min + int(i) * classes_size + 1, TM);
						TRef2.SetRef(options.m_min + int(i + 1) * classes_size, TM);
					}
					
					lable1 = ANSI_UTF8(TRef1.GetFormatedString(options.m_date_format));
					lable2 = ANSI_UTF8(TRef2.GetFormatedString(options.m_date_format));
				}

				string lable;
				if (i == 0)
					lable = "&lt;=" + lable2;
				else if (i < options.m_nb_classes - 1)
					lable = lable1 + " - " + lable2;
				else
					lable = "&gt;=" + lable1;

				file << "          <item color=\"" + color + "\" value=\"" + value + "\" label=\"" + lable + "\" alpha=\"255\"/>" << endl;
			}
			//file << "          <item color=\"#c44a21\" value=\"737213.6856875\" label=\"&lt;= 737213.6856875\" alpha=\"255\"/>";
			//file << "          <item color=\"#ea8d4b\" value=\"737215.7433125\" label=\"737213.6856875 - 737215.7433125\" alpha=\"255\"/>";
			//file << "          <item color=\"#fec980\" value=\"737217.104\" label=\"737215.7433125 - 737217.104\" alpha=\"255\"/>";
			//file << "          <item color=\"#ffffbf\" value=\"737218.6638125\" label=\"737217.104 - 737218.6638125\" alpha=\"255\"/>";
			//file << "          <item color=\"#d4eeb1\" value=\"737221.21925\" label=\"737218.6638125 - 737221.21925\" alpha=\"255\"/>";
			//file << "          <item color=\"#a8dca0\" value=\"737224.3056875\" label=\"737221.21925 - 737224.3056875\" alpha=\"255\"/>";
			//file << "          <item color=\"#5bb339\" value=\"inf\" label=\"> 737224.3056875\" alpha=\"255\"/>";


			file << "        </colorrampshader>" << endl;
			file << "      </rastershader>" << endl;
			file << "    </rasterrenderer>" << endl;
			file << "    <brightnesscontrast contrast=\"0\" brightness=\"0\"/>" << endl;
			file << "    <huesaturation colorizeStrength=\"100\" grayscaleMode=\"0\" colorizeRed=\"255\" colorizeGreen=\"128\" saturation=\"0\" colorizeBlue=\"128\" colorizeOn=\"0\"/>" << endl;
			file << "    <rasterresampler maxOversampling=\"2\"/>" << endl;
			file << "  </pipe>" << endl;
			file << "</qgis>" << endl;

			file.close();
		}//open style
	

		return msg;
	}


	//*************************************************************************************
	ERMsg CQGISPalettes::load(const std::string file_path)
	{
		ERMsg msg;
		msg = zen::LoadXML(file_path, "palettes", "", *this);
		
		return msg;
	}
}