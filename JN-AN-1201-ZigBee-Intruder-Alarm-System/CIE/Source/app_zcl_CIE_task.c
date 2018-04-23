/*****************************************************************************
 *
 * MODULE:             JN-AN-1201 ZHA Demo
 *
 * COMPONENT:          app_zcl_CIE_task.c
 *
 * DESCRIPTION:        ZHA CIE Application Behavior - Implementation
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
 * Copyright NXP B.V. 2012. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include "os.h"
#include "os_gen.h"
#include "pdum_gen.h"
#include "pdm.h"
#include "dbg.h"
#include "zps_gen.h"
#include "PDM_IDs.h"
#include "zcl_options.h"
#include "zps_apl_af.h"
#include "ha.h"
#include "string.h"
#include "app_zcl_CIE_task.h"
#include "zha_CIE_node.h"
#include "app_common.h"
#include "app_events.h"
#include "app_zone_client.h"
#include "app_CIE_save.h"
#include "app_cmd_handle.h"
#include "app_data_handle.h"
#include "Array_list.h"
#include "app_CIE_uart.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifdef DEBUG_ZCL
    #define TRACE_ZCL   TRUE
#else
    #define TRACE_ZCL   TRUE
#endif

#ifdef DEBUG_CIE_TASK
    #define TRACE_CIE_TASK  TRUE
#else
    #define TRACE_CIE_TASK FALSE
#endif

#ifdef DEBUG_PATH
    #define TRACE_PATH  TRUE
#else
    #define TRACE_PATH  FALSE
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void APP_ZCL_cbGeneralCallback(tsZCL_CallBackEvent *psEvent);
PRIVATE void APP_ZCL_cbEndpointCallback(tsZCL_CallBackEvent *psEvent);
PRIVATE void vHandleAppACEServer(tsCLD_IASACECallBackMessage *psCallBackMessage);
PRIVATE void vAPP_ZCL_DeviceSpecific_Init(void);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
uint8 u8LastPanelStatus = E_CLD_IASACE_PANEL_STATUS_PANEL_DISARMED;
uint8 u8PanelStatusB4NotReadyToArm = E_CLD_IASACE_PANEL_STATUS_PANEL_DISARMED;
uint8 u8ConfigFlag = 0;
static uint8 u8LinkTxBuffer[200];
static uint8 u8cjpTxBuffer[200];
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PUBLIC tsHA_IASCIE sDevice;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: APP_ZCL_vInitialise
 *
 * DESCRIPTION:
 * Initialises ZCL related functions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vInitialise(void)
{
    teZCL_Status eZCL_Status;

    /* Initialise ZHA */
    eZCL_Status = eHA_Initialise(&APP_ZCL_cbGeneralCallback, apduZCL);
    if (eZCL_Status != E_ZCL_SUCCESS)
    {
        DBG_vPrintf(TRACE_ZCL, "\nErr: eHA_Initialise:%d", eZCL_Status);
    }

    /* Start the tick timer */
    OS_eStartSWTimer(APP_TickTimer, APP_TIME_MS(1000), NULL);

    /* Register EndPoint */
    eZCL_Status = eHA_RegisterIASCIEEndPoint(1,APP_ZCL_cbEndpointCallback,&sDevice);//���˵�1ע��ص�����

    if (eZCL_Status != E_ZCL_SUCCESS)
    {
            DBG_vPrintf(TRACE_ZCL, "Error: eApp_HA_RegisterEndpoint:%d\r\n", eZCL_Status);
    }

    vAPP_ZCL_DeviceSpecific_Init();

}
/****************************************************************************
 *
 * NAME: ZCL_Task
 *
 * DESCRIPTION:
 * ZCL Task for the Light
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(ZCL_Task)
{

    ZPS_tsAfEvent sStackEvent;
    tsZCL_CallBackEvent sCallBackEvent;
    sCallBackEvent.pZPSevent = &sStackEvent;

    /* If there is a stack event to process, pass it on to ZCL */
    sStackEvent.eType = ZPS_EVENT_NONE;
    if ( OS_eCollectMessage(APP_msgZpsEvents_ZCL, &sStackEvent) == OS_E_OK)
    {
        DBG_vPrintf(TRACE_ZCL, "\nZCL_Task event:%d",sStackEvent.eType);
        sCallBackEvent.eEventType = E_ZCL_CBET_ZIGBEE_EVENT;
        vZCL_EventHandler(&sCallBackEvent);  //����ZCL������
    }
}


