/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2015, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TEncSlice.cpp
    \brief    slice encoder class
*/

#include "TEncTop.h"
#include "TEncSlice.h"
#include <math.h>
#include "TLibCommon/linear.h"


using namespace std;

#if SVM_LOAD_OFFLINE_MODEL
extern model* model_0;
extern model* model_1;
extern model* model_2;
extern model* model_3;
#endif

#if GEN_OUTLIER
#if DCT_SIZE_IS_FOUR
Void partialButterfly(TCoeff *src, TCoeff *dst, Int shift, Int line)
{
    Int j;
    TCoeff E[2],O[2];
    TCoeff add = (shift > 0) ? (1<<(shift-1)) : 0;
    
    for (j=0; j<line; j++)
    {
        /* E and O */
        E[0] = src[0] + src[3];
        O[0] = src[0] - src[3];
        E[1] = src[1] + src[2];
        O[1] = src[1] - src[2];
        
        dst[0]      = (g_aiT4[TRANSFORM_FORWARD][0][0]*E[0] + g_aiT4[TRANSFORM_FORWARD][0][1]*E[1] + add)>>shift;
        dst[2*line] = (g_aiT4[TRANSFORM_FORWARD][2][0]*E[0] + g_aiT4[TRANSFORM_FORWARD][2][1]*E[1] + add)>>shift;
        dst[line]   = (g_aiT4[TRANSFORM_FORWARD][1][0]*O[0] + g_aiT4[TRANSFORM_FORWARD][1][1]*O[1] + add)>>shift;
        dst[3*line] = (g_aiT4[TRANSFORM_FORWARD][3][0]*O[0] + g_aiT4[TRANSFORM_FORWARD][3][1]*O[1] + add)>>shift;
        
        src += 4;
        dst ++;
    }
}

void partialButterflyInverse(TCoeff *src, TCoeff *dst, Int shift, Int line, const TCoeff outputMinimum, const TCoeff outputMaximum)
{
    Int j;
    TCoeff E[2],O[2];
    TCoeff add = (shift > 0) ? (1<<(shift-1)) : 0;
    
    for (j=0; j<line; j++)
    {
        /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
        O[0] = g_aiT4[TRANSFORM_INVERSE][1][0]*src[line] + g_aiT4[TRANSFORM_INVERSE][3][0]*src[3*line];
        O[1] = g_aiT4[TRANSFORM_INVERSE][1][1]*src[line] + g_aiT4[TRANSFORM_INVERSE][3][1]*src[3*line];
        E[0] = g_aiT4[TRANSFORM_INVERSE][0][0]*src[0]    + g_aiT4[TRANSFORM_INVERSE][2][0]*src[2*line];
        E[1] = g_aiT4[TRANSFORM_INVERSE][0][1]*src[0]    + g_aiT4[TRANSFORM_INVERSE][2][1]*src[2*line];
        
        /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */
        dst[0] = Clip3( outputMinimum, outputMaximum, (E[0] + O[0] + add)>>shift );
        dst[1] = Clip3( outputMinimum, outputMaximum, (E[1] + O[1] + add)>>shift );
        dst[2] = Clip3( outputMinimum, outputMaximum, (E[1] - O[1] + add)>>shift );
        dst[3] = Clip3( outputMinimum, outputMaximum, (E[0] - O[0] + add)>>shift );
        
        src   ++;
        dst += 4;
    }
}

#else
Void partialButterfly(TCoeff *src, TCoeff *dst, Int shift, Int line)
{
    Int j,k;
    TCoeff E[4],O[4];
    TCoeff EE[2],EO[2];
    TCoeff add = (shift > 0) ? (1<<(shift-1)) : 0;
    
    for (j=0; j<line; j++)
    {
        /* E and O*/
        for (k=0;k<4;k++)
        {
            E[k] = src[k] + src[7-k];
            O[k] = src[k] - src[7-k];
        }
        /* EE and EO */
        EE[0] = E[0] + E[3];
        EO[0] = E[0] - E[3];
        EE[1] = E[1] + E[2];
        EO[1] = E[1] - E[2];
        
        dst[0]      = (g_aiT8[TRANSFORM_FORWARD][0][0]*EE[0] + g_aiT8[TRANSFORM_FORWARD][0][1]*EE[1] + add)>>shift;
        dst[4*line] = (g_aiT8[TRANSFORM_FORWARD][4][0]*EE[0] + g_aiT8[TRANSFORM_FORWARD][4][1]*EE[1] + add)>>shift;
        dst[2*line] = (g_aiT8[TRANSFORM_FORWARD][2][0]*EO[0] + g_aiT8[TRANSFORM_FORWARD][2][1]*EO[1] + add)>>shift;
        dst[6*line] = (g_aiT8[TRANSFORM_FORWARD][6][0]*EO[0] + g_aiT8[TRANSFORM_FORWARD][6][1]*EO[1] + add)>>shift;
        
        dst[line]   = (g_aiT8[TRANSFORM_FORWARD][1][0]*O[0] + g_aiT8[TRANSFORM_FORWARD][1][1]*O[1] + g_aiT8[TRANSFORM_FORWARD][1][2]*O[2] + g_aiT8[TRANSFORM_FORWARD][1][3]*O[3] + add)>>shift;
        dst[3*line] = (g_aiT8[TRANSFORM_FORWARD][3][0]*O[0] + g_aiT8[TRANSFORM_FORWARD][3][1]*O[1] + g_aiT8[TRANSFORM_FORWARD][3][2]*O[2] + g_aiT8[TRANSFORM_FORWARD][3][3]*O[3] + add)>>shift;
        dst[5*line] = (g_aiT8[TRANSFORM_FORWARD][5][0]*O[0] + g_aiT8[TRANSFORM_FORWARD][5][1]*O[1] + g_aiT8[TRANSFORM_FORWARD][5][2]*O[2] + g_aiT8[TRANSFORM_FORWARD][5][3]*O[3] + add)>>shift;
        dst[7*line] = (g_aiT8[TRANSFORM_FORWARD][7][0]*O[0] + g_aiT8[TRANSFORM_FORWARD][7][1]*O[1] + g_aiT8[TRANSFORM_FORWARD][7][2]*O[2] + g_aiT8[TRANSFORM_FORWARD][7][3]*O[3] + add)>>shift;
        
        src += 8;
        dst ++;
    }
}
void partialButterflyInverse(TCoeff *src, TCoeff *dst, Int shift, Int line, const TCoeff outputMinimum, const TCoeff outputMaximum)
{
    Int j,k;
    TCoeff E[4],O[4];
    TCoeff EE[2],EO[2];
    TCoeff add = (shift > 0) ? (1<<(shift-1)) : 0;
    
    for (j=0; j<line; j++)
    {
        /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
        for (k=0;k<4;k++)
        {
            O[k] = g_aiT8[TRANSFORM_INVERSE][ 1][k]*src[line]   + g_aiT8[TRANSFORM_INVERSE][ 3][k]*src[3*line] +
            g_aiT8[TRANSFORM_INVERSE][ 5][k]*src[5*line] + g_aiT8[TRANSFORM_INVERSE][ 7][k]*src[7*line];
        }
        
        EO[0] = g_aiT8[TRANSFORM_INVERSE][2][0]*src[ 2*line ] + g_aiT8[TRANSFORM_INVERSE][6][0]*src[ 6*line ];
        EO[1] = g_aiT8[TRANSFORM_INVERSE][2][1]*src[ 2*line ] + g_aiT8[TRANSFORM_INVERSE][6][1]*src[ 6*line ];
        EE[0] = g_aiT8[TRANSFORM_INVERSE][0][0]*src[ 0      ] + g_aiT8[TRANSFORM_INVERSE][4][0]*src[ 4*line ];
        EE[1] = g_aiT8[TRANSFORM_INVERSE][0][1]*src[ 0      ] + g_aiT8[TRANSFORM_INVERSE][4][1]*src[ 4*line ];
        
        /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */
        E[0] = EE[0] + EO[0];
        E[3] = EE[0] - EO[0];
        E[1] = EE[1] + EO[1];
        E[2] = EE[1] - EO[1];
        for (k=0;k<4;k++)
        {
            dst[ k   ] = Clip3( outputMinimum, outputMaximum, (E[k] + O[k] + add)>>shift );
            dst[ k+4 ] = Clip3( outputMinimum, outputMaximum, (E[3-k] - O[3-k] + add)>>shift );
        }
        src ++;
        dst += 8;
    }
}

#endif //dct size 4 or 8
// Assume the max for DCT coefficients as 12bits. Need to be checked.
#define MaxAmp 65536
#define LambdaDelta 0.1
#define MinLikelyhood  -1.e30
//(-(1<<30))
#define StartPointProb 0.1










typedef struct {
    int count;
    double AcumAbsAmp;
    double AcumSampNum;
    double prob;
    double lambda;
    double likelyhood ;
} tBucket;

double ComputeLambdaGivenYc(double Yc, double sumYi, double totalNumYi)
{
    double c = sumYi/totalNumYi ;
    double lambda, lambda_old ;
    
    if( c/Yc>=0.95) return -1.0 ; // Too much away from Laplacian
    
    lambda_old = c ;
    
    lambda = c - Yc * (1.0 - 1.0/(1.0 - exp(-Yc/lambda_old)) ) ;
    
    { int k;
        for(k=0;k<5;k++)
        {
            lambda_old = lambda ;
            
            lambda = c - Yc * (1.0 - 1.0/(1.0 - exp(-Yc/lambda_old)) ) ;
        }
    }
    while( fabs(lambda - lambda_old) > LambdaDelta )
    {
        lambda_old = lambda ;
        
        lambda = c - Yc * (1.0 - 1.0/(1.0 - exp(-Yc/lambda_old)) ) ;
    }
    
    return lambda ;
    
}


int FindStartPoint(tBucket *buck, int peak, int N)
{
    int k ;
    for(k=peak;k>0;k--)
    {
        if( buck[k].count==0) continue ;
        
        if( buck[k].AcumSampNum < N* (1.0-StartPointProb)) break ;
    }
    
    if(buck[0].count>N/100 && buck[1].count>N/100 && buck[2].count>N/100  && buck[3].count>N/100)
    {
        if(k<3) k=3;
    }
    else if(buck[0].count>N/100 && buck[1].count>N/100 && buck[2].count>N/100 )
    {
        if(k<2) k=2;
    }
    else
    {
        if(k<1) k=1;
    }
    return k;
}

