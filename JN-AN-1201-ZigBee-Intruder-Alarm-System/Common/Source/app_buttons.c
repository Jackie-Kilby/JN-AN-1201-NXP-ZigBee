/*****************************************************************************
 *
 * MODULE:             JN-AN-1201
 *
 * COMPONENT:          app_buttons.c
 *
 * DESCRIPTION:        DK4 (DR1175/DR1199) Button Press detection (Implementation)
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5164,
 * JN5161, JN5148, JN5142, JN5139].
 * You, and any third parties must reproduce the copyright and warranty notice
 * and any other legend of ownership on each copy or partial copy of the
 * software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright NXP B.V. 2013. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include "os.h"
#include "os_gen.h"
#include "DBG.h"
#include "AppHardwareApi.h"
#include "app_events.h"
#include "app_timer_driver.h"
#include "pwrm.h"
#include "app_buttons.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef TRACE_APP_BUTTON
    #define TRACE_APP_BUTTON               TRUE
#else
    #define TRACE_APP_BUTTON               TRUE
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/


    PRIVATE uint8 s_u8ButtonDebounce[APP_BUTTONS_NUM] = { 0xff };
    PRIVATE uint8 s_u8ButtonState[APP_BUTTONS_NUM] = { 0 };
    PRIVATE const uint8 s_u8ButtonDIOLine[APP_BUTTONS_NUM] =
    {
        APP_BUTTONS_BUTTON_1
    };



/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: APP_bButtonInitialise
 *
 * DESCRIPTION:
 * Button Initialization
 *
 * PARAMETER: void
 *
 * RETURNS: bool
 *
 ****************************************************************************/
PUBLIC bool_t APP_bButtonInitialise(void)
{
    /* Set DIO lines to inputs with buttons connected */
    vAHI_DioSetDirection(APP_BUTTONS_DIO_MASK, 0);

#ifdef KEY_CON_ENABLE
    /* Turn on pull-ups for DIO lines with buttons connected */
       vAHI_DioSetPullup(APP_BUTTONS_DIO_MASK, 0);
#else
    /* Turn on pull-ups for DIO lines with buttons connected */
    vAHI_DioSetPullup(0, APP_BUTTONS_DIO_MASK);
#endif

    /* Set the edge detection for falling edges */
    vAHI_DioInterruptEdge(0, APP_BUTTONS_DIO_MASK);  //�ش���

    /* Enable interrupts to occur on selected edge */
    vAHI_DioInterruptEnable(APP_BUTTONS_DIO_MASK, 0);

    uint32 u32Buttons = u32AHI_DioReadInput() & APP_BUTTONS_DIO_MASK;
    if (u32Buttons != APP_BUTTONS_DIO_MASK)
    {
        return TRUE;
    }
    return FALSE;
}

/****************************************************************************
 *
 * NAME: vISR_SystemController
 *
 * DESCRIPTION:
 * ISR called on system controller interrupt. This may be from the synchronous driver
 * (if used) or user pressing a button the the DK4 build
 *
 * PARAMETER:
 *
 * RETURNS:
 *
 ****************************************************************************/
OS_ISR(vISR_SystemController)
{
    DBG_vPrintf(TRACE_APP_BUTTON, "In vISR_SystemController\n");
    /* clear pending DIO changed bits by reading register */
    uint8 u8WakeInt = u8AHI_WakeTimerFiredStatus();
    uint32 u32IOStatus=u32AHI_DioWakeStatus();


    if( u32IOStatus & APP_BUTTONS_DIO_MASK )
    {
#ifdef KEY_CON_ENABLE
    	 /* disable edge detection until scan complete */
    	 vAHI_DioInterruptEnable(0, APP_BUTTONS_DIO_MASK);
    	 OS_eStartSWTimer(APP_ButtonsScanTimer, APP_TIME_MS(20), NULL);
    	 DBG_vPrintf(TRACE_APP_BUTTON, "APP: Wake button 1\n");
#else
        /* disable edge detection until scan complete */
        vAHI_DioInterruptEnable(0, APP_BUTTONS_DIO_MASK);
        OS_eStartSWTimer(APP_ButtonsScanTimer, APP_TIME_MS(5), NULL);
        DBG_vPrintf(TRACE_APP_BUTTON, "APP: Wake button 1\n");
#endif

    }



    if (u8WakeInt & E_AHI_WAKE_TIMER_MASK_1)
    {
        /* wake timer interrupt got us here */
        DBG_vPrintf(TRACE_APP_BUTTON, "APP: Wake Timer 1 Interrupt\n");
        PWRM_vWakeInterruptCallback();
    }
#ifdef SLEEP_ENABLE
    vManageWakeUponSysControlISR();
#endif
}

