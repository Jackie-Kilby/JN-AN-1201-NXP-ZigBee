/*
 * app_uart.h
 *
 *  Created on: 2017��10��23��
 *      Author: 1
 */

#ifndef APP_CIE_UART_H_
#define APP_CIE_UART_H_

#include "app_events.h"
#include "jendefs.h"
#include "zcl.h"
#define BUILD_UINT16(loByte, hiByte) \
          ((uint16)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))


#define UART_RX_MAX_NUM            150
#define UART_RX_DATA_MAX_NUM       130  //������ݶ��ֽ�����
#define UART_TX_MAX_NUM            200
#define CJP_HEAD_LEN               (sizeof(sCJP_Head))
#define CJP_SIMPLE_RESP_LEN        3
/*
 * Э��������
 */
#define C_ZIGBEE_DEV      0x01
#define C_LORA_DEV        0x02
#define C_BLE_DEV         0x03

#define C_DEV_TYPE       C_ZIGBEE_DEV

/*
 * ֡����
*/
#define F_JNI_END        0x00
#define F_JNI_COOR       0x01

/*
 * �˿ں�
 */
#define PORT_NUM        0x01

#define PROFILE_ID      0x0104

#define COOR_YCL_ID     0x00000100UL

#define PERMIT_JOIN_TIME  50

#define BASIC_CLUSTER    0x0000

#pragma pack(1)    //����1�ֽڶ���,ָ�����뷽ʽ

typedef enum
{
	/*
	 * Zigbee �豸������ ID ��ָ Zigbee Э������ JNI ͨ�ŵ����� ID ���壬������ ID ��ΧΪ
	 * 0x00~0x8F��
	 */
	CJP_BUILD_NET_REQ       =0x01,
	CJP_BUILD_NET_RESP      =0x02,
	CJP_RESET_DEV_REQ       =0x03,
	CJP_RESET_DEV_RESP      =0x04,
	CJP_JOIN_NET_SET_REQ 	=0x05,
	CJP_JOIN_NET_SET_RESP   =0x06,
	CJP_BUILD_NET_NOTICE    =0x07,
	CJP_DEV_JOINED_NOTICE   =0x09,
	CJP_DEV_LEAVED_NOTICE   =0x0B,
	CJP_DEL_DEV_REQ         =0x0C,
	CJP_DEL_DEV_RESP        =0x0D,
	CJP_DEV_JOIN_REQ        =0x0E,
	CJP_DEV_JOIN_RESP       =0x0F,
	CJP_RESET_DEF_SET_REQ   =0x10,
	CJP_RESET_DEF_SET_RESP  =0x11,
	CJP_READ_COOR_DEV_INF_REQ         =0x12,
	CJP_READ_COOR_DEV_INF_RESP        =0x13,
	CJP_READ_END_DEV_INF_REQ          =0x14,
	CJP_READ_END_DEV_INF_RESP         =0x15,
	CJP_COOR_SELF_TEST_REQ            =0x70,
	CJP_COOR_SELF_TEST_RESULT_NOTICE  =0x71,

	/*
	 * ֡����=0 ʱ�����ն��� JNI ��ͨ�����ݣ����� ID �� CJP �������ʽ�������������͵�
	 * �豸���� Zigbee �豸��LoRa �豸��BLE �豸������ ID ��ΧΪ 0x90~0xBF��
	 */
	CJP_END_NORMAL_DATA_REPORT_REQ         =0x91,
	CJP_END_NORMAL_DATA_REPORT_RESP        =0x92,
	CJP_END_READ_ATTR_REQ                  =0x93,
	CJP_END_READ_ATTR_RESP                 =0x94,
	CJP_END_WRITE_ATTR_REQ                 =0x95,
	CJP_END_WRITE_ATTR_RESP                =0x96,
	CJP_END_ALARM_DATA_REPORT_REQ          =0x97,
	CJP_END_ALARM_DATA_REPORT_RESP         =0x98,
	CJP_END_HEART_DATA_REPORT_REQ          =0x99,
	CJP_END_HEART_DATA_REPORT_RESP           =0x9A,

	/*
	 * �������� ID ����ָ�������͵�Э���������е����� ID�������� ID �ķ�ΧΪ 0xC0~0xFF��
	 * �������͵����� ID �����ڴ˷�Χ�ڶ��塣
	*/
	CJP_COM_END_TEST_REPORT_REQ          =0xC0,





} CJP_CmdID;

