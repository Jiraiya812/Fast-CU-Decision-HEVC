#include "globals_YS.h"


//////////////////// parameter 
double g_C[3];
double g_W_P[3];
double g_W_N[3];

///////////////////


// Statistics for Parameters 
Double* g_ScaleFactor = new Double[NUM_CU_DEPTH];
Double* g_d2NRDCost = new Double[NUM_CU_DEPTH];
Double* g_d2NBits = new Double[NUM_CU_DEPTH];
Double* g_d2NDistortion = new  Double[NUM_CU_DEPTH];
Double* g_dSOC = new  Double[NUM_CU_DEPTH];

Double***   g_iVerResult;//			[uiDepth][ModelType][ResultType]
Bool***     g_bDecisionSwitch;//	[uiDepth][ModelType][DecisionType]

///sequence information 
char*	g_pchInputFile;
Int		g_iQP = 0;
Int		g_iSourceWidth = 0;
Int		g_iSourceHeight = 0;
Double	g_dBitRate	=	0;
Double	g_dYUVPSNR	=	0;
Double	g_dET	=	0;
//////////////////////////////////////////////////////////////////////////
// New feature system 
///////////////////////////////////////////////////////////////////////////
Mat*		g_TrainingDataMat; // [uiDepth]
ofstream*	g_TrainingDataSet; // [uiDepth]

// name g_File + txtFilename 
FILE*	  g_pFile_Performance;//   fopen("precison.txt", "at");
ofstream* g_InsightDataSet; // [uiDepth]
/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////
//global control variables  
//////////////////////////////////////
Int	g_iPOC;

Int* g_iP;
Int* g_iT;
Int* g_iV;
Double*** g_dPrecision_Th; //  [uiDepth][ModelType][DecisionType]
Double g_dPrecision_Th_Default;
ModelType	g_mainModelType;
FeatureType g_mainFeatureType;

ModelType   g_modelType_2;
FeatureType g_featureType_2;


ModelType   g_modelType_3;
FeatureType g_featureType_3;

// control bool
Bool		g_bTrustAssistantSkip;
Bool		g_bMultiModel;
Bool*       g_bModelSwitch;
Bool		g_bDepthException;

// model 
model* **   model_linear; // [uiDepth][FeatureType]
CvSVM**     model_CvSVM; // [uiDepth][FeatureType]
CvRTrees**  model_CvRTrees; // [uiDepth][FeatureType]
// TrainingSet
ofstream*** g_TrainingSet;//[uiDepth][FeatureFormat][FeatureType]

Int**		g_iFeatureLength; // [uiDepth][FeatureType]
Int**       g_iSampleNumber; // [uiDepth][FeatureType]

// variables for SkipRDO rhis are counted during testing 
UInt g_uiNumCU_Decision[NUM_CU_DEPTH][NUM_DECISIONTYPE];

const char * FeatureFormat_Names[] = {
	"Unknown",
	"LibLinear",
	"OpenCV",
	"BayesStat"
};

const char * FeatureType_Names[] = {
	"N_OBF",
	"OBF",
	"EOBF",
	"SOC",
	"SUBSOC",
	"BOutlier",
	"MIXED",
	"Outlier",
	"NewFeature",
	"OBF_SATD",
	"OBF_SATD_RDCost",
	"NoneFeature"
};

const char * ModelType_Names[] = {
	"SVM_0",
	"SVM_1",
	"SVM_2",
	"SVM_OpenCV",
	"RTS_OpenCV",
	"Naive",
	"BayesDecision",
	"Assistant_Skip",
	"BayesNew",
	"NoneModel"
};

