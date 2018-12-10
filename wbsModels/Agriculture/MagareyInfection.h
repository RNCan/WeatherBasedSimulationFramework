#pragma once

#include "ModelBase/BioSIMModelBase.h"

namespace WBSF
{
	//O_DEFICIT
	enum TMIDailyStat{ O_INFECTION, O_CUMUL_INFECTION, O_RAIN_INFECTION, O_CUMUL_RAIN_INFECTION, O_INFECTION_EVENT, NB_MAGAREY_INFECTION_OUTPUT };
	extern const char MAGAREY_HEADER[];
	typedef CModelStatVectorTemplate<NB_MAGAREY_INFECTION_OUTPUT, MAGAREY_HEADER> CMagareyInfectionStatVector;


	class CMagareyInfection
	{
	public:
		enum TPathogen {
			ALBUGO_OCCIDENTALIS_SPINACH, ALTERNARIA_BRASSICAE_OILSEED_RAPE, ALTERNARIA_CUCUMIS_MUSKMELON, ALTERNARIA_MALI_APPLE,
			ALTERNARIA_PORI_ONION, ALTERNARIA_SP_MINEOLA_TANGELO, ASCOCHYTA_RABIEI_CHICK_PEA, BIPOLARIS_ORYZAE_RICE, BOTRYOSPHAERIA_DOTHIDEA_APPLE,
			BOTRYOSPHAERIA_OBTUSA_APPLE, BOTRYTIS_CINEREA_GRAPES, BOTRYTIS_CINEREA_STRAWBERRY, BOTRYTIS_CINEREA_GERANIUM, BOTRYTIS_CINEREA_GRAPE_FLOWER,
			BOTRYTIS_SQUAMOSA_ONION, BREMIA_LACTUCAE_LETTUCE, CERCOSPORA_ARACHIDICOLA_PEANUT, CERCOSPORA_CAROTAE_CARROT, CERCOSPORA_KIKUCHII_SOYBEAN,
			COCCOMYCES_HIEMALIS_PRUNUS_SP, COLLETOTRICHUM_ACUTATUM_STRAWBERRY_FRUIT, COLLETOTRICHUM_COCCODES_TOMATO, COLLETOTRICHUM_ORBICULARE_WATERMELON,
			DIDYMELLA_ARACHIDICOLA_PEANUT, DIPLOCARPON_EARLIANUM_STRAWBERRY, ELSINOE_AMPELINA_GRAPE, GUIGNARDIA_BIDEWELLI_GRAPE, GYMNOPORANGIUM_JUNIPERI_VIRGINIANAE_APPLE,
			LEPTOSPHAERIA_MACULANS_RAPE, MELAMPSORA_MEDUSAE_POPLAR, MICROCYLUS_ULEI_RUBBER, MONILINIA_FRUCTICOLA_PRUNUS_FRUIT, MYCOPSHAERELLA_PINODES_PEA,
			MYCOSPHAERELLA_FRAGARIAE_STRAWBERRY, MYCOSPHAERELLA_GRAMINICOLA_WHEAT, PHAEOISARPIS_PERSONATA_PEANUT, PHAKOPSORA_PACHYRHIZI_SOYBEAN, PHYTOPHTHORA_CACTORUM_APPLE_FRUIT,
			PHYTOPHTHORA_CACTORUM_STRAWBERRY_FRUIT, PHYTOPHTHORA_INFESTANS_POTATO, PLASMOPARA_VITICOLA_GRAPE, PSUEDOPERONOSPORA_CUBENSIS_CUCUMBER, PUCCINIA_ARACHIDIS_GROUNDNUT,
			PUCCINIA_MENTHAE_PEPPERMINT, PUCCINIA_PSIDII_EUCALYPTUS, PUCCINIA_RECONDITA_WHEAT, PUCCINIA_STRIIFORMIS_WHEAT, PYRENOPEZZIA_BRASSICAE_OILSEED_RAPE,
			PYRENOPHORA_TERES_BARLEY, RHIZOCTONIA_SOLANI_RYE_GRASS, RHYNCHOSPORIUM_SECALIS_BARLEY, SCLEROTINIA_SCLEROTIORUM_BEANS, SEPTORIA_APIICOLA_CELERY,
			SEPTORIA_GLYCINES_SOYBEAN, UROMYCES_PHASEOLI_BEAN, VENTURIA_INEQUALIS_APPLE, VENTURIA_PIRINA_PEAR, WILSONOMYCES_CARPOPHILUS_ALMOND, NB_PATHOGEN
		};


		//input 
		int m_pathogenType;
		double m_prcpThreshold; //mm, Precipitation threshold for infection, 2 by default
		double m_interruption; //h, The number of dry hours to interupt wetness events from previous day. 1 by default
		CTRef m_beginning;

		CLocation m_loc;


		CMagareyInfection()
		{
			m_pathogenType = 0;
			m_prcpThreshold = 2;
			m_interruption = 1;
		}

		void Execute(const CWeatherStation& weather, CMagareyInfectionStatVector& output);

	protected:

		double GetF1(double T);
		double GetF2(double F1);
		double GetInfection(double WD, double F1);


		enum TParam    { L_TMIN, L_TMAX, L_TOPT, L_WMIN, L_WMAX, NB_PARAM };
		static const double LIBRARY[NB_PATHOGEN][NB_PARAM];
	};




	class CMagareyInfectionModel : public CBioSIMModelBase
	{
	public:

		CMagareyInfectionModel();
		virtual ~CMagareyInfectionModel();

		virtual ERMsg OnExecuteDaily();
		virtual ERMsg ProcessParameters(const CParameterVector& parameters);
		//virtual void AddDailyResult(const StringVector& header, const StringVector& data);
		//virtual void GetFValueDaily(CStatisticXY& stat);
		static CBioSIMModelBase* CreateObject(){ return new CMagareyInfectionModel; }


	protected:

		double GetF1(double T);
		double GetF2(double F1);
		double GetInfection(double WD, double F1);


		int m_pathogenType;
		double m_prcpThreshold; //mm, Precipitation threshold for infection, 2 by default
		double m_interruption; //h, The number of dry hours to interupt wetness events from previous day. 1 by default
		CTRef m_beginning;


	};
}