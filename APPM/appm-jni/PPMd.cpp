/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 1997,        *
 *  1999-2001                                                               *
 *  Contents: main routine                                                  *
 *  Comments: system & compiler dependent file                              *
 *  Comments: Modified by Abhinav Parate for Android App Prediction         *
 ****************************************************************************/


//BEGIN_INCLUDE(all)
#include <jni.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define DEBUG_LOG 0
#if DEBUG_LOG
#include <android/log.h>
#endif
#include <math.h>
#include "PPMd.h"
#include "Model.h"

using namespace std;

#define CLK_TCK CLOCKS_PER_SEC

/* Variables for initialization */
int N_CTXFIRST_ENCODERS = 6;
int N_CTXSECOND_ENCODERS = 40;
static int MaxOrder = 4;
static int BUCKET=5;
static PPMEncoder** ctxFirstEncoders;
static PPMEncoder** ctxSecondEncoders;
static PPMEncoder* ppmEncoder;

/* Crap variables to be removed */
static const char* pFName;
static DWORD StartFilePosition;
static BOOL EncodeFlag;
static clock_t StartClock;
static struct ARC_INFO { // FileLength & CRC? Hmm, maybe in another times...
    DWORD signature,attrib;
    WORD  info,FNLen,time,date;
} _PACK_ATTR ai;

/*
 * Function to update prediction model for given encoder
 */
inline void updatePredictionModel(int currentApp, PPMEncoder *encoder, int BUCKET)
{
    encoder->Sought =encoder->MinContext;
    encoder->Sought->updateAccuracy(currentApp, BUCKET);
    encoder->EncodeChar(currentApp,NULL,NULL);
}


/*
 * Function to update prediction stats for given encoder
 */
inline void updatePredictionStats(PPMEncoder *encoder, int symArray[],double freqArray[],int NSYM, int BUCKET)
{
    encoder->Sought =encoder->MinContext;
    encoder->setSymbolCounts(symArray,freqArray,NSYM,BUCKET);
}

/*
 * Function resets arrays containing stats
 */
inline void resetArray(int symArray[],double freqArray[],int len)
{
    for(int i=0;i<len;i++)
    {
        symArray[i]=-1; freqArray[i]=0;
    }
}

/*
 * Sort symbol array based on probabilities in frequency array
 */
