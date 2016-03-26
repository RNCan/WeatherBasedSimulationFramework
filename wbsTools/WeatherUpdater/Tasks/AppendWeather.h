#pragma once
#include "TaskBase.h"

namespace WBSF
{

	class CAppendWeather : public CTaskBase
	{
	public:


		enum TAttributes { INPUT_FILEPATH_1, INPUT_FILEPATH_2, OUTPUT_FILEPATH, NB_ATTRIBUTES };

		static const char* CLASS_NAME();
		static CTaskPtr create(){ return CTaskPtr(new CAppendWeather); }

		CAppendWeather(void);
		virtual ~CAppendWeather(void);


		virtual const char* ClassName()const{ return CLASS_NAME(); }
		virtual TType ClassType()const;

		virtual ERMsg Execute(CCallback& callback = DEFAULT_CALLBACK);

		virtual size_t GetNbAttributes()const{ return NB_ATTRIBUTES; }
		virtual size_t Type(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_TYPE[i]; }
		virtual const char* Name(size_t i)const{ ASSERT(i < NB_ATTRIBUTES);  return ATTRIBUTE_NAME[i]; }
		virtual const std::string& Title(size_t i)const{ ASSERT(i < NB_ATTRIBUTES); return ATTRIBUTE_TITLE[i]; }
		virtual std::string Option(size_t i)const;


	protected:


		//ERMsg ExecuteHourly(const std::string inputFilePath1, const std::string inputFilePath2, const std::string outputFilePath, CCallback& callback);
		//ERMsg ExecuteDaily(const std::string inputFilePath1, const std::string inputFilePath2, const std::string outputFilePath, CCallback& callback);
		//ERMsg ExecuteNormal(const std::string inputFilePath1, const std::string inputFilePath2, const std::string outputFilePath, CCallback& callback);


		static const size_t ATTRIBUTE_TYPE[NB_ATTRIBUTES];
		static const char* ATTRIBUTE_NAME[NB_ATTRIBUTES];
		static const StringVector ATTRIBUTE_TITLE;
	};

}