/****************************************************************************
 *
 * NAME: Tick_Task
 *
 * DESCRIPTION:
 * Task kicked by the tick timer
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(Tick_Task)
{
    tsZCL_CallBackEvent sCallBackEvent;

    OS_eContinueSWTimer(APP_TickTimer, APP_TIME_MS(1000), NULL);
    sCallBackEvent.pZPSevent = NULL;
    sCallBackEvent.eEventType = E_ZCL_CBET_TIMER;
    vZCL_EventHandler(&sCallBackEvent);
    vPermitJoinIndication();
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: APP_ZCL_cbGeneralCallback
 *
 * DESCRIPTION:
 * General callback for ZCL events
 *���������˵��ZCL��������ò���
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_ZCL_cbGeneralCallback(tsZCL_CallBackEvent *psEvent)
{
    switch (psEvent->eEventType)
    {

    case E_ZCL_CBET_LOCK_MUTEX:
        break;

    case E_ZCL_CBET_UNLOCK_MUTEX:
        break;

    case E_ZCL_CBET_UNHANDLED_EVENT:
        DBG_vPrintf(TRACE_ZCL, "\nEVT:General Unhandled Event\r\n");
        break;

    case E_ZCL_CBET_READ_ATTRIBUTES_RESPONSE:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: Read attributes response");
        break;

    case E_ZCL_CBET_READ_REQUEST:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: Read request");
        break;

    case E_ZCL_CBET_DEFAULT_RESPONSE:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: Default response");
        break;

    case E_ZCL_CBET_ERROR:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: Error");
        break;

    case E_ZCL_CBET_TIMER:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: Timer");
        break;

    case E_ZCL_CBET_ZIGBEE_EVENT:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: ZigBee");
        break;

    case E_ZCL_CBET_CLUSTER_CUSTOM:
        DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Custom");
        break;

    default:
        DBG_vPrintf(TRACE_ZCL, "\nInvalid event type");
        break;
    }
}
/****************************************************************************
 *
 * NAME: APP_ZCL_cbEndpointCallback
 *
 * DESCRIPTION:
 * Endpoint specific callback for ZCL events
 *����˵�1�Ĺ���ZCL���������
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_ZCL_cbEndpointCallback(tsZCL_CallBackEvent *psEvent)
{

    DBG_vPrintf(TRACE_ZCL, "\nEntering cbZCL_Endpoint 1 Callback");
    DBG_vPrintf(TRACE_ZCL, "eEventType %02x\n" ,psEvent->eEventType);
    static uYcl tycl;
    static uint16 netaddr;
    static uint16 tclusterID;
    static uint16 tattrID;
    static uint16 u16Length=0;
    static eZCL_Frametype ZCL_Frametype=0;
    static uint8 cjp_commandID=0;
    static uint8 tattr_num =0;
    switch (psEvent->eEventType)
    {

    case E_ZCL_CBET_LOCK_MUTEX:
        break;

    case E_ZCL_CBET_UNLOCK_MUTEX:
        break;

    case E_ZCL_CBET_UNHANDLED_EVENT:
         DBG_vPrintf(TRACE_ZCL, "\nEP EVT:Endpoint Unhandled event");
        break;

        //�յ��ϱ�attribute����ÿһ��attribute����һ������Ϣ.
        //�ϱ�������ԡ�д�����Իظ�����ȡ���Իظ�ʱ��ÿ����һ�����Ծͻ�ִ������ط�����һ֡���ݵ���������ȫ����������Ժ�ͻ���ã�E_ZCL_CBET_REPORT_ATTRIBUTES��
    case E_ZCL_CBET_WRITE_ATTRIBUTES_RESPONSE:
    case E_ZCL_CBET_READ_INDIVIDUAL_ATTRIBUTE_RESPONSE:
    case E_ZCL_CBET_REPORT_INDIVIDUAL_ATTRIBUTE:
    {
    	uint16    u16SizeOfAttribute = 0;
        uint8     u16Elements =  0;
        uint16    i =  0;
        DBG_vPrintf(TRACE_APP_UART,"Into E_ZCL_CBET_REPORT_INDIVIDUAL_ATTRIBUTE ");

        //����ԭ��ַ�����ͽ���MAC��ַ�ĸ�ֵ��ԭ��ַ�����Ϳ���������̵�ַ��MAC��ַ
        if(psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8SrcAddrMode != 0)
        {
        	tycl.sYCL.Mac = ZPS_u64AplZdoLookupIeeeAddr(psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr);
        }
        else
        {
        	tycl.sYCL.Mac = psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u64Addr;
        }
        tclusterID = psEvent->pZPSevent->uEvent.sApsDataIndEvent.u16ClusterId;
        tattrID  = psEvent->uMessage.sIndividualAttributeResponse.u16AttributeEnum;
        DBG_vPrintf(TRACE_APP_UART,"tclusterID = %04x" , tclusterID);
        DBG_vPrintf(TRACE_APP_UART,"tattrID = %04x" , tattrID);
        //���Ե����ݸ�ʽ
        switch ( psEvent->uMessage.sIndividualAttributeResponse.eAttributeDataType )
        {
             case(E_ZCL_OSTRING):
             case(E_ZCL_CSTRING):
             	 DBG_vPrintf(TRACE_APP_UART,"receive a E_ZCL_OSTRING attr");
             	 if ( psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData != NULL )
             	 {
             		 u16Elements =  ( (uint8*)psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData ) [ 0 ];
             	 }
             	 else
             	 {
             		 u16Elements   =  0 ;
             	 }
             	 break;
              case(E_ZCL_LOSTRING):
              case(E_ZCL_LCSTRING):
                   if ( psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData != NULL )
                   {
                       u16Elements =  ( (uint16*)psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData ) [ 0 ];
                   }
                   else
                   {
                       u16Elements   =  0 ;
                   }
               break;
               default:
                   u16Elements   =  1;
                   break;
            }
        	u16SizeOfAttribute =  APP_u16GetAttributeActualSize ( psEvent->uMessage.sIndividualAttributeResponse.eAttributeDataType, u16Elements );//���Ե����ݳ���
        	if ( u16SizeOfAttribute !=  0 )
        	{
        		tattr_num++;
        		 //��ͬ��clusterID�в�ͬ�������ʽ
        		if(tclusterID == 0x0000)  //basic clusterID
        		{
        			if(tattrID == E_CLD_BAS_ATTR_ID_ATTR_SW_TABLE)
        			{
        				ZCL_Frametype=E_ZCL_FRAME_DEV_SW_MODEL_DATA_REPORT;//������֡���ݴ������ת��ģ��
        			}
        			else if(tattrID == E_CLD_BAS_ATTR_ID_M_YCL)
        			{
        				ZCL_Frametype = E_ZCL_FRAME_DEV_INF_DATA_REPORT;//������֡���ݴ�������豸������Ϣ
        			}

        		}
        		else if(tclusterID == 0x0019) //OTA
        		{
                    break;
        		}
        		else  //�������е�clusterID
        		{
        			if(tattrID == E_CLD_COMMON_ATTR_ID_DEV_STATUS)
        			{
        				if(*( ( uint8* ) psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData) == DEV_STATUS_ALARM)
        				{
        					ZCL_Frametype = E_ZCL_FRAME_ALARM_DATA_REPORT; //������֡���ݴ�������Ǳ�������
        					cjp_commandID = CJP_END_ALARM_DATA_REPORT_REQ;
        				}
        				else if(*( ( uint8* ) psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData) == DEV_STATUS_HEART_DATA)
        				{
        					ZCL_Frametype = E_ZCL_FRAME_HEART_DATA_REPORT; //������֡���ݴ��������������
        					cjp_commandID = CJP_END_HEART_DATA_REPORT_REQ;
        				}
        				else
        				{
        					ZCL_Frametype = E_ZCL_FRAME_NORMAL_DATA_REPORT; //������֡���ݴ�������������ݻ���ͨ����
        					cjp_commandID = CJP_END_NORMAL_DATA_REPORT_REQ;
        				}
        			 }

        		}
        		if ( u16Elements >0)
        		{

        			ZNC_BUF_U16_UPD(&u8LinkTxBuffer [u16Length], tattrID, u16Length) ;//����IDд��BUF
        			ZNC_BUF_U8_UPD(&u8LinkTxBuffer [u16Length], psEvent->uMessage.sIndividualAttributeResponse.eAttributeDataType, u16Length) ;//������������д��BUF
        			if( ( psEvent->uMessage.sIndividualAttributeResponse.eAttributeDataType ==  E_ZCL_OSTRING ) ||
        					( psEvent->uMessage.sIndividualAttributeResponse.eAttributeDataType ==  E_ZCL_CSTRING ) )
        			{

        				tsZCL_OctetString sString = *( ( tsZCL_OctetString* ) psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData );
        				ZNC_BUF_U8_UPD  ( &u8LinkTxBuffer [u16Length],   sString.u8Length,    u16Length );
        				for(i=0;i<sString.u8Length;i++)
        				{
        					ZNC_BUF_U8_UPD  ( &u8LinkTxBuffer [u16Length],   ( ( uint8* ) sString.pu8Data )[i], u16Length );
        				}
        			}
        			//uint8
        			else if ( u16SizeOfAttribute / u16Elements == sizeof(uint8) )
        			{
        				uint8    u8value =  *( ( uint8* ) psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData );
        				ZNC_BUF_U8_UPD  ( &u8LinkTxBuffer [u16Length],   u8value,    u16Length );
        			}
        			//uint16
        			else if ( u16SizeOfAttribute / u16Elements == sizeof(uint16) )
        			{
        				ZNC_BUF_U16_UPD( &u8LinkTxBuffer [u16Length],  *((uint16 *)(psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData)), u16Length);
        				//App_u16BufferReadNBO ( &u8LinkTxBuffer [u16Length],  "h",  psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData);
        				//u16Length += sizeof(uint16);
        			}
        			//uint32
        			else if ( u16SizeOfAttribute / u16Elements == sizeof(uint32) )
        			{
        				ZNC_BUF_U32_UPD( &u8LinkTxBuffer [u16Length],  *((uint32 *)(psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData)), u16Length);
        				//App_u16BufferReadNBO ( &u8LinkTxBuffer [u16Length],  "w",  psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData);
        				//u16Length += sizeof(uint32);
        			}
        			//uint64
        			else if ( u16SizeOfAttribute / u16Elements == sizeof(uint64) )
        			{
        				ZNC_BUF_U64_UPD( &u8LinkTxBuffer [u16Length], *((uint64 *)(psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData)) , u16Length);
        				//App_u16BufferReadNBO ( &u8LinkTxBuffer [u16Length],  "l",  psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData);
        				//u16Length += sizeof(uint64);
        			}


        		}
        	}

               /* Send event upwards */
            if((psEvent->eEventType == E_ZCL_CBET_READ_INDIVIDUAL_ATTRIBUTE_RESPONSE))
            {
            	ZCL_Frametype = E_ZCL_FRAME_READ_INDIVIDUAL_ATTRIBUTE_RESPONSE;
            	cjp_commandID = CJP_END_READ_ATTR_RESP;
            	DBG_vPrintf(TRACE_APP_UART,"E_ZCL_CBET_READ_INDIVIDUAL_ATTRIBUTE_RESPONSE ");
            	fEndDev_ReportAttr_Handle(tycl.sYCL.Mac , tclusterID ,cjp_commandID ,&u8LinkTxBuffer[0] , tattr_num , u16Length);
             	u16Length=0;//��ʼ�����µ�һ֡����
				ZCL_Frametype = 0;//׼�������µ�һ֡����
				cjp_commandID = 0;
				tattr_num = 0;
            }
            else if((psEvent->eEventType == E_ZCL_CBET_REPORT_INDIVIDUAL_ATTRIBUTE))
            {

            }
            else if((psEvent->eEventType == E_ZCL_CBET_WRITE_ATTRIBUTES_RESPONSE))
            {
            	ZCL_Frametype = E_ZCL_FRAME_WRITE_ATTRIBUTES_RESPONSE;
            	cjp_commandID = CJP_END_WRITE_ATTR_RESP;
            	if(psEvent->uMessage.sIndividualAttributeResponse.eAttributeStatus == E_ZCL_CMDS_SUCCESS)
            	{
            		u8LinkTxBuffer[0]= CJP_SUCCESS;
            	}
            	else
            	{
            		u8LinkTxBuffer[0]= CJP_ERROR;
            	}
            	DBG_vPrintf(TRACE_APP_UART,"E_ZCL_FRAME_WRITE_ATTRIBUTES_RESPONSE ");
            	fEndDev_WriteAttr_Resp_Handle(tycl.sYCL.Mac , tclusterID ,cjp_commandID ,&u8LinkTxBuffer[0] , tattr_num , u16Length);
             	u16Length=0;//��ʼ�����µ�һ֡����
				ZCL_Frametype = 0;//׼�������µ�һ֡����
				cjp_commandID = 0;
				tattr_num = 0;
            }

          }
          break;
           //�յ��Ѿ��Ե����ڵ������attributes������ɣ����ɴ���Ϣ
    case   E_ZCL_CBET_REPORT_ATTRIBUTES:
    	//���ݽ��յ���֡���ͽ��д���
    	DBG_vPrintf(TRACE_APP_UART,"A frame finish ");
    	DBG_vPrintf(TRACE_APP_UART," MAC address = %16x" , tycl.sYCL.Mac);
    	DBG_vPrintf(TRACE_APP_UART," clusterID   = %04x" , tclusterID);
    	DBG_vPrintf(TRACE_APP_UART," commandID   = %02x" , cjp_commandID);
    	DBG_vPrintf(TRACE_APP_UART," attr num   = %01x" , tattr_num);
    	DBG_vPrintf(TRACE_APP_UART," bufLength   = %02x" , u16Length);
    	switch(ZCL_Frametype)
    	{
    		case E_ZCL_FRAME_DEV_INF_DATA_REPORT: //�豸������Ϣ�ϱ�����
    			DBG_vPrintf(TRACE_APP_UART,"E_ZCL_FRAME_DEV_INF_DATA_REPORT ");
    			fEndDev_BasicInf_Handle(tycl.sYCL.Mac , tclusterID ,cjp_commandID ,&u8LinkTxBuffer[0], tattr_num , u16Length);
    			break;
    		case E_ZCL_FRAME_DEV_SW_MODEL_DATA_REPORT://�豸ת��ģ�ʹ���
    			DBG_vPrintf(TRACE_APP_UART,"E_ZCL_FRAME_DEV_SW_MODEL_DATA_REPORT ");
    			fEndDev_SwModle_Handle(tycl.sYCL.Mac , tclusterID ,cjp_commandID ,&u8LinkTxBuffer[0], tattr_num , u16Length);
    			break;
    		case E_ZCL_FRAME_WRITE_ATTRIBUTES_RESPONSE:// �豸д���Իظ�����
    			DBG_vPrintf(TRACE_APP_UART,"E_ZCL_FRAME_WRITE_ATTRIBUTES_RESPONSE ");
    			fEndDev_WriteAttr_Resp_Handle(tycl.sYCL.Mac , tclusterID ,cjp_commandID ,&u8LinkTxBuffer[0] , tattr_num , u16Length);
    			break;
    		case E_ZCL_FRAME_HEART_DATA_REPORT:
    		case E_ZCL_FRAME_NORMAL_DATA_REPORT:
    		case E_ZCL_FRAME_ALARM_DATA_REPORT:
    		case E_ZCL_FRAME_READ_INDIVIDUAL_ATTRIBUTE_RESPONSE://�豸�ϱ����ԡ������Իظ�����
    		//����ָ��  --  ������Ч����   ���Ը���  ---   ����ID----
    			DBG_vPrintf(TRACE_APP_UART,"E_ZCL_FRAME_NORMAL_DATA_REPORT ");
    			fEndDev_ReportAttr_Handle(tycl.sYCL.Mac , tclusterID ,cjp_commandID ,&u8LinkTxBuffer[0] , tattr_num , u16Length);
    			break;
    		default :
    			break;

    	}
    	u16Length=0;//��ʼ�����µ�һ֡����
    	ZCL_Frametype = 0;//׼�������µ�һ֡����
    	cjp_commandID = 0;
    	tattr_num = 0;
    	DBG_vPrintf(TRACE_ZCL,"E_ZCL_CBET_REPORT_ATTRIBUTES");
    		break;

    case E_ZCL_CBET_REPORT_INDIVIDUAL_ATTRIBUTES_CONFIGURE:
        {
            tsZCL_AttributeReportingConfigurationRecord    *psAttributeReportingRecord= &psEvent->uMessage.sAttributeReportingConfigurationRecord;
            DBG_vPrintf(TRACE_ZCL,"Individual Configure attribute for Cluster = %d\n",psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum);
            DBG_vPrintf(TRACE_ZCL,"eAttributeDataType = %d\n",psAttributeReportingRecord->eAttributeDataType);
            DBG_vPrintf(TRACE_ZCL,"u16AttributeEnum = %d\n",psAttributeReportingRecord->u16AttributeEnum );
            DBG_vPrintf(TRACE_ZCL,"u16MaximumReportingInterval = %d\n",psAttributeReportingRecord->u16MaximumReportingInterval );
            DBG_vPrintf(TRACE_ZCL,"u16MinimumReportingInterval = %d\n",psAttributeReportingRecord->u16MinimumReportingInterval );
            DBG_vPrintf(TRACE_ZCL,"u16TimeoutPeriodField = %d\n",psAttributeReportingRecord->u16TimeoutPeriodField );
            DBG_vPrintf(TRACE_ZCL,"u8DirectionIsReceived = %d\n",psAttributeReportingRecord->u8DirectionIsReceived );
            DBG_vPrintf(TRACE_ZCL,"uAttributeReportableChange = %d\n",psAttributeReportingRecord->uAttributeReportableChange );
        }
        break;



    case E_ZCL_CBET_READ_ATTRIBUTES_RESPONSE:
        /* DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Read attributes response"); */
        break;

    case E_ZCL_CBET_READ_REQUEST:
        /* DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Read request"); */
        break;

    case E_ZCL_CBET_DEFAULT_RESPONSE:
        /* DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Default response"); */
        break;

    case E_ZCL_CBET_ERROR:
        /* DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Error"); */
        break;

    case E_ZCL_CBET_TIMER:
        /* DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Timer"); */
        break;

    case E_ZCL_CBET_ZIGBEE_EVENT:
        /* DBG_vPrintf(TRACE_ZCL, "\nEP EVT: ZigBee"); */
        break;

        /*����ĳ��Cluster���ض������ON/OFF cluster �е�ON���OFF����
         * �����ж�����һ��clusterID��Ȼ���ж�����һ�����Ȼ�������һ���������д���
         */
    case E_ZCL_CBET_CLUSTER_CUSTOM:
        DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Custom Cl %04x\n", psEvent->uMessage.sClusterCustomMessage.u16ClusterId);

        switch(psEvent->uMessage.sClusterCustomMessage.u16ClusterId)
        {
            case SECURITY_AND_SAFETY_CLUSTER_ID_IASZONE:
            {
                tsCLD_IASZoneCallBackMessage *psCallBackMessage = (tsCLD_IASZoneCallBackMessage *)psEvent->uMessage.sClusterCustomMessage.pvCustomData;
                uint16 u16ShortAddress = psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr;
                vHandleAppZoneClient(psCallBackMessage,u16ShortAddress);

            }
            break;

            case SECURITY_AND_SAFETY_CLUSTER_ID_IASACE:
            {
                uint8 Count = 0;
                bool bIsEnrolledACE = TRUE;
                tsCLD_IASACECallBackMessage *psCallBackMessage = (tsCLD_IASACECallBackMessage *)psEvent->uMessage.sClusterCustomMessage.pvCustomData;
                /* Only take request from an enrolled ACE device */
                Count = u8GetTableIndex(psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr);
                /* Check has device ever joined and enrolled */
                if((Count == 0xFF) ||
                        (sDiscovedZoneServers[Count].u8ZoneId == 0xFF))
                {
                    bIsEnrolledACE = FALSE;
                }
                /* For bypass if asked from un-enrolled device do not take request */
                if(psCallBackMessage->u8CommandId == E_CLD_IASACE_CMD_BYPASS)
                {
                    for(Count=0; Count < u8Discovered; Count++)
                    {
                        if((sDiscovedZoneServers[Count].bValid == TRUE) &&
                                (sDiscovedZoneServers[Count].u8ZoneId != 0xFF))
                        {
                            if(bIsEnrolledACE)
                                ZCL_BIT_CLEAR(uint8,sDiscovedZoneServers[Count].u8Config,CLD_IASACE_ZONE_CONFIG_FLAG_NOT_BYPASSED);
                            else
                                ZCL_BIT_SET(uint8,sDiscovedZoneServers[Count].u8Config,CLD_IASACE_ZONE_CONFIG_FLAG_NOT_BYPASSED);
                            eCLD_IASACESetZoneParameterValue (
                                        CIE_EP,
                                        E_CLD_IASACE_ZONE_PARAMETER_ZONE_CONFIG_FLAG,
                                        sDiscovedZoneServers[Count].u8ZoneId,
                                        sDiscovedZoneServers[Count].u8Config);
                        }
                    }
                }
                if(bIsEnrolledACE)
                {
                    vHandleAppACEServer(psCallBackMessage);
                }
            }
            break;

            case GENERAL_CLUSTER_ID_GROUPS:
            break;
            case GENERAL_CLUSTER_ID_IDENTIFY:
            break;
        }
        break;

    case E_ZCL_CBET_WRITE_INDIVIDUAL_ATTRIBUTE:
        DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Write Individual Attribute");
        break;

    case E_ZCL_CBET_CLUSTER_UPDATE:
        if (psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum == SECURITY_AND_SAFETY_CLUSTER_ID_IASACE)
        {
            /* Update local structure to save in PDM for any changes in zone parameter*/
            vDisplayPanel();

           /* PDM_eSaveRecordData( PDM_ID_APP_IASACE_ZONE_PARAM,
                                 (tsCLD_IASACE_ZoneParameter *)&sDevice.sIASACEServerCustomDataStructure.asCLD_IASACE_ZoneParameter[0],
                                 sizeof(tsCLD_IASACE_ZoneParameter) * CLD_IASACE_ZONE_TABLE_SIZE);*/
            PDM_eSaveRecordData( PDM_ID_APP_IASCIE_STRUCT,
                                 &sDiscovedZoneServers[0],
                                 sizeof(tsDiscovedZoneServers) * MAX_ZONE_SERVER_NODES);
        }
        DBG_vPrintf(TRACE_CIE_TASK, "Update Id %04x\n", psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum);
        break;

    default:
        DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Invalid evt type 0x%x", (uint8)psEvent->eEventType);
        break;
    }

}

