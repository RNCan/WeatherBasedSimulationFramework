using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

namespace TestBioSIMAPI
{
    [Microsoft.AspNetCore.Mvc.BindProperties(SupportsGet = true)]
    public class Location
    {
        public Location(String id, String name, double lat, double lon, double elev)
        {
            this.id = id;
            this.name = name;
            this.lat = lat;
            this.lon = lon;
            this.elev = elev;
        }

        public String id { get; set; }
        public String name { get; set; }
        public double lat { get; set; }
        public double lon { get; set; }
        public double elev { get; set; }
    }
}
