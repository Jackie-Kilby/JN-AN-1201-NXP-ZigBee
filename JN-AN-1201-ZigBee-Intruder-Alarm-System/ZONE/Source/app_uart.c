/*
 * app_uart.c
 *
 *  Created on: 2017��10��23��
 *      Author: 1
 */


/*********************************************************************
 * CONSTANTS
 */
#include <jendefs.h>
#include <AppApi.h>
#include <pwrm.h>
#include <os.h>
#include "os_gen.h"
#include <dbg.h>
#include <dbg_uart.h>
#include <app_exceptions.h>
#include <app_pdm.h>
#include <zps_nwk_pub.h>
#include <zps_apl_af.h>
#include <zps_apl_aib.h>
#include "zps_apl_zdp.h"
#include "zps_nwk_nib.h"
#include "zps_gen.h"
#include <app_timer_driver.h>
#include "pdm.h"
#include "pdum_gen.h"
#include "pdum_apl.h"
#include "app_uart.h"
#include "AppHardwareApi.h"
#include "zha_ZONE_node.h"
#include "app_zcl_ZONE_task.h"
#include "app_sleep_functions.h"
#include "zcl.h"
#include "zcl_common.h"
#include "app_events.h"
#include "PDM_IDs.h"
#include "IASZone.h"


#define TRACE_APP_UART    TRUE


#define UART_APP_PORT  E_AHI_UART_1


#if !defined( UART_APP_BAUD )
#define UART_APP_BAUD  E_AHI_UART_RATE_9600
#endif

// When the Rx buf space is less than this threshold, invoke the Rx callback.
#if !defined( UART_APP_THRESH )
#define UART_APP_THRESH  64
#endif

#if !defined( UART_APP_RX_SZ )
#define UART_APP_RX_SZ  128
#endif

#if !defined( UART_APP_TX_SZ )
#define UART_APP_TX_SZ  128
#endif

// Millisecs of idle time after a byte is received before invoking Rx callback.
#if !defined( UART_APP_IDLE )
#define UART_APP_IDLE  6
#endif

// Loopback Rx bytes to Tx for throughput testing.
#if !defined( UART_APP_LOOPBACK )
#define UART_APP_LOOPBACK  FALSE
#endif
/**/
#define UART_HEAD_H               0xAA
#define UART_HEAD_L               0xBB

#define CMD_KEY_RESP              0xA0
#define CMD_SENSOR_STATUS_RESP    0xA1
#define CMD_SENSOR_INFO_REQ       0xA2
#define CMD_SENSOR_STATUS_REQ     0XA3
#define CDM_SENSOR_TEST           0XA4


#define FAULT_ALARM_VALUE         0x11
#define ALARM_VALUE               0x55
#define NORMAL_VALUE              0xaa
#define TEXT_VALUE                0xbb
#define MUTE_VALUE                0x50
#define ILLEGAL_MOVE_VALUE        0xa0
#define UNNORMAL_VALUE            0x66


#define SUCCESS_RESP              0x00
#define ERROR_RESP                0x01
#define JOIN_NET_SUCCESS          0x03
#define JOIN_NET_ERROR            0x04
#define NON_JOIN_NET_SUCCESS      0x05
#define NON_JOIN_NET_ERROR        0x06

#define UART_LONG_KEY             0x01
#define UART_THREE_TIMES_KEY      0x02


#define UART_RX_NULL               0
#define UART_RX_HEAD               1
#define UART_RX_DATA               2
#define UART_RX_CRC                3
#define UART_RX_BUSY               4

#define UART_TX_MAX_NUM            10
#define UART_RX_MAX_NUM            50
#define UART_RX_DATA_MAX_NUM       0x0C  //������ݶ��ֽ�����
#define UART_HEAD_LEN              0x03  //����ͷ��3�ֽ�


#define UART_TIMEOUT_VALUE         100




#define  GAS_ALARM_DEV             0x1006  //GAS������
#define  SM_ALARM_DEV              0x0001  //SM������
#define  PIR_DETECTOR_DEV          0x0100  // PIR̽����
#define  ENMG_DETECTOR_DEV         0x0101  //�Ŵ�̽����
#define  WATER_ALARM_DEV           0x0004  //ˮ�б�����
#define  SOS_ALARM_DEV             0x0300  //SOS����

#define  WATER_QUALITY_DEV         0x4455 //ˮ�ʼ��


#define ALARM_TIMEOUT_VALUE        600000  //��ʱʱ��Ϊ10����,10���Ӻ��������
#define UART_DMA_RXBUF_LEN          256
#define UART_DMA_TXBUF_LEN          256

usLinkKey               sLinkKey={{0x5a, 0x69, 0x67, 0x42, 0x65, 0x65, 0x41, 0x6c,0x6c, 0x69, 0x61, 0x6e, 0x63, 0x65, 0x30, 0x39}};//����������Կ
   tsMS_Type_Sw MS_Type_Sw_Tab[]={
		{GAS_ALARM_DEV, 	YCLID_METHANE},
		{SM_ALARM_DEV,  	YCLID_SMOKE},
		{PIR_DETECTOR_DEV,  YCLID_PIR},
		{ENMG_DETECTOR_DEV, YCLID_ED},
		{WATER_ALARM_DEV,   YCLID_WA},
		{SOS_ALARM_DEV,     YCLID_CALL}
};



tsCluster_Basic_Attr   sCluster_Basic_Attr;
tsCluster_IASZONE_Attr sCluster_IASZONE_Attr;
/**/
tsEP_Dev_Inf  sEP_Dev_Inf={
		/*VALID_VALUE,                                                    //    Data_valid;
		{VALID_VALUE,DEF_HEARTBEAT_VALUE},                              // 	sEP_Dev_HeartBeat
		{{0}},                                                           //  				sM_YCL
		M_PROFILE_ID,                                                    //               M_ProfileID;
		M_CLUSTER_ID,                                                    // 			M_ClusterID;
		{{0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x18,0x03,0x11}},             // 			sM_Sv[10]ͨ��ģ�������汾
		{{0x48,0x00,0x00,0x00,0x00,0x40,0x60,0x17,0x12,XTHN_COMPANY,0x01}},// 			sM_Hv[11]ͨ��ģ���Ӳ���汾
		0x01,                                                              // 				Sensing_flag����ģ��ʹ��
		0xFF,                                                               //               S_valid_flag����ģ�� =������Ϣ����Ч�Լ��
		0xFF,                                                                //              Serve_CompanyCode���г��Ҵ���
		{{0}},                                                             // 				sS_YCL[12]����ģ���YCL
		{},                                                             // 				sS_Sv[10]����ģ�������汾
		{{0x48,0x00,0x00,0x00,0x00,0x60,0x80,0x18,0x03,STWE_COMPANY,0x00}}// 				sS_Hv[11]����ģ���Ӳ���汾*/
	};//�����豸������Ϣ;

