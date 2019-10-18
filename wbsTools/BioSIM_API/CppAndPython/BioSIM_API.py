
if __name__ == "__main__":
    import lib.BioSIM_API as BioSIM_API

    WG = BioSIM_API.WeatherGenerator('Context Name')

    
    #example of weather generator
    print ("Initialize Weather Generator")
    msg = WG.Initialize("Shore=./Layers/Shore.ann&Normals=./Weather/Normals/Canada-USA 1981-2010.NormalsDB&Daily=./Weather/Daily/Canada-USA 2018-2019.DailyDB&DEM=./DEM/Monde 30s(SRTM30).tif&Gribs=./Weather/Gribs/HRDPS daily 2019.Gribs" );
    print (msg)

    #example of extracting normals
    print ("Extract Normals")
    WGout = WG.GetNormals("Source=FromNormals&Compress=1&Variables=TN T TX P H R&ID=1&Name=Point1&Latitude=37.2089&Longitude=-80.5898" )
    print(WGout.msg)
    print(WGout.metadata)
    f = open("NormalsOutput.gz", "wb")
    f.write(WGout.data)
    f.close()


    #example of extracting weather form gribs 
    #output = WG.GenerateGribs("Variables=TN T TX P TD H WS WD R Z&Latitude=37.2089&Longitude=-80.5898&First_year=2019&Last_year=2019&Compress=1" )
    #print(output.msg)
    #print(output.metadata)
    #f = open("GribsOutput.gz", "wb")
    #f.write(output.data)
    #f.close()


    #example of extracting weather from 
    print ("Generate weather for 2018-2019, 3 replications")
    WGout = WG.Generate("Compress=1&Variables=TN T TX P H R&ID=1&Name=Point1&Latitude=37.2089&Longitude=-80.5898&First_year=2018&Last_year=2019&Replications=3" )
    print(WGout.msg)
    f = open("WGOutput.gz", "wb")
    f.write(WGout.data)
    f.close()


    #example of model execution
    #now call model with weather
    model = BioSIM_API.Model('Context Name')
    #we use Degree Day model
    print ("Initialize DegreeDay Model")
    msg = model.Initialize("Model=./Models/DegreeDay (Annual).mdl" )
    print (msg)


    print ("Execute DegreeDay Model")
    modelOut = model.Execute("Compress=1&Replications=1&Parameters=Method:6 LowerThreshold:4", WGout)
    print(modelOut.msg)
    print(modelOut.metadata)
    
    f = open("ModelOutput.gz", "wb")
    f.write(modelOut.data)
    f.close()


    #&ELEVATION=570.8