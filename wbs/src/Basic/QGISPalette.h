//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#pragma warning( disable : 4786)

#include <map>
#include "basic/ERMsg.h"
#include "basic/zenXml.h"
#include "Basic/UtilZen.h"
#include "Basic/UtilTime.h"

namespace WBSF
{

	//****************************************************************
	// CQGISColorRamp

	class CCQGISColorStop
	{
	public:

		double m_f;
		std::array<unsigned char, 4> m_RGBA;
	};

	typedef std::vector< CCQGISColorStop> CCQGISColorProfile;


	class CCreateStyleOptions
	{
		public:

			enum TBreaks { BY_NB_CLASS, BY_CLASS_SIZE, NB_BREAK_TYPE };
			enum TMinMax{ BY_MINMAX, BY_STDDEV, BY_USER, NB_MINMAX_TYPE};
			enum TColorRamp{ CR_DISCRETE, CR_INTERPOLATED, NB_COLORRAMP_TYPE };
			static const char* RAMP_NAME[NB_COLORRAMP_TYPE];

			CCreateStyleOptions()
			{
				m_create_style_file=false;
				m_palette_name="Default";
				m_reverse_palette=false;
				m_color_ramp_type = CR_DISCRETE;

				m_breaks_type= BY_NB_CLASS;
				m_nb_classes=7;
				m_class_size=7;

				m_min_max_type= BY_STDDEV;
				m_var_factor=2;
				m_min=0;
				m_max=0;

				m_number_format="%.2f";
				m_date_format = "%d %B";
			}

			bool operator == (const CCreateStyleOptions& in)const;
			bool operator != (const CCreateStyleOptions& in)const { return !operator==(in); }


			void SaveToRegistry();
			void LoadFromRegistry();


			bool m_create_style_file;
			std::string m_palette_name;
			bool m_reverse_palette;
			size_t m_color_ramp_type;

			size_t m_breaks_type;
			size_t m_nb_classes;
			double m_class_size;

			size_t m_min_max_type;
			double m_var_factor;
			double m_min;
			double m_max;
			std::string m_number_format;
			std::string m_date_format;
			
			
			std::string GetDescription()
			{
				return m_create_style_file ? m_palette_name : "----";
			}

	};

	class CQGISColorRamp
	{
	public:

		CQGISColorRamp(const std::string& name = "", const std::string& type = "gradient");
		CQGISColorRamp(const CQGISColorRamp& in) { operator=(in); }
		CQGISColorRamp& operator =(const CQGISColorRamp& in);

		std::string GetColor(size_t i, size_t nb_class)const;
		CCQGISColorProfile GetColorProfile()const;
		ERMsg CreateStyleFile(const std::string& file_path, CCreateStyleOptions& options, CTM TM);

		std::string m_name;
		std::string m_type;

		std::map<std::string, std::string> m_prop;
	};


	typedef std::map<std::string, 	class CQGISColorRamp> CColorRampMap;
	class CQGISPalettes : public CColorRampMap
	{
	public:

		CQGISPalettes()
		{}

		ERMsg load(const std::string file_path);
	};




	//****************************************************************
	// CCounter


}


namespace zen
{

	template <> inline
		void writeStruc(const WBSF::CCreateStyleOptions& in, XmlElement& output)
	{
		zen::XmlOut out(output);
		out["create_style_file"](in.m_create_style_file);
		out["palette_name"](in.m_palette_name);
		out["breaks_type"](in.m_breaks_type);
		out["nb_classes"](in.m_nb_classes);
		out["class_size"](in.m_class_size);
		out["min_max_type"](in.m_min_max_type);
		out["var_factor"](in.m_var_factor);
		out["user_min"](in.m_min);
		out["user_max"](in.m_max);
		out["reverse_palette"](in.m_reverse_palette);
		out["color_ramp_type"](in.m_color_ramp_type);
		out["number_format"](in.m_number_format);
		out["date_format"](in.m_date_format);
		
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CCreateStyleOptions& out)
	{
		zen::XmlIn in(input);
		in["create_style_file"](out.m_create_style_file);
		in["palette_name"](out.m_palette_name);
		in["breaks_type"](out.m_breaks_type);
		in["nb_classes"](out.m_nb_classes);
		in["class_size"](out.m_class_size);
		in["min_max_type"](out.m_min_max_type);
		in["var_factor"](out.m_var_factor);
		in["user_min"](out.m_min);
		in["user_max"](out.m_max);
		in["reverse_palette"](out.m_reverse_palette);
		in["color_ramp_type"](out.m_color_ramp_type);
		in["number_format"](out.m_number_format);
		in["date_format"](out.m_date_format);

		return true;
	}


	template <> inline
		void writeStruc(const WBSF::CQGISColorRamp& in, XmlElement& output)
	{
		output.setAttribute("name", in.m_name);
		output.setAttribute("type", in.m_type);
		output.setValue(in.m_prop);

		std::for_each(in.m_prop.begin(), in.m_prop.end(), [&](const std::pair<std::string,std::string> & prop)
		{
			XmlElement& newChild = output.addChild("prop");
			newChild.setAttribute("k", prop.first);
			newChild.setAttribute("v", prop.second);
		}
		);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CQGISColorRamp& out)
	{
		input.getAttribute("name", out.m_name);
		input.getAttribute("type", out.m_type);
		//input.getValue(out.m_prop);

		bool success = true;
		

		auto iterPair = input.getChildren("prop");
		//out.resize(std::distance(iterPair.first, iterPair.second));
		//CQGISPalettes::iterator it = out.begin();
		for (auto iter = iterPair.first; iter != iterPair.second; ++iter)
		{
			std::string name;
			std::string value;
			if( iter->getAttribute("k", name) && iter->getAttribute("v", value))
				out.m_prop[name] = value;
		}

		return success;

		
	}


	template <> inline
		void writeStruc(const WBSF::CQGISPalettes& in, XmlElement& output)
	{
		
		std::for_each(in.begin(), in.end(),[&](const WBSF::CQGISPalettes::value_type & childVal)
			{
				XmlElement& newChild = output.addChild("colorramp");
				zen::writeStruc(childVal, newChild);
			}
		);
	}

	template <> inline
		bool readStruc(const XmlElement& input, WBSF::CQGISPalettes& out)
	{
		bool success = true;
		out.clear();

		auto iterPair = input.getChildren("colorramp");
		//out.resize(std::distance(iterPair.first, iterPair.second));
		//CQGISPalettes::iterator it = out.begin();
		for (auto iter = iterPair.first; iter != iterPair.second; ++iter)
		{
			std::string name;
			
			if (!iter->getAttribute("name", name)||!zen::readStruc(*iter, out[name]))
				success = false;
		}

		return success;
		
	}

}