void  ComputeLikelyhood(int thePoint, int N, tBucket *buck, int peak)
{
    
    double N1 = buck[thePoint].AcumSampNum ;
    
    double N2 = N - N1 ;
    double Yc = thePoint ;
    
    double sumYi = buck[thePoint].AcumAbsAmp ;
    double totalNumYi = buck[thePoint].AcumSampNum ;
    
    double lambda =  ComputeLambdaGivenYc(Yc,  sumYi, totalNumYi) ;
    
    double prob = (double)N1 / (double)N ;
    
    if(lambda>0)
    {
        buck[thePoint].likelyhood = N2 * log(1- prob) + N1 * log(prob) - N2 * log( (peak - Yc)*2.0)
        -N1 * log(1-exp(-Yc/lambda))
        -N1 * log(2*lambda) - sumYi/lambda ;
        
        buck[thePoint].lambda = lambda ;
        buck[thePoint].prob = prob ;
    }
    else
    {
        buck[thePoint].likelyhood = -MinLikelyhood ;
        
        buck[thePoint].lambda = lambda ;
        buck[thePoint].prob = 1 ;
    }
}

void TCMprocessOneSequence(int *C, int Len, int *peak, double*prob, double *lambda, double*Yc )
{
    int k, MaxPos, StartPoint;
    double MaxLikelyhood, likelyhood ;
    tBucket buck[10000];//YS add
	//cout << Len<<" ";
    *peak = 0 ;
    for(k=0;k<Len;k++)
    {
        int absv = abs(C[k]);
        if(absv> (*peak) ) *peak = absv ;
    }
    
    if( *peak ==0 || *peak>=MaxAmp  )
    {
        printf("Input data either all zero or exceed %d (%d)\n", MaxAmp, *peak);
        
        *lambda = -1 ;
        *Yc = 0;
        *prob = 1 ;
		//cout << *Yc << "[error 1]";
        return ;
    }
    
    for(k=0;k<= (*peak);k++)
    {
        buck[ k ].count = 0 ;
        buck[ k ].AcumAbsAmp = 0 ;
        buck[ k ].AcumSampNum = 0 ;
    }
    
    for(k=0;k<Len;k++)
    {
        int cabs = abs(C[k]);
        buck[ cabs ].count ++ ;
    }
    buck[0].AcumSampNum = buck[0].count ;
    
    for(k=1;k<= (*peak);k++)
    {
        buck[ k ].AcumAbsAmp  = buck[ k-1 ].AcumAbsAmp + k * buck[ k ].count ;
        buck[ k ].AcumSampNum = buck[ k-1 ].AcumSampNum + buck[ k ].count ;
    }
    
    // saftey check
    /*if(buck[0].count < (buck[1].count>>1) || buck[1].count < (buck[2].count>>1))
    {
        *lambda = -1 ;
        *Yc = 0;
        *prob = 1 ;
		cout << *Yc << "[error 2]";
        return ;
    }*/
    
    StartPoint = FindStartPoint(buck, *peak, Len);
    
    //if( StartPoint<=0 )
    //{
    //    *prob = 1 ;
    //    *lambda = -1 ;
    //    *Yc = 0 ;
    //    printf("Data distribution is not like Laplacian1\n");
    //    return ;
    //}
    
    ComputeLikelyhood(StartPoint, Len, buck, *peak)  ;
    
    MaxLikelyhood = buck[StartPoint].likelyhood ;
    
    MaxPos = StartPoint ;
    for(k=StartPoint+1; k<=(*peak); k++)
    {
        if(buck[k].count==0) continue ;
        
        ComputeLikelyhood(k, Len, buck, *peak)  ;
        
        likelyhood = buck[k].likelyhood ;
        
        if( likelyhood > MaxLikelyhood)
        {
            MaxPos = k ;
            MaxLikelyhood = likelyhood ;
        }
    }
    
    if( MaxLikelyhood > MinLikelyhood)
    {
        *prob = buck[MaxPos].prob ;
        *lambda = buck[MaxPos].lambda ;
        *Yc = MaxPos ;
    }
    else  // the search failed
    {
        *prob = 1 ;
        *lambda = -1 ;
        *Yc = 0 ;
        // QQQQ printf("Data distribution is not like Laplacian2\n");
		//cout << *Yc << "[error 3]";
    }

	//cout << *Yc<<" ";
}
#endif
//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncSlice::TEncSlice()
 : m_encCABACTableIdx(I_SLICE)
{
  m_apcPicYuvPred = NULL;
  m_apcPicYuvResi = NULL;

  m_pdRdPicLambda = NULL;
  m_pdRdPicQp     = NULL;
  m_piRdPicQp     = NULL;
}

TEncSlice::~TEncSlice()
{
}

Void TEncSlice::create( Int iWidth, Int iHeight, ChromaFormat chromaFormat, UInt iMaxCUWidth, UInt iMaxCUHeight, UChar uhTotalDepth )
{
  // create prediction picture
  if ( m_apcPicYuvPred == NULL )
  {
    m_apcPicYuvPred  = new TComPicYuv;
    m_apcPicYuvPred->create( iWidth, iHeight, chromaFormat, iMaxCUWidth, iMaxCUHeight, uhTotalDepth );
  }

  // create residual picture
  if( m_apcPicYuvResi == NULL )
  {
    m_apcPicYuvResi  = new TComPicYuv;
    m_apcPicYuvResi->create( iWidth, iHeight, chromaFormat, iMaxCUWidth, iMaxCUHeight, uhTotalDepth );
  }
}

Void TEncSlice::destroy()
{
  // destroy prediction picture
  if ( m_apcPicYuvPred )
  {
    m_apcPicYuvPred->destroy();
    delete m_apcPicYuvPred;
    m_apcPicYuvPred  = NULL;
  }

  // destroy residual picture
  if ( m_apcPicYuvResi )
  {
    m_apcPicYuvResi->destroy();
    delete m_apcPicYuvResi;
    m_apcPicYuvResi  = NULL;
  }

  // free lambda and QP arrays
  if ( m_pdRdPicLambda )
  {
    xFree( m_pdRdPicLambda );
    m_pdRdPicLambda = NULL;
  }
  if ( m_pdRdPicQp )
  {
    xFree( m_pdRdPicQp );
    m_pdRdPicQp = NULL;
  }
  if ( m_piRdPicQp )
  {
    xFree( m_piRdPicQp );
    m_piRdPicQp = NULL;
  }
}

Void TEncSlice::init( TEncTop* pcEncTop )
{
  m_pcCfg             = pcEncTop;
  m_pcListPic         = pcEncTop->getListPic();

  m_pcGOPEncoder      = pcEncTop->getGOPEncoder();
  m_pcCuEncoder       = pcEncTop->getCuEncoder();
  m_pcPredSearch      = pcEncTop->getPredSearch();

  m_pcEntropyCoder    = pcEncTop->getEntropyCoder();
  m_pcSbacCoder       = pcEncTop->getSbacCoder();
  m_pcBinCABAC        = pcEncTop->getBinCABAC();
  m_pcTrQuant         = pcEncTop->getTrQuant();

  m_pcRdCost          = pcEncTop->getRdCost();
  m_pppcRDSbacCoder   = pcEncTop->getRDSbacCoder();
  m_pcRDGoOnSbacCoder = pcEncTop->getRDGoOnSbacCoder();

  // create lambda and QP arrays
  m_pdRdPicLambda     = (Double*)xMalloc( Double, m_pcCfg->getDeltaQpRD() * 2 + 1 );
  m_pdRdPicQp         = (Double*)xMalloc( Double, m_pcCfg->getDeltaQpRD() * 2 + 1 );
  m_piRdPicQp         = (Int*   )xMalloc( Int,    m_pcCfg->getDeltaQpRD() * 2 + 1 );
  m_pcRateCtrl        = pcEncTop->getRateCtrl();
}



Void
TEncSlice::setUpLambda(TComSlice* slice, const Double dLambda, Int iQP)
{
  // store lambda
  m_pcRdCost ->setLambda( dLambda );

  // for RDO
  // in RdCost there is only one lambda because the luma and chroma bits are not separated, instead we weight the distortion of chroma.
  Double dLambdas[MAX_NUM_COMPONENT] = { dLambda };
  for(UInt compIdx=1; compIdx<MAX_NUM_COMPONENT; compIdx++)
  {
    const ComponentID compID=ComponentID(compIdx);
    Int chromaQPOffset = slice->getPPS()->getQpOffset(compID) + slice->getSliceChromaQpDelta(compID);
    Int qpc=(iQP + chromaQPOffset < 0) ? iQP : getScaledChromaQP(iQP + chromaQPOffset, m_pcCfg->getChromaFormatIdc());
    Double tmpWeight = pow( 2.0, (iQP-qpc)/3.0 );  // takes into account of the chroma qp mapping and chroma qp Offset
    m_pcRdCost->setDistortionWeight(compID, tmpWeight);
    dLambdas[compIdx]=dLambda/tmpWeight;
  }

#if RDOQ_CHROMA_LAMBDA
// for RDOQ
  m_pcTrQuant->setLambdas( dLambdas );
#else
  m_pcTrQuant->setLambda( dLambda );
#endif

// For SAO
  slice   ->setLambdas( dLambdas );
}



/**
 - non-referenced frame marking
 - QP computation based on temporal structure
 - lambda computation based on QP
 - set temporal layer ID and the parameter sets
 .
 \param pcPic         picture class
 \param pocLast       POC of last picture
 \param pocCurr       current POC
 \param iNumPicRcvd   number of received pictures
 \param iGOPid        POC offset for hierarchical structure
 \param rpcSlice      slice header class
 \param pSPS          SPS associated with the slice
 \param pPPS          PPS associated with the slice
 \param isField       true for field coding
 */