/****************************************************************************
 *
 * NAME: vHandleAppACEServer
 *
 * DESCRIPTION:
 * Handles all the ACE sever received commands
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void vHandleAppACEServer(tsCLD_IASACECallBackMessage *psCallBackMessage)
{
    uint8 u8Alarmstatus=0,u8LastAlarmstatus = 0;
    uint8 u8PanelStatus=0;
    uint8 u8WarningMode=0,u8LastAudibleNotification = 0;
    uint8 u8StrobeDutyCycle=0;


    eCLD_IASACEGetPanelParameter (
        CIE_EP,                                             /*uint8                                       u8SourceEndPointId, */
        E_CLD_IASACE_PANEL_PARAMETER_PANEL_STATUS,          /*teCLD_IASACE_PanelParameterID               eParameterId, */
        &u8PanelStatus);

    eCLD_IASACEGetPanelParameter (
        CIE_EP,                                              /*uint8                                       u8SourceEndPointId, */
        E_CLD_IASACE_PANEL_PARAMETER_ALARM_STATUS,           /*teCLD_IASACE_PanelParameterID               eParameterId,*/
        &u8LastAlarmstatus);

    eCLD_IASACEGetPanelParameter (
        CIE_EP,                                              /*uint8                                       u8SourceEndPointId, */
        E_CLD_IASACE_PANEL_PARAMETER_AUDIBLE_NOTIFICATION,   /*teCLD_IASACE_PanelParameterID               eParameterId, */
        &u8LastAudibleNotification);

    switch (psCallBackMessage->u8CommandId)
    {
    case E_CLD_IASACE_CMD_ARM:
    {
        tsCLD_IASACE_ArmPayload  *psArmPayload=psCallBackMessage->uMessage.psArmPayload;
        if (psArmPayload->eArmMode == E_CLD_IASACE_ARM_MODE_DISARM)
        {
            if(u8PanelStatus != E_CLD_IASACE_PANEL_STATUS_PANEL_DISARMED){
                /* Make sure to stop exit/entry delay timer & accept the request */
                eCLD_IASACESetPanelParameter (
                        CIE_EP,
                        E_CLD_IASACE_PANEL_PARAMETER_SECONDS_REMAINING,
                        0);

                OS_eStopSWTimer(APP_EntryExitDelayTmr);

                /* if disarming the system stop all the warnings & change ePanelStatus to E_CLD_IASACE_PANEL_STATUS_PANEL_DISARMED
                 *  for ACE server Panel parameter */
                bStayInExitDelay = FALSE;
                vStartWarning(DISABLE_WARNING,DISABLE_WARNING,DISABLE_WARNING,DISABLE_WARNING);
                vSetPanelParamter(E_CLD_IASACE_PANEL_STATUS_PANEL_DISARMED,E_CLD_IASACE_ALARM_STATUS_NO_ALARM,E_CLD_IASACE_AUDIBLE_NOTIF_MUTE);
                vStartSquawk(SQUAWK_MODE_STROBE_AND_LEVEL_DISARMED);
            }
        }
        else
        {
            /* Move to arm state only if you are disarmed && no alarm on any zones*/
            if(u8PanelStatus == E_CLD_IASACE_PANEL_STATUS_PANEL_DISARMED)
            {
                switch (psArmPayload->eArmMode)
                {
                case E_CLD_IASACE_ARM_MODE_ARM_DAY_HOME_ZONES_ONLY:
                    u8PanelStatus = E_CLD_IASACE_PANEL_STATUS_PANEL_ARMING_STAY;
                    u8ConfigFlag = CLD_IASACE_ZONE_CONFIG_FLAG_DAY_HOME;
                    break;
                case E_CLD_IASACE_ARM_MODE_ARM_NIGHT_SLEEP_ZONES_ONLY:
                    u8PanelStatus = E_CLD_IASACE_PANEL_STATUS_PANEL_ARMING_NIGHT;
                    u8ConfigFlag = CLD_IASACE_ZONE_CONFIG_FLAG_NIGHT_SLEEP;
                    break;
                case E_CLD_IASACE_ARM_MODE_ARM_ALL_ZONES:
                    u8ConfigFlag = 0xFF;
                    u8PanelStatus = E_CLD_IASACE_PANEL_STATUS_PANEL_ARMING_AWAY;
                    break;
                default:
                    break;
                }
                if(bCheckNotReadyToArm(u8ConfigFlag))
                {
                    u8PanelStatusB4NotReadyToArm = E_CLD_IASACE_PANEL_STATUS_PANEL_DISARMED;
                    DBG_vPrintf(TRACE_CIE_TASK,"\nChanging state to not ready when check zones\n");
                    vSetPanelParamter(E_CLD_IASACE_PANEL_STATUS_PANEL_NOT_READY_TO_ARM,u8LastAlarmstatus,u8LastAudibleNotification);
                }
                else
                {
                    vSetPanelParamter(u8PanelStatus,E_CLD_IASACE_ALARM_STATUS_NO_ALARM,E_CLD_IASACE_AUDIBLE_NOTIF_MUTE);
                    u8LastPanelStatus = u8PanelStatus;
                    vStartArmingSystem();
                }
            }else
            {
                /* Change status to not ready to arm to send arm response back and not allowing ZCL to take action
                 * and then revert back to last panel state
                 */
                DBG_vPrintf(TRACE_CIE_TASK,"\nChanging state to not ready\n");
                u8PanelStatusB4NotReadyToArm = u8PanelStatus;
                vSetPanelParamter(E_CLD_IASACE_PANEL_STATUS_PANEL_NOT_READY_TO_ARM,u8LastAlarmstatus,u8LastAudibleNotification);
            }
        }
        break;
    }
    case E_CLD_IASACE_CMD_BYPASS:
    {
        tsCLD_IASACE_BypassPayload *psBypassPayload = psCallBackMessage->uMessage.psBypassPayload;
        /* Call Ace display for filling up the ACE string*/

        break;
    }
    case E_CLD_IASACE_CMD_EMERGENCY:
        u8Alarmstatus = E_CLD_IASACE_ALARM_STATUS_EMERGENCY;
        u8WarningMode  = WARNING_MODE_STROBE_AND_SIREN_LEVEL_EMERGENCY;
        u8StrobeDutyCycle = 50;
        break;
    case E_CLD_IASACE_CMD_FIRE:
        u8Alarmstatus = E_CLD_IASACE_ALARM_STATUS_FIRE;
        u8WarningMode  = WARNING_MODE_STROBE_AND_SIREN_LEVEL_FIRE;
        u8StrobeDutyCycle = 20;
        break;
    case E_CLD_IASACE_CMD_PANIC:
        u8Alarmstatus = E_CLD_IASACE_ALARM_STATUS_POLICE_PANIC;
        u8WarningMode  = WARNING_MODE_STROBE_AND_SIREN_LEVEL_BURGLAR;
        u8StrobeDutyCycle = 50;
        break;
    case E_CLD_IASACE_CMD_GET_ZONE_ID_MAP:
        break;
    case E_CLD_IASACE_CMD_GET_ZONE_INFO:
    {
        tsCLD_IASACE_GetZoneInfoPayload  *psGetZoneInfoPayload=psCallBackMessage->uMessage.psGetZoneInfoPayload;
        /* Call Ace display for filling up the ACE string*/

        break;
    }
    case E_CLD_IASACE_CMD_GET_PANEL_STATUS:
    {

        break;
    }
    case E_CLD_IASACE_CMD_GET_BYPASSED_ZONE_LIST:
    {

    }
    break;
    case E_CLD_IASACE_CMD_GET_ZONE_STATUS:
    {
        tsCLD_IASACE_GetZoneStatusPayload  *psGetZoneStatusPayload = psCallBackMessage->uMessage.psGetZoneStatusPayload;
        /* Call Ace display for filling up the ACE string*/

        /* If panel status was Not ready to arm , go back to original state */
        if(u8PanelStatus == E_CLD_IASACE_PANEL_STATUS_PANEL_NOT_READY_TO_ARM)
        {
            DBG_vPrintf(TRACE_CIE_TASK,"\nChanging state back to %d\n",u8PanelStatusB4NotReadyToArm);
            vSetPanelParamter(u8PanelStatusB4NotReadyToArm,u8LastAlarmstatus,u8LastAudibleNotification);
        }
        break;
    }

    default :
        break;

    }

    if(u8Alarmstatus)
    {
        /* If the Emergency, fire , and panic is received set the ePanelStatus to E_CLD_IASACE_PANEL_STATUS_PANEL_IN_ALARM
         * and issue warning to the warning device
         */
        eCLD_IASACESetPanelParameter (
                CIE_EP,                                        /*uint8                                       u8SourceEndPointId,*/
                E_CLD_IASACE_PANEL_PARAMETER_SECONDS_REMAINING,/*teCLD_IASACE_PanelParameterID               eParameterId,*/
                0);
        vSetPanelParamter(E_CLD_IASACE_PANEL_STATUS_PANEL_IN_ALARM,u8Alarmstatus,E_CLD_IASACE_AUDIBLE_NOTIF_DEFAULT_SOUND);
        /*Alarm condition for the Zone-Start WD*/
        vStartWarning(STROBE_LEVEL,ALARM_WARNING_DURATION,u8StrobeDutyCycle,u8WarningMode);
    }

    /* Update local structure and refresh display */

}

