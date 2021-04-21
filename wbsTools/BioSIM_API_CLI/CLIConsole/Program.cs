using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using BioSIM_Wrapper;

namespace CLIConsole
{
    class Program
    {
        static void Main(string[] args)
        {
            
            WeatherGenerator WG = new WeatherGenerator("WG1");

            string[] arguments = Environment.GetCommandLineArgs();
            string path = System.IO.Path.GetDirectoryName(arguments[0]);
            //string path = "G:\\Travaux\\BioSIM_API";
            Console.WriteLine("trying path: " + path);

            


            string options = "Shore=" + path + "\\Layers\\Shore.ann&Normals=" + path + "\\Weather\\Normals\\Canada-USA 1981-2010.NormalsDB&Daily=" + path + "\\Weather\\Daily\\Canada-USA 2018-2019.DailyDB&DEM=" + path + "\\DEM\\Monde 30s(SRTM30).tif";
            //String options = "Normals=" + path + "Weather/Normals/C2010.NormalsDB";

            Console.WriteLine("Initialize Weather Generator");
            string msg = WG.Initialize(options);
            Console.WriteLine(msg);

            ////simple call to the  weather Generator
            //Console.WriteLine("Generate weather for 1981-2010, 1 replications");
            //string cmd = "Compress=0&Variables=Tn+T+Tx&ID=1&Name=Logan&Latitude=41.73333333&Longitude=-111.8&Elevation=2800&First_year=2018&Last_year=2018&Replications=1";
            //TeleIO WGout = WG.Generate(cmd);
            //Console.WriteLine(WGout.msg);



            ModelExecution model = new ModelExecution("Model1");

            Console.WriteLine("Initialize DegreeDay model");
            msg = model.Initialize("Model="+path+ "\\Models\\DegreeDay (Annual).mdl");
            Console.WriteLine(msg);

            string variables = model.GetWeatherVariablesNeeded();
            string parameters = model.GetDefaultParameters();
            bool compress = true;
            string compress_str = compress? "1":"0";


            //&Elevation=2800
            //example of extracting weather from
            Console.WriteLine("Generate weather for 2018-2019, 1 replications");
            TeleIO WGout = WG.Generate("Compress="+ compress_str+"&Variables=" + variables + "&ID=1&Name=Logan&Latitude=41.73333333&Longitude=-111.8&First_year=2018&Last_year=2019&Replications=1");
            Console.WriteLine(WGout.msg);
            Console.WriteLine(WGout.metadata);


            //example how to create TeleIO from string and bytes
            //TeleIO = BioSIM_API.teleIO(WGout.compress, WGout.msg, WGout.comment, WGout.metadata, WGout.text, WGout.data)


            Console.WriteLine("Execute DegreeDay Model");
            TeleIO modelOut = model.Execute("Compress="+ compress_str, WGout);
            Console.WriteLine(modelOut.msg);


            //string s = System.Text.Encoding.UTF8.GetString(modelOut.data);
            //Console.WriteLine(s);

            string path_out = path + "\\test" + (compress ? ".gz" : ".csv");
            System.IO.FileStream fs = System.IO.File.Create(path_out);
            //System.IO.BufferedStream file = fs.;
            //array < unsigned char>^ arr = gcnew array < unsigned char> (10);
            //file->Read(arr, 0, 10);
            fs.Write(modelOut.data, 0, modelOut.data.Length);
            fs.Close();

        }
    }
}
