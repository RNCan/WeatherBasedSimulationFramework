using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Logging;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using BioSIM_Wrapper;

namespace TestBioSIMAPI.Controllers
{
    [ApiController]
    [Route("[controller]")]
    public class BioSIMAPIController : ControllerBase
    {
        private readonly ILogger<BioSIMAPIController> _logger;

        private static readonly Dictionary<String, WeatherGenerator> m_WG = new Dictionary<String, WeatherGenerator>();



        public BioSIMAPIController(ILogger<BioSIMAPIController> logger)
        {
            _logger = logger;
        }



       [HttpGet("WeatherGeneration")]
       public String Get(String shore, String normals, String daily)
       {
           String options = "Shore=" + shore + "&Normals=" + normals + "&Daily=" + daily;

           var watch = System.Diagnostics.Stopwatch.StartNew();
           String msg;
           // return "Lat: " + lat + "     lon: "+lon +"     Year:"+ year;

           //string exe_path = System.Reflection.Assembly.GetExecutingAssembly().Location;
           //exe_path = System.IO.Path.GetDirectoryName(exe_path);
           String exe_path = "G:\\Travaux\\BioSIM_API";

           //string path = Environment.GetEnvironmentVariable("PATH");
           //Environment.SetEnvironmentVariable("PATH", exe_path + ";" + path);



           //Environment.CurrentDirectory = exe_path;
           // SetDllDirectory();
           System.Diagnostics.Debug.WriteLine("trying path: " + exe_path);
           System.Diagnostics.Debug.WriteLine("trying options: " + options);

           if (!m_WG.ContainsKey(options))
           {

               System.Diagnostics.Debug.WriteLine("Initialize Weather Generator");
               watch.Restart();
               m_WG[options] = new WeatherGenerator("WG" + m_WG.Count.ToString());
               msg = m_WG[options].Initialize(options);
               System.Diagnostics.Debug.WriteLine(msg);
               System.Diagnostics.Debug.WriteLine("Time to initialize Weather Generator: " + watch.ElapsedMilliseconds + " ms");

               if (msg != "Success")
                   return msg;
           }


           WeatherGenerator WG = m_WG[options];


           System.Diagnostics.Debug.WriteLine("Initialize DegreeDay model");
           watch.Restart();
           ModelExecution model = new ModelExecution("Model1");
           msg = model.Initialize("Model=" + exe_path + "\\Models\\DegreeDay (Annual).mdl");
           System.Diagnostics.Debug.WriteLine(msg);
           System.Diagnostics.Debug.WriteLine("Time to initialize DegreeDay model: " + watch.ElapsedMilliseconds + " ms");
           if (msg != "Success")
               return msg;

           string variables = model.GetWeatherVariablesNeeded();
           string parameters = model.GetDefaultParameters();
           bool compress = true;
           string compress_str = compress ? "1" : "0";


           //Generate weather for 2018-2019
           System.Diagnostics.Debug.WriteLine("Generate weather for 2018-2019, 1 replications");
           watch.Restart();
           TeleIO WGout = WG.Generate("Compress=" + compress_str + "&Variables=" + variables + "&ID=1&Name=Logan&Latitude=41.73333333&Longitude=-111.8&Elevation=120&First_year=2018&Last_year=2019&Replications=1");
           System.Diagnostics.Debug.WriteLine(WGout.msg);
           System.Diagnostics.Debug.WriteLine("Time to generate weather: " + watch.ElapsedMilliseconds + " ms");
           if (WGout.msg != "Success")
               return WGout.msg;

           //execute DD model
           System.Diagnostics.Debug.WriteLine("Execute DegreeDay Model");
           watch.Restart();
           TeleIO modelOut = model.Execute("Compress=0", WGout);//I think that Web API automatically compress text
           System.Diagnostics.Debug.WriteLine(modelOut.msg);
           System.Diagnostics.Debug.WriteLine("Time to run DD model: " + watch.ElapsedMilliseconds + " ms");

           if (modelOut.msg != "Success")
               return modelOut.msg;


           return System.Text.Encoding.UTF8.GetString(modelOut.data);

       }


        [HttpGet("GetModelOutput")]
        public String Get(String id, String name, double lat, double lon, double elev)
        {
            String msg;
            String options = "Shore=G:/Travaux/BioSIM_API/Layers/Shore.ann&Normals=G:/Travaux/BioSIM_API/Weather/Normals/World 1991-2020.NormalsDB.bin.gz&Daily=G:/Travaux/BioSIM_API/Weather/Daily/Canada-USA 2018-2019.DailyDB.bin.gz";
            Location loc = new Location(id, name, lat, lon, elev);


            if (!m_WG.ContainsKey(options))
            {

                System.Diagnostics.Debug.WriteLine("Initialize Weather Generator");
                m_WG[options] = new WeatherGenerator("WG" + m_WG.Count.ToString());
                msg = m_WG[options].Initialize(options);

                if (msg != "Success")
                    return msg;
            }


            WeatherGenerator WG = m_WG[options];

            ModelExecution model = new ModelExecution("Model1");
            msg = model.Initialize("Model=G:/Travaux/BioSIM_API/Models/DegreeDay (Annual).mdl");
            if (msg != "Success")
                return msg;

            string variables = model.GetWeatherVariablesNeeded();
            string parameters = model.GetDefaultParameters();
            bool compress = true;
            string compress_str = compress ? "1" : "0";

            //Generate weather for 2018-2019
            TeleIO WGout = WG.Generate("Compress=1" + compress_str + "&Variables=" + variables + "&ID="+id+"&Name="+name+"&Latitude="+lat+"&Longitude="+lon+"&Elevation="+elev+"&First_year=2018&Last_year=2019&Replications=1");
            if (WGout.msg != "Success")
                return WGout.msg;


            TeleIO modelOut = model.Execute("Compress=0", WGout);//I think that Web API automatically compress text
            if (modelOut.msg != "Success")
                return modelOut.msg;


            return System.Text.Encoding.UTF8.GetString(modelOut.data);
        }

    }
}
