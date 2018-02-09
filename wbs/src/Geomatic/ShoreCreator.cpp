//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"   
#include "Basic/NormalsDatabase.h"
#include "Geomatic/ShoreCreator.h"
#include "Geomatic/ShapeFileBase.h"
#include "Geomatic/ProjectionTransformation.h"

using namespace std;


namespace WBSF
{


	ERMsg CShoreCreator::Shape2ANN(const string& fielpathIn, const string& filePathOut)
	{
		ERMsg msg;

		CShapeFileBase shape;

		msg = shape.Read(fielpathIn);
		if (msg)
		{
			ofStream stream;
			msg = stream.open(filePathOut, std::ios::binary);
			if (msg)
			{
				
				CLocationVector locations;
				for (size_t i = 0; i < shape.GetNbShape(); i++)
				{
					CSFShape& poly = shape[int(i)].GetShape();
					for (int j = 0; j < poly.GetNbRing(); j++)
					{
						int first = 0;
						int last = 0;
						poly.GetPointIndex(j, first, last);
						if (poly.GetNbRing()==1 || (last - first > 2) )
						{
							for (int k = first; k <= last; k++)
							{
								const CSFPoint& pt = poly.GetPoint(k);
								CLocation location("", "", pt.m_lat, pt.m_lon);

								if (k > first)
								{
									
									while (locations.back().GetDistance(location, false, false) > 5000)
									{
										//mean point between 
										CGeoPoint pt2 = (locations.back() + location) / 2;
										
										while (((CGeoPoint&)locations.back()).GetDistance(pt2) > 5000)
										{
											pt2 = (locations.back() + pt2) / 2;
										}
										
										locations.push_back(CLocation("", "", pt2.m_lat, pt2.m_lon));
									}
								}
								
								locations.push_back(location);
							}
						}
					}
				}

				if (!locations.empty())
				{
					/*CLocationVector locationsII;
					locationsII.reserve(locations.size());
					locationsII.push_back(locations.front());
					for (size_t i = 1; (i + 1) < locations.size(); i++)
					{
						if (locationsII.back().GetDistance(locations[i], false, false) > 2000)
						{
							locationsII.push_back(locations[i]);
						}
					}
					locationsII.push_back(locations.back());*/

					//CLocationVector locationsII;
					//locationsII.reserve(locations.size());
					//locationsII.push_back(locations.front());
					//for (size_t i = 1; i < locations.size(); i++)
					//{
					//	while(locationsII.back().GetDistance(locations[i], false, false) > 5000)
					//	{
					//		//mean point between 
					//		CGeoPoint pt = (locationsII.back() + locations[i])/2;
					//		locationsII.push_back(CLocation("", "", pt.m_lat, pt.m_lon));
					//	}

					//	locationsII.push_back(locations[i]);
					//}
					
					CApproximateNearestNeighbor ANN;
					ANN.set(locations, false, false);
					stream << ANN;
					stream.close();

					//locations.Save("D:/test.csv");
				}
			}
		}

		return msg;
	}

	ERMsg CShoreCreator::AddPoints(const string& shoreFilepath, const string& LocFilepathIn)
	{
		ERMsg msg;

		CLocationVector pointToAdd;
		msg += pointToAdd.Load(LocFilepathIn);

		
		ifStream stream;
		msg += stream.open(shoreFilepath, std::ios::binary);
		if (msg)
		{

			CApproximateNearestNeighbor ANN;
			ANN << stream;
			stream.close();

			CLocationVector locations(ANN.size() + pointToAdd.size());
			for (size_t i = 0; i < ANN.size(); i++)
			{
				locations[i] = ANN.at(i);
			}
			
			for (size_t i = 0; i < pointToAdd.size(); i++)
			{
				locations[ANN.size()+i] = pointToAdd.at(i);
			}

			ofStream stream;
			msg = stream.open(shoreFilepath, std::ios::binary);
			if (msg)
			{

				CApproximateNearestNeighbor ANN;
				ANN.set(locations, false, false);
				stream << ANN;
				stream.close();


			}
		}

		return msg;
	}

	//"U:\\Geomatique\\Shapefile\\water\\water-polygons-generalized-3857\\water_polygons_z3.shp"
	ERMsg CShoreCreator::ComputeDistance(const string& ShoreFilepath, const string& LocFilepathIn, const string& LocFilepathOut)
	{
		ERMsg msg;

		CShapeFileBase shapeIn;

		msg = shapeIn.Read(ShoreFilepath);
		if (msg)
		{
			//shapeIn.ComputeInternalElimination(100, 100);
			CLocationVector locationsIn;
			msg = locationsIn.Load(LocFilepathIn);
			if (msg)
			{
				CProjectionTransformation PT(PRJ_WGS_84, shapeIn.GetPrjID());

				CLocationVector locationsOut(locationsIn.size());
#pragma omp parallel for 
				for (__int64 i = 0; i < (__int64)locationsIn.size(); i++)
				{

					CGeoPoint pt(locationsIn[i]);
					pt.Reproject(PT);

					double d = 0;
					if (shapeIn.IsInside(pt))
					{
						int polyNo = 0;
						d = shapeIn.GetMinimumDistance(pt, &polyNo);
						//	if (polyNo == -1)
						//	d = 0;
					}

					locationsOut[i] = locationsIn[i];
					locationsOut[i].SetSSI(CLocation::GetDefaultSSIName(CLocation::SHORE_DIST), ToString(d / 1000, 1));

				}

				locationsOut.Save(LocFilepathOut);
			}
		}

		return msg;
	}




}