const tsZCL_AttributeDefinition IASZoneClusterAttributeDefinitions[] = {
       { E_CLD_IASZONE_STATUS,                     E_ZCL_AF_RD|E_ZCL_AF_RP,            E_ZCL_UINT8,      (uint32)(&((tsCLD_IASZone*)(0))->e8ZoneState),0},
       { E_CLD_IASZONE_ZONE_POWER_VALUE,                E_ZCL_AF_RD|E_ZCL_AF_RP,            E_ZCL_UINT8,      (uint32)(&((tsCLD_IASZone*)(0))->e16ZoneType),0}

   };

const tsZCL_AttributeDefinition BasicClusterAttributeDefinitions[] = {
       { E_CLD_BASIC_M_YCL,                        E_ZCL_AF_RD|E_ZCL_AF_RP,            E_ZCL_OSTRING ,    (uint32)(&((tsCluster_Basic_Attr*)(0))->sM_YCL),0},
       { E_CLD_BASIC_M_CLUSTERID,                  E_ZCL_AF_RD|E_ZCL_AF_RP,            E_ZCL_UINT16,      (uint32)(&((tsCluster_Basic_Attr*)(0))->M_ClusterID),0},
       { E_CLD_BASIC_HEARTBEAT_VALUE,              E_ZCL_AF_RD|E_ZCL_AF_RP,            E_ZCL_UINT16,      (uint32)(&((tsCluster_Basic_Attr*)(0))->heartbeat_value),0},
       { E_CLD_BASIC_M_SV,                   	   E_ZCL_AF_RD|E_ZCL_AF_RP,            E_ZCL_OSTRING ,    (uint32)(&((tsCluster_Basic_Attr*)(0))->sM_Sv),0},
       { E_CLD_BASIC_M_HV,                         E_ZCL_AF_RD|E_ZCL_AF_RP,            E_ZCL_OSTRING ,    (uint32)(&((tsCluster_Basic_Attr*)(0))->sM_Hv),0},
       { E_CLD_BASIC_S_SV,                         E_ZCL_AF_RD|E_ZCL_AF_RP,            E_ZCL_OSTRING ,    (uint32)(&((tsCluster_Basic_Attr*)(0))->sS_Sv),0},

   };




tsSen_Status_Data  	 sSen_Status_Data;
uint8  nv_join_flag=0;
uint8  alarm_state=0;
uint64 Dev_IeeeAddr=0xFFFFFFFF;


/**/
static tsEP_SenDev_inf sSenDev_inf;
static uint8  Uart_Analysis_Step = UART_RX_NULL;
static uint8  Uart_DMA_RxBuf[UART_DMA_RXBUF_LEN];
static uint8  Uart_DMA_TxBuf[UART_DMA_TXBUF_LEN];
static uint8  Uart_SRxBuf[UART_RX_MAX_NUM+1];//
static uint8  Uart_STxBuf[UART_TX_MAX_NUM +1];
static uint16 Uart_SRxLen = 0;
static uint8 s_eUart_state=0;
static bool  chip_reset_flag=FALSE;
static bool start_join_flag=TRUE;

/**/
void vHwDeviceIntCallback(uint32 u32Device, uint32 u32ItemBitmap);
static void Uart_Analysis(void);
static uint8 CRC_Calculate(uint8 *pBuf, uint8 len);
static void  Uart_TxCMDData(uint8 cmd_type, uint8 value );
static void Uart_Rx_CMDDeal(uint8 *rx_buf);
static void vStartup(void);
static uint16 HalUARTWrite(uint8 u8Uart,uint8 *pu8Data,uint16 u16DataLength);
static void DevInf_init(void);
static usLinkKey Linkkey_Calculate(uYcl Ycl);



void Uart_Task_Init(void)
{
	User_Uart_Init();
	DevInf_init();
}

void DevInf_init(void)
{
	uint16 u16ByteRead;
	tsEP_Dev_Inf sTemp_Dev_Inf;
	sEP_Dev_Inf.sM_YCL.sYCL.Mac=ZPS_u64AplZdoGetIeeeAddr();//�豸��MAC��ַ
	 //��ȡ������EEROM�е�����
	PDM_eReadDataFromRecord(PDM_ID_APP_EP_DEV_INF,&sTemp_Dev_Inf,sizeof(tsEP_Dev_Inf),&u16ByteRead);
	  //���ݳ�ʼ��
	if(sTemp_Dev_Inf.Data_valid!=VALID_VALUE)
	{
		sEP_Dev_Inf.Data_valid = VALID_VALUE;
		sEP_Dev_Inf.M_ClusterID = 0x0500;
		sEP_Dev_Inf.M_ProfileID = 0x0104;
		sEP_Dev_Inf.S_valid_flag = VALID_VALUE;
		sEP_Dev_Inf.Sensing_flag = VALID_VALUE;
		sEP_Dev_Inf.Serve_CompanyCode = STWE_COMPANY;
		sEP_Dev_Inf.sEP_Dev_HeartBeat = DEF_HEARTBEAT_VALUE;

		sEP_Dev_Inf.sM_Hv.sHard_Ver.Hv_Dev_Company = STWE_COMPANY;
		sEP_Dev_Inf.sM_Hv.sHard_Ver.Hv_YCL_ID= YCLID_ED ;
		sEP_Dev_Inf.sM_Hv.sHard_Ver.Hv_Logo = 0x48;
		sEP_Dev_Inf.sM_Hv.sHard_Ver.Hv_TecPro = 0x0101;

		sEP_Dev_Inf.sM_Sv.sSoft_Ver.Sv_YCL_ID = YCLID_ED;
		sEP_Dev_Inf.sM_Sv.sSoft_Ver.Sv_Mainv_Num =0x01;
		sEP_Dev_Inf.sM_Sv.sSoft_Ver.Sv_Secv_Num = 0x00;

		sEP_Dev_Inf.sM_YCL.sYCL.YCL_ID = YCLID_ED;
		sEP_Dev_Inf.sM_YCL.sYCL.Mac = ZPS_u64AplZdoGetIeeeAddr();//�豸��MAC��ַ

		sEP_Dev_Inf.sS_Sv.sSoft_Ver.Sv_YCL_ID = YCLID_ED+1;
		sEP_Dev_Inf.sS_Sv.sSoft_Ver.Sv_Mainv_Num = 0x02;
		sEP_Dev_Inf.sS_Sv.sSoft_Ver.Sv_Secv_Num = 0x03;

		PDM_eSaveRecordData(PDM_ID_APP_EP_DEV_INF,&sEP_Dev_Inf,sizeof(tsEP_Dev_Inf));//���浽EEPROM��
		app_UartSendMesg(APP_U_EVENT_READ_DEV_INFO);//���Ͷ�ȡ�豸��Ϣ��Ϣ
	}
	else
	{
		sEP_Dev_Inf=sTemp_Dev_Inf;

	}
	//basic clusterID������Ϣ��ʼ��
	 sLinkKey=Linkkey_Calculate(sEP_Dev_Inf.sM_YCL);//����������Կ
	 sCluster_Basic_Attr.sM_YCL=sEP_Dev_Inf.sM_YCL;
	 sCluster_Basic_Attr.M_ClusterID=sEP_Dev_Inf.M_ClusterID;
	 sCluster_Basic_Attr.heartbeat_value=sEP_Dev_Inf.sEP_Dev_HeartBeat;
	 sCluster_Basic_Attr.sM_Hv=sEP_Dev_Inf.sM_Hv;
	 sCluster_Basic_Attr.sM_Sv=sEP_Dev_Inf.sM_Sv;
	 sCluster_Basic_Attr.Serve_CompanyCode=sEP_Dev_Inf.Serve_CompanyCode;
	 sCluster_Basic_Attr.sS_Sv=sEP_Dev_Inf.sS_Sv;

}