/****************************************************************************
 *
 * NAME: APP_EntryExitDelay
 *
 * DESCRIPTION:
 * Task kicked by Exit/Entry Delay timer
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_EntryExitDelay)
{
    uint8 u8SecondsRemaining = 0,u8PanelStatus = 0;

    /* If stuck in exit delay cause of some alarm on some zone, stay at exit delay */
    eCLD_IASACEGetPanelParameter(CIE_EP,E_CLD_IASACE_PANEL_PARAMETER_SECONDS_REMAINING,&u8SecondsRemaining);
    /* Find the panel status to check to keep sending warning only for exit delay*/
    eCLD_IASACEGetPanelParameter(CIE_EP,E_CLD_IASACE_PANEL_PARAMETER_PANEL_STATUS,&u8PanelStatus);

    if(bStayInExitDelay == FALSE)
    {
        /* decrement the seconds remaining every second to update */
        if(u8SecondsRemaining > 0)
        {
            u8SecondsRemaining--;
            eCLD_IASACESetPanelParameter (
                    CIE_EP,
                    E_CLD_IASACE_PANEL_PARAMETER_SECONDS_REMAINING,
                    u8SecondsRemaining);

        }
        OS_eContinueSWTimer(APP_EntryExitDelayTmr, APP_TIME_MS(1000), NULL );
    }else if((u8SecondsRemaining > 0) &&
            (u8PanelStatus == E_CLD_IASACE_PANEL_STATUS_PANEL_EXIT_DELAY))
    {
        /* Keep raising the alarm to WD for every one sec so that user can see the zone under alarm */
        vStartWarning(STROBE_LEVEL,1,50/*STROBE_DUTY_CYCLE*/,WARNING_MODE_STROBE_AND_SIREN_LEVEL_ENTRY_EXIT_DELAY);
        OS_eContinueSWTimer(APP_EntryExitDelayTmr, APP_TIME_MS(1000), NULL );
    }

    /* If in exit/entry delay timer has expired move to appropriate state
     *  with appropriate actions taken
     */
    if(u8SecondsRemaining == 0)
    {
        OS_eStopSWTimer(APP_EntryExitDelayTmr);

        /* If in exit delay timer has expired move ePanelStatus from arming to armed
         */
        if(u8PanelStatus == E_CLD_IASACE_PANEL_STATUS_PANEL_EXIT_DELAY)
        {
            switch(u8LastPanelStatus)
            {
                case E_CLD_IASACE_PANEL_STATUS_PANEL_ARMING_STAY:
                    u8PanelStatus = E_CLD_IASACE_PANEL_STATUS_PANEL_ARMED_DAY;
		            vStartSquawk(SQUAWK_MODE_STROBE_AND_LEVEL_ARMED);
                break;
                case E_CLD_IASACE_PANEL_STATUS_PANEL_ARMING_NIGHT:
                    u8PanelStatus = E_CLD_IASACE_PANEL_STATUS_PANEL_ARMED_NIGHT;
		            vStartSquawk(SQUAWK_MODE_STROBE_AND_LEVEL_ARMED);
                break;
                case E_CLD_IASACE_PANEL_STATUS_PANEL_ARMING_AWAY:
                    u8PanelStatus = E_CLD_IASACE_PANEL_STATUS_PANEL_ARMED_AWAY;
		            vStartSquawk(SQUAWK_MODE_STROBE_AND_LEVEL_ARMED);
                break;
                default:
                    break;
            }
        }else if(u8PanelStatus == E_CLD_IASACE_PANEL_STATUS_PANEL_ENTRY_DELAY)
        {
            /* If in exit delay timer has expired move ePanelStatus to E_CLD_IASACE_PANEL_STATUS_PANEL_IN_ALARM
             * and start issue to warning device
             */
            u8PanelStatus = E_CLD_IASACE_PANEL_STATUS_PANEL_IN_ALARM;
            vStartWarning(STROBE_LEVEL,ALARM_WARNING_DURATION,STROBE_DUTY_CYCLE,WARNING_MODE_STROBE_AND_SIREN_LEVEL_BURGLAR);
        }

        eCLD_IASACESetPanelParameter (
                CIE_EP,                                   /*uint8                                       u8SourceEndPointId,*/
                E_CLD_IASACE_PANEL_PARAMETER_PANEL_STATUS,/*teCLD_IASACE_PanelParameterID               eParameterId,*/
                u8PanelStatus);


       /* PDM_eSaveRecordData( PDM_ID_APP_IASACE_PANEL_PARAM,
                             (tsCLD_IASACE_PanelParameter *)&sDevice.sIASACEServerCustomDataStructure.sCLD_IASACE_PanelParameter,
                             sizeof(tsCLD_IASACE_PanelParameter));*/
    }

}