Void TEncSlice::initEncSlice( TComPic* pcPic, Int pocLast, Int pocCurr, Int iNumPicRcvd, Int iGOPid, TComSlice*& rpcSlice, const TComSPS* pSPS, const TComPPS *pPPS, Bool isField )
{
  Double dQP;
  Double dLambda;

  rpcSlice = pcPic->getSlice(0);
  rpcSlice->setSliceBits(0);
  rpcSlice->setPic( pcPic );
  rpcSlice->initSlice();
  rpcSlice->setPicOutputFlag( true );
  rpcSlice->setPOC( pocCurr );

  // depth computation based on GOP size
  Int depth;
  {
    Int poc = rpcSlice->getPOC();
    if(isField)
    {
      poc = (poc/2) % (m_pcCfg->getGOPSize()/2);
    }
    else
    {
      poc = poc % m_pcCfg->getGOPSize();   
    }

    if ( poc == 0 )
    {
      depth = 0;
    }
    else
    {
      Int step = m_pcCfg->getGOPSize();
      depth    = 0;
      for( Int i=step>>1; i>=1; i>>=1 )
      {
        for ( Int j=i; j<m_pcCfg->getGOPSize(); j+=step )
        {
          if ( j == poc )
          {
            i=0;
            break;
          }
        }
        step >>= 1;
        depth++;
      }
    }

#if HARMONIZE_GOP_FIRST_FIELD_COUPLE
    if(poc != 0)
    {
#endif
      if (isField && ((rpcSlice->getPOC() % 2) == 1))
      {
        depth ++;
      }
#if HARMONIZE_GOP_FIRST_FIELD_COUPLE
    }
#endif
  }

  // slice type
  SliceType eSliceType;

  eSliceType=B_SLICE;
#if EFFICIENT_FIELD_IRAP
  if(!(isField && pocLast == 1))
  {
#endif // EFFICIENT_FIELD_IRAP
#if ALLOW_RECOVERY_POINT_AS_RAP
    if(m_pcCfg->getDecodingRefreshType() == 3)
    {
      eSliceType = (pocLast == 0 || pocCurr % m_pcCfg->getIntraPeriod() == 0             || m_pcGOPEncoder->getGOPSize() == 0) ? I_SLICE : eSliceType;
    }
    else
    {
#endif
      eSliceType = (pocLast == 0 || (pocCurr - (isField ? 1 : 0)) % m_pcCfg->getIntraPeriod() == 0 || m_pcGOPEncoder->getGOPSize() == 0) ? I_SLICE : eSliceType;
#if ALLOW_RECOVERY_POINT_AS_RAP
    }
#endif
#if EFFICIENT_FIELD_IRAP
  }
#endif

  rpcSlice->setSliceType    ( eSliceType );

  // ------------------------------------------------------------------------------------------------------------------
  // Non-referenced frame marking
  // ------------------------------------------------------------------------------------------------------------------

  if(pocLast == 0)
  {
    rpcSlice->setTemporalLayerNonReferenceFlag(false);
  }
  else
  {
    rpcSlice->setTemporalLayerNonReferenceFlag(!m_pcCfg->getGOPEntry(iGOPid).m_refPic);
  }
  rpcSlice->setReferenced(true);

  // ------------------------------------------------------------------------------------------------------------------
  // QP setting
  // ------------------------------------------------------------------------------------------------------------------

  dQP = m_pcCfg->getQP();
  if(eSliceType!=I_SLICE)
  {
    if (!(( m_pcCfg->getMaxDeltaQP() == 0 ) && (dQP == -rpcSlice->getSPS()->getQpBDOffset(CHANNEL_TYPE_LUMA) ) && (rpcSlice->getPPS()->getTransquantBypassEnableFlag())))
    {
      dQP += m_pcCfg->getGOPEntry(iGOPid).m_QPOffset;
    }
  }

  // modify QP
  Int* pdQPs = m_pcCfg->getdQPs();
  if ( pdQPs )
  {
    dQP += pdQPs[ rpcSlice->getPOC() ];
  }

  if (m_pcCfg->getCostMode()==COST_LOSSLESS_CODING)
  {
    dQP=LOSSLESS_AND_MIXED_LOSSLESS_RD_COST_TEST_QP;
    m_pcCfg->setDeltaQpRD(0);
  }

  // ------------------------------------------------------------------------------------------------------------------
  // Lambda computation
  // ------------------------------------------------------------------------------------------------------------------

  Int iQP;
  Double dOrigQP = dQP;

  // pre-compute lambda and QP values for all possible QP candidates
  for ( Int iDQpIdx = 0; iDQpIdx < 2 * m_pcCfg->getDeltaQpRD() + 1; iDQpIdx++ )
  {
    // compute QP value
    dQP = dOrigQP + ((iDQpIdx+1)>>1)*(iDQpIdx%2 ? -1 : 1);

    // compute lambda value
    Int    NumberBFrames = ( m_pcCfg->getGOPSize() - 1 );
    Int    SHIFT_QP = 12;

    Double dLambda_scale = 1.0 - Clip3( 0.0, 0.5, 0.05*(Double)(isField ? NumberBFrames/2 : NumberBFrames) );

#if FULL_NBIT
    Int    bitdepth_luma_qp_scale = 6 * (g_bitDepth[CHANNEL_TYPE_LUMA] - 8);
#else
    Int    bitdepth_luma_qp_scale = 0;
#endif
    Double qp_temp = (Double) dQP + bitdepth_luma_qp_scale - SHIFT_QP;
#if FULL_NBIT
    Double qp_temp_orig = (Double) dQP - SHIFT_QP;
#endif
    // Case #1: I or P-slices (key-frame)
    Double dQPFactor = m_pcCfg->getGOPEntry(iGOPid).m_QPFactor;
    if ( eSliceType==I_SLICE )
    {
      dQPFactor=0.57*dLambda_scale;
    }
    dLambda = dQPFactor*pow( 2.0, qp_temp/3.0 );

    if ( depth>0 )
    {
#if FULL_NBIT
        dLambda *= Clip3( 2.00, 4.00, (qp_temp_orig / 6.0) ); // (j == B_SLICE && p_cur_frm->layer != 0 )
#else
        dLambda *= Clip3( 2.00, 4.00, (qp_temp / 6.0) ); // (j == B_SLICE && p_cur_frm->layer != 0 )
#endif
    }

    // if hadamard is used in ME process
    if ( !m_pcCfg->getUseHADME() && rpcSlice->getSliceType( ) != I_SLICE )
    {
      dLambda *= 0.95;
    }

    iQP = max( -pSPS->getQpBDOffset(CHANNEL_TYPE_LUMA), min( MAX_QP, (Int) floor( dQP + 0.5 ) ) );

    m_pdRdPicLambda[iDQpIdx] = dLambda;
    m_pdRdPicQp    [iDQpIdx] = dQP;
    m_piRdPicQp    [iDQpIdx] = iQP;
  }

  // obtain dQP = 0 case
  dLambda = m_pdRdPicLambda[0];
  dQP     = m_pdRdPicQp    [0];
  iQP     = m_piRdPicQp    [0];

  if( rpcSlice->getSliceType( ) != I_SLICE )
  {
    dLambda *= m_pcCfg->getLambdaModifier( m_pcCfg->getGOPEntry(iGOPid).m_temporalId );
  }

  setUpLambda(rpcSlice, dLambda, iQP);

#if HB_LAMBDA_FOR_LDC
  // restore original slice type

#if EFFICIENT_FIELD_IRAP
  if(!(isField && pocLast == 1))
  {
#endif // EFFICIENT_FIELD_IRAP
#if ALLOW_RECOVERY_POINT_AS_RAP
    if(m_pcCfg->getDecodingRefreshType() == 3)
    {
      eSliceType = (pocLast == 0 || (pocCurr)                     % m_pcCfg->getIntraPeriod() == 0 || m_pcGOPEncoder->getGOPSize() == 0) ? I_SLICE : eSliceType;
    }
    else
    {
#endif
      eSliceType = (pocLast == 0 || (pocCurr - (isField ? 1 : 0)) % m_pcCfg->getIntraPeriod() == 0 || m_pcGOPEncoder->getGOPSize() == 0) ? I_SLICE : eSliceType;
#if ALLOW_RECOVERY_POINT_AS_RAP
    }
#endif
#if EFFICIENT_FIELD_IRAP
  }
#endif // EFFICIENT_FIELD_IRAP

  rpcSlice->setSliceType        ( eSliceType );
#endif

  if (m_pcCfg->getUseRecalculateQPAccordingToLambda())
  {
    dQP = xGetQPValueAccordingToLambda( dLambda );
    iQP = max( -pSPS->getQpBDOffset(CHANNEL_TYPE_LUMA), min( MAX_QP, (Int) floor( dQP + 0.5 ) ) );
  }

  rpcSlice->setSliceQp           ( iQP );
#if ADAPTIVE_QP_SELECTION
  rpcSlice->setSliceQpBase       ( iQP );
#endif
  rpcSlice->setSliceQpDelta      ( 0 );
  rpcSlice->setSliceChromaQpDelta( COMPONENT_Cb, 0 );
  rpcSlice->setSliceChromaQpDelta( COMPONENT_Cr, 0 );
  rpcSlice->setUseChromaQpAdj( pPPS->getChromaQpAdjTableSize() > 0 );
  rpcSlice->setNumRefIdx(REF_PIC_LIST_0,m_pcCfg->getGOPEntry(iGOPid).m_numRefPicsActive);
  rpcSlice->setNumRefIdx(REF_PIC_LIST_1,m_pcCfg->getGOPEntry(iGOPid).m_numRefPicsActive);

  if ( m_pcCfg->getDeblockingFilterMetric() )
  {
    rpcSlice->setDeblockingFilterOverrideFlag(true);
    rpcSlice->setDeblockingFilterDisable(false);
    rpcSlice->setDeblockingFilterBetaOffsetDiv2( 0 );
    rpcSlice->setDeblockingFilterTcOffsetDiv2( 0 );
  }
  else if (rpcSlice->getPPS()->getDeblockingFilterControlPresentFlag())
  {
    rpcSlice->setDeblockingFilterOverrideFlag( rpcSlice->getPPS()->getDeblockingFilterOverrideEnabledFlag() );
    rpcSlice->setDeblockingFilterDisable( rpcSlice->getPPS()->getPicDisableDeblockingFilterFlag() );
    if ( !rpcSlice->getDeblockingFilterDisable())
    {
      if ( rpcSlice->getDeblockingFilterOverrideFlag() && eSliceType!=I_SLICE)
      {
        rpcSlice->setDeblockingFilterBetaOffsetDiv2( m_pcCfg->getGOPEntry(iGOPid).m_betaOffsetDiv2 + m_pcCfg->getLoopFilterBetaOffset()  );
        rpcSlice->setDeblockingFilterTcOffsetDiv2( m_pcCfg->getGOPEntry(iGOPid).m_tcOffsetDiv2 + m_pcCfg->getLoopFilterTcOffset() );
      }
      else
      {
        rpcSlice->setDeblockingFilterBetaOffsetDiv2( m_pcCfg->getLoopFilterBetaOffset() );
        rpcSlice->setDeblockingFilterTcOffsetDiv2( m_pcCfg->getLoopFilterTcOffset() );
      }
    }
  }
  else
  {
    rpcSlice->setDeblockingFilterOverrideFlag( false );
    rpcSlice->setDeblockingFilterDisable( false );
    rpcSlice->setDeblockingFilterBetaOffsetDiv2( 0 );
    rpcSlice->setDeblockingFilterTcOffsetDiv2( 0 );
  }

  rpcSlice->setDepth            ( depth );

  pcPic->setTLayer( m_pcCfg->getGOPEntry(iGOPid).m_temporalId );
  if(eSliceType==I_SLICE)
  {
    pcPic->setTLayer(0);
  }
  rpcSlice->setTLayer( pcPic->getTLayer() );

  assert( m_apcPicYuvPred );
  assert( m_apcPicYuvResi );

  pcPic->setPicYuvPred( m_apcPicYuvPred );
  pcPic->setPicYuvResi( m_apcPicYuvResi );
  rpcSlice->setSliceMode            ( m_pcCfg->getSliceMode()            );
  rpcSlice->setSliceArgument        ( m_pcCfg->getSliceArgument()        );
  rpcSlice->setSliceSegmentMode     ( m_pcCfg->getSliceSegmentMode()     );
  rpcSlice->setSliceSegmentArgument ( m_pcCfg->getSliceSegmentArgument() );
  rpcSlice->setMaxNumMergeCand        ( m_pcCfg->getMaxNumMergeCand()        );
}