typedef enum
{
	/*
	 * CJP������󷵻�ֵ
	 */
	CJP_SUCCESS         =0x00,
	CJP_ERROR          	=0x01,
	CJP_FSEQ_ERROR      =0x02,    /*֡��Ŵ���*/
	CJP_FTYPE_ERROR     =0x03

} CJP_Status;
typedef  union {
       uint8 YCL_Array[4];
       uint32 YCL_ID;
}uYclID;

 typedef  union {
        uint8 YCL_Array[13];
        struct {
        	uint8  Num;
        	uYclID YCL_ID;
        	uint64 Mac;
        };
}uYcl;

//������Կ�Ķ���
typedef  union {
       uint8 LinkKeyArray[16];
       struct {
    	   	   uYcl    LinkKey_YCL;
       	       uint32  LinkKey_Last4By;
       };
}usLinkKey;

typedef  union {
	uint8 Sv_Array[11];
	struct {
		uint8 Sv_Num;
		uint32 Sv_YCL_ID;
		uint8 Sv_Mainv_Num;
		uint8 Sv_Secv_Num;
		uint8 Sv_Modv_Num;
		uint8 Sv_Dev_Date[3];
	};

}uSoft_Ver;//����汾

typedef  union {
	uint8 Hv_Array[12];
	struct {
		uint8 Hv_Num;
		uint8 Hv_Logo;
		uint32 Hv_YCL_ID;
		uint16 Hv_TecPro;
		uint16 Hv_Dev_Date[2];
		uint8  Hv_Dev_Company;
		uint8  Hv_Prod_Ser;
	};
}uHard_Ver;//Ӳ���汾


typedef union {
	uint8  datatest[2];
	uint16 data;
}uData_test;
/*
 * �ṹ�嶨��
 */
typedef struct{
	uint16 u16FSeq;
	uint8  u8CType;
	uint8  u8FrType;
	uint64 u64Mac;
	uint8  u8EPAddr;
	uint16 u16ProfileID;
	uint16 u16ClusterID;
	CJP_CmdID  u8CmdID;
	uint8  u8DataLen;
}sCJP_Head;


typedef struct{
	uYcl   ycl;
	uint16 clusterID;   //
	uint16 hearttime;   //����ʱ��
	uint16 Msoftver;   //ͨ��ģ�������汾
	uint16 Ssoftver;  //���г�������汾
	uint16 Hardver;  //Ӳ���汾
	uint8  Factcode; //���Ҵ���
}sEnddev_BasicInf;

typedef union{
	struct{
		uint16 clusterID;
		uint8  attrnum ;
	}head;
	struct{
		uint16 zattrID; //ZigBee������ID
		uint8  CattrID;  //��ӦCJPЭ�������ģ���е�����ID
	}attr;
}uAttr_Model;


typedef struct{
	uint8 valid_flag;
	uint8 dev_num;
	uint8 dev_full_flag;
	uint8 model_num;
	uint8 model_full_flag;
	uint8 model_memory_manage;
}sCoor_Dev_manage;//Э�����������豸����ʹ洢��Ϣ


typedef  teZCL_ZCLAttributeType  CJP_DataType;

#pragma pack()    //����1�ֽڶ������

extern uint16 Frame_Seq;
extern uint8  Uart_STxBuf[UART_TX_MAX_NUM+1];
extern uSoft_Ver CIE_soft_ver;
extern uYcl      CIE_Ycl;

extern void Uart_Task_Init(void);
PUBLIC void User_Uart_Init(void);
PUBLIC uint16 Uart_ProcessEvent( uint8 task_id, uint16 events );
PUBLIC void app_UartSendMesg(APP_uartEventType  type);
PUBLIC CJP_Status CJP_TxData(uint8 len);

#endif /* APP_UART_H_ */
