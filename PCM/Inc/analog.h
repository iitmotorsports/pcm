#ifndef PCM_ANALOG_H
#define PCM_ANALOG_H

#include <stdbool.h>
#include <stdint.h>

#include "tx_api.h"

#define PCM_ANALOG_THREAD_STACK_SIZE 1024
#define PCM_ANALOG_THREAD_PRIORITY   10

#define PCM_ANALOG_SAMPLE_PERIOD_MS  10
#define PCM_ANALOG_CHANNEL_COUNT     5

typedef enum
{
    PCM_ANALOG_APPS_H = 0,
    PCM_ANALOG_APPS_L,
    PCM_ANALOG_FRONT_BRAKE,
    PCM_ANALOG_REAR_BRAKE,
    PCM_ANALOG_STEERING,
} pcm_analog_channel_t;

typedef struct
{
    uint16_t apps_h;
    uint16_t apps_l;
    uint16_t front_brake;
    uint16_t rear_brake;
    uint16_t steering;
} pcm_analog_values_t;

void pcm_analog_init(void);

UINT pcm_analog_create(TX_BYTE_POOL *byte_pool);

uint16_t pcm_analog_get(pcm_analog_channel_t channel);

void pcm_analog_get_all(pcm_analog_values_t *values);

uint16_t pcm_analog_get_apps_h(void);
uint16_t pcm_analog_get_apps_l(void);
uint16_t pcm_analog_get_front_brake(void);
uint16_t pcm_analog_get_rear_brake(void);
uint16_t pcm_analog_get_steering(void);

VOID pcm_analog_thread_entry(ULONG thread_input);

#endif /* PCM_ANALOG_H */