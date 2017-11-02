
pair<double, size_t> ComputeRMSE(const vector<pair<double, size_t>>& data, size_t i)
{
	CStatisticXY stat;
	for (size_t ii = 0; ii < 2; ii++)
	{
		ASSERT(data[i + ii].IsValid());
		stat.Add(data[i + ii].first);
	}

	ASSERT(stat[NB_VALUE] == 3);
	return make_pair(stat[RMSE], data[i + 1].second));
}

vector<size_t> Segmentation(vector<CLandsatPixel> dataIn, size_t max_nb_seg, double max_error)//,option1,option2,option3
{
	CTimer timer(true);

	vector<pair<double, size_t>> data;
	data.reserve(dataIn.size());
	for (size_t i = 0; i < data; i++)
	{
		if(data[i].IsValid())
			data.push_back(make_pair(dataIn[i].GetNBR(), i));
	}

	if (data.size() < 3)
		return vector<size_t>();

	//remove no data
	vector<pair<double, size_t>> myres_RMSE;// (data.size());
	//nbrow = N_ELEMENTS(NBR[*, 0]);
	//matrix<__int16>> BKP_YEAR=MAKE_ARRAY(nbrow,nbcol,/INTEGER, VALUE = -32768) // Vérifier pourquoi BKP prend nbseg * 2 et + 2??
	//vector<__int16 > BKP_YEAR; 
	//BKP_YEAR.insert(BKP_YEAR.begin(), data.size(), -32768);
	//BKP_NBR= MAKE_ARRAY(nbrow,nbcol,/INTEGER, VALUE = -32768) // Vérifier pourquoi BKP prend nbseg * 2 et + 2??
	//vector<__int16 > BKP_NBR;
	//BKP_NBR.insert(BKP_YEAR.begin(), data.size(), -32768);

	


	//******* Test DATA Test with a NBR slice
	//myslice =[7511,7548,7420,7084,7708,7050,-32768,7497,7166,-32768,7510,7283,7327,7563,7741,7701]
	//          0    1    2    3     4   5     6    7    8     9    10   11    12    13   14   15

	//1999  2000  2001  2002  2003  2004  2005  2006  2007  2008  2009  2010  2011  2012  2013  2014
	//No Change 6363  6691  6271  6743  6910  6561  6749  6479  6774  7061  7020  6123  6478  6557  5948  6709
	//Fire Severe 6522  6816  6312  6742  6812  6587  6587  6616  6714  7017  7017  -3795 -2299 629 140 711
	//Tordeuse severe  6659  7295  7082  7264  6870  6559  7188  7055  6355  6375  5568  5971  5187  4628  4069  3063
	//tordeuse light 7563  7774  7695  7851  7932  7428  7820  7914  7865  7796  7783  7371  7060  7411  7282  6905

	//myslice_start =[6363,6691,6271,6743,6910,6561,6749,6479,6774,7061,7020,6123,6478,6557,5948,6709] // No Change
	//myslice_start =[6522,6816,6312,6742,6812,6666,6587,6616,6714,7017,7017,-3795,-2299,629,140,711]//Fire Severe
	//myslice_start =[6659,7295,7082,7264,6870,6559,7188,7055,6355,6375,5568,5971,5187,4628,4069,3063]//Tordeuse severe

	//myslice_start =[7563,7774,7695,7851,7932,7428,7820,7914,7865,7796,7783,7371,7060,7411,7282,6905] // Tordeuse light


	//myslice =[ND,ND,ND,ND,ND,ND,ND,7497,7166,ND,7510,ND,ND,ND,ND,ND]
	//myslice =[7511,7548,7420,7084,7708,10497,7497,7166,7510,7283,7327,7563,7741,8000,8100,8250] //SPIKEe have to spike (with user val of 0.75), one in 2005 (0.1) and 2007 (0.0)


	//test with the a year slice
	//                    0    1    2    3    4   5     6    7    8    9   10   11   12   13   14   15
	//myslice_yr_start=[1999,2000,2001,2002,2003,2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014]


	//$$$$$ CHECK FOR AT LEAST 3 POINTS
	//index = WHERE(myslice[*] NE ND, count) // Attention, peut-être tout les pixels seront du NO DATA, à vérifier plus tard!!!!!!!!!!

	//IF count lt 3  THEN return,[max_drop1,max_drop2,TOT_NBR,myslope,my_rmse]

	//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
	//!!!!! YOU need at least 3 point to make a regression so this is the limiT
	//IF count lt 3  THEN return,[ND,ND,ND,ND,ND]
	//Only subscript the array if it is safe:
	//myslice=myslice[index]
	//myslice_yr=myslice_yr[index]

	//*** LOOP EACH PIXEL *******
	//for (size_t k = 0; k < nbrow; k++)
	//{

		//if option1 eq 'debug' then print,'running pixel:',k // simply REVERSE the order, like a mirror.  This is the only option now


		//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		//REMOVE NO DATA because it produce unwanted results
		//!!!!! attention la dectection du ND se fait sur les années et sur le NBR maintenant!!!!!!!!!!!!!!!!!!!!!!!!
		//myslice_yr_start = REFORM(YEAR[k, *]);
		//myslice_start = REFORM(NBR[k, *]);

		//myND = where(myslice_yr_start ne ND and myslice_start ne ND, ndcount, /NULL);

		////IF there is NO DATA VALUE, simply overwrite the 
		//if (ndcount > 0)
		//{
		//	myslice_yr_start = myslice_yr_start[myND]
		//	myslice_start = myslice_start[myND]
		//}

		//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

		//myslice = myslice_start; //transfer from the original
		//myslice_yr = myslice_yr_start; //transfer from the original

		//max_error = 0.150 * 10000 // not severe
		//max_error = 0.125 * 10000 // Dans Article de Hemormossill c'est 0.125
		//max_error = 0.100 * 10000
		//max_error = 0.075 * 10000 // really severe

		//max_error = max_error * 10000  // ATTENTION car le NBR est * 10000
		//max_nb_seg= 4

		//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

		//*** if less then 3 point   
		//if (myslice.size() < 3)
		//{
			//BKP_YEAR[k,0]=Reverse(myslice_yr)
			//BKP_NBR[k,0]=Reverse(myslice) 

			//BKP_YEAR = Reverse(myslice_yr); // we should flip  in the other dim
			//BKP_NBR = Reverse(myslice);

			//if option1 eq 'debug' then print, 'Less than 2 point:', N_ELEMENTS(myslice);
			
		//}
		//else
		//{

			//************************************************************************************************
			//nb=N_ELEMENTS(myslice)

			//1-first scan make pair and calculate the RMSE 
			//create a n-1 array to store the paire results
			//vector<double> myres_RMSE(myslice.size() - 2); //take the RMSE and later the SLOPE

			//for (size_t i = 1; i < myslice.size() - 2; i++)
			for (size_t i = 0; i < data.size() - 2; i++)
			{
				myres_RMSE.push_back(ComputeRMSE(data, i));
			}
			
			ASSERT(myres_RMSE.size() == data.size() - 2);
			//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
			//while  min(merge_cost) < max_error DO 
			//While  (min(myres_RMSE) le max_error) DO BEGIN
			//While  ((min(myres_RMSE) le max_error) or ((N_ELEMENTS(myres_RMSE) ge max_nb_seg)) and (N_ELEMENTS(myres_RMSE) gt 1)) DO BEGIN
			//While  (min(myres_RMSE) le max_error) and (N_ELEMENTS(myres_RMSE) gt 1) DO BEGIN
			//mycond = 'go';
			vector<pair<double, size_t>> minIt = std::min_element(myres_RMSE.begin(), myres_RMSE.end();
			while (myres_RMSE.size() > 1 && 
				((*minIt <= max_error) || (myres_RMSE.size() >= max_nb_seg)) )
			{
				//erase data item
				auto it = std::find_if(data.begin(), data.end(), [](const pair<short, string>& p) { return p.second == minIt->second; });
				ASSERT(it != data.end());
				data.erase(it);

				//A-Detect the point - Remove it - and recompute all the sats
				//1- GET the min value in the RMSE array and REMOVE it 
				//remove RMSE item and update RMSE for points that have change
				size_t pos = std::distance(myres_RMSE.begin(), minIt);

				if (pos - 1<myres_RMSE.size())
					myres_RMSE[pos - 1] = ComputeRMSE(data, myres_RMSE[pos - 1].second);
				if (pos + 1<myres_RMSE.size())
					myres_RMSE[pos + 1] = ComputeRMSE(data, myres_RMSE[pos + 1].second);

				
				myres_RMSE.erase(minIt);

				ASSERT(myres_RMSE.size() == data.size() - 2);
				//mymin = myres_RMSE[WHERE(myres_RMSE EQ min(myres_RMSE), count)] // get the min value
				//mo = WHERE(myres_RMSE NE min(myres_RMSE), count)// this the index to get all value except the min

				//myres_RMSE_new = myres_RMSE[mo]// use the index to create a new array with only the good value. This array will be fix later for the LEFt and Right portion that have change
					//Clipboard.Set,STRING(myres_RMSE_new)

				//REMOVE the MIDDLE point.  IE create a new array with the LEFT and Right<
				//2- All those steps because the array RMSE have not compatible index as the slice.  The RMSE is for three contiguous point, the slice is unique point. 
				//mi = WHERE(myres_RMSE EQ min(myres_RMSE), count)// this the index to get the min

				//ATTENTION PEUT_IL Y AVOIR DEUX MIN identique, lE COUNT DOIT TJOURS ETRE EGAL A 1?????????????????????????????????????????????????????????????????
				//mi = mi[0]// FIX FIX FIX  in case there's two RMSE exactly the same... JUST TAKE THE FIRST ONE  

				// GO by ARRAY INDEX (TAKE LEFT + TAKE RIGHT) -- SKIP the MIDDLE POINT
				//DEBUG
				//MYLEFT = myslice_yr [0:mi] // take the left
				//MYRIGHT = myslice_yr [mi+2:n_elements(myslice_yr)-1] // take the right portion

				//myslice_yr_new = [MYLEFT,MYRIGH// concatenate the two array

				//Take LEFT and Right (formula based on previous DEBUG)
				//myslice_new = [myslice[0:mi], myslice[mi + 2:n_elements(myslice) - 1]]
				//myslice_yr_new = [myslice_yr[0:mi], myslice_yr[mi + 2:n_elements(myslice_yr) - 1]]


				//3-RECOMPUTE  Update the stats. Choice of RECOMPUTE all th array OR only the points that have change
				//A-recompute all the stats 
				//mrmse= myres_RMSE_new // simly tansfer to be sure it won't erase 

				// FILL all the new with RMSE.  Recompute everything, much simple but much slower
				//For i=1, N_ELEMENTS(myslice_new)-2 DO mrmse [i-1] = f_Get_RMSE( myslice_new[i-1:i+1 ],myslice_yr_new[i-1:i+1],-32768,'RMSE')
				//print,mrmse
				//Clipboard.Set,STRING(mrmse)

				//B-recompute ONLY the changed point.  Note 2004 has disappears  

				//MYLEFT = myslice_yr_new [mi-1:mi+1] //2002-2003-2005 point
				//MYRIGHT = myslice_yr_new [mi:mi+2] //2003-2005-2006)

				//mLEFT = myslice_yr_new [mi-1:mi+1] //2002-2003-2005 point
				//mRIGHT = myslice_yr_new [mi:mi+2] //2003-2005-2006)


				//ONLY the LEFT and RIGHT cell that have change
				//IF mi ne 0 then myres_RMSE_new[mi - 1] = f_Get_RMSE(myslice_new[mi - 1:mi + 1], myslice_yr_new[mi - 1:mi + 1], -32768, 'RMSE', 'Nprint');//FOR LEFT SIDE
				//If mi le N_ELEMENTS(myres_RMSE_new) - 2  then myres_RMSE_new[mi] = f_Get_RMSE(myslice_new[mi:mi + 2], myslice_yr_new[mi:mi + 2], -32768, 'RMSE', 'Nprint');//FOR LEFT SIDE

					//If (mi ne 0) and (mi ne N_ELEMENTS(myres_RMSE_new)-1)  then myres_RMSE_new [mi]   = f_Get_RMSE( myslice_new [mi:mi+2],myslice_yr_new [mi:mi+2],-32768,'RMSE')//FOR LEFT SIDE

					//print,myres_RMSE_new
					//Clipboard.Set,STRING(myres_RMSE_new)


					//4- %%check the point
					//PLOT, myslice_yr,myslice, XRANGE = [1999,2014], XSTYLE = 1, XTITLE = 'Year',YTITLE = 'NBR',LINESTYLE = 4, THICK = 1
					//OPLOT,myslice_yr_new,myslice_new,PSYM=2, SYMSIZE=3 , COLOR=240 //LINESTYLE = 5, THICK = 2 // plot the new line 

					//5- %%replace the original slice at the end of this loop
					//myslice = myslice_new
					//myslice_yr = myslice_yr_new
					//myres_RMSE = myres_RMSE_new
					//print,myres_RMSE           

					//IF (N_ELEMENTS(myres_RMSE) eq 1) THEN print,'break at ',rownumber //exit if there's only 3 points ie 1 RMSE

					//if (myres_RMSE.size() == 1)
						//break; //exit if there's only 3 points ie 1 RMSE

				minIt = std::min_element(myres_RMSE.begin(), myres_RMSE.end());
			}

			//FIX manually THE LAST THREE POINT
			// MANUALLY IF THERE'S 3 point left and RMSE is low, MERGE them manually, to create one transect.  Otherwise it's to complicated to manage the index in the array
			if (N_ELEMENTS(myres_RMSE) >= 3)
			{
				if (N_ELEMENTS(myres_RMSE) eq 1) and(min(myres_RMSE) le max_error) then myslice_yr = [myslice_yr[0], myslice_yr[2]]
					if (N_ELEMENTS(myres_RMSE) eq 1) and(min(myres_RMSE) le max_error) then myslice = [myslice[0], myslice[2]]
			}

			//* in the case that ther is 2 points left
			//if (N_ELEMENTS(myres_RMSE) < 3) 
			else
			{
				if (N_ELEMENTS(myres_RMSE) eq 1) and(min(myres_RMSE) le max_error) then myslice_yr = [myslice_yr[0], myslice_yr[1]]
					if (N_ELEMENTS(myres_RMSE) eq 1) and(min(myres_RMSE) le max_error) then myslice = [myslice[0], myslice[1]]
			}


			if option1 eq 'debug' then print, 'running pixel: ', k, ' and nb seg: ', N_ELEMENTS(myslice_yr) - 1 // simply REVERSE the order, like a mirror.  This is the only option now


				//*** IGNORE SMALL SEGMENT WITH TWO POINTS AND AND DNBR LESS THAN 150 ??? IN the MIDDLE ONLY ???

				//**** REVERSE SEGMENT ******************          
				//BKP_YEAR[k,0]= Reverse(myslice_yr)
				//BKP_NBR[k,0]=Reverse(myslice)

				BKP_YEAR[k, 0] = transpose(Reverse(myslice_yr)) // we should flip  in the other dim
				BKP_NBR[k, 0] = transpose(Reverse(myslice))



				// CASE k OF
				// (floor(0.01 * nbrow)): print,'NB iteration 1%
				// (floor(0.10 * nbrow)): print,'NB iteration 10%
				// (floor(0.25 * nbrow)): print,'NB iteration 25%
				// (floor(0.50 * nbrow)): print,'NB iteration 50%
				// (floor(0.75 * nbrow)): print,'NB iteration 75%
				// (floor(0.90 * nbrow)): print,'NB iteration 90%
				// (floor(0.98 * nbrow)): print,'NB iteration 98%
				// ELSE: BREAK                      
				// ENDCASE

				//}// for k
		//}

	print, 'Total time for Segmentation', (SYSTIME(1) - Time1) / 60, '  Minutes  OR  ', ((SYSTIME(1) - Time1) / 60) / 60, ' Hours'

		//ex of output with 1 segement but 4 allowed (ie 5 case) ---> 2015    1999  -32768  -32768  -32768  4673    6826  -32768  -32768  -32768
	//	if option3 eq 'year' then return, BKP_YEAR
		//	if option3 eq 'nbr'  then return, BKP_NBR
			//	if option3 eq 'year-nbr'  then return, [[BKP_YEAR], [BKP_NBR]]
					//endif



}