/****************************************************************************
 *
 * NAME: bCheckNotReadyToArm
 *
 * DESCRIPTION:
 * Check whether system is not ready to arm
 *
 * RETURNS:
 * bool
 *
 ****************************************************************************/
PUBLIC bool bCheckNotReadyToArm(uint8 u8ConfigFlag)
{
    int Count;
     /* Check for any pending warnings before arming a system,
      * if any of zone is in alarm change the ePanelStatus to E_CLD_IASACE_PANEL_STATUS_PANEL_NOT_READY_TO_ARM
      *  for ACE server Panel parameter */
     for(Count=0; Count < u8Discovered; Count++)
     {
         if(sDiscovedZoneServers[Count].bValid == TRUE)
         {
             if(sDiscovedZoneServers[Count].u16ZoneStatus != 0 &&
                     (sDiscovedZoneServers[Count].u8Config & u8ConfigFlag || u8ConfigFlag == 0xFF) &&
                     (!(sDiscovedZoneServers[Count].u8ArmBypass & CLD_IASACE_ZONE_STATUS_FLAG_BYPASS))){
                 return TRUE;
             }
         }
     }
     return FALSE;
}

/****************************************************************************
 *
 * NAME: vStartArmingSystem
 *
 * DESCRIPTION:
 * Starts to arm the sytem
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vStartArmingSystem(void)
{
    /* Enter into the exit delay mode before arming the system
     * by changing the ePanelStatus to E_CLD_IASACE_PANEL_STATUS_PANEL_EXIT_DELAY
     *  for ACE server Panel parameter */
    eCLD_IASACESetPanelParameter (
            CIE_EP,
            E_CLD_IASACE_PANEL_PARAMETER_SECONDS_REMAINING,
            CLD_IASACE_PANEL_PARAMTER_SECONDS_REMAINING);
    vSetPanelParamter(E_CLD_IASACE_PANEL_STATUS_PANEL_EXIT_DELAY,E_CLD_IASACE_ALARM_STATUS_NO_ALARM,E_CLD_IASACE_AUDIBLE_NOTIF_DEFAULT_SOUND);
    /*Resoultion of this timer goes only till 1 mins */
    OS_eStopSWTimer(APP_EntryExitDelayTmr);
    OS_eStartSWTimer(APP_EntryExitDelayTmr, APP_TIME_MS(1000), NULL );
    vStartWarning(STROBE_LEVEL,ENTRY_EXIT_DELAY_WARNING_DURATION,50/*STROBE_DUTY_CYCLE*/,WARNING_MODE_STROBE_AND_SIREN_LEVEL_ENTRY_EXIT_DELAY);
}