inline void sortArrays(int symArray[], double freqArray[],int len)
{
    for(int i=0; i<len-1; i++) { // Bubble sort to get current predictions
        for(int j=1; j<len-i;j++)
            if(freqArray[j-1]+0.00001<freqArray[j])
            {   
                double tFreq = freqArray[j-1];
                freqArray[j-1] = freqArray[j];
                freqArray[j] = tFreq;
                int tSym = symArray[j-1];
                symArray[j-1] = symArray[j];
                symArray[j] = tSym;
            }   
    }
#if DEBUG_LOG
    char WrkStr[100];
    for(int i=0;i<len;i++){
        sprintf(WrkStr,"ACR Post sort %d %d %f\n",len,symArray[i],freqArray[i]);
        __android_log_write(ANDROID_LOG_ERROR, "ACR", WrkStr);
    }
#endif
}
extern "C" {
/*
 * Class:     edu_umass_cs_sensors_falcon_model_PPM
 * Method:    init
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_edu_umass_cs_sensors_falcon_model_PPM_deinit
(JNIEnv *env, jobject thiz)
{
    for(int i=0;i<N_CTXFIRST_ENCODERS;i++){  
        ctxFirstEncoders[i]->DeInit();
    }
    for(int i=0;i<N_CTXSECOND_ENCODERS;i++)  {
        ctxSecondEncoders[i]->DeInit();
    }
    ppmEncoder->DeInit();
}

JNIEXPORT void JNICALL Java_edu_umass_cs_sensors_falcon_model_PPM_init
(JNIEnv *env, jobject thiz, jint j_nFirstClusters, jint j_nSecondClusters)
{
    int SASize = 10;
    N_CTXFIRST_ENCODERS = (int)j_nFirstClusters;
    if(N_CTXFIRST_ENCODERS>0){
    	ctxFirstEncoders = new PPMEncoder*[N_CTXFIRST_ENCODERS];
    	for(int i=0;i<N_CTXFIRST_ENCODERS;i++){  
        	ctxFirstEncoders[i] = new PPMEncoder; 
        	ctxFirstEncoders[i]->Initialize(MaxOrder,SASize);
    	}
    }
    N_CTXSECOND_ENCODERS = (int)j_nSecondClusters;
    if(N_CTXSECOND_ENCODERS>0){
    	ctxSecondEncoders = new PPMEncoder*[N_CTXSECOND_ENCODERS];
    	for(int i=0;i<N_CTXSECOND_ENCODERS;i++)  {
        	ctxSecondEncoders[i] = new PPMEncoder; 
        	ctxSecondEncoders[i]->Initialize(MaxOrder,SASize);
    	}
    }
    ppmEncoder = new PPMEncoder; 
    ppmEncoder->Initialize(MaxOrder,SASize);
}

/*
 * Class:     edu_umass_cs_sensors_falcon_model_PPM
 * Method:    setTopK
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_edu_umass_cs_sensors_falcon_model_PPM_setTopK
(JNIEnv *env, jobject thiz, jint j_topk){
    BUCKET = (int)j_topk;
}

/*
 * Class:     edu_umass_cs_sensors_falcon_model_PPM
 * Method:    getTopPredictions
 * Signature: (III)[[D
 */
JNIEXPORT jobjectArray JNICALL Java_edu_umass_cs_sensors_falcon_model_PPM_getTopPredictions
(JNIEnv *env, jobject thiz, jint j_firstCluster, jint j_secondCluster, jint j_topk)
{
    int NSYM = 30; int BUCKET=(int)j_topk;
    //int symArray[NSYM];double freqArray[NSYM];
    int *symArray = new int[NSYM];double* freqArray = new double[NSYM];
    resetArray(symArray, freqArray, NSYM);
    
    char WrkStr[100];
#if DEBUG_LOG
    for(int i=0;i<NSYM;i++){
        sprintf(WrkStr,"ACR Reset %d %d %f\n",NSYM,symArray[i],freqArray[i]);
        __android_log_write(ANDROID_LOG_ERROR, "ACR", WrkStr);
    }
#endif
    
    //Update stats from main encoder
    updatePredictionStats(ppmEncoder,symArray,freqArray,NSYM,BUCKET);
    
    //Update stats from time encoder
    int index = (int)j_firstCluster;
    if(N_CTXFIRST_ENCODERS>0)
    	updatePredictionStats(ctxFirstEncoders[index],symArray,freqArray,NSYM,BUCKET);
    
    //Update stats from location encoder
    index = (int)j_secondCluster;
    if(N_CTXSECOND_ENCODERS>0)
    	updatePredictionStats(ctxSecondEncoders[index],symArray,freqArray,NSYM,BUCKET);
    
#if DEBUG_LOG
    for(int i=0;i<NSYM;i++){
        sprintf(WrkStr,"ACR Presort %d %d %f\n",NSYM,symArray[i],freqArray[i]);
        __android_log_write(ANDROID_LOG_ERROR, "ACR", WrkStr);
    }
#endif
    //Sort stats in decreasing order
    sortArrays(symArray,freqArray,NSYM);
    
    //Get double array class
    jclass doubleArrayClass = env->FindClass("[D");
    if(doubleArrayClass == NULL){
        delete symArray; delete freqArray;
        return NULL;
    }
    jobjectArray resultArray = env->NewObjectArray( (jsize)2, doubleArrayClass, NULL);
    
    double* dSymArray = new double[NSYM];
    for(int i=0;i<NSYM;i++)
        dSymArray[i] = symArray[i]*1.0;
    
    //Set symbol array in results
    jdoubleArray doubleArray = env->NewDoubleArray( BUCKET);
    env->SetDoubleArrayRegion( doubleArray, (jsize) 0, (jsize) BUCKET, (jdouble*) dSymArray);
    env->SetObjectArrayElement( resultArray, (jsize) 0, doubleArray);
    env->DeleteLocalRef( doubleArray);
    
    //Set frequency array values in results
    doubleArray = env->NewDoubleArray( BUCKET);
    env->SetDoubleArrayRegion( doubleArray, (jsize) 0, (jsize) BUCKET, (jdouble*) freqArray);
    env->SetObjectArrayElement( resultArray, (jsize) 1, doubleArray);
    env->DeleteLocalRef( doubleArray);
    
    delete symArray; delete freqArray; delete dSymArray;
    return resultArray;
}

/*
 * Class:     edu_umass_cs_sensors_falcon_model_PPM
 * Method:    getEventProbability
 * Signature: (II)D
 */
JNIEXPORT jdouble JNICALL Java_edu_umass_cs_sensors_falcon_model_PPM_getEventProbability
(JNIEnv *env, jobject thiz, jint j_targetApp, jint j_depth, jint j_firstCluster, jint j_secondCluster)
 {
     int depth = (int)j_depth;
     if(depth>2) return 1.0;
     int NSYM = 30; 
     int *symArray = new int[NSYM];double* freqArray = new double[NSYM];
     resetArray(symArray, freqArray, NSYM);
     
     char WrkStr[100];
#if DEBUG_LOG
     for(int i=0;i<5;i++){
         sprintf(WrkStr,"ACR Reset %d %d %f\n",NSYM,symArray[i],freqArray[i]);
         __android_log_write(ANDROID_LOG_ERROR, "ACR", WrkStr);
     }
#endif
     
     //Update stats from main encoder
     updatePredictionStats(ppmEncoder,symArray,freqArray,NSYM,BUCKET);
     
     //Update stats from time encoder
     int index = (int)j_firstCluster;
     if(N_CTXFIRST_ENCODERS>0)
     	updatePredictionStats(ctxFirstEncoders[index],symArray,freqArray,NSYM,BUCKET);
     
     //Update stats from location encoder
     index = (int)j_secondCluster;
     if(N_CTXSECOND_ENCODERS>0)
     	updatePredictionStats(ctxSecondEncoders[index],symArray,freqArray,NSYM,BUCKET);
     
#if DEBUG_LOG
     for(int i=0;i<5;i++){
         sprintf(WrkStr,"ACR Presort %d %d %f\n",NSYM,symArray[i],freqArray[i]);
         __android_log_write(ANDROID_LOG_ERROR, "ACR", WrkStr);
     }
#endif
     //Sort stats in decreasing order
     sortArrays(symArray,freqArray,NSYM);
     
     int targetApp = (int)j_targetApp;
     //Compute depth 1 probability
     if(depth==1)
     {
         double ppmProb = 0.0;
         for(int i=0;i<NSYM;i++) {
             if(symArray[i]==targetApp) {
                 ppmProb = freqArray[i];
                 break;
             }
             if(symArray[i]==-1) break;
         }
         delete symArray; delete freqArray;
         return ppmProb;
     }
     //Compute depth 2 probability
     if(depth==2)
     {
         double ppmProb=0.0;
         NSYM=30;
         int nSymArray[NSYM];double nFreqArray[NSYM];
         for(int i=0;i<5;i++) {
             if(symArray[i]!=-1) {//Valid Symbol
                 ppmEncoder->Sought = ppmEncoder->SeekContext(symArray[i]);
                 resetArray(nSymArray,nFreqArray,NSYM);
                 if(ppmEncoder->Sought!=NULL){
                     ppmEncoder->setSymbolCounts(nSymArray,nFreqArray,NSYM,BUCKET);
                     for(int j=0;j<NSYM;j++) {
                         if(nSymArray[j]==targetApp){
                             ppmProb += freqArray[i]*nFreqArray[j];
                             break;
                         }
                     }
                 }//Got prob from valid symbol
             }//If valid symbol
         }//Top-5 only to save computation
         delete symArray; delete freqArray;
         return ppmProb;
     }
     delete symArray; delete freqArray;
     return 1.0;
 }

/*
 * Class:     edu_umass_cs_sensors_falcon_model_PPM
 * Method:    updateModel
 * Signature: (III)V
 */
JNIEXPORT void JNICALL Java_edu_umass_cs_sensors_falcon_model_PPM_updateModel
 (JNIEnv *env, jobject thiz, jint j_currentApp, jint j_firstCluster, jint j_secondCluster){
     
     int currentApp = (int)j_currentApp;
     //Update model from main encoder
     updatePredictionModel(currentApp,ppmEncoder,BUCKET);
     
     //Update stats from time encoder
     int index = (int)j_firstCluster;
     if(N_CTXFIRST_ENCODERS>0)
     	updatePredictionModel(currentApp,ctxFirstEncoders[index],BUCKET);
     
     //Update stats from location encoder
     index = (int)j_secondCluster;
     if(N_CTXSECOND_ENCODERS>0)
     	updatePredictionModel(currentApp,ctxSecondEncoders[index],BUCKET);
     
 }
    
}//extern c

void _STDCALL PrintInfo(_PPMD_FILE* DecodedFile,_PPMD_FILE* EncodedFile)
{
    /** TODO Update Print Info */
}




//END_INCLUDE(all)
