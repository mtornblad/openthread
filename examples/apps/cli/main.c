/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <openthread/config.h>
#include <openthread-core-config.h>
#include <assert.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>


#include <openthread/cli.h>
#include <openthread/diag.h>
#include <openthread/openthread.h>
#include <openthread/coap.h>
#include <openthread/platform/platform.h>
#include <openthread/platform/logging.h>

//EFR32
#include "em_chip.h"

#define THREAD_CHANNEL 11
#define THREAD_PANID 0xABCD

static void light_request_handler(void                * p_context,
                                  otCoapHeader        * p_header,
                                  otMessage           * p_message,
                                  const otMessageInfo * p_message_info);


typedef enum
{
    DEVICE_TYPE_REMOTE_CONTROL,
    DEVICE_TYPE_LIGHT
} device_type_t;

typedef enum
{
    LIGHT_OFF = 0,
    LIGHT_ON,
    LIGHT_TOGGLE
} light_command_t;

typedef struct
{
    otInstance     * p_ot_instance;         /**< A pointer to the OpenThread instance. */
    otCoapResource   light_resource;        /**< CoAP light resource. */
} application_t;

application_t m_app =
{
    .p_ot_instance         = NULL,
    .light_resource        = {"light", light_request_handler, NULL, NULL},
};

/***************************************************************************************************
 * @section CoAP
 **************************************************************************************************/

static void light_on(void)
{
    GPIO_PinModeSet(gpioPortA, 0, gpioModePushPull, 1);
}

static void light_off(void)
{
    GPIO_PinModeSet(gpioPortA, 0, gpioModePushPull, 0);
}

static void light_toggle(void)
{
    //LEDS_INVERT(BSP_LED_3_MASK);
}

static void light_response_send(void                * p_context,
                                otCoapHeader        * p_request_header,
                                const otMessageInfo * p_message_info)
{
    otError      error = OT_ERROR_NONE;
    otCoapHeader header;
    otMessage  * p_response;

    do
    {
        otCoapHeaderInit(&header, OT_COAP_TYPE_ACKNOWLEDGMENT, OT_COAP_CODE_CHANGED);
        otCoapHeaderSetMessageId(&header, otCoapHeaderGetMessageId(p_request_header));
        otCoapHeaderSetToken(&header,
                             otCoapHeaderGetToken(p_request_header),
                             otCoapHeaderGetTokenLength(p_request_header));

        p_response = otCoapNewMessage(p_context, &header);
        if (p_response == NULL)
        {
            break;
        }

        error = otCoapSendResponse(p_context, p_response, p_message_info);

    } while (false);

    if (error != OT_ERROR_NONE && p_response != NULL)
    {
        otMessageFree(p_response);
    }
}

static void light_request_handler(void                * p_context,
                                  otCoapHeader        * p_header,
                                  otMessage           * p_message,
                                  const otMessageInfo * p_message_info)
{
    (void)p_message;
    uint8_t command;

    do
    {
        if (otCoapHeaderGetType(p_header) != OT_COAP_TYPE_CONFIRMABLE &&
            otCoapHeaderGetType(p_header) != OT_COAP_TYPE_NON_CONFIRMABLE)
        {
            break;
        }

        if (otCoapHeaderGetCode(p_header) != OT_COAP_CODE_PUT)
        {
            break;
        }

        if (otMessageRead(p_message, otMessageGetOffset(p_message), &command, 1) != 1)
        {
  //          NRF_LOG_INFO("light handler - missing command\r\n");
        }

        switch (command)
        {
            case LIGHT_ON:
                light_on();
                break;

            case LIGHT_OFF:
                light_off();
                break;

            case LIGHT_TOGGLE:
                light_toggle();
                break;

            default:
                break;
        }

        if (otCoapHeaderGetType(p_header) == OT_COAP_TYPE_CONFIRMABLE)
        {
            light_response_send(p_context, p_header, p_message_info);
        }

    } while (false);
}



void otTaskletsSignalPending(otInstance *aInstance)
{
    (void)aInstance;
}

static void coap_init()
{
    m_app.light_resource.mContext = m_app.p_ot_instance;

    assert(otCoapStart(m_app.p_ot_instance, OT_DEFAULT_COAP_PORT) == OT_ERROR_NONE);
    assert(otCoapAddResource(m_app.p_ot_instance, &m_app.light_resource) == OT_ERROR_NONE);
}


int main(int argc, char *argv[])
{
    otInstance *sInstance;

    PlatformInit(argc, argv);
    sInstance = otInstanceInitSingle();
    assert(sInstance);

    otCliUartInit(sInstance);

    /*
    if (!otDatasetIsCommissioned(sInstance))
    {
        assert(otLinkSetChannel(sInstance, THREAD_CHANNEL) == OT_ERROR_NONE);
        assert(otLinkSetPanId(sInstance, THREAD_PANID) == OT_ERROR_NONE);
    }

    assert(otIp6SetEnabled(sInstance, true) == OT_ERROR_NONE);
    assert(otThreadSetEnabled(sInstance, true) == OT_ERROR_NONE);
*/
    m_app.p_ot_instance = sInstance;
    coap_init();


#if OPENTHREAD_ENABLE_DIAG
    otDiagInit(sInstance);
#endif

    while (1)
    {
        otTaskletsProcess(sInstance);
        PlatformProcessDrivers(sInstance);
    }

coap_init();

    return 0;
}

/*
 * Provide, if required an "otPlatLog()" function
 */
#if OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_APP
void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    OT_UNUSED_VARIABLE(aLogLevel);
    OT_UNUSED_VARIABLE(aLogRegion);
    OT_UNUSED_VARIABLE(aFormat);

    va_list ap;
    va_start(ap, aFormat);
    otCliPlatLogv(aLogLevel, aLogRegion, aFormat, ap);
    va_end(ap);
}
#endif
