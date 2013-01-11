/**
 *  @file ringBuffer.c
 *  @brief Ring buffer, multithread safe with overwriting possibility.
 *  @date 28/09/2012
 *  @author maxime.maitre@parrot.com
**/

/*****************************************
 * 
 *             include file :
 *
******************************************/

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <libSAL/print.h>
#include <libSAL/mutex.h>

#include <libNetwork/status.h>

#include "ringBuffer.h"

/*****************************************
 * 
 *             define :
 *
******************************************/
#define TAG "RingBuffer"

/*****************************************
 * 
 *             implementation :
 *
******************************************/


network_ringBuffer_t* NETWORK_NewRingBuffer( unsigned int numberOfCell, unsigned int cellSize )
{
    /** -- Create a new ring buffer not overwritable -- */
    return NETWORK_NewRingBufferWithOverwriting( numberOfCell, cellSize, 0 );
}

network_ringBuffer_t* NETWORK_NewRingBufferWithOverwriting( unsigned int numberOfCell, 
                                                               unsigned int cellSize, 
                                                               int isOverwriting )
{
    /** -- Create a new ring buffer -- */
    
    /** local declarations */
    network_ringBuffer_t* pRingBuff =  malloc( sizeof(network_ringBuffer_t) );
    
    if(pRingBuff)
    {
        pRingBuff->numberOfCell = numberOfCell;
        pRingBuff->cellSize = cellSize;
        pRingBuff->indexInput = 0;
        pRingBuff->indexOutput = 0;
        pRingBuff->isOverwriting = isOverwriting;
        sal_mutex_init( &(pRingBuff->mutex) );
        pRingBuff->dataBuffer = malloc( cellSize * numberOfCell );
        
        if( pRingBuff->dataBuffer == NULL)
        {
            NETWORK_DeleteRingBuffer(&pRingBuff);
        }
    }
    
    return pRingBuff;
}

void NETWORK_DeleteRingBuffer( network_ringBuffer_t** ppRingBuff )
{
    /** -- Delete the ring buffer -- */
    
    /** local declarations */
    network_ringBuffer_t* pRingBuff = NULL;
    
    if(ppRingBuff)
    {
        pRingBuff = *ppRingBuff;
        
        if(pRingBuff)
        {
            sal_mutex_destroy(&(pRingBuff->mutex));
            free(pRingBuff->dataBuffer);
            pRingBuff->dataBuffer = NULL;
        
            free(pRingBuff);
            pRingBuff = NULL;
        }
        *ppRingBuff = NULL;
    }
}

eNETWORK_Error NETWORK_RingBuffPushBack( network_ringBuffer_t* pRingBuff, const uint8_t* pNewData )
{
    /** -- Add the new data at the back of the ring buffer -- */
    
    /** local declarations */
    int error = NETWORK_OK;
    uint8_t* buffPointor = NULL;
    
    sal_mutex_lock(&(pRingBuff->mutex));
    
    if( NETWORK_RingBuffGetFreeCellNb(pRingBuff) || pRingBuff->isOverwriting)
    {    
        if( !NETWORK_RingBuffGetFreeCellNb(pRingBuff) )
        {
            (pRingBuff->indexOutput) += pRingBuff->cellSize;
        }
        
        buffPointor = pRingBuff->dataBuffer + 
                    ( pRingBuff->indexInput % (pRingBuff->numberOfCell * pRingBuff->cellSize) );
        
        memcpy(buffPointor, pNewData, pRingBuff->cellSize);
        
        pRingBuff->indexInput += pRingBuff->cellSize;
    }
    else
    {
        error = NETWORK_ERROR_BUFFER_SIZE;
    }
    
    sal_mutex_unlock(&(pRingBuff->mutex));

    return error;
}

