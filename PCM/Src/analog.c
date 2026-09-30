//
// Created by Noah Husby on 9/30/26.
//
/*
 * Change this if CubeMX uses a different ADC.
 */
#include "analog.h"

#include <string.h>
#include "main.h"

extern ADC_HandleTypeDef hadc1;

typedef struct
{
    pcm_analog_values_t values;

    TX_THREAD thread;
    TX_MUTEX mutex;
    TX_SEMAPHORE conversion_complete;

    VOID *thread_stack;

    uint16_t dma_buffer[PCM_ANALOG_CHANNEL_COUNT];
} pcm_analog_t;

static pcm_analog_t analog;

void pcm_analog_init(void)
{
    memset(&analog, 0, sizeof(analog));
}

UINT pcm_analog_create(TX_BYTE_POOL *byte_pool)
{
    UINT status;

    status = tx_byte_allocate(
        byte_pool,
        &analog.thread_stack,
        PCM_ANALOG_THREAD_STACK_SIZE,
        TX_NO_WAIT);

    if (status != TX_SUCCESS)
    {
        return status;
    }

    status = tx_mutex_create(
        &analog.mutex,
        "PCM Analog Mutex",
        TX_NO_INHERIT);

    if (status != TX_SUCCESS)
    {
        return status;
    }

    status = tx_semaphore_create(
        &analog.conversion_complete,
        "PCM Analog Conversion Complete",
        0);

    if (status != TX_SUCCESS)
    {
        return status;
    }

    status = tx_thread_create(
        &analog.thread,
        "PCM Analog",
        pcm_analog_thread_entry,
        0,
        analog.thread_stack,
        PCM_ANALOG_THREAD_STACK_SIZE,
        PCM_ANALOG_THREAD_PRIORITY,
        PCM_ANALOG_THREAD_PRIORITY,
        TX_NO_TIME_SLICE,
        TX_AUTO_START);

    return status;
}

VOID pcm_analog_thread_entry(ULONG thread_input)
{
    (void)thread_input;

    while (1)
    {
        HAL_StatusTypeDef status = HAL_ADC_Start_DMA(
            &hadc1,
            (uint32_t *)analog.dma_buffer,
            PCM_ANALOG_CHANNEL_COUNT);

        if (status != HAL_OK)
        {
            /*
             * ADC/DMA failed to start.
             * Don't kill the thread; try again next sample period.
             */
            tx_thread_sleep(
                (TX_TIMER_TICKS_PER_SECOND *
                 PCM_ANALOG_SAMPLE_PERIOD_MS) / 1000);

            continue;
        }

        /*
         * Wait for HAL_ADC_ConvCpltCallback() to signal that
         * all five conversions have been transferred by DMA.
         */
        if (tx_semaphore_get(
                &analog.conversion_complete,
                TX_WAIT_FOREVER) != TX_SUCCESS)
        {
            continue;
        }

        /*
         * ADC1 regular conversion sequence:
         *
         * Rank 1 -> APPS_H
         * Rank 2 -> APPS_L
         * Rank 3 -> FRONT_BRAKE
         * Rank 4 -> REAR_BRAKE
         * Rank 5 -> STEERING
         */
        tx_mutex_get(&analog.mutex, TX_WAIT_FOREVER);

        analog.values.apps_h      = analog.dma_buffer[0];
        analog.values.apps_l      = analog.dma_buffer[1];
        analog.values.front_brake = analog.dma_buffer[2];
        analog.values.rear_brake  = analog.dma_buffer[3];
        analog.values.steering    = analog.dma_buffer[4];

        tx_mutex_put(&analog.mutex);

        /*
         * Normal DMA mode: stop the ADC after this sequence.
         */
        HAL_ADC_Stop_DMA(&hadc1);

        tx_thread_sleep(
            (TX_TIMER_TICKS_PER_SECOND *
             PCM_ANALOG_SAMPLE_PERIOD_MS) / 1000);
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance != ADC1)
    {
        return;
    }

    /*
     * ThreadX semaphore put is ISR-safe.
     * Wake the analog thread to process the completed buffer.
     */
    tx_semaphore_put(&analog.conversion_complete);
}

uint16_t pcm_analog_get(pcm_analog_channel_t channel)
{
    uint16_t value = 0;

    tx_mutex_get(&analog.mutex, TX_WAIT_FOREVER);

    switch (channel)
    {
        case PCM_ANALOG_APPS_H:
            value = analog.values.apps_h;
            break;

        case PCM_ANALOG_APPS_L:
            value = analog.values.apps_l;
            break;

        case PCM_ANALOG_FRONT_BRAKE:
            value = analog.values.front_brake;
            break;

        case PCM_ANALOG_REAR_BRAKE:
            value = analog.values.rear_brake;
            break;

        case PCM_ANALOG_STEERING:
            value = analog.values.steering;
            break;

        default:
            break;
    }

    tx_mutex_put(&analog.mutex);

    return value;
}

void pcm_analog_get_all(pcm_analog_values_t *values)
{
    if (values == NULL)
    {
        return;
    }

    tx_mutex_get(&analog.mutex, TX_WAIT_FOREVER);

    *values = analog.values;

    tx_mutex_put(&analog.mutex);
}

uint16_t pcm_analog_get_apps_h(void)
{
    return pcm_analog_get(PCM_ANALOG_APPS_H);
}

uint16_t pcm_analog_get_apps_l(void)
{
    return pcm_analog_get(PCM_ANALOG_APPS_L);
}

uint16_t pcm_analog_get_front_brake(void)
{
    return pcm_analog_get(PCM_ANALOG_FRONT_BRAKE);
}

uint16_t pcm_analog_get_rear_brake(void)
{
    return pcm_analog_get(PCM_ANALOG_REAR_BRAKE);
}

uint16_t pcm_analog_get_steering(void)
{
    return pcm_analog_get(PCM_ANALOG_STEERING);
}