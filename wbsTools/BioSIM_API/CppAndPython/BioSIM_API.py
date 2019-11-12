if __name__ == "__main__":
    import lib.BioSIM_API as BioSIM_API


    

    #create and initialize WeatherGenerator
    WG = BioSIM_API.WeatherGenerator('Context Name')
    print("Initialize Weather Generator")
    msg = WG.Initialize("Shore=./Layers/Shore.ann&Normals=./Weather/Normals/Canada-USA 1981-2010.NormalsDB&Daily=./Weather/Daily/Canada-USA 2018-2019.DailyDB&DEM=./DEM/Monde 30s(SRTM30).tif&Gribs=./Weather/Gribs/HRDPS daily 2019.Gribs")
    print(msg)

    
      #create and initialize Model
    model = BioSIM_API.Model('Context Name')

    #we use Degree Day model as example
    print("Initialize DegreeDay Model")
    msg = model.Initialize("Model=./Models/DegreeDay (Annual).mdl")
    print(msg)

    variables = model.GetWeatherVariablesNeeded()
    parameters = model.GetDefaultParameters()
    print("DegreeDay model:")
    print("\tVariables needed: " + variables)
    print("\tDefault parameters: \r\n\t\t" + parameters.replace('+',"\r\n\t\t"))
    print(model.Help())


   

    #example of extracting normals
    #print ("Extract Normals")
    #WGout =
    #WG.GetNormals("Source=FromNormals&Compress=1&Variables="+variables+"&ID=1&Name=Point1&Latitude=37.2089&Longitude=-80.5898"
    #)
    #print(WGout.msg)
    #print(WGout.metadata)
    #f = open("NormalsOutput.gz", "wb")
    #f.write(WGout.data)
    #f.close()


    #example of extracting weather form gribs
    #output =
    #WG.GenerateGribs("Variables="+variables+"&Latitude=37.2089&Longitude=-80.5898&First_year=2019&Last_year=2019&Compress=1"
    #)
    #print(output.msg)
    #print(output.metadata)
    #f = open("GribsOutput.gz", "wb")
    #f.write(output.data)
    #f.close()

    compress = True
    compress_str = "1" if compress else "0"
	
    #example of extracting weather from
    print("Generate weather for 2018-2019, 3 replications")
    WGout = WG.Generate("Compress=" + compress_str + "&Variables=" + variables + "&ID=1&Name=Point1&Latitude=37.2089&Longitude=-80.5898&First_year=2018&Last_year=2019&Replications=3")
    print(WGout.msg)
    if compress:
        f = open("WGOutput.gz", "wb")
        f.write(WGout.data)
    else:
        f = open("WGOutput.csv", "w")
        f.write(WGout.text)
    
    f.close()

    
    #example of model execution
    #now call model with weather

    #example how to create TeleIO from string and bytes
    TeleIO = BioSIM_API.teleIO(WGout.compress, WGout.msg, WGout.comment, WGout.metadata, WGout.text, WGout.data)

    print("Execute DegreeDay Model")
    modelOut = model.Execute("Compress=" + compress_str + "&Replications=1&Parameters=+Method:6+LowerThreshold:4", TeleIO)
    print(modelOut.msg)
    print(modelOut.metadata)
    
    if compress:
        f = open("ModelOutput.gz", "wb")
        f.write(modelOut.data)
    else:
        f = open("ModelOutput.csv", "w")
        f.write(modelOut.text)
        
    
    f.close()
