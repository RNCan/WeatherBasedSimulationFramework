#pragma once

#include "ApproximateNearestNeighbor.h"
#include "ToolsBase.h"
#include "GeoBasic.h"
class CDailyDatabase;
class CDailyStation;


class CClipLOC : public CToolsBase
{
public:

	enum TATTRIBUTE {INPUT_DB, SHAPEFILE, BOUNDINGBOX, OUT_FILEPATH, NB_ATTRIBUTE};
	enum TATTRIBUTE_I {I_INPUT_DB=CToolsBase::NB_ATTRIBUTE, I_SHAPEFILE, I_BOUNDINGBOX, I_OUT_FILEPATH, I_NB_ATTRIBUTE};


	CClipLOC(void);
	virtual ~CClipLOC(void);
	CClipLOC(const CClipLOC& in);

	void Reset();
	CClipLOC& operator =(const CClipLOC& in);
	bool operator ==(const CClipLOC& in)const;
	bool operator !=(const CClipLOC& in)const;
	bool Compare(const CParameterBase& in)const;
	CParameterBase& Assign(const CParameterBase& in);

	virtual void GetSelection(short param, CSelectionDlgItemVector& items)const;
	virtual void SetSelection(short param, const CSelectionDlgItemVector& items);

	ERMsg ExecuteLOC(CFL::CCallback& callback=DEFAULT_CALLBACK);
	

	virtual ERMsg Execute(CFL::CCallback& callback=DEFAULT_CALLBACK);
	virtual void InitClass(const StringVector& option = EMPTY_OPTION);
	virtual std::string GetClassID()const{return CLASS_NAME;}

	
	virtual int GetNbAttribute()const{return I_NB_ATTRIBUTE; }
	virtual std::string GetValue(size_t type)const;
	virtual void SetValue(size_t type, const std::string& value);

	
protected:

	
	std::string m_inputFilePath;
	std::string m_shapeFilePath;
	
	//short m_type;
	GeoBasic::CGeoRect m_boundingBox;
	std::string m_outputFilePath;
	
	static const char* ATTRIBUTE_NAME[NB_ATTRIBUTE];
	static const char* CLASS_NAME;	

};