/****************************************************************************
 *
 * NAME: vAPP_ZCL_DeviceSpecific_Init
 *
 * DESCRIPTION:
 * ZCL Device Specific initialization
 *
 * PARAMETER: void
 *
 * RETURNS: void
 *
 ****************************************************************************/
PRIVATE void vAPP_ZCL_DeviceSpecific_Init(void)
{
    /* Initialize the strings in Basic */
    memcpy(sDevice.sBasicServerCluster.au8ManufacturerName, "NXP", CLD_BAS_MANUF_NAME_SIZE);
    memcpy(sDevice.sBasicServerCluster.au8ModelIdentifier, "ZHA-CIE", CLD_BAS_MODEL_ID_SIZE);
    memcpy(sDevice.sBasicServerCluster.au8DateCode, "20150112", CLD_BAS_DATE_SIZE);
}



/*
 * �����ն˵��ϱ��Ļ�����Ϣ����
 * mac:�ն˵�MAC��ַ
 * clusterID:clusterID
 * commandID :CJP Э�������ID
 * sdata :����ָ��  ��������ݸ�ʽΪ{����ID ,�������� ,����  ,����ID ,�������� ,����  ,.........}
 * u8attrnum :���������Եĸ���
 * u16len :����ĳ���(�ֽ�)
 */
PUBLIC CJP_Status fEndDev_BasicInf_Handle(uint64 mac ,uint16 clusterID ,uint8 commandID , uint8 * sdata , uint8 u8attrnum , uint16 u16len)
{

	sEnddev_BasicInf tEnddev_BasicInf;
	sDevJoin_Notice  tDevJoin_Notice;
	uint16 buflen=0;
	uint8 u8SizeOfAttribute=0;
	uint8 cjp_buflen=0;

	sAttr_Charact tAttr_Charact[6]={
			    {E_ZCL_UINT8  ,  (uint32)(&((sDevJoin_Notice*)(0))->bJoinType) ,1},
				{E_ZCL_OSTRING , (uint32)(&((sDevJoin_Notice*)(0))->M_YCL), YCL_LENGTH},
				{E_ZCL_OSTRING , (uint32)(&((sDevJoin_Notice*)(0))->M_Soft_Ver) , SV_LENGTH},
				{E_ZCL_OSTRING , (uint32)(&((sDevJoin_Notice*)(0))->M_Hard_ver) , HV_LENGTH},
				{E_ZCL_OSTRING , (uint32)(&((sDevJoin_Notice*)(0))->S_Soft_Ver) , SV_LENGTH},
				{E_ZCL_UINT16 ,  (uint32)(&((sDevJoin_Notice*)(0))->HeartCyc) , 2}
		};

	if(u8attrnum != 6)
	{
		return CJP_SUCCESS;
	}
	tDevJoin_Notice.bJoinType =join_way;
	buflen+= 3;
	if(*(sdata+buflen) != sizeof(uYcl))
	{
		return CJP_ERROR;
	}
	buflen++;
	memcpy((uint8 *)&(tDevJoin_Notice.M_YCL),sdata+buflen,sizeof(uYcl));
	buflen+=sizeof(uYcl);

	buflen+=3;
	tEnddev_BasicInf.clusterID = BUILD_UINT16(*(sdata+buflen+1), *(sdata+buflen));
	buflen+=sizeof(uint16);

	buflen+=3;
	tDevJoin_Notice.HeartCyc= BUILD_UINT16(*(sdata+buflen+1), *(sdata+buflen));
	DBG_vPrintf(TRACE_APP_UART," HeartCyc = %04x" , tDevJoin_Notice.HeartCyc);
	buflen+=sizeof(uint16);

	buflen+=3;
	if(*(sdata+buflen) != sizeof(uSoft_Ver))
	{
		return CJP_ERROR;
	}
	buflen++;
	memcpy((uint8 *)&(tDevJoin_Notice.M_Soft_Ver),sdata+buflen,sizeof(uSoft_Ver));
	buflen+=sizeof(uSoft_Ver);

	buflen+=3;
	if(*(sdata+buflen) != sizeof(uHard_Ver))
	{
		return CJP_ERROR;
	}
	buflen++;
	memcpy((uint8 *)&(tDevJoin_Notice.M_Hard_ver),sdata+buflen,sizeof(uHard_Ver));
	buflen+=sizeof(uHard_Ver);

	buflen+=3;
	if(*(sdata+buflen) != sizeof(uSoft_Ver))
	{
		return CJP_ERROR;
	}
	buflen++;
	memcpy((uint8 *)&(tDevJoin_Notice.S_Soft_Ver),sdata+buflen,sizeof(uSoft_Ver));
	buflen+=sizeof(uSoft_Ver);


	tEnddev_BasicInf.ycl =tDevJoin_Notice.M_YCL;
	tEnddev_BasicInf.hearttime =tDevJoin_Notice.HeartCyc;
	tEnddev_BasicInf.Msoftver = BUILD_UINT16(tDevJoin_Notice.M_Soft_Ver.sSoft_Ver.Sv_Secv_Num , tDevJoin_Notice.M_Soft_Ver.sSoft_Ver.Sv_Mainv_Num );
	tEnddev_BasicInf.Hardver =tDevJoin_Notice.M_Hard_ver.sHard_Ver.Hv_TecPro ;
	tEnddev_BasicInf.Ssoftver = BUILD_UINT16(tDevJoin_Notice.S_Soft_Ver.sSoft_Ver.Sv_Secv_Num , tDevJoin_Notice.S_Soft_Ver.sSoft_Ver.Sv_Mainv_Num );
	tEnddev_BasicInf.Factcode = tDevJoin_Notice.M_Hard_ver.sHard_Ver.Hv_Dev_Company;
	//���豸�б�������豸�����������Ϣ
	add_dev_data_manage(tEnddev_BasicInf);
	//��JNI�㷢���豸����֪ͨ��
	Frame_Seq++;
	cjp_buflen = fCJP_Attr_Handle(&u8cjpTxBuffer[0] , (uint8 *)&tDevJoin_Notice, tAttr_Charact , 6);
	return fCJP_Tx_Coor(CIE_Ycl , CJP_DEV_JOINED_NOTICE, &u8cjpTxBuffer[0] , cjp_buflen); //֪ͨ�ϲ��豸��ӳɹ�

}




/*
 * �����ն˵��ϱ�������ģ��ת������
 * mac:�ն˵�MAC��ַ
 * clusterID:clusterID
 * commandID :CJP Э�������ID
 * sdata :����ָ��  ��������ݸ�ʽΪ{����ID ,�������� ,����  ,����ID ,�������� ,����  ,.........}
 * u8attrnum :���������Եĸ���
 * u16len :����ĳ���(�ֽ�)
 */