eNETWORK_Error NETWORK_RingBuffPopFront( network_ringBuffer_t* pRingBuff, uint8_t* pPopData )
{
    /** -- Pop the oldest data -- */
    
    /** local declarations */
    uint8_t* buffPointor = NULL;
    eNETWORK_Error error = NETWORK_OK;
    
    sal_mutex_lock(&(pRingBuff->mutex));
    
    if( !NETWORK_RingBuffIsEmpty(pRingBuff) )
    {
        if(pPopData != NULL)
        {
            buffPointor =     pRingBuff->dataBuffer + 
                    (pRingBuff->indexOutput % (pRingBuff->numberOfCell * pRingBuff->cellSize) );
                    
            memcpy(pPopData, buffPointor, pRingBuff->cellSize);
        }
        
        (pRingBuff->indexOutput) += pRingBuff->cellSize;
    }
    else
    {
        error = NETWORK_ERROR_BUFFER_EMPTY;
    }
    
    sal_mutex_unlock(&(pRingBuff->mutex));
    
    return error;
}

eNETWORK_Error NETWORK_RingBuffFront(network_ringBuffer_t* pRingBuff, uint8_t* pFrontData)
{
    /** -- Return a pointer on the front data -- */
    
    /** local declarations */
    eNETWORK_Error error = NETWORK_OK;
    uint8_t* buffPointor = NULL;
                    
    sal_mutex_lock(&(pRingBuff->mutex));
    
    buffPointor = pRingBuff->dataBuffer + 
                    (pRingBuff->indexOutput % (pRingBuff->numberOfCell * pRingBuff->cellSize) );
    
    if( !NETWORK_RingBuffIsEmpty(pRingBuff) )
    {
        memcpy(pFrontData, buffPointor, pRingBuff->cellSize);
    }
    else
    {
        error = NETWORK_ERROR_BUFFER_SIZE;
    }
    
    sal_mutex_unlock(&(pRingBuff->mutex));
    
    return error;
}

void NETWORK_RingBuffPrint(network_ringBuffer_t* pRingBuff)
{
    /** -- Print the state of the ring buffer -- */
    
    sal_mutex_lock(&(pRingBuff->mutex));
    
    SAL_PRINT(PRINT_WARNING, TAG," pointer dataBuffer :%d \n",pRingBuff->dataBuffer);
    SAL_PRINT(PRINT_WARNING, TAG," numberOfCell :%d \n",pRingBuff->numberOfCell);
    SAL_PRINT(PRINT_WARNING, TAG," cellSize :%d \n",pRingBuff->cellSize);
    SAL_PRINT(PRINT_WARNING, TAG," indexOutput :%d \n",pRingBuff->indexOutput);
    SAL_PRINT(PRINT_WARNING, TAG," indexInput :%d \n",pRingBuff->indexInput);
    SAL_PRINT(PRINT_WARNING, TAG," overwriting :%d \n",pRingBuff->isOverwriting);
    SAL_PRINT(PRINT_WARNING, TAG," data : \n");
    
    sal_mutex_unlock(&(pRingBuff->mutex));
    
    NETWORK_RingBuffDataPrint(pRingBuff);
}

void NETWORK_RingBuffDataPrint(network_ringBuffer_t* pRingBuff)
{
    /** -- Print the contents of the ring buffer -- */
    
    /** local declarations */
    uint8_t* it = NULL;
    int  iindex = 0;
    int  ii = 0;
    
    sal_mutex_lock(&(pRingBuff->mutex));
    
    for( iindex = pRingBuff->indexOutput ; 
         iindex < pRingBuff->indexInput ;
         iindex += pRingBuff->cellSize )
    {
        it = pRingBuff->dataBuffer + (iindex % (pRingBuff->numberOfCell * pRingBuff->cellSize) );
        
        SAL_PRINT(PRINT_WARNING, TAG,"    - 0x: ");
        for(ii = 0 ; ii < pRingBuff->cellSize ; ++ii)
        {
            SAL_PRINT(PRINT_WARNING, TAG,"%2x | ",*((uint8_t*)it));
            ++it;
        }
        SAL_PRINT(PRINT_WARNING, TAG,"\n");
    }
    
    sal_mutex_unlock(&(pRingBuff->mutex));
}
