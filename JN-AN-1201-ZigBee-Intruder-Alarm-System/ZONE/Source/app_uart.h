/*
 * app_uart.h
 *
 *  Created on: 2017��10��23��
 *      Author: 1
 */

#ifndef APP_UART_H_
#define APP_UART_H_

#include "app_events.h"
#define BUILD_UINT16(loByte, hiByte) \
          ((uint16)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))


//���Ҵ��붨��
#define XTHN_COMPANY        0x81
#define STWE_COMPANY		0x03
#define SWTL_COMPANY        0x05
//�豸YCLID����
#define YCLID_METHANE                       0x59121112  //��Ȼ��й©������
#define YCLID_SMOKE                         0x59121106  //��������
#define YCLID_CALL                          0x59121136  //�������д�����
#define YCLID_PIR                           0x59121102 //���⴫����
#define YCLID_ED                            0x59121132  //�Ŵ�̽������
#define YCLID_WA                            0x59121116 //ˮ��������
#define YCLID_WQ                            0x59040102  //ˮ�ʼ��


#define UART_RX_TIMEOUT_VALUE     1000  //�������˳��͹��ĺ�Ľ�����ʱʱ��
#define UART_TX_TIMEOUT_VALUE     100    //���ڷ�����ʱʱ�䣬100ms


#define JOIN_FLAG_VALUE           55

#define VALID_VALUE               0xAA

#define M_PROFILE_ID              HA_PROFILE_ID
#define M_CLUSTER_ID              HOME_AUTOMATION_IASZONE_CLUSTER_ID  //0x0500
#define ATTYSW_CLUSTER_ID         0xFC01

//Ĭ�ϵ͵���ֵ
#define DEF_LOW_POWER_VALUE        10

//Ĭ������ʱ��
#define DEF_HEARTBEAT_VALUE        0x0001

#pragma pack(1)    //����1�ֽڶ���,ָ�����뷽ʽ

typedef uint16 tDev_TypeCode;
typedef uint8  tDev_SvCode;
typedef uint8  tDev_HvCode;
typedef uint32 tDev_NumID;
typedef uint32 tDev_YCLID;


 typedef  union {
        uint8 YCL_Array[12];
        struct {
        	uint32 YCL_ID;
        	uint64 Mac;
        }sYCL;
}uYcl;

typedef  union {
       uint8 YCL_Array[4];
       uint32 YCL_ID;
}ucs;

typedef  union {
	uint8 Sv_Array[10];
	struct {
		uint32 Sv_YCL_ID;
		uint8 Sv_Mainv_Num;
		uint8 Sv_Secv_Num;
		uint8 Sv_Modv_Num;
		uint8 Sv_Dev_Date[3];
	}sSoft_Ver;

}uSoft_Ver;

typedef  union {
	uint8 Hv_Array[11];
	struct {
		uint8 Hv_Logo;
		uint32 Hv_YCL_ID;
		uint16 Hv_TecPro;
		uint8  Hv_Dev_Company;
		uint8 Hv_Dev_Date[2];
		uint8  Hv_Prod_Ser;
	}sHard_Ver;
}uHard_Ver;

typedef struct
{
    uint8 				dev_state ;
    uint8 				dev_power_state;
    uint8 				*dev_data;
    uint8 				dev_data_num;
 }tsSen_Status_Data;


typedef struct
{
	uint8    			Dev_Serve_CompanyCode;//���������Ҵ���
	tDev_TypeCode   	Dev_TypeCode;//�豸���룬����˾��Ʒ����
	tDev_SvCode         Dev_SvCode;//����汾
	tDev_HvCode         Dev_HvCode;//Ӳ���汾
	tDev_NumID          Dev_NumID; //�豸���
	tDev_YCLID 			Dev_YCLID;
}tsEP_SenDev_inf;


typedef struct
{
	uint8    			Valid;
	uint16   			heartbeat_value;//�������ڣ���λ����
}tsEP_Dev_HeartBeat;


