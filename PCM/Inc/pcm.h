//
// Created by Noah Husby on 9/30/26.
//

#ifndef PCM_PCM_H
#define PCM_PCM_H
#include "tx_api.h"

UINT pcm_threads_create(TX_BYTE_POOL *byte_pool);

void pcm_init(void);

#endif //PCM_PCM_H
