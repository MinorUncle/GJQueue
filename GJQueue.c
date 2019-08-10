//
//  GJQueue.c
//  GJQueue
//
//  Created by 未成年大叔 on 16/12/27.
//  Copyright © 2016年 MinorUncle. All rights reserved.
//

#include "GJQueue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#ifdef DEBUG
#define GJQueueLOG(format, ...) printf(format,##__VA_ARGS__)
#else
#define GJQueueLOG(format, ...)
#endif

#define DEFAULT_MAX_COUNT 3

#define DEFAULT_TIME 100000000



RetainBuffer* retainBufferAlloc(int size,void (*releaseCallBack)(void*),void* parm ){
    RetainBuffer* buffer = (RetainBuffer*)malloc(sizeof(RetainBuffer));
    buffer->size = size;
    buffer->data = malloc(size);
    buffer->retainCount = 1;
    buffer->releaseCallBack = releaseCallBack;
    buffer->parm = parm;
    return buffer;
};
RetainBuffer* retainBufferPack(void* data, int size,void (*releaseCallBack)(void* ),void* parm){
    RetainBuffer* buffer = (RetainBuffer*)malloc(sizeof(RetainBuffer));
    buffer->size = size;
    buffer->data = data;
    buffer->retainCount = 1;
    buffer->releaseCallBack = releaseCallBack;
    buffer->parm = parm;
    return buffer;
}
void retainBufferRetain(RetainBuffer* buffer){
    if (buffer && buffer->retainCount >0) {
        buffer->retainCount ++;
    }
}
void retainBufferUnRetain(RetainBuffer* buffer){
    buffer->retainCount--;
    if (buffer->retainCount < 1) {
        if (buffer->releaseCallBack) {
            buffer->releaseCallBack(buffer->parm);
        }else{
            free(buffer->data);
        }
        free(buffer);
        buffer = NULL;
    }
}
void retainBufferFree(RetainBuffer* buffer){
    if (buffer->releaseCallBack) {
        buffer->releaseCallBack(buffer->parm);
    }else{
        free(buffer->data);
    }
    free(buffer);
    buffer = NULL;
}



//////queue
bool queueWaitPop(GJQueue* queue,int ms);
bool queueWaitPush(GJQueue* queue,int ms);
bool queueSignalPop(GJQueue* queue);
bool queueSignalPush(GJQueue* queue);
bool queueBroadcastPop(GJQueue* queue);
bool queueBroadcastPush(GJQueue* queue);



inline bool queueWaitPop(GJQueue* queue,int ms)
{
    if (queue->atomic)return false;

    struct timespec ts;
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    ms += tv.tv_usec / 1000;
    ts.tv_sec = tv.tv_sec + ms / 1000;
    ts.tv_nsec = ms % 1000 * 1000000;
    int ret = pthread_cond_timedwait(&queue->outCond, &queue->popLock, &ts);
    printf("ret:%d,,%d\n",ret,!ret);
    return ret==0;
}
inline bool queueWaitPush(GJQueue* queue,int ms)
{
    if (queue->atomic)return false;
    
    struct timespec ts;
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    ms += tv.tv_usec / 1000;
    ts.tv_sec = tv.tv_sec + ms / 1000;
    ts.tv_nsec = ms % 1000 * 1000000;
    int ret = pthread_cond_timedwait(&queue->inCond, &queue->pushLock, &ts);
    printf("ret:%d,,%d\n",ret,!ret);
    return !ret;
}
inline bool queueSignalPop(GJQueue* queue)
{
    if (queue->atomic)return false;
    return !pthread_cond_signal(&queue->outCond);
}
inline bool queueSignalPush(GJQueue* queue)
{
    if (queue->atomic)return false;
    return !pthread_cond_signal(&queue->inCond);
}
inline bool queueBroadcastPop(GJQueue* queue)
{
    if (queue->atomic)return false;
    return !pthread_cond_broadcast(&queue->outCond);
}
inline bool queueBroadcastPush(GJQueue* queue)
{
    if (queue->atomic)return false;
    return !pthread_cond_broadcast(&queue->inCond);
}
inline bool queueUnLockPop(GJQueue* q){
    if (q->atomic)return false;
    return !pthread_mutex_unlock(&q->popLock);
}
inline bool queueLockPush(GJQueue* q){
    if (q->atomic)return false;
    return !pthread_mutex_lock(&q->pushLock);
}
inline bool queueUnLockPush(GJQueue* q){
    if (q->atomic)return false;
    return !pthread_mutex_unlock(&q->pushLock);
}

inline bool queueLockPop(GJQueue* q){
    if (q->atomic)return false;
    return !pthread_mutex_lock(&q->popLock);
}

#pragma mark DELEGATE

    
  

long queueGetLength(GJQueue* q){
    return (q->inPointer -  q->outPointer);
}