OS_TASK(APP_taskuart)
{
	APP_uartEvent sUartEvent;
    /* check if any messages to collect */
    if (OS_E_OK != OS_eCollectMessage(APP_msgUartEvents, &sUartEvent))
   {
    	switch (sUartEvent.eType)
    	{
    	    /*��������*/
    	 case APP_U_EVENT_ESEND_DATA:
    		 app_SendsSatusDate();
    		 DBG_vPrintf(TRACE_APP_UART, "app_u_event_send_data");
    		 break;
    		 //�����豸��Ϣ
    	 case APP_U_EVENT_ESEND_DEV_INFO:
    		 app_SendDveInf();
    		 app_SendSW_Model();
    		 break;
    		 /*�豸����*/
    	 case APP_U_EVENT_JOINORUNJOIN:

    		 if(ZPS_E_SUCCESS!=ZPS_eAplZdoLeaveNetwork(0, FALSE, FALSE))
    		 {
    			 DBG_vPrintf(TRACE_APP_UART, "app_Leave_new_faild");
    		 }
    		 app_vStartNodeFactoryNew();//��������
    		 DBG_vPrintf(TRACE_APP_UART, "app_joinorleave123");
    		 app_StartJoinConfig(TRUE);
    		 PDM_vDeleteAllDataRecords();//
    		 vAHI_SwReset();
    		 break;
    		 /*��ȡ�豸������Ϣ*/

    	 case APP_U_EVENT_READ_DEV_STATUS:
    		 Uart_TxCMDData(CMD_SENSOR_STATUS_REQ, SUCCESS_RESP);
    		 break;

    	 case  APP_U_EVENT_READ_DEV_INFO:
    		 Uart_TxCMDData(CMD_SENSOR_INFO_REQ, SUCCESS_RESP);
    	    	break;

    		 /*�����豸��Ϣ*/
    	 case  APP_U_EVENT_UPDATE_DEV_INFO://�����豸��Ϣ
    		 sEP_Dev_Inf.Data_valid=VALID_VALUE;
    		 sEP_Dev_Inf.S_valid_flag=VALID_VALUE ;
    		 sEP_Dev_Inf.sM_YCL.sYCL.YCL_ID=sSenDev_inf.Dev_YCLID;
    		 sEP_Dev_Inf.Serve_CompanyCode=sSenDev_inf.Dev_Serve_CompanyCode;
    		 sEP_Dev_Inf.sM_Hv.sHard_Ver.Hv_YCL_ID=sSenDev_inf.Dev_YCLID;
    		 sEP_Dev_Inf.sM_Sv.sSoft_Ver.Sv_YCL_ID=sSenDev_inf.Dev_YCLID;
    		 sEP_Dev_Inf.sS_Sv.sSoft_Ver.Sv_YCL_ID=sSenDev_inf.Dev_YCLID;
    		 sEP_Dev_Inf.sS_Sv.sSoft_Ver.Sv_Mainv_Num=sSenDev_inf.Dev_SvCode;//����ģ�������汾����Ҫ�����汾��ȷ��


    		 //����Basic ClusterID���������
    		 sCluster_Basic_Attr.sM_YCL=sEP_Dev_Inf.sM_YCL;
    		 sCluster_Basic_Attr.M_ClusterID=sEP_Dev_Inf.M_ClusterID;
    		 sCluster_Basic_Attr.heartbeat_value=sEP_Dev_Inf.sEP_Dev_HeartBeat;
    		 sCluster_Basic_Attr.sM_Hv=sEP_Dev_Inf.sM_Hv;
    		 sCluster_Basic_Attr.sM_Sv=sEP_Dev_Inf.sM_Sv;
    		 sCluster_Basic_Attr.Serve_CompanyCode=sEP_Dev_Inf.Serve_CompanyCode;
    		 sCluster_Basic_Attr.sS_Sv=sEP_Dev_Inf.sS_Sv;

    		 sLinkKey=Linkkey_Calculate(sEP_Dev_Inf.sM_YCL);//����������Կ
    		 eCLD_IASZoneUpdateZoneType(ZONE_ZONE_ENDPOINT, sEP_Dev_Inf.M_ClusterID);//�����豸����
    		 vSaveIASZoneAttributes(ZONE_ZONE_ENDPOINT);//���б��浽EEPROM��
    		 PDM_eSaveRecordData(PDM_ID_APP_EP_DEV_INF,&sEP_Dev_Inf,sizeof(tsEP_Dev_Inf));//���浽EEPROM��
    		 DBG_vPrintf(TRACE_APP_UART, "app_read_device_inf");
    		 break;

    	 default:
    		 break;

    	}

    }

    if (OS_eGetSWTimerStatus(APP_uart_timeout) == OS_E_SWTIMER_EXPIRED) //
    {
    	Uart_Analysis_Step = UART_RX_NULL;

    }


    switch (s_eUart_state)
    {

       case 0:
    	  vStartup();
    	  s_eUart_state =1;
    	  break;
       case 1 :
    	   break;
    	   default :
    	   break;

    }
    //һ�����������������
    if (OS_eGetSWTimerStatus(APP_ActiveTime) == OS_E_SWTIMER_EXPIRED) //
    {
    	DBG_vPrintf(TRACE_APP_UART, "APP task finish");
    	OS_eStopSWTimer(APP_ActiveTime);
    	App_BusyConfig(FALSE);//APP��æ�ˣ�׼������͹���
    	if((start_join_flag==FALSE)&&(sDeviceDesc.eNodeState==E_STARTUP))
    	{
    		vScheduleSleep();
    	}
     }
}

