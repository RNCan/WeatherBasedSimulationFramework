#include "stdafx.h"
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>
#include "gzip.h"

namespace bio = boost::iostreams;

namespace WBSF
{
	ERMsg Gzip::compress(const std::string& data, std::string& compress)
	{
		ERMsg msg;

		
		try
		{
			
			std::stringstream origin(data);
			bio::filtering_streambuf<bio::input> out;
			out.push(bio::gzip_compressor(bio::gzip_params(bio::gzip::best_compression)));
			out.push(origin);

			std::stringstream compressed;
			bio::copy(out, compressed);
			compress = compressed.str();

		}//try
		catch (const boost::iostreams::gzip_error& exception)
		{
			int error = exception.error();
			if (error == boost::iostreams::gzip::zlib_error)
			{
				//check for all error code    
				msg.ajoute(exception.what());
			}
		}

		return msg;
	}

	ERMsg Gzip::decompress(const std::string& compress, std::string& data)
	{
		ERMsg msg;
		try
		{
			std::stringstream compressed(compress);
			bio::filtering_streambuf<bio::input> out;
			out.push(bio::gzip_decompressor());
			out.push(compressed);

			std::stringstream decompressed;
			bio::copy(out, decompressed);
			data = decompressed.str();
		}//try
		catch (const boost::iostreams::gzip_error& exception)
		{
			int error = exception.error();
			if (error == boost::iostreams::gzip::zlib_error)
			{
				//check for all error code    
				msg.ajoute(exception.what());
			}
		}
		return msg; 
	}

}