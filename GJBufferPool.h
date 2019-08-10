//
//  GJBufferPool.h
//  GJQueue
//
//  Created by 未成年大叔 on 16/12/28.
//  Copyright © 2016年 MinorUncle. All rights reserved.
//

#ifndef GJBufferPool_h
#define GJBufferPool_h
#include "GJQueue.h"
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

/**
    多线程支持，可以是不同size；
 */
typedef struct _GJBufferPool{
    GJQueue* queue;
}GJBufferPool;
bool GJBufferPoolCreate(GJBufferPool** pool);
GJBufferPool* defauleBufferPool();
bool GJBufferPoolRelease(GJBufferPool** pool);

void* GJBufferPoolGetData(GJBufferPool* p,int size);
bool GJBufferPoolSetData(GJBufferPool* p,void* data);

#ifdef __cplusplus
    }
#endif

#endif /* GJBufferPool_h */
