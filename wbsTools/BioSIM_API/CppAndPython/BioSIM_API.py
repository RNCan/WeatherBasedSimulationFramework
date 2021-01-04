if __name__ == "__main__":
    import lib.BioSIM_API as BioSIM_API
    import time

   
#create and initialize WeatherGenerator
    WG = BioSIM_API.WeatherGenerator('Context Name')
    
    print("Initialize Weather Generator")
    msg = WG.Initialize("Shore=./Layers/Shore.ann&Normals=./Weather/Normals/Canada-USA 1981-2010.NormalsDB&Daily=./Weather/Daily/Canada-USA 2018-2019.DailyDB&DEM=./DEM/Monde 30s(SRTM30).tif")
    print(msg)


#example of extracting weather from
    model = BioSIM_API.Model('Context Name')
    
    
    print("Initialize MPB-SLR")
    #msg = model.Initialize("Model=./Models/MPB-SLR.mdl")
    msg = model.Initialize("Model=E:/Project/bin/Debugx64/Models/MPB-SLR.mdl")
    print(msg)

    variables = model.GetWeatherVariablesNeeded()
    parameters = model.GetDefaultParameters()
    compress = False
    compress_str = "1" if compress else "0"

    #example of extracting weather from
    print("Generate weather for 1981-2010, 1 replications")
    WGout = WG.Generate("Compress=" + compress_str + "&Variables=" + variables + "&ID=1&Name=Logan&Latitude=41.73333333&Longitude=-111.8&Elevation=2800&First_year=2018&Last_year=2018&Replications=1")
    print(WGout.msg)
    
    #example of model execution
    #now call model with weather
    
    #example how to create TeleIO from string and bytes
    TeleIO = BioSIM_API.teleIO(WGout.compress, WGout.msg, WGout.comment, WGout.metadata, WGout.text, WGout.data)
    
    print("Execute MPB-SLR Model")
    modelOut = model.Execute("Compress=" + compress_str, TeleIO)
    print(modelOut.msg)
   