PUBLIC void app_StartJoinConfig(bool flag)
{
	start_join_flag=flag;
}

PUBLIC bool  app_SendsSatusDate(void)
{
	ZPS_tsAfProfileDataReq psProfileDataReq1;
	ZPS_tuAddress  tuAddress;
	static uint8 sqen=1;
	bool old_send_type=FALSE;
	volatile uint16 u16PayloadSize=0;
	PDUM_thAPduInstance hAPduInst;

	tuAddress.u16Addr=0;
	psProfileDataReq1.uDstAddr=tuAddress;
	DBG_vPrintf(TRACE_APP_UART, "dev_inf=0x%x",sEP_Dev_Inf.M_ClusterID);
	psProfileDataReq1.u16ClusterId=0x0500;//
	psProfileDataReq1.u16ProfileId=HA_PROFILE_ID;
	psProfileDataReq1.u8SrcEp=ZONE_ZONE_ENDPOINT;
	psProfileDataReq1.eDstAddrMode=ZPS_E_ADDR_MODE_SHORT;
	psProfileDataReq1.u8DstEp=1;
	psProfileDataReq1.eSecurityMode=ZPS_E_APL_AF_UNSECURE;
	psProfileDataReq1.u8Radius=0;
	hAPduInst=PDUM_hAPduAllocateAPduInstance(apduMyData);
	if(hAPduInst == NULL)
	 {
		/*�����ڴ治�ɹ�*/
		return FALSE;

	 }
	 else
	 {
	      sqen = u8GetTransactionSequenceNumber();
	      u16PayloadSize = u16ZCL_WriteCommandHeader(hAPduInst,
	                   	   	   	   	   	 eFRAME_TYPE_COMMAND_ACTS_ACCROSS_ENTIRE_PROFILE,//ͳһ�������ʽ
	        		                     TRUE,
	        		                     ZCL_MANUFACTURER_CODE,
	        		                     TRUE,
	        		                     TRUE,
	        		                     &sqen,
	        		                     E_ZCL_REPORT_ATTRIBUTES);


	    	  u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h",E_CLD_IASZONE_STATUS);//ID
	    	  u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",E_ZCL_UINT8);//map16
	    	  u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",sSen_Status_Data.dev_state);//�豸״̬


	    	  u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h",E_CLD_IASZONE_ZONE_POWER_VALUE);//ID
	    	  u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",E_ZCL_UINT8);//map16
	    	  u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",sSen_Status_Data.dev_power_state);//��ص���



	    	  PDUM_eAPduInstanceSetPayloadSize(hAPduInst, u16PayloadSize);
	    	  ZPS_eAplAfApsdeDataReq(hAPduInst,(ZPS_tsAfProfileDataReq*)&psProfileDataReq1,&sqen);

	   }
	 return TRUE;

}

PUBLIC bool  app_Read_Hearttime_Resp(void)
{
	ZPS_tsAfProfileDataReq psProfileDataReq1;
	ZPS_tuAddress  tuAddress;
	static uint8 sqen=1;
	bool old_send_type=FALSE;
	volatile uint16 u16PayloadSize=0;
	PDUM_thAPduInstance hAPduInst;

	tuAddress.u16Addr=0;
	psProfileDataReq1.uDstAddr=tuAddress;
	DBG_vPrintf(TRACE_APP_UART, "dev_inf=0x%x",sEP_Dev_Inf.M_ClusterID);
	psProfileDataReq1.u16ClusterId=0x0500;//
	psProfileDataReq1.u16ProfileId=HA_PROFILE_ID;
	psProfileDataReq1.u8SrcEp=ZONE_ZONE_ENDPOINT;
	psProfileDataReq1.eDstAddrMode=ZPS_E_ADDR_MODE_SHORT;
	psProfileDataReq1.u8DstEp=1;
	psProfileDataReq1.eSecurityMode=ZPS_E_APL_AF_UNSECURE;
	psProfileDataReq1.u8Radius=0;
	hAPduInst=PDUM_hAPduAllocateAPduInstance(apduMyData);
	if(hAPduInst == NULL)
	 {
		/*�����ڴ治�ɹ�*/
		return FALSE;

	 }
	 else
	 {
	      sqen = u8GetTransactionSequenceNumber();
	      u16PayloadSize = u16ZCL_WriteCommandHeader(hAPduInst,
	                   	   	   	   	   	 eFRAME_TYPE_COMMAND_ACTS_ACCROSS_ENTIRE_PROFILE,//ͳһ�������ʽ
	        		                     TRUE,
	        		                     ZCL_MANUFACTURER_CODE,
	        		                     TRUE,
	        		                     TRUE,
	        		                     &sqen,
	        		                     E_ZCL_READ_ATTRIBUTES_RESPONSE);

//��׼��ZigBee�Ķ����Իظ��������ʽ: ����ID(2�ֽ�)  ״̬(1�ֽ�)   ��������(1�ֽ�)  ����(����)
	    	  u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h",E_CLD_IASZONE_ZONE_HEARTBEAT_TIME);//ID
	    	  u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",E_ZCL_CMDS_SUCCESS);//
	    	  u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",E_ZCL_UINT16);//
	    	  u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h",sCluster_Basic_Attr.heartbeat_value);//����ʱ��

	    	  PDUM_eAPduInstanceSetPayloadSize(hAPduInst, u16PayloadSize);
	    	  ZPS_eAplAfApsdeDataReq(hAPduInst,(ZPS_tsAfProfileDataReq*)&psProfileDataReq1,&sqen);

	   }
	 return TRUE;

}

