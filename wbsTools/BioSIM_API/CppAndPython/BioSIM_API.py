
if __name__ == "__main__":
    import lib.BioSIM_API as BioSIM_API

    WG = BioSIM_API.WeatherGenerator('Context Name')

    msg = WG.Initialize("Shore=./Layers/Shore.ann&Normals=./Weather/Normals/Canada-USA 1981-2010.NormalsDB&Daily=./Weather/Daily/Canada-USA 2018-2019.DailyDB&DEM=./DEM/Monde 30s(SRTM30).tif" );
    print (msg)
    
    output = WG.Generate("Variables=TN T TX P H R&Latitude=37.2089&Longitude=-80.5898&First_year=2018&Last_year=2019")
    f = open("output.csv", "w+")
    f.write(output)
    f.close()

    #&ELEVATION=570.8