PUBLIC CJP_Status fEndDev_SwModle_Handle(uint64 mac ,uint16 clusterID ,uint8 commandID ,uint8 * sdata ,uint8 u8attrnum , uint16 u16len)
{
	sAttr_Model_Array tModel_Array;
	uint16 tclusterID=0;
	uint8 cjp_attrnum=0;
	uint8 len=0,i=0;
	tclusterID = BUILD_UINT16(*(sdata+4), *(sdata+3));
	len =  *(sdata+8);
	if((len%3) == 0)
	{
		cjp_attrnum = len/3;
	}
	else
	{
		return CJP_ERROR;
	}
	tModel_Array.Attr_Model[0].head.attrnum = cjp_attrnum;
	tModel_Array.Attr_Model[0].head.clusterID =tclusterID;
	for(i=0 ; i<cjp_attrnum ;i++)
	{
		tModel_Array.Attr_Model[i+1].attr.zattrID = BUILD_UINT16(*(sdata+10+3*i), *(sdata+9+3*i));
		tModel_Array.Attr_Model[i+1].attr.CattrID = *(sdata+11+3*i);
	}

	if(add_dev_model_data_manage( tModel_Array))//���ģ��
	{

	   DBG_vPrintf(TRACE_APP_UART," Save Sw model success ");
	   DBG_vPrintf(TRACE_APP_UART,"Attr_Model[0].head.clusterID %04x \n " ,tModel_Array.Attr_Model[0].head.clusterID);
	   DBG_vPrintf(TRACE_APP_UART,"Attr_Model[0].head.attrnum   %02x \n " ,tModel_Array.Attr_Model[0].head.attrnum);
	   DBG_vPrintf(TRACE_APP_UART,"Attr_Model[1].attr.CattrID   %02x \n " ,tModel_Array.Attr_Model[1].attr.CattrID);
	   DBG_vPrintf(TRACE_APP_UART,"Attr_Model[1].attr.zattrID   %04x \n " ,tModel_Array.Attr_Model[1].attr.zattrID);
	   DBG_vPrintf(TRACE_APP_UART,"Attr_Model[2].attr.CattrID   %02x \n " ,tModel_Array.Attr_Model[2].attr.CattrID);
	   DBG_vPrintf(TRACE_APP_UART,"Attr_Model[2].attr.zattrID   %04x \n " ,tModel_Array.Attr_Model[2].attr.zattrID);
	   DBG_vPrintf(TRACE_APP_UART,"Attr_Model[3].attr.CattrID   %02x \n " ,tModel_Array.Attr_Model[3].attr.CattrID);
	   DBG_vPrintf(TRACE_APP_UART,"Attr_Model[3].attr.zattrID   %04x \n " ,tModel_Array.Attr_Model[3].attr.zattrID);
	   DBG_vPrintf(TRACE_APP_UART,"Attr_Model[4].attr.CattrID   %02x \n " ,tModel_Array.Attr_Model[4].attr.CattrID);
	   DBG_vPrintf(TRACE_APP_UART,"Attr_Model[4].attr.zattrID   %04x \n " ,tModel_Array.Attr_Model[4].attr.zattrID);

	   DBG_vPrintf(TRACE_APP_UART,"Attr_Model[0].head.clusterID %04x \n " ,Attr_Model_Array[0].Attr_Model[0].head.clusterID);
	   DBG_vPrintf(TRACE_APP_UART,"Attr_Model[0].head.attrnum   %02x \n " ,Attr_Model_Array[0].Attr_Model[0].head.attrnum);
	   DBG_vPrintf(TRACE_APP_UART,"Attr_Model[1].attr.CattrID   %02x \n " ,Attr_Model_Array[0].Attr_Model[1].attr.CattrID);
	   DBG_vPrintf(TRACE_APP_UART,"Attr_Model[1].attr.zattrID   %04x \n " ,Attr_Model_Array[0].Attr_Model[1].attr.zattrID);
	   DBG_vPrintf(TRACE_APP_UART,"Attr_Model[2].attr.CattrID   %02x \n " ,Attr_Model_Array[0].Attr_Model[2].attr.CattrID);
	   DBG_vPrintf(TRACE_APP_UART,"Attr_Model[2].attr.zattrID   %04x \n " ,Attr_Model_Array[0].Attr_Model[2].attr.zattrID);
	   DBG_vPrintf(TRACE_APP_UART,"Attr_Model[3].attr.CattrID   %02x \n " ,Attr_Model_Array[0].Attr_Model[3].attr.CattrID);
	   DBG_vPrintf(TRACE_APP_UART,"Attr_Model[3].attr.zattrID   %04x \n " ,Attr_Model_Array[0].Attr_Model[3].attr.zattrID);
	   DBG_vPrintf(TRACE_APP_UART,"Attr_Model[4].attr.CattrID   %02x \n " ,Attr_Model_Array[0].Attr_Model[4].attr.CattrID);
	   DBG_vPrintf(TRACE_APP_UART,"Attr_Model[4].attr.zattrID   %04x \n " ,Attr_Model_Array[0].Attr_Model[4].attr.zattrID);
	   return CJP_SUCCESS;
	}
	return CJP_ERROR;
}



/*
 * �����ն˵�д���Իظ�����
 * mac:�ն˵�MAC��ַ
 * clusterID:clusterID
 * commandID :CJP Э�������ID
 * sdata :����ָ��  ��������ݸ�ʽΪ{����ID ,�������� ,����  ,����ID ,�������� ,����  ,.........}
 * u8attrnum :���������Եĸ���
 * u16len :����ĳ���(�ֽ�)
 */
PUBLIC CJP_Status fEndDev_WriteAttr_Resp_Handle(uint64 mac ,uint16 clusterID ,uint8 commandID ,uint8 * sdata ,uint8 u8attrnum , uint16 u16len)
{
	sEnddev_BasicInf tEnddev_BasicInf;
	sAttr_Model_Array tAttr_Model_Array;
	uint8 place=0;
	uYcl ycl;

	ycl.sYCL.Mac = mac;
	if(ZPS_u16AplZdoLookupAddr(mac)==0x0000)
	{
		return CJP_ERROR;
	}
		//��������ID��ת������
		//���豸�б��в����豸�Ļ�����Ϣ
	place = LocateElem(&Galist,ycl);
	if(place == 0)
	{
		return CJP_ERROR;
	}

	if(!GetElem(&Galist,place ,&tEnddev_BasicInf))
	{
		return CJP_ERROR;
	}
	if(!get_dev_model(tEnddev_BasicInf.clusterID, &tAttr_Model_Array))
	{
		return CJP_ERROR;
	}
    //ֻҪ�ܹ�ִ���������˵��д���Ѿ��ɹ�
	u8cjpTxBuffer[0] = 0x01;
	u8cjpTxBuffer[1] = E_ZCL_UINT8;
	u8cjpTxBuffer[2] = (uint8)*sdata;
	return fCJP_Tx_Coor(tEnddev_BasicInf.ycl , commandID, &u8cjpTxBuffer[0] ,3 );

}

/*
 * �����ն˵��ϱ����������ȡ���Իظ�����
 * mac:�ն˵�MAC��ַ
 * clusterID:clusterID
 * commandID :CJP Э�������ID
 * sdata :����ָ��  ��������ݸ�ʽΪ{����ID ,�������� ,����  ,����ID ,�������� ,����  ,.........}
 * u8attrnum :���������Եĸ���
 * u16len :����ĳ���(�ֽ�)
 */
