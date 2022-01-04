// Azure.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "azure_weather.h"
#include "storage_credential.h"
#include "storage_account.h"
#include "blob/blob_client.h"
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>

#include "Basic/WeatherStation.h"

using namespace std;
using namespace azure::storage_lite;
using namespace WBSF;
 
 



	bool load_azure_weather_years_cpp(const std::string& account_name, const std::string& account_key, const std::string& container_name, const std::string &blob_name, void* pData)
	{
		bool bRep = false;

		if (!account_name.empty() && !account_key.empty() && !container_name.empty())
		{
			//from azure
			std::shared_ptr<storage_credential> cred = std::make_shared<shared_key_credential>(account_name, account_key);
			std::shared_ptr<storage_account> account = std::make_shared<storage_account>(account_name, cred, /* use_https */ true);
			std::shared_ptr<blob_client> client = std::make_shared<blob_client>(account, 16);

			std::stringstream azure_stream;
			auto ret = client->download_blob_to_stream(container_name, blob_name, 0, 0, azure_stream).get();
			if (ret.success())
			{
				try
				{
					boost::iostreams::filtering_istreambuf in;
					in.push(boost::iostreams::gzip_decompressor());
					in.push(azure_stream);
					std::istream incoming(&in);

					CWeatherYears* pWeather = static_cast<CWeatherYears*>(pData);

					CTPeriod period;
					CWVariables variable;

					read_value(incoming, period);
					read_value(incoming, variable);
					assert(period.IsInit());
					//pWeather->CreateYears(period);

					for (CTRef TRef = period.Begin(); TRef <= period.End(); TRef++)
						pWeather->Get(TRef).ReadStream(incoming, variable, false);

					bRep = true;
				}
				catch (const boost::iostreams::gzip_error& /*exception*/)
				{
					//int error = exception.error();
					//if (error == boost::iostreams::gzip::zlib_error)
					//{
						//check for all error code    
						//msg.ajoute(exception.what());
					//}
				}

				//std::cout << out_stream.str();
			}
		}
		else 
		{
			//load local file
			assert(!blob_name.empty());

			
			ifstream local_stream;
			local_stream.open(blob_name, ios::in | ios::binary);
			if (local_stream.is_open())
			{
				try
				{
					boost::iostreams::filtering_istreambuf in;
					in.push(boost::iostreams::gzip_decompressor());
					in.push(local_stream);
					std::istream incoming(&in);

					CWeatherYears* pWeather = static_cast<CWeatherYears*>(pData);

					CTPeriod period;
					CWVariables variable;

					read_value(incoming, period);
					read_value(incoming, variable);
					assert(period.IsInit());
					//pWeather->CreateYears(period);

					for (CTRef TRef = period.Begin(); TRef <= period.End(); TRef++)
						pWeather->Get(TRef).ReadStream(incoming, variable, false);

					bRep = true;
				}
				catch (const boost::iostreams::gzip_error& /*exception*/)
				{
					//int error = exception.error();
					//if (error == boost::iostreams::gzip::zlib_error)
					//{
						//check for all error code    
						//msg.ajoute(exception.what());
					//}
				}

				//std::cout << out_stream.str();
			}
		}


		return bRep;

	}

extern "C"
{

	__declspec(dllexport) bool load_azure_weather_years(const char* account_name, const char* account_key, const char*  container_name, const char* blob_name, void* pData)
	{
		return load_azure_weather_years_cpp( std::string(account_name), std::string(account_key), std::string(container_name), std::string(blob_name), pData);
	}
}