PUBLIC bool  app_Write_Hearttime_Resp(uint8  write_status)
{
	ZPS_tsAfProfileDataReq psProfileDataReq1;
	ZPS_tuAddress  tuAddress;
	static uint8 sqen=1;
	bool old_send_type=FALSE;
	volatile uint16 u16PayloadSize=0;
	PDUM_thAPduInstance hAPduInst;

	tuAddress.u16Addr=0;
	psProfileDataReq1.uDstAddr=tuAddress;
	DBG_vPrintf(TRACE_APP_UART, "dev_inf=0x%x",sEP_Dev_Inf.M_ClusterID);
	psProfileDataReq1.u16ClusterId=0x0500;//
	psProfileDataReq1.u16ProfileId=HA_PROFILE_ID;
	psProfileDataReq1.u8SrcEp=ZONE_ZONE_ENDPOINT;
	psProfileDataReq1.eDstAddrMode=ZPS_E_ADDR_MODE_SHORT;
	psProfileDataReq1.u8DstEp=1;
	psProfileDataReq1.eSecurityMode=ZPS_E_APL_AF_UNSECURE;
	psProfileDataReq1.u8Radius=0;
	hAPduInst=PDUM_hAPduAllocateAPduInstance(apduMyData);
	if(hAPduInst == NULL)
	 {
		/*�����ڴ治�ɹ�*/
		return FALSE;

	 }
	 else
	 {
	      sqen = u8GetTransactionSequenceNumber();
	      u16PayloadSize = u16ZCL_WriteCommandHeader(hAPduInst,
	                   	   	   	   	   	 eFRAME_TYPE_COMMAND_ACTS_ACCROSS_ENTIRE_PROFILE,//ͳһ�������ʽ
	        		                     TRUE,
	        		                     ZCL_MANUFACTURER_CODE,
	        		                     TRUE,
	        		                     TRUE,
	        		                     &sqen,
	        		                     E_ZCL_WRITE_ATTRIBUTES_RESPONSE);

//��׼��ZigBee��д���Իظ��������ʽ:  ״̬(1�ֽ�)  ����ID(2�ֽ�)
	          u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",write_status);//
	    	  u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h",E_CLD_IASZONE_ZONE_HEARTBEAT_TIME);//ID

	    	  PDUM_eAPduInstanceSetPayloadSize(hAPduInst, u16PayloadSize);
	    	  ZPS_eAplAfApsdeDataReq(hAPduInst,(ZPS_tsAfProfileDataReq*)&psProfileDataReq1,&sqen);

	   }
	 return TRUE;

}

PUBLIC bool  app_Sendtest(void)
{
	ZPS_tsAfProfileDataReq psProfileDataReq1;
	ZPS_tuAddress  tuAddress;
	static uint8 sqen=1;
	bool old_send_type=FALSE;
	volatile uint16 u16PayloadSize=0;
	PDUM_thAPduInstance hAPduInst;

	tuAddress.u16Addr=0;
	psProfileDataReq1.uDstAddr=tuAddress;
	DBG_vPrintf(TRACE_APP_UART, "dev_inf=0x%x",sEP_Dev_Inf.M_ClusterID);
	psProfileDataReq1.u16ClusterId=0x0006;//
	psProfileDataReq1.u16ProfileId=HA_PROFILE_ID;
	psProfileDataReq1.u8SrcEp=ZONE_ZONE_ENDPOINT;
	psProfileDataReq1.eDstAddrMode=ZPS_E_ADDR_MODE_SHORT;
	psProfileDataReq1.u8DstEp=1;
	psProfileDataReq1.eSecurityMode=ZPS_E_APL_AF_UNSECURE;
	psProfileDataReq1.u8Radius=0;
	hAPduInst=PDUM_hAPduAllocateAPduInstance(apduMyData);
	if(hAPduInst == NULL)
	 {
		/*�����ڴ治�ɹ�*/
		return FALSE;

	 }
	 else
	 {
	      sqen = u8GetTransactionSequenceNumber();
	      u16PayloadSize = u16ZCL_WriteCommandHeader(hAPduInst,
	                   	   	   	   	   	 eFRAME_TYPE_COMMAND_ACTS_ACCROSS_ENTIRE_PROFILE,//ͳһ�������ʽ
	        		                     FALSE,
	        		                     ZCL_MANUFACTURER_CODE,
	        		                     TRUE,
	        		                     TRUE,
	        		                     &sqen,
	        		                     E_ZCL_REPORT_ATTRIBUTES);

	    	  u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h",0x0000);//ID
	    	  u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",E_ZCL_BOOL);//map16
	    	  u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",0);//�豸״̬

	    	  PDUM_eAPduInstanceSetPayloadSize(hAPduInst, u16PayloadSize);
	    	  ZPS_eAplAfApsdeDataReq(hAPduInst,(ZPS_tsAfProfileDataReq*)&psProfileDataReq1,&sqen);


	      return TRUE;
	   }

}

/*�����豸�Ļ�����Ϣ
 *
 * ���͵���������˳��:YCL  ClusterID  ����ʱ��    ͨ��ģ�������汾    Ӳ���汾   ������������汾
 * ���͵����Ե�˳���ܸı䣬�����������������豸��Ϣ�������������ӡ�
 */
PUBLIC bool  app_SendDveInf(void)
{
	ZPS_tsAfProfileDataReq psProfileDataReq1;
	ZPS_tuAddress  tuAddress;
	static uint8 sqen=1;
	bool old_send_type=TRUE;
	volatile uint16 u16PayloadSize=0;
	PDUM_thAPduInstance hAPduInst;

	teZCL_Status eStatus=app_IASZoneAppPowerValueUpdate (ZONE_ZONE_ENDPOINT,sSen_Status_Data.dev_power_state);

	tuAddress.u16Addr=0;
	psProfileDataReq1.uDstAddr=tuAddress;
	DBG_vPrintf(TRACE_APP_UART, "dev_inf=0x%x",sEP_Dev_Inf.M_ClusterID);
	psProfileDataReq1.u16ClusterId=0x0000;//
	psProfileDataReq1.u16ProfileId=HA_PROFILE_ID;
	psProfileDataReq1.u8SrcEp=ZONE_ZONE_ENDPOINT;
	psProfileDataReq1.eDstAddrMode=ZPS_E_ADDR_MODE_SHORT;
	psProfileDataReq1.u8DstEp=1;
	psProfileDataReq1.eSecurityMode=ZPS_E_APL_AF_UNSECURE;
	psProfileDataReq1.u8Radius=0;
	hAPduInst=PDUM_hAPduAllocateAPduInstance(apduMyData);
	if(hAPduInst == NULL)
	 {
		/*�����ڴ治�ɹ�*/
		return FALSE;

	 }
	 else
	 {
		  sqen = u8GetTransactionSequenceNumber();
		  u16PayloadSize = u16ZCL_WriteCommandHeader(hAPduInst,
										 eFRAME_TYPE_COMMAND_ACTS_ACCROSS_ENTIRE_PROFILE,//ͳһ�������ʽ
										 TRUE,
										 ZCL_MANUFACTURER_CODE,
										 TRUE,
										 TRUE,
										 &sqen,
										 E_ZCL_REPORT_ATTRIBUTES);


		DBG_vPrintf(TRACE_APP_UART,"Send device information");


		//�豸YCL
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h",E_CLD_BASIC_M_YCL);//ID
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",E_ZCL_OSTRING);//str
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",sizeof(uYcl));//str
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "a\x0c",sCluster_Basic_Attr.sM_YCL);

		//clusterID
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h",E_CLD_BASIC_M_CLUSTERID);//ID
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",E_ZCL_UINT16);//map16
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h",sCluster_Basic_Attr.M_ClusterID);

		//��������
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h",E_CLD_BASIC_HEARTBEAT_VALUE);//ID
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",E_ZCL_UINT16);//map16
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h",sCluster_Basic_Attr.heartbeat_value);
		//ͨѶģ������汾
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h",E_CLD_BASIC_M_SV);//ID
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",E_ZCL_OSTRING);//str
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",sizeof(uSoft_Ver));//str
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "a\x0a",sCluster_Basic_Attr.sM_Sv);

		//ͨѶģ��Ӳ���汾
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h",E_CLD_BASIC_M_HV);//ID
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",E_ZCL_OSTRING);//str
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",sizeof(uHard_Ver));//str
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "a\x0b",sCluster_Basic_Attr.sM_Hv);

		//����������汾
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h",E_CLD_BASIC_S_SV);//ID
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",E_ZCL_OSTRING);//str
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",sizeof(uSoft_Ver));//str
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "a\x0a",sCluster_Basic_Attr.sS_Sv);


		PDUM_eAPduInstanceSetPayloadSize(hAPduInst, u16PayloadSize);
		ZPS_eAplAfApsdeDataReq(hAPduInst,(ZPS_tsAfProfileDataReq*)&psProfileDataReq1,&sqen);
	 }


	return TRUE;
}


