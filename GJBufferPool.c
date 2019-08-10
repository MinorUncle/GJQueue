//
//  GJBufferPool.c
//  GJQueue
//
//  Created by 未成年大叔 on 16/12/28.
//  Copyright © 2016年 MinorUncle. All rights reserved.
//

#include "GJBufferPool.h"
#include <stdlib.h>



bool GJBufferPoolCreate(GJBufferPool** pool){
    GJBufferPool* p = (GJBufferPool*)malloc(sizeof(GJBufferPool));
    if (!p || !queueCreate(&p->queue, 5,false)){
        return false;
    }
    *pool = p;
    return true;
};
GJBufferPool* defauleBufferPool(){
    static GJBufferPool* _defaultPool = NULL;
    if (_defaultPool == NULL) {
       _defaultPool = (GJBufferPool*)malloc(sizeof(GJBufferPool));
    }
    return _defaultPool;
}
bool GJBufferPoolRelease(GJBufferPool** pool){
    if (!pool || !*pool) {
        return false;
    }
    GJBufferPool* p = *pool;
    queueRelease(&p->queue);
    return true;
};

void* GJBufferPoolGetData(GJBufferPool* p,int size){
    void* data;
    if (queuePop(p->queue, &data, 0)) {
        if ( *(int*)data < size) {
            free(data);
            data = malloc(size + sizeof(int));
            *(int*)data = size;
        }
    }else{
        data = malloc(size + sizeof(int));
        *(int*)data = size;
    }
    return (int*)data + 1;
}
bool GJBufferPoolSetData(GJBufferPool* p,void* data){
    
    return queuePush(p->queue, (int*)data - 1, 0);
}