/****************************************************************************
 *
 * NAME: vISR_Timer2
 *
 * DESCRIPTION:
 * Stub function to allow DK4 'bulbs' to build
 *
 * PARAMETER:
 *
 * RETURNS:
 *
 ****************************************************************************/
OS_ISR(vISR_Timer2){}

/****************************************************************************
 *
 * NAME: vISR_Timer2
 *
 * DESCRIPTION:
 * Button scan task -only present on NON SSL builds
 *
 * PARAMETER:
 *
 * RETURNS:
 *
 ****************************************************************************/
OS_TASK(APP_ButtonsScanTask)
{
    /*
     * The DIO changed status register is reset here before the scan is performed.
     * This avoids a race condition between finishing a scan and re-enabling the
     * DIO to interrupt on a falling edge.
     */
    (void) u32AHI_DioInterruptStatus();
    static uint16 u16key_down_time=0;
    uint32 u32DIOState = u32AHI_DioReadInput() & APP_BUTTONS_DIO_MASK;

#ifdef KEY_CON_ENABLE

           if (0 == u32DIOState)
           {
        	   u16key_down_time++;
        	   if(u16key_down_time>=500)//����10��Ϊ������������
        	   {
        		   APP_tsEvent sButtonEvent;
        		   sButtonEvent.eType = APP_E_EVENT_BUTTON_DOWN;
        		   sButtonEvent.uEvent.sButton.u8Button =0;
        		   sButtonEvent.uEvent.sButton.u32DIOState=u32DIOState;
        		   OS_ePostMessage(APP_msgEvents, &sButtonEvent);

        		   u16key_down_time=0;
        		   vAHI_DioInterruptEnable(APP_BUTTONS_DIO_MASK, 0);//ʹ���ж�
        		   DBG_vPrintf(TRACE_APP_BUTTON, "Buttonkey_10s", 0);
        		   if (OS_eGetSWTimerStatus(APP_ButtonsScanTimer) != OS_E_SWTIMER_STOPPED)
        		   {
        		       OS_eStopSWTimer(APP_ButtonsScanTimer);
        		   }
        	   }
        	   else
        	   {
        		   DBG_vPrintf(TRACE_APP_BUTTON, "Buttonkey_20ms", 0);
        		   OS_eContinueSWTimer(APP_ButtonsScanTimer, APP_TIME_MS(20), NULL);
        	   }

           }
           else
           {
              if(u16key_down_time>=1)
              {
            	  APP_tsEvent sButtonEvent;
            	  sButtonEvent.eType = APP_E_EVENT_BUTTON_UP;
            	  sButtonEvent.uEvent.sButton.u8Button =0;
            	  sButtonEvent.uEvent.sButton.u32DIOState=u32DIOState;
            	  OS_ePostMessage(APP_msgEvents, &sButtonEvent);
            	  //���ͱ�����Ϣ
              }
               DBG_vPrintf(TRACE_APP_BUTTON, "Button UP\n");
               u16key_down_time=0;
               vAHI_DioInterruptEnable(APP_BUTTONS_DIO_MASK, 0);//ʹ���ж�
                if (OS_eGetSWTimerStatus(APP_ButtonsScanTimer) != OS_E_SWTIMER_STOPPED)
                  {
                      OS_eStopSWTimer(APP_ButtonsScanTimer);
                  }
           }

#else

     APP_tsEvent sButtonEvent;
     sButtonEvent.eType = APP_E_EVENT_BUTTON_DOWN;
     sButtonEvent.uEvent.sButton.u8Button = 0;
     sButtonEvent.uEvent.sButton.u32DIOState=u32DIOState;
     DBG_vPrintf(TRACE_APP_BUTTON, "Button DN=%d\n", 0);
     OS_ePostMessage(APP_msgEvents, &sButtonEvent);//�������¼�
     vAHI_DioInterruptEnable(APP_BUTTONS_DIO_MASK, 0);//ʹ���ж�
#endif
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
