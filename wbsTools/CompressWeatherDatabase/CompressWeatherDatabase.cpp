// CompressWeatherDatabase.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include <string>
#include "Basic/ERMsg.h"
#include "Basic/NormalsDatabase.h"
#include "Basic/DailyDatabase.h"
#include "Basic/WeatherDefine.h"
#include "Basic/DynamicRessource.h"
#include "Basic/Shore.h"
#include "ModelBase/CommunicationStream.h"
#include "WeatherBasedSimulationString.h"

//using namespace boost;
using namespace std;


using namespace WBSF;
using namespace WBSF::WEATHER;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::NORMALS_DATA;


using namespace std;
using namespace WBSF;

ERMsg CreateNormalBinary(string file_path_in, string file_path_out);
ERMsg CreateDailyBinary(string file_path_in, string file_path_out);

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cout << "Two parameters must by supply: input and output file" << endl;
		std::cout << "For example: CompressWeatherDatabase.exe \"input.DailyDB\" \"output.DailyDB.bin.gz\"" << endl;
		return 1;
	}

	CDynamicResources::set(GetModuleHandle(NULL));

	ERMsg msg;

	string file_path_in = argv[1];
	string file_path_out = argv[2];
	if (WBSF::IsEqual(file_path_in, file_path_out))
	{
		std::cout << "Input and output file have the same name" << endl;
		return 1;
	}

	string ext = GetFileExtension(file_path_in);
	if (WBSF::IsEqual(ext, ".NormalsDB"))
		msg += CreateNormalBinary(file_path_in, file_path_out);
	else if (WBSF::IsEqual(ext, ".DailyDB"))
		msg += CreateDailyBinary(file_path_in, file_path_out);
	else
		msg.ajoute("Unknown database type: " + ext);
	
	if (!msg)
	{
		for (unsigned int i = 0; i < msg.dimension(); i++)
			std::cout << msg[i] << endl;

		return 1;
	}

	return 0;
}



ERMsg CreateNormalBinary(string file_path_in, string file_path_out)
{
	ERMsg msg;

	//SetFileExtension(file_path_out, ".NormalsDB.bin.gz");
	std::cout << "Compress: " << GetFileName(file_path_in) << endl;

	CCallback callback;
	CNormalsDatabase DB;
	msg += DB.Open(file_path_in, CNormalsDatabase::modeRead);
	if (msg)
		DB.OpenSearchOptimization(callback);


	if (msg)
	{
		DB.SaveAsBinary(file_path_out);
		DB.Close();
	}

	return msg;
}
//
//ERMsg UploadNormalsToAzure(string file_path)
//{
//	ERMsg msg;
//
//	string blobName = "Weather/Normals/" + GetFileName(file_path);
//
//	std::shared_ptr<storage_credential> cred = std::make_shared<shared_key_credential>(ACCOUNT_NAME, ACCOUNT_KEY);
//	std::shared_ptr<storage_account> account = std::make_shared<storage_account>(ACCOUNT_NAME, cred, /* use_https */ true);
//	std::shared_ptr<blob_client> client = std::make_shared<blob_client>(account, 16);
//	blob_client_wrapper client_wrapper(client);
//
//
//
//	auto ret = client->create_container(CONTAINER_NAME).get();
//	std::ifstream fin(file_path, std::ios_base::in | std::ios_base::binary);
//	std::vector<std::pair<std::string, std::string>> metadata;
//	metadata.emplace_back(std::make_pair("type", "Normals"));
//	metadata.emplace_back(std::make_pair("extents", "-180,-90,180,90"));
//	metadata.emplace_back(std::make_pair("period", "1991-2020"));
//	ret = client->upload_block_blob_from_stream(CONTAINER_NAME, blobName, fin, metadata).get();
//	if (!ret.success())
//	{
//		msg.ajoute("Failed to upload Normals, Error: " + ret.error().code + ", " + ret.error().code_name);
//	}
//	fin.close();
//	//client.delete_blob(container_name, blob_name).wait();
//
//
//
//
//	// Save blob contents to a file.
//
//	ASSERT(client_wrapper.blob_exists(CONTAINER_NAME, blobName));
//
//
//	auto blobs = client_wrapper.list_blobs_segmented(CONTAINER_NAME, "/", "", "");
//	std::cout << "Size of BLobs: " << blobs.blobs.size() << std::endl;
//	std::cout << "Error Size of BLobs: " << errno << std::endl;
//	assert(errno == 0);
//
//	time_t last_modified;
//	client_wrapper.download_blob_to_file(CONTAINER_NAME, blobName, file_path + ".new", last_modified);
//	std::cout << "Download Blob done: " << errno << std::endl;
//
//
//	return msg;
//}

