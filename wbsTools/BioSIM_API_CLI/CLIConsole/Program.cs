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
            string exe_path = System.Reflection.Assembly.GetExecutingAssembly().Location;
            exe_path = System.IO.Path.GetDirectoryName(exe_path);
            Console.WriteLine("trying path: " + exe_path);

            string[] arguments = Environment.GetCommandLineArgs(); 
            string options = arguments[1];
            Console.WriteLine("trying options: " + options);

            var watch = System.Diagnostics.Stopwatch.StartNew();


            Console.WriteLine("Initialize Weather Generator");
            WeatherGenerator WG = new WeatherGenerator("WG1");
            string msg = WG.Initialize(options);
            Console.WriteLine(msg);

            watch.Stop();
            Console.WriteLine("Time to initialize Weather Generator: " + watch.ElapsedMilliseconds + " ms");
            watch.Restart();

            if (msg == "Success")
            {

                Console.WriteLine("Initialize DegreeDay model");
                ModelExecution model = new ModelExecution("Model1");
                msg = model.Initialize("Model=" + exe_path + "\\Models\\DegreeDay (Annual).mdl");
                Console.WriteLine(msg);

                watch.Stop();
                Console.WriteLine("Time to initialize DegreeDay model: " + watch.ElapsedMilliseconds + " ms");
                watch.Restart();

                if (msg == "Success")
                {


                    string variables = model.GetWeatherVariablesNeeded();
                    string parameters = model.GetDefaultParameters();
                    bool compress = false;
                    string compress_str = compress ? "1" : "0";


                    //&Elevation=2800
                    //example of extracting weather from
                    Console.WriteLine("Generate weather for 2018-2019, 1 replications");
                    TeleIO WGout = WG.Generate("Compress=" + compress_str + "&Variables=" + variables + "&ID=1&Name=Logan&Latitude=41.73333333&Longitude=-111.8&Elevation=120&First_year=2018&Last_year=2019&Replications=1");
                    Console.WriteLine(WGout.msg);
                    


                    // the code that you want to measure comes here
                    watch.Stop();
                    Console.WriteLine("Time to generate weather: " + watch.ElapsedMilliseconds + " ms");
                    watch.Restart();

                    if (msg == "Success")
                    {

                        Console.WriteLine("Execute DegreeDay Model");
                        TeleIO modelOut = model.Execute("Compress=" + compress_str, WGout);
                        Console.WriteLine(modelOut.msg);

                        string s = System.Text.Encoding.UTF8.GetString(modelOut.data);
                        Console.WriteLine(s);

                        watch.Stop();
                        Console.WriteLine("Time to run DD model: " + watch.ElapsedMilliseconds + " ms");
                        watch.Restart();


                        Console.WriteLine();
                        Console.WriteLine();
                        Console.WriteLine("Second generation");
                        WGout = WG.Generate("Compress=" + compress_str + "&Variables=" + variables + "&ID=1&Name=Logan&Latitude=41.73333333&Longitude=-111.8&Elevation=120&First_year=2018&Last_year=2019&Replications=1");

                        watch.Stop();
                        Console.WriteLine("Time to generate weather: " + watch.ElapsedMilliseconds + " ms");
                        watch.Restart();

                        modelOut = model.Execute("Compress=" + compress_str, WGout);
                        Console.WriteLine(s);

                        watch.Stop();
                        Console.WriteLine("Time to run DD model: " + watch.ElapsedMilliseconds + " ms");
                    }
                }
            }
            //string path_out = path + "\\test" + (compress ? ".gz" : ".csv");
            //System.IO.FileStream fs = System.IO.File.Create(path_out);
            //System.IO.BufferedStream file = fs.;
            //array < unsigned char>^ arr = gcnew array < unsigned char> (10);
            //file->Read(arr, 0, 10);
            //fs.Write(modelOut.data, 0, modelOut.data.Length);
            //fs.Close();

        }
    }
}