PUBLIC CJP_Status fEndDev_ReportAttr_Handle(uint64 mac ,uint16 clusterID ,uint8 commandID ,uint8 * sdata ,uint8 u8attrnum , uint16 u16len)
{
	sEnddev_BasicInf tEnddev_BasicInf;
	sAttr_Model_Array tAttr_Model_Array;
	uint16 buflen=0;
	uint8 i=0;
	uint8 datatype=0;
	uint8 place=0;
	uint16 tattrID=0;
	uint16 u16Elements=0;
	uint16 u16SizeOfAttribute=0;
	uint8 cjp_attrID=0;
	uint8 cjp_buflen=0;
	uYcl ycl ;
	ycl.sYCL.Mac = mac;
	DBG_vPrintf(TRACE_APP_UART,"Into Report  device status data \n");
	if(ZPS_u16AplZdoLookupAddr(mac)==0x0000)
	{
		DBG_vPrintf(TRACE_APP_UART,"Cannot find device mac  \n");
		return CJP_ERROR;
	}
	//��������ID��ת������
	//���豸�б��в����豸�Ļ�����Ϣ
	place = LocateElem(&Galist,ycl);
	if(place == 0)
	{
		DBG_vPrintf(TRACE_APP_UART,"Cannot find device in device list  \n");
		return CJP_ERROR;
	}

	if(!GetElem(&Galist,place ,&tEnddev_BasicInf))
	{
		DBG_vPrintf(TRACE_APP_UART,"Cannot find device basic information  \n");
	    return CJP_ERROR;
	}
	if(!get_dev_model(tEnddev_BasicInf.clusterID, &tAttr_Model_Array))
	{
		DBG_vPrintf(TRACE_APP_UART,"Cannot find device SW model  \n");
	    return CJP_ERROR;
	}
	//��ӡ�ҵ����豸������Ϣ��ģ����Ϣ
	DBG_vPrintf(TRACE_APP_UART,"Device basic information Factcode %04x" ,tEnddev_BasicInf.Factcode);
	DBG_vPrintf(TRACE_APP_UART,"Device basic information Hardver %04x" ,tEnddev_BasicInf.Hardver);
	DBG_vPrintf(TRACE_APP_UART,"Device basic information Msoftver %04x" ,tEnddev_BasicInf.Msoftver);
	DBG_vPrintf(TRACE_APP_UART,"Device basic information Ssoftver %04x" ,tEnddev_BasicInf.Ssoftver);
	DBG_vPrintf(TRACE_APP_UART,"Device basic information clusterID %04x" ,tEnddev_BasicInf.clusterID);
	DBG_vPrintf(TRACE_APP_UART,"Device basic information hearttime %04x" ,tEnddev_BasicInf.hearttime);
	DBG_vPrintf(TRACE_APP_UART,"Device basic information YCL ");
	printf_array(&tEnddev_BasicInf.ycl  , sizeof(uYcl));


	DBG_vPrintf(TRACE_APP_UART,"Attr_Model[0].head.clusterID %04x \n " ,tAttr_Model_Array.Attr_Model[0].head.clusterID);
	DBG_vPrintf(TRACE_APP_UART,"Attr_Model[0].head.attrnum   %02x \n " ,tAttr_Model_Array.Attr_Model[0].head.attrnum);
	DBG_vPrintf(TRACE_APP_UART,"Attr_Model[1].attr.CattrID   %02x \n " ,tAttr_Model_Array.Attr_Model[1].attr.CattrID);
	DBG_vPrintf(TRACE_APP_UART,"Attr_Model[1].attr.zattrID   %04x \n " ,tAttr_Model_Array.Attr_Model[1].attr.zattrID);
	DBG_vPrintf(TRACE_APP_UART,"Attr_Model[2].attr.CattrID   %02x \n " ,tAttr_Model_Array.Attr_Model[2].attr.CattrID);
	DBG_vPrintf(TRACE_APP_UART,"Attr_Model[2].attr.zattrID   %04x \n " ,tAttr_Model_Array.Attr_Model[2].attr.zattrID);
	DBG_vPrintf(TRACE_APP_UART,"Attr_Model[3].attr.CattrID   %02x \n " ,tAttr_Model_Array.Attr_Model[3].attr.CattrID);
	DBG_vPrintf(TRACE_APP_UART,"Attr_Model[3].attr.zattrID   %04x \n " ,tAttr_Model_Array.Attr_Model[3].attr.zattrID);
	DBG_vPrintf(TRACE_APP_UART,"Attr_Model[4].attr.CattrID   %02x \n " ,tAttr_Model_Array.Attr_Model[4].attr.CattrID);
	DBG_vPrintf(TRACE_APP_UART,"Attr_Model[4].attr.zattrID   %04x \n " ,tAttr_Model_Array.Attr_Model[4].attr.zattrID);

	u8cjpTxBuffer[cjp_buflen] = u8attrnum*2;
	cjp_buflen++;
	if(u8attrnum !=0)
	{
		for(i=0 ; i<u8attrnum ; i++)
		{
			if(buflen>=u16len)
			{
				return CJP_ERROR;
			}
			tattrID = BUILD_UINT16(*(sdata+buflen+1), *(sdata+buflen));
			printf_array(sdata  , u16len);
			buflen += 2;
			datatype = *(uint8 *)(sdata+buflen);
			buflen++;
		//���Ե����ݸ�ʽ
			switch ( datatype)
			{
				case(E_ZCL_OSTRING):
				case(E_ZCL_CSTRING):
					u16Elements =  ( (uint8*)(sdata+buflen) ) [0]+1;
					break;
				default:
					u16Elements   =  1;
					break;
			}
			u16SizeOfAttribute =  APP_u16GetAttributeActualSize (datatype, u16Elements );//���Ե����ݳ���
			cjp_attrID = get_CJP_attrID(&tAttr_Model_Array , tattrID);
			DBG_vPrintf(TRACE_APP_UART,"Zigbee attrID  %04x \n " ,tattrID);
			DBG_vPrintf(TRACE_APP_UART,"cjp attrID   %02x \n " ,cjp_attrID);
			u8cjpTxBuffer[cjp_buflen] = E_ZCL_UINT8;
			cjp_buflen++;
			u8cjpTxBuffer[cjp_buflen] = cjp_attrID;
			cjp_buflen++;
			u8cjpTxBuffer[cjp_buflen] = datatype;
			cjp_buflen++;
			memcpy(&u8cjpTxBuffer[cjp_buflen],sdata+buflen ,u16SizeOfAttribute);
			cjp_buflen += u16SizeOfAttribute;
			buflen += u16SizeOfAttribute;

		}
		if(commandID != CJP_END_READ_ATTR_RESP)
		{
			Frame_Seq++;
		}

	    DBG_vPrintf(TRACE_APP_UART,"Report  device status data success %02x" ,cjp_buflen);
	    printf_array(&u8cjpTxBuffer[0],cjp_buflen);
		return fCJP_Tx_Coor(tEnddev_BasicInf.ycl , commandID, &u8cjpTxBuffer[0] , cjp_buflen);
	}


	return CJP_ERROR;
}


/*
 * ��8λ��16λ��32λ��64λ�������ݸ��Ƶ�ָ���ĵ�ַ�ϣ�Ϊʲ�ǲ�ֱ������ָ����и�ֵ�أ���Ϊ�ֽڶ������⡣
 */
PUBLIC uint16 App_u16BufferReadNBO ( uint8         *pu8Struct,
                                     const char    *szFormat,
                                     void          *pvData)
{
    uint8 *pu8Start = (uint8*)pvData;
    uint32 u32Offset = 0;

    if(!pu8Struct || !szFormat || !pvData)
    {
        return 0;
    }

    for(; *szFormat != '\0'; szFormat++)
    {
        if(*szFormat == 'b') {
            pu8Struct[u32Offset++] = *(uint8*)pvData++;
        } else if (*szFormat == 'h') {
            uint16 u16Val = *( uint8* )pvData++ << 8;
            u16Val |= *( uint8* )pvData;

            /* align to half-word boundary */
            u32Offset = ALIGN( sizeof(uint16), u32Offset );

            memcpy(pu8Struct + u32Offset, &u16Val, sizeof(uint16));

            u32Offset += sizeof(uint16);
        } else if (*szFormat == 'w') {
            uint32 u32Val = *( uint8* )pvData << 24;
            u32Val |= *( uint8* )pvData << 16;
            u32Val |= *( uint8* )pvData << 8;
            u32Val |= *( uint8* )pvData;

            /* align to word (32 bit) boundary */
            u32Offset = ALIGN(sizeof(uint32), u32Offset);

            memcpy(pu8Struct + u32Offset, &u32Val, sizeof(uint32));

            u32Offset += sizeof(uint32);
        } else if (*szFormat == 'l') {
        	uint64 u64Val;
            u64Val =  (uint64) *( uint8* )pvData << 56;
            u64Val |= (uint64) *( uint8* )pvData << 48;
            u64Val |= (uint64) *( uint8* )pvData << 40;
            u64Val |= (uint64) *( uint8* )pvData << 32;
            u64Val |= (uint64) *( uint8* )pvData << 24;
            u64Val |= (uint64) *( uint8* )pvData << 16;
            u64Val |= (uint64) *( uint8* )pvData << 8;
            u64Val |= (uint64) *( uint8* )pvData ;


            /*
             *  align to long long word (64 bit) boundary
             *  but relative to structure start
             */
            u32Offset = ALIGN(sizeof(uint64), u32Offset);

            memcpy(pu8Struct + u32Offset, &u64Val, sizeof(uint64));

            u32Offset += sizeof(uint64);

        } else if (*szFormat == 'a') {
            uint8 u8Size = *++szFormat;
            unsigned int i;

            for (i = 0; i < u8Size; i++) {
                *(pu8Struct + u32Offset) = *( uint8* )pvData;
                u32Offset++;
            }
        } else if (*szFormat == 'p') {
            pvData += *++szFormat;
        }
    }

    return (uint16)((uint8*)pvData - pu8Start);
}


//��ȡ�����������͵ĳ���
PUBLIC uint16 APP_u16GetAttributeActualSize ( uint32    u32Type , uint16    u16NumberOfItems )
{
    uint16    u16Size =  0;

    switch(u32Type)
    {
        case(E_ZCL_GINT8):
        case(E_ZCL_UINT8):
        case(E_ZCL_INT8):
        case(E_ZCL_ENUM8):
        case(E_ZCL_BMAP8):
        case(E_ZCL_BOOL):
        case(E_ZCL_OSTRING):
        case(E_ZCL_CSTRING):
           u16Size = sizeof(uint8);
        break;

        case(E_ZCL_LOSTRING):
        case(E_ZCL_LCSTRING):
        case(E_ZCL_STRUCT):
        case (E_ZCL_INT16):
        case (E_ZCL_UINT16):
        case (E_ZCL_ENUM16):
        case (E_ZCL_CLUSTER_ID):
        case (E_ZCL_ATTRIBUTE_ID):
           u16Size = sizeof(uint16);
        break;


        case E_ZCL_UINT24:
        case E_ZCL_UINT32:
        case E_ZCL_TOD:
        case E_ZCL_DATE:
        case E_ZCL_UTCT:
        case E_ZCL_BACNET_OID:
        case E_ZCL_INT24:
        case E_ZCL_FLOAT_SINGLE:
           u16Size = sizeof(uint32);
        break;

        case E_ZCL_UINT40:
        case E_ZCL_UINT48:
        case E_ZCL_UINT56:
        case E_ZCL_UINT64:
        case E_ZCL_IEEE_ADDR:
           u16Size = sizeof(uint64);
        break;

        default:
           u16Size = 0;
        break;
    }

    return ( u16Size * u16NumberOfItems );
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