ERMsg CreateDailyBinary(string file_path_in, string file_path_out)
{
	ERMsg msg;

	std::cout << "Compress: " << GetFileName(file_path_in) << endl;
	//SetFileExtension(file_path_out, ".DailyDB.bin.gz");

	CCallback callback;
	CDailyDatabase DB;
	msg += DB.Open(file_path_in, CWeatherDatabase::modeRead);
	if (msg)
		DB.OpenSearchOptimization(callback);

	if (msg)
	{
		DB.SaveAsBinary(file_path_out);
		DB.Close();
	}

	return msg;
}
//
//ERMsg UploadDailyToAzure(string file_path)
//{
//	ERMsg msg;
//
//
//	std::shared_ptr<storage_credential> cred = std::make_shared<shared_key_credential>(ACCOUNT_NAME, ACCOUNT_KEY);
//	std::shared_ptr<storage_account> account = std::make_shared<storage_account>(ACCOUNT_NAME, cred, /* use_https */ true);
//	std::shared_ptr<blob_client> client = std::make_shared<blob_client>(account, 8);
//
//	//
//	//
//	//
//	//		std::vector<std::pair<std::string, std::string>> metadata;
//	//		metadata.emplace_back(std::make_pair("type", "Daily"));
//	//
//	//
//	////		client_wrapper.create_container(CONTAINER_NAME);
//	//		//std::ifstream fin(file_path, std::ios_base::in | std::ios_base::binary);
//	//		//metadata.emplace_back(std::make_pair("period", "2020-2021"));
//	//
//	//		ASSERT(client_wrapper.blob_exists(CONTAINER_NAME, "Weather/Daily/" + GetFileName(file_path)));
//	//
//	//
//	//		client_wrapper.upload_file_to_blob(file_path, CONTAINER_NAME, "Weather/Daily/" + GetFileName(file_path), metadata);
//	//
//	string base_path = GetPath(file_path) + GetFileTitle(file_path) + "\\";
//	StringVector files = WBSF::GetFilesList(base_path + "*.bin.gz", 2, true);
//	//		StringVector files;
//	//		files.push_back("G:\\Travaux\\BioSIM_API\\Weather\\Daily\\Canada-USA 2018-2019.DailyDB.bin\\2018\\Cutler Dam [USC00421918].bin.gz");
//	//
//	//		
//	//
//	//		string the_file = "G:\\Travaux\\BioSIM_API\\Weather\\Daily\\Canada-USA 2018-2019.DailyDB.bin\\2018\\Cutler Dam [USC00421918].bin.gz";
//	//		string blobName = WBSF::GetRelativePath(GetPath(file_path), the_file);
//	//		ReplaceString(blobName, "\\", "/");
//	//		blobName = WBSF::UTF8_ANSI(blobName);
//	//		blobName = RemoveAccented(blobName);//remove all accent caracters;
//	//		blobName = "Weather/Daily/test2.gz";// + blobName;
//	//
//	//		
//	//		//std::ifstream fin(the_file, std::ios_base::in | std::ios_base::binary);
//	//		//auto ret = client->upload_block_blob_from_stream(CONTAINER_NAME, blobName, fin, metadata).get();
//	//		//if (!ret.success())
//	//		//{
//	//		//	msg.ajoute("Failed to upload Normals, Error: " + ret.error().code + ", " + ret.error().code_name);
//	//		//}
//	//		//fin.close();
//	//
//	//		client_wrapper.upload_file_to_blob(the_file, CONTAINER_NAME, blobName, metadata);
//	//		int err = errno;
//	//		ASSERT(client_wrapper.blob_exists(CONTAINER_NAME, blobName));
//	//		
//
//	std::vector<std::pair<std::string, std::string>> metadata;
//	metadata.clear();
//
//	int count = 0;
//	//#pragma omp parallel for 
//	for (__int64 i = 0; i < (__int64)files.size(); i++)
//	{
//		string blob_name = WBSF::GetRelativePath(GetPath(file_path), files[i]);
//		ReplaceString(blob_name, "\\", "/");
//		blob_name = WBSF::UTF8_ANSI(blob_name);
//		blob_name = RemoveAccented(blob_name);//remove all accent caracters;
//		blob_name = "Weather/Daily/" + blob_name;
//
//		std::ifstream fin(files[i], std::ios_base::in | std::ios_base::binary);
//		auto ret = client->upload_block_blob_from_stream(CONTAINER_NAME, blob_name, fin, metadata).get();
//		fin.close();
//
//
//		if (!ret.success())
//		{
//			msg.ajoute("Failed to upload Normals, Error: " + ret.error().code + ", " + ret.error().code_name);
//			break;
//		}
//
//
//		//client_wrapper.upload_file_to_blob(files[i], CONTAINER_NAME, blob_name);
//
////#pragma omp atomic 
//		count++;
//	}
//
//
//	//client.delete_blob(container_name, blob_name).wait();
//
//
//	return msg;
//}
//
//ERMsg CreateShoreBinary(string file_path)
//{
//	ERMsg msg;
//
//
//	string bin_file_path = file_path;
//	SetFileExtension(bin_file_path, ".ann.gz");
//	//ReplaceString(bin_file_path, " ", "_");
//
//
//	ifStream file_in;
//	msg += file_in.open(file_path, std::ios::in | std::ios::binary);
//
//	ofStream file_out;
//	msg += file_out.open(bin_file_path, std::ios::out | std::ios::binary);
//
//	if (msg)
//	{
//		CApproximateNearestNeighbor shore;
//		shore << file_in;
//		try
//		{
//			boost::iostreams::filtering_ostreambuf out;
//			out.push(boost::iostreams::gzip_compressor());
//			out.push(file_out);
//			std::ostream outcoming(&out);
//
//			//save coordinate and search optimisation
//			//size_t version = CNormalsDatabase::VERSION;
//			//outcoming.write((char*)(&version), sizeof(version));
//			outcoming << shore;
//		}
//		catch (const boost::iostreams::gzip_error& exception)
//		{
//			int error = exception.error();
//			if (error == boost::iostreams::gzip::zlib_error)
//			{
//				//check for all error code    
//				msg.ajoute(exception.what());
//			}
//		}
//
//		file_in.close();
//		file_out.close();
//	}
//
//	return msg;
//}
//
//ERMsg UploadShoreToAzure(string file_path)
//{
//	ERMsg msg;
//
//
//	std::shared_ptr<storage_credential> cred = std::make_shared<shared_key_credential>(ACCOUNT_NAME, ACCOUNT_KEY);
//	std::shared_ptr<storage_account> account = std::make_shared<storage_account>(ACCOUNT_NAME, cred, /* use_https */ true);
//	blob_client client(account, 16);
//
//	auto ret = client.create_container(CONTAINER_NAME).get();
//	std::ifstream fin(file_path, std::ios_base::in | std::ios_base::binary);
//	std::vector<std::pair<std::string, std::string>> metadata;
//	metadata.emplace_back(std::make_pair("type", "Shore"));
//	ret = client.upload_block_blob_from_stream(CONTAINER_NAME, "Layers/" + GetFileName(file_path), fin, metadata).get();
//	if (!ret.success())
//	{
//		msg.ajoute("Failed to upload blob, Error: " + ret.error().code + ", " + ret.error().code_name);
//	}
//	fin.close();
//	//client.delete_blob(container_name, blob_name).wait();
//
//
//	return msg;
//}
//
//ERMsg UploadDEMToAzure(string file_path)
//{
//	ERMsg msg;
//
//
//	std::shared_ptr<storage_credential> cred = std::make_shared<shared_key_credential>(ACCOUNT_NAME, ACCOUNT_KEY);
//	std::shared_ptr<storage_account> account = std::make_shared<storage_account>(ACCOUNT_NAME, cred, /* use_https */ true);
//	std::shared_ptr<blob_client> client = std::make_shared<blob_client>(account, 16);
//	blob_client_wrapper client_wrapper(client);
//	std::vector<std::pair<std::string, std::string>> metadata;
//	metadata.emplace_back(std::make_pair("type", "DEM"));
//	//	ret = client.upload_block_blob_from_stream(CONTAINER_NAME, "DEM/" + GetFileName(file_path), fin, metadata).get();
//
//	//	// Perform UploadStreamToBlockBlob
//	////bufferSize: = 2 * 1024 * 1024 // Configure the size of the rotating buffers that are used when uploading
//	//	//maxBuffers : = 3               // Configure the number of rotating buffers that are used when uploading
//	//	//_,/ err = UploadStreamToBlockBlob(ctx, bytes.NewReader(data), blockBlobURL,
//	//		//UploadStreamToBlockBlobOptions{ BufferSize: bufferSize, MaxBuffers : maxBuffers })
//	//	client.upload_block_from_stream()
//
//	client_wrapper.upload_file_to_blob(file_path, CONTAINER_NAME, "DEM/" + GetFileName(file_path), metadata,/* parallel */8);
//
//	//if (!ret.success())
//	//{
//		//msg.ajoute("Failed to upload blob, Error: " + ret.error().code + ", " + ret.error().code_name);
//	//}
////	fin.close();
//	//client.delete_blob(container_name, blob_name).wait();
//
//
//	return msg;
//}
