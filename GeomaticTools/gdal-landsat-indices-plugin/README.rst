gdal-landsat-indices-plugin
==================



A gdal plugin that provides a small set of "lansat indices pixel functions" (see
http://www.gdal.org/gdal_vrttut.html) that can be used to create derived
raster bands.

The package provides:

* the implementation of a set of GDALDerivedPixelFunc(s) to be used with
  source raster band of virtual GDAL datasets
* a fake GDAL driver to register pixel functions

.. note::

    using the plugin mechanism is a hack aimed to enable python users
    to use pixel functions without C++ coding


List of Landsat indices pixel functions
-----------------------

:"NBR": Normalized Burn Ratio. (B4-B7)/(B4+B7)
:"NBR2": Normalized Burn Ratio 2. (B5-B7)/(B5+B7)
:"EVI": Enhanced Vegetation Index.  2.5 * ((B4–B3)/(B4 + 6*B3 – 7.5*B1 + 1))
:"SAVI": Soil Adjusted Vegetation Index. 
:"MSAVI": Modified Soil Adjusted Vegetation Index
:"NDVI": Normalized Difference Vegetation Index. (B4-B3)/(B4+B3)
:"NDMI": Normalized Difference Moisture Index. (B4-B5)/(B4+B5)
:"TCB": Tassel Cap Brightness.
:"TCG": Tassel Cap Greenness.
:"TCW": Tassel Cap Wetness.
:"SR": B4/B3
:"CL": B1/B6
:"HZ": B1/B3



How to get it
-------------

The project home page is at https://github.com/RNCan/WeatherBasedSimulationFramework/tree/master/GeomaticTools


How to build, test and install
------------------------------

The gdal-landsat-indices-plugin can be built using Visual Studio 2013


To install the plugin just copy the generated dll (gdal_LandsatIndices.dll)
into the GDAL plugin directory ("<GDAL directoy>\gdalplugins\" on Windows) or with QGIS (at least version 2.18.14) :
"<QGIS directory>\bin\gdalplugins\"


The plugin can also be used without installing it.
The user just needs to set the GDAL_DRIVER_PATH environment variable


License
-------

See the LICENSE.txt file.