bool queuePeekValue(GJQueue* q, const long index,void** value){
    if (index < q->outPointer || index >= q->inPointer) {
        return false;
    }
    long current = index%q->allocSize;
    *value = q->queue[current];
    return true;
}
bool queuePeekTopOutValue(GJQueue* q,void** value, int ms){
    void* retV = NULL;
    queueLockPop(q);
    if (  q->outPointer == q->inPointer) {
        GJQueueLOG("begin Wait peek in ----------\n");
        if (ms <= 0 || !queueWaitPop(q, ms) || q->outPointer == q->inPointer) {
            GJQueueLOG("fail Wait in ----------\n");
            queueUnLockPop(q);
            *value = NULL;
            return false;
        }
        GJQueueLOG("after Wait peek in.  incount:%ld  outcount:%ld----------\n",q->inPointer,q->outPointer);
    }
    
    retV = q->queue[q->outPointer % q->allocSize];
    queueUnLockPop(q);
    *value = retV;
    return true;
}

/**
 出队列

 @param q q description
 @param temBuffer temBuffer description
 @param ms ms description 等待时间
 @return return value description
 */
bool queuePop(GJQueue* q,void** temBuffer,int ms){
    queueLockPop(q);
    if (q->inPointer == q->outPointer) {
        GJQueueLOG("begin Wait in ----------\n");
        if (ms <= 0 || !queueWaitPop(q, ms) || q->inPointer == q->outPointer) {
            GJQueueLOG("fail Wait in ----------\n");
            queueUnLockPop(q);
            return false;
        }
        GJQueueLOG("after Wait in.  incount:%ld  outcount:%ld----------\n",q->inPointer,q->outPointer);
    }
    int index = q->outPointer%q->allocSize;
    *temBuffer = q->queue[index];
    memset(&q->queue[index], 0, sizeof(void*));//防止在oc里的引用一直不释放；
    
    q->outPointer++;
    queueSignalPush(q);
    GJQueueLOG("after signal out.  incount:%ld  outcount:%ld----------\n",q->inPointer,q->outPointer);
    queueUnLockPop(q);
    assert(*temBuffer);
    return true;
}

/**
 如队列

 @param q q description
 @param temBuffer temBuffer description
 @param ms ms description
 @return return value description
 */
bool queuePush(GJQueue* q,void* temBuffer,int ms){
    queueLockPush(q);
    if (q->inPointer - q->outPointer == q->allocSize) {
        if (q->autoResize) {
            //resize
            void** temBuffer = (void**)malloc(sizeof(void*)*(q->allocSize * 2));
            assert(temBuffer);
            for (long i = q->outPointer,j =0; i<q->inPointer; i++,j++) {
                temBuffer[j] = q->queue[i%q->allocSize];
            }
            free(q->queue);
            q->queue = temBuffer;
            q->inPointer = q->allocSize;
            q->outPointer = 0;
            q->allocSize += q->capacity;
        }else{
            GJQueueLOG("begin Wait out ----------\n");
            if (ms <= 0 || !queueWaitPush(q, ms) || q->inPointer - q->outPointer == q->allocSize) {//溢出时会有问题
                GJQueueLOG("fail begin Wait out ----------\n");
                queueUnLockPush(q);
                return false;
            }
            GJQueueLOG("after Wait out.  incount:%ld  outcount:%ld----------\n",q->inPointer,q->outPointer);
        }
    }
    q->queue[q->inPointer%q->allocSize] = temBuffer;
    q->inPointer++;
    queueSignalPop(q);
    GJQueueLOG("after signal in. incount:%ld  outcount:%ld----------\n",q->inPointer,q->outPointer);
    queueUnLockPush(q);
    assert(temBuffer);
    
    return true;
}

void queueClean(GJQueue* q){
    queueLockPop(q);
    queueBroadcastPush(q);//确保可以锁住下一个,避免循环锁
    queueLockPush(q);
    while (q->outPointer<q->inPointer) {
        memset(&q->queue[q->outPointer++%q->allocSize], 0, sizeof(void*));//防止在oc里的引用一直不释放；
    }
    q->inPointer=q->outPointer=0;
    queueBroadcastPush(q);
    queueUnLockPush(q);
    queueUnLockPop(q);
}

bool queueCreate(GJQueue** outQ,int capacity,int atomic){
    GJQueue* q = (GJQueue*)malloc(sizeof(GJQueue));
    if (!q) {
        return false;
    }
    memset(q, 0, sizeof(GJQueue));
    if (atomic > 0) {
        pthread_condattr_t cond_attr;
        pthread_condattr_init(&cond_attr);
        pthread_cond_init(&q->inCond, &cond_attr);
        pthread_cond_init(&q->outCond, &cond_attr);
        pthread_mutex_init(&q->popLock, NULL);
        pthread_mutex_init(&q->pushLock, NULL);
    }
    if (capacity<=0) {capacity = DEFAULT_MAX_COUNT;}
    q->capacity = capacity;
    q->allocSize = capacity;
    q->queue = (void**)malloc(sizeof(void*) * q->capacity);
    if (!q->queue) {
        free(q);
        return false;
    }
    *outQ = q;
    return true;
}

bool queueRelease(GJQueue** inQ){
    GJQueue* q = *inQ;
    if (!q) {
        return false;
    }
    queueClean(q);
    free(q->queue);
    pthread_cond_destroy(&q->inCond);
    pthread_cond_destroy(&q->outCond);
    pthread_mutex_destroy(&q->popLock);
    pthread_mutex_destroy(&q->pushLock);
    *inQ = NULL;
    return true;
}