typedef struct
{
	uint8               Data_valid;
	uint16 	            sEP_Dev_HeartBeat;//��������
	uYcl  				sM_YCL;//ͨ��ģ���YCL
	uint16              M_ProfileID;
	uint16 				M_ClusterID;//Cluster ID
	uSoft_Ver 			sM_Sv;//ͨ��ģ�������汾
	uHard_Ver 			sM_Hv;//ͨ��ģ���Ӳ���汾
	uint8 				Sensing_flag;//����ģ��ʹ��
	uint8               S_valid_flag;//����ģ�� =������Ϣ����Ч�Լ��
	uint8               Serve_CompanyCode;//���г��Ҵ���
	uSoft_Ver 			sS_Sv;//����ģ�������汾
}tsEP_Dev_Inf;

typedef struct
{
	tDev_TypeCode Dev_TypeCode;//
	uint32 S_YCL_ID;

}tsMS_Type_Sw;

//Attr_Type�Ķ���
#define ATTR_ID         0x00  //�豸��ClusterID������
#define CMD_ID          0x01  //�豸����ID
#define SIG_ID     		0x02  //�豸�ź�ǿ��
#define BASIC_ATTR_ID   0x10
//Attr_ZID����




typedef struct
{
	uYcl  				sM_YCL;//ͨ��ģ���YCL
	uint16 				M_ClusterID;//Cluster ID
	uint16   			heartbeat_value;//����ʱ��
	uSoft_Ver 			sM_Sv;//ͨ��ģ�������汾
	uHard_Ver 			sM_Hv;//ͨ��ģ���Ӳ���汾
	uint8               Serve_CompanyCode;//����ģ��ĳ���
	uSoft_Ver 			sS_Sv;//����ģ�������汾

}tsCluster_Basic_Attr;

/* Basic Type */
typedef enum
{
	 E_CLD_BASIC_M_YCL             = 0xF001, //ͨ��ģ���YCL
	 E_CLD_BASIC_M_CLUSTERID       = 0xF002, // Cluster ID
	 E_CLD_BASIC_HEARTBEAT_VALUE   = 0xF003,//����ʱ��
	 E_CLD_BASIC_M_SV              = 0xF004,//ͨ��ģ�������汾
	 E_CLD_BASIC_M_HV              = 0xF005,//ͨ��ģ���Ӳ���汾
	 E_CLD_BASIC_S_SV			   = 0xF022,//����ģ�������汾
	 E_CLD_BASIC_SW_MODEL         = 0xF023 //�豸����ת��ģ�ͱ�
} tsCluster_Basic_AttrID;


typedef struct
{
	uint16 				dev_status;
	uint8  				power_value;
}tsCluster_IASZONE_Attr;


/* IAS Zone Type */
typedef enum
{

	E_CLD_IASZONE_ZONE_POWER_VALUE = 0xFF01,
	E_CLD_IASZONE_ZONE_RSSI,
	E_CLD_IASZONE_STATUS,
	E_CLD_IASZONE_ZONE_HEARTBEAT_TIME

} tsCluster_IASZONE_AttrID;




//�豸ģ��ת����
typedef	struct{
		uint16 zattrID; //ZigBee������ID
		uint8  CattrID;  //��ӦCJPЭ�������ģ���е�����ID
}sEnd_SW_Model;

//������Կ�Ķ���
typedef  union {
       uint8 LinkKeyArray[16];
       struct {
    	   	   uYcl    LinkKey_YCL;
       	       uint32  LinkKey_Last4By;
       };
}usLinkKey;


tsEP_Dev_Inf  	        sEP_Dev_Inf; //ͨ���豸������Ϣ�Ĵ洢
tsSen_Status_Data  		sSen_Status_Data;
uint8  			    	alarm_state;
uint8 			        Uart_TaskID;
usLinkKey               sLinkKey;
tsCluster_Basic_Attr   sCluster_Basic_Attr;
tsCluster_IASZONE_Attr sCluster_IASZONE_Attr;


#pragma pack()


PUBLIC void Uart_Task_Init(void);

PUBLIC void User_Uart_Init(void);
PUBLIC uint16 Uart_ProcessEvent( uint8 task_id, uint16 events );
PUBLIC void app_StartJoinConfig(bool flag);
PUBLIC bool app_SendsSatusDate(void);
PUBLIC void app_UartSendMesg(APP_uartEventType  type);
PUBLIC bool  app_SendDveInf(void);
PUBLIC bool  app_SendSW_Model(void);
PUBLIC bool  app_Sendtest(void);
PUBLIC bool  app_Read_Hearttime_Resp(void);
PUBLIC bool  app_Write_Hearttime_Resp(uint8  write_status);
#endif /* APP_UART_H_ */