/*
 * �����豸��ת��ģ��
 *
 */

PUBLIC bool  app_SendSW_Model(void)
{
	ZPS_tsAfProfileDataReq psProfileDataReq1;
	ZPS_tuAddress  tuAddress;
	uint8 i=0;
	static uint8 sqen=1;
	volatile uint16 u16PayloadSize=0;
	PDUM_thAPduInstance hAPduInst;
	sEnd_SW_Model asEnd_SW_Model[4]={
			{E_CLD_IASZONE_ZONE_POWER_VALUE,0x01},
			{E_CLD_IASZONE_ZONE_RSSI,0x02},
			{E_CLD_IASZONE_STATUS,0x03},
			{E_CLD_IASZONE_ZONE_HEARTBEAT_TIME,0x04}
	};
	tuAddress.u16Addr=0;
	psProfileDataReq1.uDstAddr=tuAddress;
	DBG_vPrintf(TRACE_APP_UART, "send SW Model");
	psProfileDataReq1.u16ClusterId=0x0000;//basic clusterID
	psProfileDataReq1.u16ProfileId=HA_PROFILE_ID;
	psProfileDataReq1.u8SrcEp=ZONE_ZONE_ENDPOINT;
	psProfileDataReq1.eDstAddrMode=ZPS_E_ADDR_MODE_SHORT;
	psProfileDataReq1.u8DstEp=1;
	psProfileDataReq1.eSecurityMode=ZPS_E_APL_AF_UNSECURE;
	psProfileDataReq1.u8Radius=0;
	hAPduInst=PDUM_hAPduAllocateAPduInstance(apduMyData);
	if(hAPduInst == NULL)
	{
			/*�����ڴ治�ɹ�*/
		return FALSE;

	}
	else
	{
		sqen = u8GetTransactionSequenceNumber();
		u16PayloadSize = u16ZCL_WriteCommandHeader(hAPduInst,
		                   	   	   	   	   	 eFRAME_TYPE_COMMAND_ACTS_ACCROSS_ENTIRE_PROFILE,//ͳһ�������ʽ
		                   	   	   	   	   	 TRUE,
		        		                     ZCL_MANUFACTURER_CODE,
		        		                     TRUE,
		        		                     TRUE,
		        		                     &sqen,
		        		                     E_ZCL_REPORT_ATTRIBUTES);

		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h",E_CLD_BASIC_M_CLUSTERID);//ID
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",E_ZCL_UINT16);//enum16 data_type
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h",sEP_Dev_Inf.M_ClusterID);//data

		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h",E_CLD_BASIC_SW_MODEL);//ID
	    u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",E_ZCL_OSTRING);//enum16 data_type
	    u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",sizeof(asEnd_SW_Model));//enum16 data_type
	    u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "a\x0c",asEnd_SW_Model);//zigbee attrID
	    /*
	    for(i=0; i<4 ;i++)
	    {
	    	u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h",asEnd_SW_Model[i].zattrID);//zigbee attrID
	    	u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",asEnd_SW_Model[i].CattrID);//CJP attrID
	    }
		*/
		PDUM_eAPduInstanceSetPayloadSize(hAPduInst, u16PayloadSize);
		ZPS_eAplAfApsdeDataReq(hAPduInst,(ZPS_tsAfProfileDataReq*)&psProfileDataReq1,&sqen);


	}
	return TRUE;
}


PUBLIC void app_UartSendMesg(APP_uartEventType  type)
{
	APP_uartEvent sUartEvent;
    sUartEvent.eType = type;
    OS_ePostMessage(APP_msgUartEvents, &sUartEvent);//���ʹ�����Ϣ
    DBG_vPrintf(TRACE_APP_UART, "uart mesg %d ",type);
}

PRIVATE void vStartup(void)
{

    OS_eStartSWTimer (APP_uart_timeout, APP_TIME_MS(1000), NULL);
}


PUBLIC void User_Uart_Init(void)
{

  //���ܴ���
#ifdef KEY_CON_ENABLE

#else
  bAHI_UartEnable(UART_APP_PORT,
  Uart_DMA_TxBuf,
  UART_DMA_TXBUF_LEN,
  Uart_DMA_RxBuf,
  UART_DMA_RXBUF_LEN);
  //���ò�����
  vAHI_UartSetBaudRate(UART_APP_PORT,UART_APP_BAUD);

  //����У��λ������λ  ֹͣλ��
  vAHI_UartSetControl(UART_APP_PORT,
		  	  	  	  E_AHI_UART_ODD_PARITY,
		  	  	  	  E_AHI_UART_PARITY_DISABLE,
		  	  	  	  E_AHI_UART_WORD_LEN_8,
		  	  	  	  E_AHI_UART_1_STOP_BIT,
		  	  	  	  E_AHI_UART_RTS_LOW);


  //�����ն�ģʽ
  vAHI_UartSetInterrupt(UART_APP_PORT,
		  	  	  	  	FALSE,
		  	  	  	  	FALSE,
		  	  	  	  	FALSE,
		  	  	  	  	TRUE,
		  	  	  	  	E_AHI_UART_FIFO_LEVEL_1
		  );
#endif

  //ע��UART�Ļص�����
  //vAHI_Uart1RegisterCallback(vHwDeviceIntCallback);
}


