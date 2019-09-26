
if __name__ == "__main__":
    import lib.BioSIM_API as BioSIM_API

    WG = BioSIM_API.WeatherGenerator('Context Name')

    
    msg = WG.Initialize("Shore=./Layers/Shore.ann&Normals=./Weather/Normals/Canada-USA 1981-2010.NormalsDB&Daily=./Weather/Daily/Canada-USA 2018-2019.DailyDB&DEM=./DEM/Monde 30s(SRTM30).tif&Gribs=./Weather/Gribs/HRDPS daily 2019.Gribs" );
    print (msg)

    output = WG.GenerateGribs("Variables=TN T TX P TD H WS WD R Z&Latitude=37.2089&Longitude=-80.5898&First_year=2019&Last_year=2019&Compress=1" )
    print(output.msg)
    f = open("output_gribs.gz", "wb")
    f.write(output.data)
    f.close()

    output = WG.GenerateGribs("Variables=TN T TX P TD H WS WD R Z&Latitude=39.2089&Longitude=-81.5898&First_year=2019&Last_year=2019&Compress=0" )
    print(output.msg)
    f = open("output_gribs.csv", "w+")
    f.write(output.text)
    f.close()



    #if uncompressed result
    output = WG.Generate("Variables=TN T TX P H R&Latitude=37.2089&Longitude=-80.5898&First_year=2018&Last_year=2019&Compress=0&Replications=3" )
    print(output.msg)
    f = open("output.csv", "w+")
    f.write(output.text)
    f.close()

    #if compressed result
    output = WG.Generate("Variables=TN T TX P H R&Latitude=37.2089&Longitude=-80.5898&First_year=2018&Last_year=2019&Compress=1&Replications=3" )
    print(output.msg)
    f = open("output.gz", "wb")
    f.write(output.data)
    f.close()


    


    #&ELEVATION=570.8