Void TEncSlice::resetQP( TComPic* pic, Int sliceQP, Double lambda )
{
  TComSlice* slice = pic->getSlice(0);

  // store lambda
  slice->setSliceQp( sliceQP );
#if ADAPTIVE_QP_SELECTION
  slice->setSliceQpBase ( sliceQP );
#endif
  setUpLambda(slice, lambda, sliceQP);
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncSlice::setSearchRange( TComSlice* pcSlice )
{
  Int iCurrPOC = pcSlice->getPOC();
  Int iRefPOC;
  Int iGOPSize = m_pcCfg->getGOPSize();
  Int iOffset = (iGOPSize >> 1);
  Int iMaxSR = m_pcCfg->getSearchRange();
  Int iNumPredDir = pcSlice->isInterP() ? 1 : 2;

  for (Int iDir = 0; iDir <= iNumPredDir; iDir++)
  {
    //RefPicList e = (RefPicList)iDir;
    RefPicList  e = ( iDir ? REF_PIC_LIST_1 : REF_PIC_LIST_0 );
    for (Int iRefIdx = 0; iRefIdx < pcSlice->getNumRefIdx(e); iRefIdx++)
    {
      iRefPOC = pcSlice->getRefPic(e, iRefIdx)->getPOC();
      Int iNewSR = Clip3(8, iMaxSR, (iMaxSR*ADAPT_SR_SCALE*abs(iCurrPOC - iRefPOC)+iOffset)/iGOPSize);
      m_pcPredSearch->setAdaptiveSearchRange(iDir, iRefIdx, iNewSR);
    }
  }
}

#if GEN_OUTLIER
Void TEncSlice::getOutlierWithDCT(TComPic* rpcPic)
{
    //for(Int chan=0; chan < rpcPic->getNumberValidComponents(); chan++)
    for(Int chan=0; chan < 1; chan++) // only for Y component
    {
        const ComponentID ch=ComponentID(chan);
        

   TComPicYuv* data = rpcPic->getPicYuvOrg();  // Using original data
	//	TComPicYuv* data = rpcPic->getPicYuvRec();   // Using reconstructed Data
	//	TComPicYuv* data = rpcPic->getPicYuvResi();   // Using resicual Data
	
        TComPicYuv* dataDst = rpcPic->getPicYuvOutlier();
		TComPicYuv* dataOBF = rpcPic->getOBF();
        
        Pel* pData = data->getAddr(ch);
        Pel* pDst = dataDst->getAddr(ch);   // pDst Pixel pointer 
		Pel* pOBF = dataOBF->getAddr(ch);

        
        UInt uiStrideDst = dataDst->getStride(ch);
        UInt uiStrideSrc = data->getStride(ch);
		UInt uiStrideOBF = dataOBF->getStride(ch);
		//cout << "uiStrideOBF:" << uiStrideOBF << endl;
		//cout << "uiStrideDst:" << uiStrideDst << endl;
		// cout << dataOBF->getWidth(COMPONENT_Y);
        const UInt uiFrameWidth = data->getWidth(ch);
        const UInt uiFrameHeight = data->getHeight(ch);
        
        //set start frequency for TCM
        Int StartFreq = 1;
        
#if DCT_SIZE_IS_FOUR
         Int DctSize = 4;
        Double DctScaling = 32.0/4.0;
#else
        Int DctSize = 8;
        Double DctScaling = 16.0;
#endif
        const Int maxTrDynamicRange = g_maxTrDynamicRange[toChannelType(ch)];
        
        TCoeff block[ MAX_TU_SIZE * MAX_TU_SIZE ];
        TCoeff coeff[ MAX_TU_SIZE * MAX_TU_SIZE ];
        
        Int TRANSFORM_MATRIX_SHIFT = g_transformMatrixShift[TRANSFORM_FORWARD];
        Int bitDepth = g_bitDepth[toChannelType(ch)];
        
        Int shift_1st = ((g_aucConvertToBit[DctSize] + 2) +  bitDepth + TRANSFORM_MATRIX_SHIFT) - maxTrDynamicRange;
        Int shift_2nd = (g_aucConvertToBit[DctSize] + 2) + TRANSFORM_MATRIX_SHIFT;
        
        TCoeff tmp[ MAX_TU_SIZE * MAX_TU_SIZE ];
        Int FrequencySize = DctSize*DctSize;
        Int BlockPerRow = uiFrameWidth/DctSize;
        Int BlockPerCol = uiFrameHeight/DctSize;
        
        Int** CoeffFrequency    = new Int* [FrequencySize];   //HM dct/scaling factor
        Int** CoeffFrequencyOrg = new Int* [FrequencySize];   //HM dct, which may be different from actual dct
        
        UInt NumCoefInOneFreq = BlockPerCol*BlockPerRow;
        for (Int x = 0 ; x<FrequencySize; x++)
        {
            CoeffFrequency[x]    = new Int[NumCoefInOneFreq];
            CoeffFrequencyOrg[x] = new Int[NumCoefInOneFreq];
        }

        //perform DCT for each 4x4 / 8x8 blcoks
        for (Int Height = 0; Height < BlockPerCol; Height++)
        {
            for (Int Width = 0; Width < BlockPerRow; Width++)
            {
                //copy data to block for DCT
                for (Int y = 0; y < DctSize; y++)
                {
                    for (Int x = 0 ; x < DctSize; x++)
                    {
                        block[(y * DctSize) + x] = pData[(y+Height*DctSize)*uiStrideSrc + Width*DctSize + x];
                    }
                }
                
                partialButterfly( block, tmp, shift_1st, DctSize );
                partialButterfly( tmp, coeff, shift_2nd, DctSize );

                for (Int x = 0; x<FrequencySize; x++)
                {
                    CoeffFrequency[x][Width+Height*BlockPerRow]    = coeff[x]/DctScaling;
                    CoeffFrequencyOrg[x][Width+Height*BlockPerRow] = coeff[x];
                }
            }
        } //DCT done

        //DC
        if (StartFreq == 0)
        {
            Double MeanDC = 0.0; Double MeanDCOrg = 0.0;
            for (Int x = 0; x<NumCoefInOneFreq ; x++)
            {
                MeanDC = MeanDC + CoeffFrequency[0][x];
                MeanDCOrg = MeanDCOrg + CoeffFrequencyOrg[0][x];
            }
            MeanDC = MeanDC/(Double)(NumCoefInOneFreq );
            MeanDCOrg = MeanDCOrg/(Double)(NumCoefInOneFreq);
            for (Int x = 0; x<NumCoefInOneFreq; x++)
            {
                CoeffFrequency[0][x] = CoeffFrequency[0][x] - MeanDC;
			    CoeffFrequencyOrg[0][x] = CoeffFrequencyOrg[0][x] - MeanDCOrg;
            }
        }
        else
        {
          memset(CoeffFrequencyOrg[0], 0, sizeof(Int)*NumCoefInOneFreq );
          memset(CoeffFrequency[0]   , 0, sizeof(Int)*NumCoefInOneFreq );
        }
        
        //perform TCM and get outlier Yc[x] for each frequency
        Int peak; double prob, lambda;  //YS add
        double* Yc= new double[FrequencySize];
        
#if DCT_DEBUG
        cout<<"\n(Yc, peak): ";
#endif
        for (Int x = StartFreq; x<FrequencySize; x++)
        {
            TCMprocessOneSequence(CoeffFrequency[x], NumCoefInOneFreq, &peak, &prob, &lambda, &Yc[x] );
#if DCT_DEBUG
            cout<<" ( "<<Yc[x]<<" , "<<peak<<" ) ";
#endif
        }
        
        for (Int x = StartFreq; x<FrequencySize; x++)
        {
            for (Int i = 0; i<NumCoefInOneFreq; i++)
            {
                if (CoeffFrequencyOrg[x][i]<Yc[x]*DctScaling && CoeffFrequencyOrg[x][i]>-Yc[x]*DctScaling)
                {
                CoeffFrequencyOrg[x][i] = 0;
                }
            }
        }
		delete[] Yc;
		
		
		
		//  YS add   Count the Outlier Block Flag( OBF )

		for (Int Height = 0; Height < BlockPerCol; Height++)
		{
			for (Int Width = 0; Width < BlockPerRow; Width++)
			{
				pOBF[Width + Height*uiStrideOBF] = 0;  
				for (Int x = StartFreq; x < FrequencySize; x++)
				{
					if (CoeffFrequencyOrg[x][Width + Height*BlockPerRow] != 0)
					{
#if BINARIZE_OBF
						pOBF[Width + Height*uiStrideOBF] = 1;
#else
						pOBF[Width + Height*uiStrideOBF] ++;
#endif
						
						//break;
					}
				}
				//cout << pOBF[Width + Height*uiStrideOBF]<<' ';
			}
		}







        //Inverse DCT, Outlier in image domain, DC is set as 0
        TRANSFORM_MATRIX_SHIFT = g_transformMatrixShift[TRANSFORM_INVERSE];
        shift_1st = TRANSFORM_MATRIX_SHIFT + 1; //1 has been added to shift_1st at the expense of shift_2nd
        shift_2nd = (TRANSFORM_MATRIX_SHIFT + maxTrDynamicRange - 1) - bitDepth;
        const TCoeff clipMinimum = -(1 << maxTrDynamicRange);
        const TCoeff clipMaximum =  (1 << maxTrDynamicRange) - 1;
        
        for (Int Height = 0; Height < BlockPerCol; Height++)
        {
            for (Int Width = 0; Width < BlockPerRow; Width++)
            {
#if DCT_DEBUG
                cout << "\nOutlierCoeff: ";
#endif
               memset(coeff, 0, MAX_TU_SIZE * MAX_TU_SIZE*sizeof(TCoeff));
                for (Int y = 0; y<DctSize; y++)
                {
#if DCT_DEBUG
                    cout << "\n";
#endif
                    for (Int x = 0; x<DctSize; x++)
                    {
                        if (y*DctSize+x != 0)
                        {
                            coeff[y*DctSize+x] = CoeffFrequencyOrg[y*DctSize+x][Height*BlockPerRow+Width];
#if DCT_DEBUG
                            cout << coeff[y*DctSize+x]<<" , ";
#endif
                        }
                    }
                }
#if DCT_DEBUG
                cout << "\nInvDCT: ";
#endif
                
                partialButterflyInverse( coeff, tmp, shift_1st, DctSize, clipMinimum, clipMaximum);
                partialButterflyInverse( tmp, block, shift_2nd, DctSize, std::numeric_limits<Pel>::min(), std::numeric_limits<Pel>::max());
                for (Int y = 0; y<DctSize; y++)
                {
#if DCT_DEBUG
                    cout << "\n";
#endif
                    for (Int x = 0; x<DctSize; x++)
                    {
                    // YS add  

						
#if REVERSETRANSFORM_OUTLIER
					pDst[(y + Height*DctSize)*uiStrideDst + x + Width*DctSize] = block[y*DctSize + x];
                      if ( pDst[(y+Height*DctSize)*uiStrideDst+x+Width*DctSize] < 0)  //positive
                            pDst[(y+Height*DctSize)*uiStrideDst+x+Width*DctSize] = -pDst[(y+Height*DctSize)*uiStrideDst+x+Width*DctSize];
                        
                        pDst[(y+Height*DctSize)*uiStrideDst+x+Width*DctSize] *= 3;
                        if ( pDst[(y+Height*DctSize)*uiStrideDst+x+Width*DctSize] > 255)  // Max 255
                            pDst[(y+Height*DctSize)*uiStrideDst+x+Width*DctSize] =255;  //
#else
			pDst[(y + Height*DctSize)*uiStrideDst + x + Width*DctSize] = coeff[y*DctSize + x];	// coeff
			if (pDst[(y + Height*DctSize)*uiStrideDst + x + Width*DctSize] < 0)                                       //positive
			{
				pDst[(y + Height*DctSize)*uiStrideDst + x + Width*DctSize] = - pDst[(y + Height*DctSize)*uiStrideDst + x + Width*DctSize];
			}

			pDst[(y + Height*DctSize)*uiStrideDst + x + Width*DctSize] = (Pel)pDst[(y + Height*DctSize)*uiStrideDst + x + Width*DctSize]/100;
		//	if ( pDst[(y+Height*DctSize)*uiStrideDst+x+Width*DctSize] > 255)  // Max 255
			//	pDst[(y+Height*DctSize)*uiStrideDst+x+Width*DctSize] =255;  //

#endif						
						//pDst[(y + Height*DctSize)*uiStrideDst + x + Width*DctSize] = block[y*DctSize + x];
					
#if BINARIZE_OUTLIER
						if (pDst[(y + Height*DctSize)*uiStrideDst + x + Width*DctSize] != 0){
							pDst[(y + Height*DctSize)*uiStrideDst + x + Width*DctSize] = 1;
						}
#endif

#if DCT_DEBUG
                        cout<<pDst[(y+Height*DctSize)*uiStrideDst+x+Width*DctSize]<<" , ";
#endif
                    }
                }
                
            }
        }


		 
#if OUT_OUTLIER  // write files
        unsigned char *bufOrgTem = new unsigned char[uiFrameWidth];
        for (UInt height = 0 ; height< uiFrameHeight; height++)
        {
            for (UInt width = 0; width < uiFrameWidth; width++)
            {

			
                bufOrgTem[width] = pDst[width];
            }
            OutlierYuvFile.write(reinterpret_cast<char*>(bufOrgTem), uiFrameWidth);
            pDst += uiStrideDst;
        }
        delete[] bufOrgTem;

		unsigned char *bufOBFTem = new unsigned char[uiFrameWidth/4];
		for (UInt height = 0; height< uiFrameHeight/4; height++)
		{
			for (UInt width = 0; width < uiFrameWidth/4; width++)
			{


				bufOBFTem[width] = pOBF[width];
			}
			OBFFile.write(reinterpret_cast<char*>(bufOBFTem), uiFrameWidth / 4);
			pOBF += uiStrideOBF;
		}
		delete[] bufOBFTem;
#endif
        for (Int x=0 ; x<FrequencySize; x++)
        {
            delete[] CoeffFrequency[x];
            delete[] CoeffFrequencyOrg[x];
        }
        delete[] CoeffFrequency;
        delete[] CoeffFrequencyOrg;
    }
}
#endif


/**
 Multi-loop slice encoding for different slice QP

 \param pcPic    picture class
 */
Void TEncSlice::precompressSlice( TComPic* pcPic )
{
  // if deltaQP RD is not used, simply return
  if ( m_pcCfg->getDeltaQpRD() == 0 )
  {
    return;
  }

  if ( m_pcCfg->getUseRateCtrl() )
  {
    printf( "\nMultiple QP optimization is not allowed when rate control is enabled." );
    assert(0);
  }

  TComSlice* pcSlice        = pcPic->getSlice(getSliceIdx());
  Double     dPicRdCostBest = MAX_DOUBLE;
  UInt       uiQpIdxBest = 0;

  Double dFrameLambda;
#if FULL_NBIT
  Int    SHIFT_QP = 12 + 6 * (g_bitDepth[CHANNEL_TYPE_LUMA] - 8);
#else
  Int    SHIFT_QP = 12;
#endif

  // set frame lambda
  if (m_pcCfg->getGOPSize() > 1)
  {
    dFrameLambda = 0.68 * pow (2, (m_piRdPicQp[0]  - SHIFT_QP) / 3.0) * (pcSlice->isInterB()? 2 : 1);
  }
  else
  {
    dFrameLambda = 0.68 * pow (2, (m_piRdPicQp[0] - SHIFT_QP) / 3.0);
  }
  m_pcRdCost      ->setFrameLambda(dFrameLambda);

  const UInt initialSliceQp=pcSlice->getSliceQp();
  // for each QP candidate
  for ( UInt uiQpIdx = 0; uiQpIdx < 2 * m_pcCfg->getDeltaQpRD() + 1; uiQpIdx++ )
  {
    pcSlice       ->setSliceQp             ( m_piRdPicQp    [uiQpIdx] );
#if ADAPTIVE_QP_SELECTION
    pcSlice       ->setSliceQpBase         ( m_piRdPicQp    [uiQpIdx] );
#endif
    setUpLambda(pcSlice, m_pdRdPicLambda[uiQpIdx], m_piRdPicQp    [uiQpIdx]);

    // try compress
    compressSlice   ( pcPic );

    Double dPicRdCost;
    UInt64 uiPicDist        = m_uiPicDist;
    // TODO: will this work if multiple slices are being used? There may not be any reconstruction data yet.
    //       Will this also be ideal if a byte-restriction is placed on the slice?
    //         - what if the last CTU was sometimes included, sometimes not, and that had all the distortion?
    m_pcGOPEncoder->preLoopFilterPicAll( pcPic, uiPicDist );

    // compute RD cost and choose the best
    dPicRdCost = m_pcRdCost->calcRdCost64( m_uiPicTotalBits, uiPicDist, true, DF_SSE_FRAME);

    if ( dPicRdCost < dPicRdCostBest )
    {
      uiQpIdxBest    = uiQpIdx;
      dPicRdCostBest = dPicRdCost;
    }
  }

  if (pcSlice->getDependentSliceSegmentFlag() && initialSliceQp!=m_piRdPicQp[uiQpIdxBest] )
  {
    // TODO: this won't work with dependent slices: they do not have their own QP.
    fprintf(stderr,"ERROR - attempt to change QP for a dependent slice-segment, having already coded the slice\n");
    assert(pcSlice->getDependentSliceSegmentFlag()==false || initialSliceQp==m_piRdPicQp[uiQpIdxBest]);
  }
  // set best values
  pcSlice       ->setSliceQp             ( m_piRdPicQp    [uiQpIdxBest] );
#if ADAPTIVE_QP_SELECTION
  pcSlice       ->setSliceQpBase         ( m_piRdPicQp    [uiQpIdxBest] );
#endif
  setUpLambda(pcSlice, m_pdRdPicLambda[uiQpIdxBest], m_piRdPicQp    [uiQpIdxBest]);
}

Void TEncSlice::calCostSliceI(TComPic* pcPic)
{
  UInt    ctuRsAddr;
  UInt    startCtuTsAddr;
  UInt    boundingCtuTsAddr;
  Int     iSumHad, shift = g_bitDepth[CHANNEL_TYPE_LUMA]-8, offset = (shift>0)?(1<<(shift-1)):0;;
  Double  iSumHadSlice = 0;

  pcPic->getSlice(getSliceIdx())->setSliceSegmentBits(0);
  TComSlice* pcSlice            = pcPic->getSlice(getSliceIdx());
  xDetermineStartAndBoundingCtuTsAddr ( startCtuTsAddr, boundingCtuTsAddr, pcPic, false );

  UInt ctuTsAddr;
  ctuRsAddr = pcPic->getPicSym()->getCtuTsToRsAddrMap( startCtuTsAddr);
  for( ctuTsAddr = startCtuTsAddr; ctuTsAddr < boundingCtuTsAddr; ctuRsAddr = pcPic->getPicSym()->getCtuTsToRsAddrMap(++ctuTsAddr) )
  {
    // initialize CU encoder
    TComDataCU* pCtu = pcPic->getCtu( ctuRsAddr );
    pCtu->initCtu( pcPic, ctuRsAddr );

    Int height  = min( pcSlice->getSPS()->getMaxCUHeight(),pcSlice->getSPS()->getPicHeightInLumaSamples() - ctuRsAddr / pcPic->getFrameWidthInCtus() * pcSlice->getSPS()->getMaxCUHeight() );
    Int width   = min( pcSlice->getSPS()->getMaxCUWidth(),pcSlice->getSPS()->getPicWidthInLumaSamples() - ctuRsAddr % pcPic->getFrameWidthInCtus() * pcSlice->getSPS()->getMaxCUWidth() );

    iSumHad = m_pcCuEncoder->updateCtuDataISlice(pCtu, width, height);

    (m_pcRateCtrl->getRCPic()->getLCU(ctuRsAddr)).m_costIntra=(iSumHad+offset)>>shift;
    iSumHadSlice += (m_pcRateCtrl->getRCPic()->getLCU(ctuRsAddr)).m_costIntra;

  }
  m_pcRateCtrl->getRCPic()->setTotalIntraCost(iSumHadSlice);
}

/** \param pcPic   picture class
 */
Void TEncSlice::compressSlice( TComPic* pcPic )
{
  UInt   startCtuTsAddr;
  UInt   boundingCtuTsAddr;
  TComSlice* pcSlice            = pcPic->getSlice(getSliceIdx());
  pcSlice->setSliceSegmentBits(0);
  xDetermineStartAndBoundingCtuTsAddr ( startCtuTsAddr, boundingCtuTsAddr, pcPic, false );

  // initialize cost values - these are used by precompressSlice (they should be parameters).
  m_uiPicTotalBits  = 0;
  m_dPicRdCost      = 0; // NOTE: This is a write-only variable!
  m_uiPicDist       = 0;

  m_pcEntropyCoder->setEntropyCoder   ( m_pppcRDSbacCoder[0][CI_CURR_BEST], pcSlice );
  m_pcEntropyCoder->resetEntropy      ();

  TEncBinCABAC* pRDSbacCoder = (TEncBinCABAC *) m_pppcRDSbacCoder[0][CI_CURR_BEST]->getEncBinIf();
  pRDSbacCoder->setBinCountingEnableFlag( false );
  pRDSbacCoder->setBinsCoded( 0 );

  TComBitCounter  tempBitCounter;
  const UInt      frameWidthInCtus = pcPic->getPicSym()->getFrameWidthInCtus();

  //------------------------------------------------------------------------------
  //  Weighted Prediction parameters estimation.
  //------------------------------------------------------------------------------
  // calculate AC/DC values for current picture
  if( pcSlice->getPPS()->getUseWP() || pcSlice->getPPS()->getWPBiPred() )
  {
    xCalcACDCParamSlice(pcSlice);
  }

  const Bool bWp_explicit = (pcSlice->getSliceType()==P_SLICE && pcSlice->getPPS()->getUseWP()) || (pcSlice->getSliceType()==B_SLICE && pcSlice->getPPS()->getWPBiPred());

  if ( bWp_explicit )
  {
    //------------------------------------------------------------------------------
    //  Weighted Prediction implemented at Slice level. SliceMode=2 is not supported yet.
    //------------------------------------------------------------------------------
    if ( pcSlice->getSliceMode()==FIXED_NUMBER_OF_BYTES || pcSlice->getSliceSegmentMode()==FIXED_NUMBER_OF_BYTES )
    {
      printf("Weighted Prediction is not supported with slice mode determined by max number of bins.\n"); exit(0);
    }

    xEstimateWPParamSlice( pcSlice );
    pcSlice->initWpScaling(pcSlice->getSPS());

    // check WP on/off
    xCheckWPEnable( pcSlice );
  }

#if ADAPTIVE_QP_SELECTION
  if( m_pcCfg->getUseAdaptQpSelect() && !(pcSlice->getDependentSliceSegmentFlag()))
  {
    // TODO: this won't work with dependent slices: they do not have their own QP. Check fix to mask clause execution with && !(pcSlice->getDependentSliceSegmentFlag())
    m_pcTrQuant->clearSliceARLCnt();
    if(pcSlice->getSliceType()!=I_SLICE)
    {
      Int qpBase = pcSlice->getSliceQpBase();
      pcSlice->setSliceQp(qpBase + m_pcTrQuant->getQpDelta(qpBase));
    }
  }
#endif



  // Adjust initial state if this is the start of a dependent slice.
  {
    const UInt      ctuRsAddr               = pcPic->getPicSym()->getCtuTsToRsAddrMap( startCtuTsAddr);
    const UInt      currentTileIdx          = pcPic->getPicSym()->getTileIdxMap(ctuRsAddr);
    const TComTile *pCurrentTile            = pcPic->getPicSym()->getTComTile(currentTileIdx);
    const UInt      firstCtuRsAddrOfTile    = pCurrentTile->getFirstCtuRsAddr();
    if( pcSlice->getDependentSliceSegmentFlag() && ctuRsAddr != firstCtuRsAddrOfTile )
    {
      // This will only occur if dependent slice-segments (m_entropyCodingSyncContextState=true) are being used.
      if( pCurrentTile->getTileWidthInCtus() >= 2 || !m_pcCfg->getWaveFrontsynchro() )
      {
        m_pppcRDSbacCoder[0][CI_CURR_BEST]->loadContexts( &m_lastSliceSegmentEndContextState );
      }
    }
  }

  // for every CTU in the slice segment (may terminate sooner if there is a byte limit on the slice-segment)

  for( UInt ctuTsAddr = startCtuTsAddr; ctuTsAddr < boundingCtuTsAddr; ++ctuTsAddr )
  {
    const UInt ctuRsAddr = pcPic->getPicSym()->getCtuTsToRsAddrMap(ctuTsAddr);
    // initialize CTU encoder
    TComDataCU* pCtu = pcPic->getCtu( ctuRsAddr );
    pCtu->initCtu( pcPic, ctuRsAddr );

    // update CABAC state
    const UInt firstCtuRsAddrOfTile = pcPic->getPicSym()->getTComTile(pcPic->getPicSym()->getTileIdxMap(ctuRsAddr))->getFirstCtuRsAddr();
    const UInt tileXPosInCtus = firstCtuRsAddrOfTile % frameWidthInCtus;
    const UInt ctuXPosInCtus  = ctuRsAddr % frameWidthInCtus;
    
    if (ctuRsAddr == firstCtuRsAddrOfTile)
    {
      m_pppcRDSbacCoder[0][CI_CURR_BEST]->resetEntropy();
    }
    else if ( ctuXPosInCtus == tileXPosInCtus && m_pcCfg->getWaveFrontsynchro())
    {
      // reset and then update contexts to the state at the end of the top-right CTU (if within current slice and tile).
      m_pppcRDSbacCoder[0][CI_CURR_BEST]->resetEntropy();
      // Sync if the Top-Right is available.
      TComDataCU *pCtuUp = pCtu->getCtuAbove();
      if ( pCtuUp && ((ctuRsAddr%frameWidthInCtus+1) < frameWidthInCtus)  )
      {
        TComDataCU *pCtuTR = pcPic->getCtu( ctuRsAddr - frameWidthInCtus + 1 );
        if ( pCtu->CUIsFromSameSliceAndTile(pCtuTR) )
        {
          // Top-Right is available, we use it.
          m_pppcRDSbacCoder[0][CI_CURR_BEST]->loadContexts( &m_entropyCodingSyncContextState );
        }
      }
    }

    // set go-on entropy coder (used for all trial encodings - the cu encoder and encoder search also have a copy of the same pointer)
    m_pcEntropyCoder->setEntropyCoder ( m_pcRDGoOnSbacCoder, pcSlice );
    m_pcEntropyCoder->setBitstream( &tempBitCounter );
    tempBitCounter.resetBits();
    m_pcRDGoOnSbacCoder->load( m_pppcRDSbacCoder[0][CI_CURR_BEST] ); // this copy is not strictly necessary here, but indicates that the GoOnSbacCoder
                                                                     // is reset to a known state before every decision process.

    ((TEncBinCABAC*)m_pcRDGoOnSbacCoder->getEncBinIf())->setBinCountingEnableFlag(true);

    Double oldLambda = m_pcRdCost->getLambda();
    if ( m_pcCfg->getUseRateCtrl() )
    {
      Int estQP        = pcSlice->getSliceQp();
      Double estLambda = -1.0;
      Double bpp       = -1.0;

      if ( ( pcPic->getSlice( 0 )->getSliceType() == I_SLICE && m_pcCfg->getForceIntraQP() ) || !m_pcCfg->getLCULevelRC() )
      {
        estQP = pcSlice->getSliceQp();
      }
      else
      {
        bpp = m_pcRateCtrl->getRCPic()->getLCUTargetBpp(pcSlice->getSliceType());
        if ( pcPic->getSlice( 0 )->getSliceType() == I_SLICE)
        {
          estLambda = m_pcRateCtrl->getRCPic()->getLCUEstLambdaAndQP(bpp, pcSlice->getSliceQp(), &estQP);
        }
        else
        {
          estLambda = m_pcRateCtrl->getRCPic()->getLCUEstLambda( bpp );
          estQP     = m_pcRateCtrl->getRCPic()->getLCUEstQP    ( estLambda, pcSlice->getSliceQp() );
        }

        estQP     = Clip3( -pcSlice->getSPS()->getQpBDOffset(CHANNEL_TYPE_LUMA), MAX_QP, estQP );

        m_pcRdCost->setLambda(estLambda);

#if RDOQ_CHROMA_LAMBDA
        // set lambda for RDOQ
        const Double chromaLambda = estLambda / m_pcRdCost->getChromaWeight();
        const Double lambdaArray[MAX_NUM_COMPONENT] = { estLambda, chromaLambda, chromaLambda };
        m_pcTrQuant->setLambdas( lambdaArray );
#else
        m_pcTrQuant->setLambda( estLambda );
#endif
      }

      m_pcRateCtrl->setRCQP( estQP );
#if ADAPTIVE_QP_SELECTION
      pCtu->getSlice()->setSliceQpBase( estQP );
#endif
    }

    // run CTU trial encoder

    m_pcCuEncoder->compressCtu( pCtu );


    // All CTU decisions have now been made. Restore entropy coder to an initial stage, ready to make a true encode,
    // which will result in the state of the contexts being correct. It will also count up the number of bits coded,
    // which is used if there is a limit of the number of bytes per slice-segment.

    m_pcEntropyCoder->setEntropyCoder ( m_pppcRDSbacCoder[0][CI_CURR_BEST], pcSlice );
    m_pcEntropyCoder->setBitstream( &tempBitCounter );
    pRDSbacCoder->setBinCountingEnableFlag( true );
    m_pppcRDSbacCoder[0][CI_CURR_BEST]->resetBits();
    pRDSbacCoder->setBinsCoded( 0 );

    // encode CTU and calculate the true bit counters.
    m_pcCuEncoder->encodeCtu( pCtu );


    pRDSbacCoder->setBinCountingEnableFlag( false );

    const Int numberOfWrittenBits = m_pcEntropyCoder->getNumberOfWrittenBits();

    // Calculate if this CTU puts us over slice bit size.
    // cannot terminate if current slice/slice-segment would be 0 Ctu in size,
    const UInt validEndOfSliceCtuTsAddr = ctuTsAddr + (ctuTsAddr == startCtuTsAddr ? 1 : 0);
    // Set slice end parameter
    if(pcSlice->getSliceMode()==FIXED_NUMBER_OF_BYTES && pcSlice->getSliceBits()+numberOfWrittenBits > (pcSlice->getSliceArgument()<<3))
    {
      pcSlice->setSliceSegmentCurEndCtuTsAddr(validEndOfSliceCtuTsAddr);
      pcSlice->setSliceCurEndCtuTsAddr(validEndOfSliceCtuTsAddr);
      boundingCtuTsAddr=validEndOfSliceCtuTsAddr;
    }
    else if(pcSlice->getSliceSegmentMode()==FIXED_NUMBER_OF_BYTES && pcSlice->getSliceSegmentBits()+numberOfWrittenBits > (pcSlice->getSliceSegmentArgument()<<3))
    {
      pcSlice->setSliceSegmentCurEndCtuTsAddr(validEndOfSliceCtuTsAddr);
      boundingCtuTsAddr=validEndOfSliceCtuTsAddr;
    }

    if (boundingCtuTsAddr <= ctuTsAddr)
    {
      break;
    }

    pcSlice->setSliceBits( (UInt)(pcSlice->getSliceBits() + numberOfWrittenBits) );
    pcSlice->setSliceSegmentBits(pcSlice->getSliceSegmentBits()+numberOfWrittenBits);

    // Store probabilities of second CTU in line into buffer - used only if wavefront-parallel-processing is enabled.
    if ( ctuXPosInCtus == tileXPosInCtus+1 && m_pcCfg->getWaveFrontsynchro())
    {
      m_entropyCodingSyncContextState.loadContexts(m_pppcRDSbacCoder[0][CI_CURR_BEST]);
    }


    if ( m_pcCfg->getUseRateCtrl() )
    {
      Int actualQP        = g_RCInvalidQPValue;
      Double actualLambda = m_pcRdCost->getLambda();
      Int actualBits      = pCtu->getTotalBits();
      Int numberOfEffectivePixels    = 0;
      for ( Int idx = 0; idx < pcPic->getNumPartitionsInCtu(); idx++ )
      {
        if ( pCtu->getPredictionMode( idx ) != NUMBER_OF_PREDICTION_MODES && ( !pCtu->isSkipped( idx ) ) )
        {
          numberOfEffectivePixels = numberOfEffectivePixels + 16;
          break;
        }
      }

      if ( numberOfEffectivePixels == 0 )
      {
        actualQP = g_RCInvalidQPValue;
      }
      else
      {
        actualQP = pCtu->getQP( 0 );
      }
      m_pcRdCost->setLambda(oldLambda);
      m_pcRateCtrl->getRCPic()->updateAfterCTU( m_pcRateCtrl->getRCPic()->getLCUCoded(), actualBits, actualQP, actualLambda,
                                                pCtu->getSlice()->getSliceType() == I_SLICE ? 0 : m_pcCfg->getLCULevelRC() );
    }

    m_uiPicTotalBits += pCtu->getTotalBits();
    m_dPicRdCost     += pCtu->getTotalCost();
    m_uiPicDist      += pCtu->getTotalDistortion();
  }   // CTU for loop end 

  // store context state at the end of this slice-segment, in case the next slice is a dependent slice and continues using the CABAC contexts.
  if( pcSlice->getPPS()->getDependentSliceSegmentsEnabledFlag() )
  {
    m_lastSliceSegmentEndContextState.loadContexts( m_pppcRDSbacCoder[0][CI_CURR_BEST] );//ctx end of dep.slice
  }

  // stop use of temporary bit counter object.
  m_pppcRDSbacCoder[0][CI_CURR_BEST]->setBitstream(NULL);
  m_pcRDGoOnSbacCoder->setBitstream(NULL); // stop use of tempBitCounter.

  // TODO: optimise cabac_init during compress slice to improve multi-slice operation
  //if (pcSlice->getPPS()->getCabacInitPresentFlag() && !pcSlice->getPPS()->getDependentSliceSegmentsEnabledFlag())
  //{
  //  m_encCABACTableIdx = m_pcEntropyCoder->determineCabacInitIdx();
  //}
  //else
  //{
  //  m_encCABACTableIdx = pcSlice->getSliceType();
  //}
}

Void TEncSlice::encodeSlice   ( TComPic* pcPic, TComOutputBitstream* pcSubstreams, UInt &numBinsCoded )
{
  TComSlice* pcSlice                 = pcPic->getSlice(getSliceIdx());

  const UInt startCtuTsAddr          = pcSlice->getSliceSegmentCurStartCtuTsAddr();
  const UInt boundingCtuTsAddr       = pcSlice->getSliceSegmentCurEndCtuTsAddr();

  const UInt frameWidthInCtus        = pcPic->getPicSym()->getFrameWidthInCtus();
  const Bool depSliceSegmentsEnabled = pcSlice->getPPS()->getDependentSliceSegmentsEnabledFlag();
  const Bool wavefrontsEnabled       = pcSlice->getPPS()->getEntropyCodingSyncEnabledFlag();

  // initialise entropy coder for the slice
  m_pcSbacCoder->init( (TEncBinIf*)m_pcBinCABAC );
  m_pcEntropyCoder->setEntropyCoder ( m_pcSbacCoder, pcSlice );
  m_pcEntropyCoder->resetEntropy      ();

  numBinsCoded = 0;
  m_pcBinCABAC->setBinCountingEnableFlag( true );
  m_pcBinCABAC->setBinsCoded(0);

#if ENC_DEC_TRACE
  g_bJustDoIt = g_bEncDecTraceEnable;
#endif
  DTRACE_CABAC_VL( g_nSymbolCounter++ );
  DTRACE_CABAC_T( "\tPOC: " );
  DTRACE_CABAC_V( pcPic->getPOC() );
  DTRACE_CABAC_T( "\n" );
#if ENC_DEC_TRACE
  g_bJustDoIt = g_bEncDecTraceDisable;
#endif


  if (depSliceSegmentsEnabled)
  {
    // modify initial contexts with previous slice segment if this is a dependent slice.
    const UInt ctuRsAddr        = pcPic->getPicSym()->getCtuTsToRsAddrMap( startCtuTsAddr );
    const UInt currentTileIdx=pcPic->getPicSym()->getTileIdxMap(ctuRsAddr);
    const TComTile *pCurrentTile=pcPic->getPicSym()->getTComTile(currentTileIdx);
    const UInt firstCtuRsAddrOfTile = pCurrentTile->getFirstCtuRsAddr();

    if( pcSlice->getDependentSliceSegmentFlag() && ctuRsAddr != firstCtuRsAddrOfTile )
    {
      if( pCurrentTile->getTileWidthInCtus() >= 2 || !wavefrontsEnabled )
      {
        m_pcSbacCoder->loadContexts(&m_lastSliceSegmentEndContextState);
      }
    }
  }

  // for every CTU in the slice segment...

  for( UInt ctuTsAddr = startCtuTsAddr; ctuTsAddr < boundingCtuTsAddr; ++ctuTsAddr )
  {
    const UInt ctuRsAddr = pcPic->getPicSym()->getCtuTsToRsAddrMap(ctuTsAddr);
    const TComTile &currentTile = *(pcPic->getPicSym()->getTComTile(pcPic->getPicSym()->getTileIdxMap(ctuRsAddr)));
    const UInt firstCtuRsAddrOfTile = currentTile.getFirstCtuRsAddr();
    const UInt tileXPosInCtus       = firstCtuRsAddrOfTile % frameWidthInCtus;
    const UInt tileYPosInCtus       = firstCtuRsAddrOfTile / frameWidthInCtus;
    const UInt ctuXPosInCtus        = ctuRsAddr % frameWidthInCtus;
    const UInt ctuYPosInCtus        = ctuRsAddr / frameWidthInCtus;
    const UInt uiSubStrm=pcPic->getSubstreamForCtuAddr(ctuRsAddr, true, pcSlice);
    TComDataCU* pCtu = pcPic->getCtu( ctuRsAddr );

    m_pcEntropyCoder->setBitstream( &pcSubstreams[uiSubStrm] );

    // set up CABAC contexts' state for this CTU
    if (ctuRsAddr == firstCtuRsAddrOfTile)
    {
      if (ctuTsAddr != startCtuTsAddr) // if it is the first CTU, then the entropy coder has already been reset
      {
        m_pcEntropyCoder->resetEntropy();
      }
    }
    else if (ctuXPosInCtus == tileXPosInCtus && wavefrontsEnabled)
    {
      // Synchronize cabac probabilities with upper-right CTU if it's available and at the start of a line.
      if (ctuTsAddr != startCtuTsAddr) // if it is the first CTU, then the entropy coder has already been reset
      {
        m_pcEntropyCoder->resetEntropy();
      }
      TComDataCU *pCtuUp = pCtu->getCtuAbove();
      if ( pCtuUp && ((ctuRsAddr%frameWidthInCtus+1) < frameWidthInCtus)  )
      {
        TComDataCU *pCtuTR = pcPic->getCtu( ctuRsAddr - frameWidthInCtus + 1 );
        if ( pCtu->CUIsFromSameSliceAndTile(pCtuTR) )
        {
          // Top-right is available, so use it.
          m_pcSbacCoder->loadContexts( &m_entropyCodingSyncContextState );
        }
      }
    }


    if ( pcSlice->getSPS()->getUseSAO() )
    {
      Bool bIsSAOSliceEnabled = false;
      Bool sliceEnabled[MAX_NUM_COMPONENT];
      for(Int comp=0; comp < MAX_NUM_COMPONENT; comp++)
      {
        ComponentID compId=ComponentID(comp);
        sliceEnabled[compId] = pcSlice->getSaoEnabledFlag(toChannelType(compId)) && (comp < pcPic->getNumberValidComponents());
        if (sliceEnabled[compId])
        {
          bIsSAOSliceEnabled=true;
        }
      }
      if (bIsSAOSliceEnabled)
      {
        SAOBlkParam& saoblkParam = (pcPic->getPicSym()->getSAOBlkParam())[ctuRsAddr];

        Bool leftMergeAvail = false;
        Bool aboveMergeAvail= false;
        //merge left condition
        Int rx = (ctuRsAddr % frameWidthInCtus);
        if(rx > 0)
        {
          leftMergeAvail = pcPic->getSAOMergeAvailability(ctuRsAddr, ctuRsAddr-1);
        }

        //merge up condition
        Int ry = (ctuRsAddr / frameWidthInCtus);
        if(ry > 0)
        {
          aboveMergeAvail = pcPic->getSAOMergeAvailability(ctuRsAddr, ctuRsAddr-frameWidthInCtus);
        }

        m_pcEntropyCoder->encodeSAOBlkParam(saoblkParam, sliceEnabled, leftMergeAvail, aboveMergeAvail);
      }
    }

#if ENC_DEC_TRACE
    g_bJustDoIt = g_bEncDecTraceEnable;
#endif
      m_pcCuEncoder->encodeCtu( pCtu );
#if ENC_DEC_TRACE
    g_bJustDoIt = g_bEncDecTraceDisable;
#endif

    //Store probabilities of second CTU in line into buffer
    if ( ctuXPosInCtus == tileXPosInCtus+1 && wavefrontsEnabled)
    {
      m_entropyCodingSyncContextState.loadContexts( m_pcSbacCoder );
    }

    // terminate the sub-stream, if required (end of slice-segment, end of tile, end of wavefront-CTU-row):
    if (ctuTsAddr+1 == boundingCtuTsAddr ||
         (  ctuXPosInCtus + 1 == tileXPosInCtus + currentTile.getTileWidthInCtus() &&
          ( ctuYPosInCtus + 1 == tileYPosInCtus + currentTile.getTileHeightInCtus() || wavefrontsEnabled)
         )
       )
    {
      m_pcEntropyCoder->encodeTerminatingBit(1);
      m_pcEntropyCoder->encodeSliceFinish();
      // Byte-alignment in slice_data() when new tile
      pcSubstreams[uiSubStrm].writeByteAlignment();

      // write sub-stream size
      if (ctuTsAddr+1 != boundingCtuTsAddr)
      {
        pcSlice->addSubstreamSize( (pcSubstreams[uiSubStrm].getNumberOfWrittenBits() >> 3) + pcSubstreams[uiSubStrm].countStartCodeEmulations() );
      }
    }
  } // CTU-loop

  if( depSliceSegmentsEnabled )
  {
    m_lastSliceSegmentEndContextState.loadContexts( m_pcSbacCoder );//ctx end of dep.slice
  }

#if ADAPTIVE_QP_SELECTION
  if( m_pcCfg->getUseAdaptQpSelect() )
  {
    m_pcTrQuant->storeSliceQpNext(pcSlice);
  }
#endif

  if (pcSlice->getPPS()->getCabacInitPresentFlag() && !pcSlice->getPPS()->getDependentSliceSegmentsEnabledFlag())
  {
    m_encCABACTableIdx = m_pcEntropyCoder->determineCabacInitIdx();
  }
  else
  {
    m_encCABACTableIdx = pcSlice->getSliceType();
  }
  
  numBinsCoded = m_pcBinCABAC->getBinsCoded();
}

Void TEncSlice::calculateBoundingCtuTsAddrForSlice(UInt &startCtuTSAddrSlice, UInt &boundingCtuTSAddrSlice, Bool &haveReachedTileBoundary,
                                                   TComPic* pcPic, const Bool encodingSlice, const Int sliceMode, const Int sliceArgument, const UInt sliceCurEndCtuTSAddr)
{
  TComSlice* pcSlice = pcPic->getSlice(getSliceIdx());
  const UInt numberOfCtusInFrame = pcPic->getNumberOfCtusInFrame();
  boundingCtuTSAddrSlice=0;
  haveReachedTileBoundary=false;

  switch (sliceMode)
  {
    case FIXED_NUMBER_OF_CTU:
      {
        UInt ctuAddrIncrement    = sliceArgument;
        boundingCtuTSAddrSlice  = ((startCtuTSAddrSlice + ctuAddrIncrement) < numberOfCtusInFrame) ? (startCtuTSAddrSlice + ctuAddrIncrement) : numberOfCtusInFrame;
      }
      break;
    case FIXED_NUMBER_OF_BYTES:
      boundingCtuTSAddrSlice  = (encodingSlice) ? sliceCurEndCtuTSAddr : numberOfCtusInFrame;
      break;
    case FIXED_NUMBER_OF_TILES:
      {
        const UInt tileIdx        = pcPic->getPicSym()->getTileIdxMap( pcPic->getPicSym()->getCtuTsToRsAddrMap(startCtuTSAddrSlice) );
        const UInt tileTotalCount = (pcPic->getPicSym()->getNumTileColumnsMinus1()+1) * (pcPic->getPicSym()->getNumTileRowsMinus1()+1);
        UInt ctuAddrIncrement   = 0;

        for(UInt tileIdxIncrement = 0; tileIdxIncrement < sliceArgument; tileIdxIncrement++)
        {
          if((tileIdx + tileIdxIncrement) < tileTotalCount)
          {
            UInt tileWidthInCtus   = pcPic->getPicSym()->getTComTile(tileIdx + tileIdxIncrement)->getTileWidthInCtus();
            UInt tileHeightInCtus  = pcPic->getPicSym()->getTComTile(tileIdx + tileIdxIncrement)->getTileHeightInCtus();
            ctuAddrIncrement    += (tileWidthInCtus * tileHeightInCtus);
          }
        }

        boundingCtuTSAddrSlice  = ((startCtuTSAddrSlice + ctuAddrIncrement) < numberOfCtusInFrame) ? (startCtuTSAddrSlice + ctuAddrIncrement) : numberOfCtusInFrame;
      }
      break;
    default:
      boundingCtuTSAddrSlice    = numberOfCtusInFrame;
      break;
  }

  // Adjust for tiles and wavefronts.
  if ((sliceMode == FIXED_NUMBER_OF_CTU || sliceMode == FIXED_NUMBER_OF_BYTES) &&
      (m_pcCfg->getNumRowsMinus1() > 0 || m_pcCfg->getNumColumnsMinus1() > 0))
  {
    const UInt ctuRSAddr                  = pcPic->getPicSym()->getCtuTsToRsAddrMap(startCtuTSAddrSlice);
    const UInt startTileIdx               = pcPic->getPicSym()->getTileIdxMap(ctuRSAddr);
    const Bool wavefrontsAreEnabled       = m_pcCfg->getWaveFrontsynchro();

    const TComTile *pStartingTile         = pcPic->getPicSym()->getTComTile(startTileIdx);
    const UInt tileStartTsAddr            = pcPic->getPicSym()->getCtuRsToTsAddrMap(pStartingTile->getFirstCtuRsAddr());
    const UInt tileStartWidth             = pStartingTile->getTileWidthInCtus();
    const UInt tileStartHeight            = pStartingTile->getTileHeightInCtus();
    const UInt tileLastTsAddr_excl        = tileStartTsAddr + tileStartWidth*tileStartHeight;
    const UInt tileBoundingCtuTsAddrSlice = tileLastTsAddr_excl;

    const UInt ctuColumnOfStartingTile    = ((startCtuTSAddrSlice-tileStartTsAddr)%tileStartWidth);
    if (wavefrontsAreEnabled && ctuColumnOfStartingTile!=0)
    {
      // WPP: if a slice does not start at the beginning of a CTB row, it must end within the same CTB row
      const UInt numberOfCTUsToEndOfRow            = tileStartWidth - ctuColumnOfStartingTile;
      const UInt wavefrontTileBoundingCtuAddrSlice = startCtuTSAddrSlice + numberOfCTUsToEndOfRow;
      if (wavefrontTileBoundingCtuAddrSlice < boundingCtuTSAddrSlice)
      {
        boundingCtuTSAddrSlice = wavefrontTileBoundingCtuAddrSlice;
      }
    }

    if (tileBoundingCtuTsAddrSlice < boundingCtuTSAddrSlice)
    {
      boundingCtuTSAddrSlice = tileBoundingCtuTsAddrSlice;
      haveReachedTileBoundary = true;
    }
  }
  else if ((sliceMode == FIXED_NUMBER_OF_CTU || sliceMode == FIXED_NUMBER_OF_BYTES) && pcSlice->getPPS()->getEntropyCodingSyncEnabledFlag() && ((startCtuTSAddrSlice % pcPic->getFrameWidthInCtus()) != 0))
  {
    // Adjust for wavefronts (no tiles).
    // WPP: if a slice does not start at the beginning of a CTB row, it must end within the same CTB row
    boundingCtuTSAddrSlice = min(boundingCtuTSAddrSlice, startCtuTSAddrSlice - (startCtuTSAddrSlice % pcPic->getFrameWidthInCtus()) + (pcPic->getFrameWidthInCtus()));
  }
}

/** Determines the starting and bounding CTU address of current slice / dependent slice
 * \param [out] startCtuTsAddr
 * \param [out] boundingCtuTsAddr
 * \param [in] pcPic
 * \param encodingSlice Identifies, if the calling function is compressSlice() [false] or encodeSlice() [true]

 * Updates startCtuTsAddr, boundingCtuTsAddr with appropriate CTU address
 */
Void TEncSlice::xDetermineStartAndBoundingCtuTsAddr  ( UInt& startCtuTsAddr, UInt& boundingCtuTsAddr, TComPic* pcPic, const Bool encodingSlice ) // TODO: this is now only ever called with encodingSlice=false
{
  TComSlice* pcSlice                 = pcPic->getSlice(getSliceIdx());

  // Non-dependent slice
  UInt startCtuTsAddrSlice           = pcSlice->getSliceCurStartCtuTsAddr();
  Bool haveReachedTileBoundarySlice  = false;
  UInt boundingCtuTsAddrSlice;
  calculateBoundingCtuTsAddrForSlice(startCtuTsAddrSlice, boundingCtuTsAddrSlice, haveReachedTileBoundarySlice, pcPic,
                                     encodingSlice, m_pcCfg->getSliceMode(), m_pcCfg->getSliceArgument(), pcSlice->getSliceCurEndCtuTsAddr());
  pcSlice->setSliceCurEndCtuTsAddr(   boundingCtuTsAddrSlice );
  pcSlice->setSliceCurStartCtuTsAddr( startCtuTsAddrSlice    );

  // Dependent slice
  UInt startCtuTsAddrSliceSegment          = pcSlice->getSliceSegmentCurStartCtuTsAddr();
  Bool haveReachedTileBoundarySliceSegment = false;
  UInt boundingCtuTsAddrSliceSegment;
  calculateBoundingCtuTsAddrForSlice(startCtuTsAddrSliceSegment, boundingCtuTsAddrSliceSegment, haveReachedTileBoundarySliceSegment, pcPic,
                                     encodingSlice, m_pcCfg->getSliceSegmentMode(), m_pcCfg->getSliceSegmentArgument(), pcSlice->getSliceSegmentCurEndCtuTsAddr());
  if (boundingCtuTsAddrSliceSegment>boundingCtuTsAddrSlice)
  {
    boundingCtuTsAddrSliceSegment = boundingCtuTsAddrSlice;
  }
  pcSlice->setSliceSegmentCurEndCtuTsAddr( boundingCtuTsAddrSliceSegment );
  pcSlice->setSliceSegmentCurStartCtuTsAddr(startCtuTsAddrSliceSegment);

  // Make a joint decision based on reconstruction and dependent slice bounds
  startCtuTsAddr    = max(startCtuTsAddrSlice   , startCtuTsAddrSliceSegment   );
  boundingCtuTsAddr = boundingCtuTsAddrSliceSegment;
}

Double TEncSlice::xGetQPValueAccordingToLambda ( Double lambda )
{
  return 4.2005*log(lambda) + 13.7122;
}

//! \}
