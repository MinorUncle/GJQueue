//
//  GJQueue.h
//  GJQueue
//
//  Created by 未成年大叔 on 16/12/27.
//  Copyright © 2016年 MinorUncle. All rights reserved.
//

#ifndef GJQueue_h
#define GJQueue_h
#include <pthread.h>
#include <stdio.h>


/* exploit C++ ability of default values for function parameters */
#ifndef QUEUE_DEFAULT
#if defined( __cplusplus )
#   define QUEUE_DEFAULT(x) =x
#else
#   define QUEUE_DEFAULT(x)

#endif
#endif

#ifndef bool
#   define bool unsigned int
#   define true 1
#   define false 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

    
typedef struct _RetainBuffer{
    int size;
    int retainCount;
    void* data;
    void (*releaseCallBack)(void* parm);
    void *parm;
}RetainBuffer;

RetainBuffer* retainBufferAlloc(int size,void (*releaseCallBack)(void* ),void* parm );
RetainBuffer* retainBufferPack(void* data, int size,void (*releaseCallBack)(void* ),void* parm);
void retainBufferRetain(RetainBuffer* buffer);
void retainBufferUnRetain(RetainBuffer* buffer);
void retainBufferFree(RetainBuffer* b);


typedef struct _GJData{
    void* data;
    unsigned int size;
}GJData;

typedef struct _GJQueue{
    long inPointer;  //尾
    long outPointer; //头
    int capacity;
    int allocSize;
    void** queue;
    pthread_cond_t inCond;
    pthread_cond_t outCond;
    pthread_mutex_t pushLock;
    pthread_mutex_t popLock;
    bool autoResize;//是否支持自动增长，当为YES时，push永远不会等待，只会重新申请内存,默认为false
    bool atomic;//是否多线程；
}GJQueue;

//大于0为真，<= 0 为假

/**
 创建queue

 @param outQ outQ description
 @param capacity 初始申请的内存大小个数
 @param atomic 是否支持多线程
 @return return value description
 */
bool queueCreate(GJQueue** outQ,int capacity,int atomic QUEUE_DEFAULT(0));
bool queueRelease(GJQueue** inQ);

bool queuePop(GJQueue* q,void** temBuffer,int ms QUEUE_DEFAULT(500));
bool queuePush(GJQueue* q,void* temBuffer,int ms QUEUE_DEFAULT(500));
long queueGetLength(GJQueue* q);

//根据index获得vause,当超过inPointer和outPointer范围则失败，用于遍历数组，不会产生压出队列作用
bool queuePeekValue(GJQueue* q,const long index,void** value);

/**
 获得出栈位置的值，没有数据则等待ms 时长
 @param q q description
 @param value value description
 @param ms ms description
 @return return value description
 */
bool queuePeekTopOutValue(GJQueue* q,void** value, int ms);

bool queueUnLockPop(GJQueue* q);
bool queueLockPush(GJQueue* q);
bool queueUnLockPush(GJQueue* q);
bool queueLockPop(GJQueue* q);

#ifdef __cplusplus
    }
#endif
        
#endif /* GJQueue_h */