static uint16 HalUARTRead( uint8 u8Uart,
		  	  	  	  	   uint8 *pu8DataBuffer,
		  	  	  	  	   uint16 u16DataBufferLength)
{

	 return  u16AHI_UartBlockReadData(
			 	 	 	 	 	 	 	 u8Uart,
			 	 	 	 	 	 	 	 pu8DataBuffer,
			 	 	 	 	 	 	 	 u16DataBufferLength);

}

static uint16 HalUARTWrite(uint8 u8Uart,
		uint8 *pu8Data,
		uint16 u16DataLength)
{
	  return u16AHI_UartBlockWriteData(u8Uart,
			  	  	  	  	  	  	   pu8Data,
			  	  	  	  	  	  	   u16DataLength);
}

//#ifndef DBG_ENABLE
OS_ISR(vISR_Uart1)
{
	Uart_Analysis();
}
//#endif

//Uart�ص�����  ������
 void vHwDeviceIntCallback(uint32 u32Device, uint32 u32ItemBitmap)
{

	 if(E_AHI_DEVICE_UART1==u32Device)
	 {
		 Uart_Analysis();
	 }

}

//Uart�ص���������
static void Uart_Analysis(void)
{
  uint8 rx_buf_num=0;
  uint8 *p_rx_buf=Uart_SRxBuf;

  switch( Uart_Analysis_Step)
  {
  case UART_RX_NULL:
    Uart_SRxLen = 0;
    Uart_Analysis_Step = UART_RX_HEAD;
  case UART_RX_HEAD:
    rx_buf_num = HalUARTRead(UART_APP_PORT, p_rx_buf+Uart_SRxLen, UART_HEAD_LEN-Uart_SRxLen);
    Uart_SRxLen+=rx_buf_num;

    if(Uart_SRxLen>=UART_HEAD_LEN)
    {
      if((*(p_rx_buf+0)!=UART_HEAD_H )||(*(p_rx_buf+1)!=UART_HEAD_L )||(*(p_rx_buf+2)>UART_RX_DATA_MAX_NUM ))
      {
        Uart_Analysis_Step = UART_RX_NULL;
        break;
      }
      Uart_Analysis_Step = UART_RX_DATA;

    }

    else
    {
      break;
    }
  case UART_RX_DATA:
     rx_buf_num = HalUARTRead(UART_APP_PORT, p_rx_buf+Uart_SRxLen, (*(p_rx_buf+2)) + UART_HEAD_LEN-Uart_SRxLen);
     Uart_SRxLen+=rx_buf_num;
     if(Uart_SRxLen >= (*(p_rx_buf+2)) + UART_HEAD_LEN)
     {

       if((*(p_rx_buf+Uart_SRxLen-1))==CRC_Calculate(p_rx_buf,  (*(p_rx_buf+2))+2))
       {
         Uart_Rx_CMDDeal(p_rx_buf);
         OS_eStopSWTimer (APP_ActiveTime);
         OS_eStartSWTimer (APP_ActiveTime, APP_TIME_MS(ACTIVE_TIME), NULL);
       }
       else
       {
         Uart_TxCMDData((*(p_rx_buf+3)), ERROR_RESP);
       }
       Uart_Analysis_Step = UART_RX_NULL;
       break;

     }
     else
     {
       break;
     }
  case UART_RX_BUSY:
    break;
  default:
    break;
  }
  OS_eStopSWTimer (APP_uart_timeout);
  OS_eStartSWTimer (APP_uart_timeout, APP_TIME_MS(20), NULL);
  //���ڳ�ʱ����Ӧ�Դ������ݶ෢�����쳣����µĽ����쳣��
  //�趨��������ֽ�֮���ʱ��������UART_TIMEOUT_VALUE���룬����Ϊ��֡���ݽ������ٴ�ͷ��ʼ����
}


static void Uart_Rx_CMDDeal(uint8 *rx_buf)
{
  uint8 *p_deal_buf=rx_buf;
  uint8 i=0;
  static uint8 pre_dev_state= NORMAL_VALUE;//Ĭ��Ϊ����ģʽ

  switch (*(p_deal_buf+3))
  {
   case CMD_KEY_RESP:
     if(*(p_deal_buf+2)==0x03)
    {

        if((*(p_deal_buf+4)==UART_LONG_KEY)||(*(p_deal_buf+4)==UART_THREE_TIMES_KEY))//��������
        {
        	/*��������������*/
        	app_UartSendMesg(APP_U_EVENT_JOINORUNJOIN);
        	DBG_vPrintf(TRACE_APP_UART, "joinorunjoin");
        	Uart_TxCMDData(CMD_KEY_RESP, SUCCESS_RESP);//
        }

     }

    else
    {

      Uart_TxCMDData(CMD_KEY_RESP, ERROR_RESP);

    }
    break;

  case CMD_SENSOR_INFO_REQ:   //��ȡ�豸��Ϣ����Ҫ��ͨ���豸��Ϣȷ���豸����
    if(*(p_deal_buf+2)==0x0C)
    {//�����豸��Ϣ����
    	//���
    	for(i=0;i<(sizeof(MS_Type_Sw_Tab) / sizeof(tsMS_Type_Sw));i++)
    	{
    		if((uint16)BUILD_UINT16( *(p_deal_buf+9), *(p_deal_buf+8) )==MS_Type_Sw_Tab[i].Dev_TypeCode)
    		{
    			sSenDev_inf.Dev_YCLID=MS_Type_Sw_Tab[i].S_YCL_ID-1;
    			sSenDev_inf.Dev_Serve_CompanyCode=STWE_COMPANY;
    			sSenDev_inf.Dev_HvCode=0x01;
    			sSenDev_inf.Dev_NumID=(uint32)*(p_deal_buf+4);
    			sSenDev_inf.Dev_SvCode=0x01;
    			sSenDev_inf.Dev_TypeCode=MS_Type_Sw_Tab[i].Dev_TypeCode;

    			sEP_Dev_Inf.S_valid_flag=VALID_VALUE;
    			app_UartSendMesg(APP_U_EVENT_UPDATE_DEV_INFO);
    			DBG_vPrintf(TRACE_APP_UART, "app_u_event_update_dev_inf");
    			break;
    			break;
    		}
    	}
     }
    //�豸������Ϣ��ȡʧ��
    DBG_vPrintf(TRACE_APP_UART, "app_dev-inf-error");
    break;
    case CMD_SENSOR_STATUS_RESP:
      if(*(p_deal_buf+2)!=0x00)
      {

        Uart_TxCMDData(CMD_SENSOR_STATUS_RESP, SUCCESS_RESP);

      }
      else
      {
        Uart_TxCMDData(CMD_SENSOR_STATUS_RESP, ERROR_RESP);
        break;
        //�豸��Ϣ���󣬷���
      }
    case CMD_SENSOR_STATUS_REQ:
     if(*(p_deal_buf+2)!=0x00)
     {

    	 sSen_Status_Data.dev_power_state=*(p_deal_buf+4);

       if((*(p_deal_buf+5)==0x01))
        {
    	   sSen_Status_Data.dev_state=0xAA;
        }
        else
        {
        	sSen_Status_Data.dev_state=*(p_deal_buf+5);
        }

       sSen_Status_Data.dev_data_num=*(p_deal_buf+2)-4;
       sSen_Status_Data.dev_data=(p_deal_buf+6);

        if((sSenDev_inf.Dev_YCLID==YCLID_ED||sSenDev_inf.Dev_YCLID==YCLID_WA)&&(sSen_Status_Data.dev_state==ALARM_VALUE||sSen_Status_Data.dev_state==ILLEGAL_MOVE_VALUE))//��һ�ε�״̬
        {
         //�Ŵź�ˮ�д�����ֻ����״ֵ̬������ת��Ϊ����ʱ���ŷ��ͱ�����Ϣ

           if((sSen_Status_Data.dev_state==pre_dev_state)||(pre_dev_state==UNNORMAL_VALUE))
           {
        	   sSen_Status_Data.dev_state=UNNORMAL_VALUE;
           }

        }
        pre_dev_state=sSen_Status_Data.dev_state;
       if(sSen_Status_Data.dev_state==ALARM_VALUE||sSen_Status_Data.dev_state==ILLEGAL_MOVE_VALUE)//������������¼�
       {

         alarm_state=sSen_Status_Data.dev_state;

         pre_dev_state=sSen_Status_Data.dev_state;

       }
      else//����������¼�
      {

      }
       //���µ�����״̬��Ϣ
      if( sSen_Status_Data.dev_state== ALARM_VALUE||sSen_Status_Data.dev_state== UNNORMAL_VALUE)
      {
    	  ZCL_BIT_SET(zbmap16,sSen_Status_Data.dev_state,CLD_IASZONE_STATUS_MASK_ALARM1);
      }
      else
      {
    	  ZCL_BIT_CLEAR(zbmap16,sSen_Status_Data.dev_state,CLD_IASZONE_STATUS_MASK_ALARM1);
      }
      app_UartSendMesg( APP_U_EVENT_ESEND_DATA);
      break;
     }
     else
    {
      Uart_TxCMDData(CMD_SENSOR_STATUS_REQ, ERROR_RESP);
      break;
      //�豸״̬��Ϣ���������ط�
    }
  case CDM_SENSOR_TEST:
     if((*(p_deal_buf+2)==0x03)&&(*(p_deal_buf+4)==SUCCESS_RESP))
     {
        //���Գɹ��������
     }
    else
    {

    	Uart_TxCMDData(CDM_SENSOR_TEST,0xBB);
      //����ʧ�ܣ����·���
     }

    break;
  default:
    break;
  }
}
/*
//�������ܣ��򴮿ڷ�����������
//cmd_type:��������
//value����ֵ
//������Ϊ9600��1ms���Ͳ���1���ֽ�
*/
static void  Uart_TxCMDData(uint8 cmd_type, uint8 value )
{
     uint8 *tx_buf=Uart_STxBuf;
     bool error_flag=0;
     *tx_buf= UART_HEAD_H ;
     *(tx_buf+1)= UART_HEAD_L ;
     *(tx_buf+3)=cmd_type;

     OS_eStopSWTimer (APP_ActiveTime);
     OS_eStartSWTimer(APP_ActiveTime, APP_TIME_MS(ACTIVE_TIME), NULL);
    switch (cmd_type)
    {
    case CMD_KEY_RESP:
       *(tx_buf+2)=0x03;
       *(tx_buf+4)=value;
      break;

    case CMD_SENSOR_STATUS_RESP:
       *(tx_buf+2)=0x03;
       *(tx_buf+4)=value;
      break;
    case CMD_SENSOR_INFO_REQ:

       *(tx_buf+2)=0x02;
      break;

    case CMD_SENSOR_STATUS_REQ:
       *(tx_buf+2)=0x02;

      break;

    case CDM_SENSOR_TEST:
       *(tx_buf+2)=0x03;
       *(tx_buf+4)=0xBB;
      break;
    default:
      error_flag=1;
      break;
    }
    if(!error_flag)
    {
      *(tx_buf+(*(tx_buf+2)+2))= CRC_Calculate(tx_buf, (*(tx_buf+2))+2);
      HalUARTWrite(UART_APP_PORT,tx_buf, (*(tx_buf+2)+3));
    }

}


/*
//�������ܣ�CRC�ۼӺͼ���
// pBuf:����ָ��
//len:���ݳ���
*/
static uint8 CRC_Calculate(uint8 *pBuf, uint8 len)
{
  uint8 sum=0,i=0;

  for( i=0;i<len;i++,pBuf++)
  {
     sum+=(*pBuf);
  }
  return sum;
}


static usLinkKey Linkkey_Calculate(uYcl Ycl)
{
	usLinkKey   linkkey_tmp;
	uYcl         ycl_tmp=Ycl;

	ycl_tmp.YCL_Array[11]=Ycl.YCL_Array[10];
	ycl_tmp.YCL_Array[10]=Ycl.YCL_Array[11];

	ycl_tmp.YCL_Array[5]=Ycl.YCL_Array[4];
	ycl_tmp.YCL_Array[4]=Ycl.YCL_Array[5];

	ycl_tmp.YCL_Array[3]=Ycl.YCL_Array[1];
	ycl_tmp.YCL_Array[2]=Ycl.YCL_Array[0];
	ycl_tmp.YCL_Array[1]=Ycl.YCL_Array[3];
	ycl_tmp.YCL_Array[0]=Ycl.YCL_Array[2];

	linkkey_tmp.LinkKey_YCL=ycl_tmp;
	linkkey_tmp.LinkKey_Last4By=((uint32)*(&(ycl_tmp.YCL_Array[11]))+(uint32)*(&(ycl_tmp.YCL_Array[7]))+(uint32)*(&(ycl_tmp.YCL_Array[3])))/0xFFFF;

	return linkkey_tmp;
}





