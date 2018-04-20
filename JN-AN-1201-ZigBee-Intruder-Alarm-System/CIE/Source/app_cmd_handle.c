/******************************************************************
 * app_cmd_handle.c
 *
 * ���ļ���Ҫ��CJPЭ������������д������ļ���
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
#include "app_CIE_uart.h"
#include "AppHardwareApi.h"
#include "zcl.h"
#include "zcl_common.h"
#include "app_events.h"
#include "PDM_IDs.h"
#include "IASZone.h"
#include "app_CIE_uart.h"
#include "app_data_handle.h"
#include "Array_list.h"
#include "app_CIE_save.h"
#include "app_cmd_handle.h"
#include "zha_CIE_node.h"


PUBLIC CJP_Status fCJP_Tx_Coor(uYcl ycl ,CJP_CmdID cmd_coor , uint8 *pdata , uint8 len)
{
	sCJP_Head *   CJP_Head;
	CJP_Status    status;
	CJP_Head=(sCJP_Head *)Uart_STxBuf ;
	CJP_Head->u16ASeq = AFrame_Seq;
	CJP_Head->u16FSeq = Frame_Seq ;
	CJP_Head->u8CType = C_ZIGBEE_DEV ;
	if((cmd_coor>=0x00) && (cmd_coor<=0x80))
	{
		CJP_Head->u8FrType = F_JNI_COOR ;
	}
	else
	{
		CJP_Head->u8FrType = F_JNI_END ;
	}
	CJP_Head->Ycl=ycl;
	CJP_Head->u8EPAddr = PORT_NUM ;
	CJP_Head->u16ProfileID =  PROFILE_ID ;
	CJP_Head->u16ClusterID = 0 ;
	CJP_Head->u8CmdID = cmd_coor;
	CJP_Head->u8DataLen = len ;

	memcpy(&(Uart_STxBuf[CJP_HEAD_LEN]), pdata , len);
	if(len>UART_TX_MAX_NUM)
		return CJP_ERROR;
	else
		status=CJP_TxData(CJP_HEAD_LEN+len);

	return status ;
}


PUBLIC CJP_Status fBuildNet(uint8 channel,uint16 panid)
{
	uint8 sdata[3];
	CJP_Status status=CJP_SUCCESS;
	sdata[0] = 0x01;
	sdata[1] = E_ZCL_UINT8;
    uint16 pdm_id=0x8000;
	if((channel>26)||(channel<11))
		sdata[2] = CJP_ERROR;
	else
		sdata[2] = CJP_SUCCESS;

	status=fCJP_Tx_Coor(CIE_Ycl, CJP_BUILD_NET_RESP, sdata , 3);
	//��ʼ����
	if(ZPS_u8AplZdoGetRadioChannel()!=channel)
	{
		DBG_vPrintf(TRACE_APP_UART, "%02x",channel );
		DBG_vPrintf(TRACE_APP_UART, "%04x",panid );
		ZPS_eAplAibSetApsChannelMask(1<<channel);
		//�����ŵ�����
		PDM_eSaveRecordData( PDM_ID_CIE_CHANNEL,
		        					 &channel,
		        					 sizeof(uint8));

		while(pdm_id < 0xFFFF)
		{
			PDM_vDeleteDataRecord(pdm_id++);
		}
		vAHI_SwReset();//��λ

	}
	else
	{
		fBuildNet_Notice(CJP_SUCCESS);
	}

	return status;

}


PUBLIC void fBuildNet_Notice(CJP_Status status)
{
	sBuildNet_Notice  BuildNetNotice;
	uint8 sdata[30],i=0;
	BuildNetNotice.u8Status = status;
	BuildNetNotice.u16PanID = ZPS_u16AplZdoGetNetworkPanId(); //��ȡPANID
	BuildNetNotice.u8Channel = ZPS_u8AplZdoGetRadioChannel(); //��ȡCHannelID

	sdata[i]= 0x03;
	i++;
	sdata[i] = E_ZCL_UINT8;
	i++;
	sdata[i] = BuildNetNotice.u8Status;
	i++;
	sdata[i] = E_ZCL_UINT16;
	i++;
	sdata[i] = (uint8)((BuildNetNotice.u16PanID>>8)&0x00FF);
	sdata[i] = (uint8)((BuildNetNotice.u16PanID)&0x00FF);
	i+=sizeof(uint16);
	sdata[i] = E_ZCL_UINT8;
	i++;
	sdata[i] = BuildNetNotice.u8Channel;
	i++;

	Frame_Seq++;//�������͵����к�+1
	fCJP_Tx_Coor(CIE_Ycl, CJP_BUILD_NET_NOTICE, sdata , i);
}


PUBLIC CJP_Status fJoinNet_Set(uint8 time)
{
	uint8 sdata[3];
	sdata[0] = 0x01;
	sdata[1] = E_ZCL_UINT8;

	if(time>60)//����ʱ�䲻�ܴ���60��
	{
		time = 60 ;
	}
	if(ZPS_eAplZdoPermitJoining(time)!= 0)
		sdata[2] = CJP_ERROR;
	else
		sdata[2] = CJP_SUCCESS;

	return fCJP_Tx_Coor(CIE_Ycl,  CJP_JOIN_NET_SET_RESP , sdata , 3);
}



PUBLIC CJP_Status fDel_Dev(uYcl ycl)
{
	uint8 sdata[20],i=0;
	sdata[i] = 2;
	i++;
	sdata[i] = E_ZCL_UINT8;
	i++;
	sdata[i] = CJP_SUCCESS;
	i++;
	sdata[i] = E_ZCL_OSTRING;
	i++;
	sdata[i] = sizeof(uYcl);
	i++;
	memcpy(&sdata[i],&ycl , sdata[4]);
	i+= sizeof(uYcl);
	if(ZPS_u16AplZdoLookupAddr(ycl.sYCL.Mac)==0x0000){
		sdata[2] = CJP_ERROR;
	}
	else{
		dele_dev_data_manage(ycl); //ɾ���豸�б���Ϣ
		ZPS_eAplZdoLeaveNetwork(ycl.sYCL.Mac,FALSE,FALSE);//��ַ
		ZPS_bAplZdoTrustCenterRemoveDevice(ycl.sYCL.Mac);//�����������Ƴ��豸��Ϣ
		ZPS_vRemoveMacTableEntry(ycl.sYCL.Mac); //��MAC��ַ�����Ƴ��豸
	}
	DBG_vPrintf(TRACE_APP_UART, "Deleting device's YCL is\n");
	printf_array(&ycl, sizeof(ycl));
	return fCJP_Tx_Coor( CIE_Ycl,CJP_DEL_DEV_RESP , sdata , i);

}


PUBLIC CJP_Status fDev_Leave_Notice(uint64 mac)
{
	uint8 sdata[20],i=0 ,place=0;
	uYcl ycl;
	ycl.sYCL.Mac = mac;
	sEnddev_BasicInf tEnddev_BasicInf;
    //���豸�б��в����豸�Ļ�����Ϣ
	place = LocateElem(&Galist ,ycl);
	if(place == 0)
	{
		return CJP_ERROR;
	}

	if(!GetElem(&Galist,place ,&tEnddev_BasicInf))
	{
		return CJP_ERROR;
	}

	if(ZPS_u16AplZdoLookupAddr(ycl.sYCL.Mac)==0x0000)
	{
		return CJP_ERROR;

	}
	else{
		dele_dev_data_manage(ycl); //ɾ���豸�б���Ϣ
	}
	DBG_vPrintf(TRACE_APP_UART, "A device leave net ycl is\n");
	printf_array(&ycl, sizeof(ycl));
	ycl = tEnddev_BasicInf.ycl;
	sdata[i] = 1;
	i++;
	sdata[i] = E_ZCL_OSTRING;
	i++;
	sdata[i] = sizeof(uYcl);
	i++;
	memcpy(&sdata[i],&ycl , sdata[4]);
	i+= sizeof(uYcl);
	i++;
	Frame_Seq ++;
	return fCJP_Tx_Coor( CIE_Ycl,CJP_DEV_LEAVED_NOTICE , sdata , i);

}

PUBLIC CJP_Status fDev_Join(uYcl ycl)
{
	/*
	 * 1�������豸������Կ
	 * 2�������豸����
	 */
	usLinkKey   linkkey_tmp;
	linkkey_tmp = Linkkey_Calculate(ycl);//����������Կ
	ZPS_vAplSecSetInitialSecurityState(ZPS_ZDO_PRECONFIGURED_LINK_KEY, (uint8 *)&linkkey_tmp, 0x00, ZPS_APS_GLOBAL_LINK_KEY);//����������Կ
	ZPS_eAplZdoPermitJoining(PERMIT_JOIN_TIME);
	DBG_vPrintf(TRACE_APP_UART, "Adding device's YCL is\n");
	printf_array(&ycl, sizeof(ycl));
	return CJP_SUCCESS;
}

PUBLIC CJP_Status fReset_Def_Set(void)
{
	//ɾ���������ݣ���մ洢��EEPROM
	PDM_vDeleteAllDataRecords();//
	vAHI_SwReset();//��λ
	return CJP_SUCCESS;
}

PUBLIC CJP_Status fRead_Coor_inf(void)
{
	sReadCoorInf_Resp readcoorinf_resp;
	uint8 sdata[30],len=0;
	sAttr_Charact tAttr_Charact[2]={
				{E_ZCL_OSTRING , (uint32)(&((sReadCoorInf_Resp*)(0))->C_YCL), YCL_LENGTH},
				{E_ZCL_OSTRING , (uint32)(&((sReadCoorInf_Resp*)(0))->C_Sofe_Ver) , SV_LENGTH},

	};
	memcpy(&readcoorinf_resp.C_YCL,&CIE_Ycl ,sizeof(uYcl));
	memcpy(&readcoorinf_resp.C_Sofe_Ver,&CIE_soft_ver ,sizeof(uSoft_Ver));

	len = fCJP_Attr_Handle(sdata , (uint8 *)&readcoorinf_resp ,tAttr_Charact ,2);
	return fCJP_Tx_Coor(CIE_Ycl, CJP_READ_COOR_DEV_INF_RESP , sdata , len);
}

/*
 * ��͹����ն��豸������Ϣʱ������Э����ֱ�����ն˷������ݡ�Э�������ն˷������ݵ�����Ϊ:
 * 1���ն��˳�˯�ߡ�
 * 2���ն�������Э����poll��Ϣ
 * 3��Э�������յ��ն˵�poll��Ϣʱ�������ݴ��䵽�ն�
 * �������Ͽ�֪��Э�������ն˷�����Ϣʱ����Ҫ�����͵���Ϣ���л��壬�ȵ�Э�������յ��ն˵�poll��Ϣʱ���ٽ�������������ȡ�������͵��նˡ�
 * ���˼·����Э��������һ����������������������˻��������JNI�㷢�͹��������������ݺ�MAC��ClusterID�ȣ�
 */

PUBLIC CJP_Status fRead_End_inf(uYcl ycl)
{

	uint8 place=1,len=0;
	uYcl tycl=ycl;
	sEnddev_BasicInf tEnddev_BasicInf ;
	sReadDevInf_Resp ReadDevInf_Resp;
	uint8 sdata[80];
	sAttr_Charact tAttr_Charact[5]={
			{E_ZCL_OSTRING , (uint32)(&((sReadDevInf_Resp*)(0))->M_YCL), YCL_LENGTH},
			{E_ZCL_OSTRING , (uint32)(&((sReadDevInf_Resp*)(0))->M_Soft_Ver) , SV_LENGTH},
			{E_ZCL_OSTRING , (uint32)(&((sReadDevInf_Resp*)(0))->M_Hard_ver) , HV_LENGTH},
			{E_ZCL_OSTRING , (uint32)(&((sReadDevInf_Resp*)(0))->S_Soft_Ver) , SV_LENGTH},
			{E_ZCL_UINT16 , (uint32)(&((sReadDevInf_Resp*)(0))->HeartCyc) , 2}
	};

	DBG_vPrintf(TRACE_APP_UART, "reading information End device's YCL is\n");
	printf_array(&ycl, sizeof(ycl));
	if(ZPS_u16AplZdoLookupAddr(ycl.sYCL.Mac)==0x0000){
		return  CJP_ERROR;
	}
	//���豸�б��в����豸�Ļ�����Ϣ
	place = LocateElem(&Galist,tycl);
	if(place == 0)
	{
		return CJP_ERROR;
	}

	if(!GetElem(&Galist,place ,&tEnddev_BasicInf))
	{
		return CJP_ERROR;
	}
	ReadDevInf_Resp.M_YCL = ycl;
	ReadDevInf_Resp.M_Soft_Ver.sSoft_Ver.Sv_YCL_ID = ycl.sYCL.YCL_ID.u32YCL_ID;
	ReadDevInf_Resp.M_Soft_Ver.sSoft_Ver.Sv_Mainv_Num =(uint8)(tEnddev_BasicInf.Msoftver>>8);
	ReadDevInf_Resp.M_Soft_Ver.sSoft_Ver.Sv_Secv_Num = (uint8)tEnddev_BasicInf.Msoftver;

	ReadDevInf_Resp.S_Soft_Ver.sSoft_Ver.Sv_YCL_ID = ycl.sYCL.YCL_ID.u32YCL_ID;
	ReadDevInf_Resp.S_Soft_Ver.sSoft_Ver.Sv_Mainv_Num = (uint8)(tEnddev_BasicInf.Ssoftver>>8);
	ReadDevInf_Resp.S_Soft_Ver.sSoft_Ver.Sv_Secv_Num = (uint8)tEnddev_BasicInf.Ssoftver;

	ReadDevInf_Resp.M_Hard_ver.sHard_Ver.Hv_Logo = HV_LOGO;
	ReadDevInf_Resp.M_Hard_ver.sHard_Ver.Hv_YCL_ID = ycl.sYCL.YCL_ID.u32YCL_ID;
	ReadDevInf_Resp.M_Hard_ver.sHard_Ver.Hv_TecPro = tEnddev_BasicInf.Hardver;
	ReadDevInf_Resp.M_Hard_ver.sHard_Ver.Hv_Dev_Company = tEnddev_BasicInf.Factcode;

	ReadDevInf_Resp.HeartCyc = tEnddev_BasicInf.hearttime;

	len = fCJP_Attr_Handle(sdata , (uint8 *)&ReadDevInf_Resp , tAttr_Charact ,5);

	return fCJP_Tx_Coor(CIE_Ycl , CJP_READ_END_DEV_INF_RESP , sdata , len);

}


/*Э�����ϱ��豸���б�����豸�Ļ�����Ϣ
 * ָ��ID=0x16
 *
 */
#define MAX_DEV_INF_NUM    10
PUBLIC CJP_Status fReport_End_Dev_List(void)
{
	uint8 sdata[200],len=0,i=0,k=0,frame_num=0,frame_seq=0;
	sEnddev_BasicInf tEnddev_BasicInf;
	uint8 *p=NULL;
	if(Coor_Dev_manage.dev_num == 0)
	{
		return CJP_SUCCESS;
	}
	frame_num=Coor_Dev_manage.dev_num/10+1;//�����豸���������Ҫ���Ͷ���֡����
	frame_seq=1;
	p=sdata;
	len=3;
	for(i=1;i<=Coor_Dev_manage.dev_num;i++)
	{
		if(!GetElem(&Galist , i , &tEnddev_BasicInf))
		{
			return CJP_ERROR;
		}
		memcpy( p+len,  &tEnddev_BasicInf.ycl , sizeof(uYcl));
		len+=sizeof(uYcl);
		memcpy(p+len,&tEnddev_BasicInf.hearttime ,sizeof(uint16));
		len+=sizeof(uint16);
		if(k>=(MAX_DEV_INF_NUM-1))
		{
			sdata[0]=k;
			sdata[1]=frame_seq;
			sdata[2]=frame_num;
			fCJP_Tx_Coor(CIE_Ycl , CJP_READ_END_DEV_LIST_RESP , sdata , len);
			len=3;
			frame_seq++;
			k=0;
		}
		else
		{
			k++;
		}

	}
	sdata[0]=k;
	sdata[1]=frame_seq;
	sdata[2]=frame_num;
	fCJP_Tx_Coor(CIE_Ycl , CJP_READ_END_DEV_LIST_RESP , sdata , len);
	return CJP_SUCCESS;
}


/*
 * ֪ͨJNI���ն��豸������ʱ�����
 * ָ��ID=0x17
 */
PUBLIC CJP_Status fUpdate_End_Dev_Hearttime_Notice(uYcl ycl)
{
	sEnd_Hearttime  tEnd_Hearttime;
	sEnddev_BasicInf tEnddev_BasicInf;
	uint8 sdata[20],len=0,place=0;
	sAttr_Charact tAttr_Charact[2]={
					{E_ZCL_OSTRING , (uint32)(&((sEnd_Hearttime*)(0))->E_YCL), YCL_LENGTH},
					{E_ZCL_UINT16 ,  (uint32)(&((sEnd_Hearttime*)(0))->E_Hearttime) , sizeof(uint16)},

	};
	place = LocateElem(&Galist,ycl);
	if(place == 0)
	{
		return CJP_ERROR;
	}

	if(!GetElem(&Galist,place ,&tEnddev_BasicInf))
	{
		return CJP_ERROR;
	}
	memcpy(&tEnd_Hearttime.E_YCL,&ycl ,sizeof(uYcl));
	memcpy(&tEnd_Hearttime.E_Hearttime,&tEnddev_BasicInf.hearttime ,sizeof(uint16));
	len = fCJP_Attr_Handle(sdata , (uint8 *)&tEnd_Hearttime ,tAttr_Charact ,2);
	Frame_Seq++;//�������͵����к�+1
	return fCJP_Tx_Coor(CIE_Ycl , CJP_UPDATE_END_DEV_HEARTTIME_REQ , sdata , len);
}


PUBLIC CJP_Status fCoor_Self_Test(void)
{
	return CJP_SUCCESS;
	//Э�����Լ첿��
}


PUBLIC CJP_Status fEnd_Read_BasicinfReq( uYcl ycl)
{
	uint16 cluster = BASIC_CLUSTER;
	uint8  attrs[9]={4,E_ZCL_UINT8 , 0xF0 ,E_ZCL_UINT8 ,0xF1,E_ZCL_UINT8 ,0xF2,E_ZCL_UINT8 , 0xF3};//������Ϣ�����Ա�

	fEnd_Read_AttrsReq( ycl , cluster , 8 , attrs);
	return CJP_SUCCESS;
}


/*
 * �����Դ�����
 * mac ��Ŀ�ĵ�ַ
 * cluster��Ŀ��clusterID
 * len    �� �����򳤶�
 * indata ��CJP������ָ��,������CJP֡ͷ����
 *
*/

PUBLIC CJP_Status fEnd_Read_AttrsReq( uYcl ycl , uint16 cluster , uint8 len , uint8 * indata )
{
	//�ж�mac�Ƿ����,
	//�ж�clusterID�Ƿ����
	//����������ݽ���ת��ΪҪ���������б�
	//�������б���ת��ģ��ת��ΪZigbee����ID
	//��֯���ݽ��з���
	uint8 i = 0,place=0;
	uint16 zattrID=0;
	uint8 attrsnum=*indata;//��ȡ�����Ը���
	sEnddev_BasicInf tEnddev_BasicInf;
	sAttr_Model_Array tAttr_Model_Array;
	ZPS_tsAfProfileDataReq psProfileDataReq1;
	static uint8 sqen=1;
	volatile uint16 u16PayloadSize=0;
	PDUM_thAPduInstance hAPduInst;
	DBG_vPrintf(TRACE_APP_UART, "read attrs req    YCL is\n");
	printf_array(&ycl, sizeof(ycl));
	DBG_vPrintf(TRACE_APP_UART, "Attr num  \n", attrsnum);
	if(ZPS_u16AplZdoLookupAddr(ycl.sYCL.Mac)==0x0000){
				return  CJP_ERROR;
	}
	if(attrsnum>10||attrsnum==0){
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

	psProfileDataReq1.uDstAddr.u64Addr = ycl.sYCL.Mac;
	psProfileDataReq1.u16ClusterId = tEnddev_BasicInf.clusterID;//
	psProfileDataReq1.u16ProfileId = PROFILE_ID;
	psProfileDataReq1.u8SrcEp = PORT_NUM;
	psProfileDataReq1.eDstAddrMode= ZPS_E_ADDR_MODE_IEEE;
	psProfileDataReq1.u8DstEp = PORT_NUM;
	psProfileDataReq1.eSecurityMode= ZPS_E_APL_AF_UNSECURE;
	psProfileDataReq1.u8Radius= 0;
	hAPduInst = PDUM_hAPduAllocateAPduInstance(apduZCL);
	if(hAPduInst == NULL){
		DBG_vPrintf(TRACE_APP_UART, "PDUM_hAPduAllocateAPduInstance  error");
				/*�����ڴ治�ɹ�*/
		return CJP_ERROR;

	}
	else{

		sqen = u8GetTransactionSequenceNumber();
		u16PayloadSize = u16ZCL_WriteCommandHeader(hAPduInst,
			                   	   	   	   	   	 eFRAME_TYPE_COMMAND_ACTS_ACCROSS_ENTIRE_PROFILE,//ͳһ�������ʽ
			        		                     TRUE,
			        		                     ZCL_MANUFACTURER_CODE,
			        		                     TRUE,
			        		                     TRUE,
			        		                     &sqen,
			        		                     E_ZCL_READ_ATTRIBUTES);  //������
		// write payload, attribute at a time
		for(i=0; i<attrsnum; i++)
		{
			zattrID=get_zigbee_attrID( &tAttr_Model_Array , *(indata+2+i*2));
			DBG_vPrintf(TRACE_APP_UART, "CattrID is %02x" , *(indata+1+i*2));
			if(zattrID==0)
			{
				DBG_vPrintf(TRACE_APP_UART, "zattrID cannot find " );
				return CJP_ERROR;
			}
			u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h", zattrID);//����ID
		}

		PDUM_eAPduInstanceSetPayloadSize(hAPduInst, u16PayloadSize);
		ZPS_eAplAfApsdeDataReq(hAPduInst,(ZPS_tsAfProfileDataReq*)&psProfileDataReq1,&sqen);
	}
	return CJP_SUCCESS;
}



/*
 * д���Դ�����
 * mac ��Ŀ�ĵ�ַ
 * cluster��Ŀ��clusterID
 * len    �� �����򳤶�
 * indata ��CJP������ָ��,������CJP֡ͷ����
 */

PUBLIC CJP_Status fEnd_Write_AttrsReq( uYcl ycl , uint16 cluster ,  uint8 len , uint8 * indata )
{
	uint8 i = 0, j = 0 ,u8stringlen=0, type_addr_offset=0 ,place=0;
	uint8 attrsnum = *indata;
	uint16 zattrID=0;
	sEnddev_BasicInf tEnddev_BasicInf;
	sAttr_Model_Array tAttr_Model_Array;
	ZPS_tsAfProfileDataReq psProfileDataReq1;
	static uint8 sqen=1;
	volatile uint16 u16PayloadSize=0;
	PDUM_thAPduInstance hAPduInst;
	if(ZPS_u16AplZdoLookupAddr(ycl.sYCL.Mac)==0x0000){
				return  CJP_ERROR;
			}
	if((attrsnum>10)||(attrsnum==0)||(attrsnum%2!=0)){
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
	//��������ID��ת������
	//����yclѰ��clusterID,����clusterIDѰ�����Ե�ID��Ӧ��ϵ��
	psProfileDataReq1.uDstAddr.u64Addr = ycl.sYCL.Mac;
	psProfileDataReq1.u16ClusterId = tEnddev_BasicInf.clusterID;//
	psProfileDataReq1.u16ProfileId = PROFILE_ID;
	psProfileDataReq1.u8SrcEp = PORT_NUM;
	psProfileDataReq1.eDstAddrMode=ZPS_E_ADDR_MODE_IEEE;
	psProfileDataReq1.u8DstEp = PORT_NUM;
	psProfileDataReq1.eSecurityMode=ZPS_E_APL_AF_UNSECURE;
	psProfileDataReq1.u8Radius=0;
	hAPduInst=PDUM_hAPduAllocateAPduInstance(apduZCL);
	if(hAPduInst == NULL){
					/*�����ڴ治�ɹ�*/
		return CJP_ERROR;

	}
	else{

		sqen = u8GetTransactionSequenceNumber();
		u16PayloadSize = u16ZCL_WriteCommandHeader(	hAPduInst,
				                   	   	   	   	   	eFRAME_TYPE_COMMAND_ACTS_ACCROSS_ENTIRE_PROFILE,//ͳһ�������ʽ
				        		                    TRUE,
				        		                    ZCL_MANUFACTURER_CODE,
				        		                    TRUE,
				        		                    TRUE,
				        		                    &sqen,
				        		                    E_ZCL_WRITE_ATTRIBUTES_UNDIVIDED);  //д����
	}
	for(i=0 ; i<(attrsnum/2) ; i++){
		//����ID
		zattrID=get_zigbee_attrID( &tAttr_Model_Array , *(indata+2+type_addr_offset));
		if(zattrID==0)
		{
			return CJP_ERROR;
		}
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h", zattrID);//zigbee����ID
		//������������
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",*(indata+1+1+1+type_addr_offset));//����ID
		//����ֵ
		switch(*(indata+1+1+1+type_addr_offset)){
			case E_ZCL_OSTRING:
				u8stringlen = *(indata+1+type_addr_offset+1);
				if((u8stringlen==0)||(u8stringlen>UART_RX_DATA_MAX_NUM)){
					return CJP_ERROR;
				}

				u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",u8stringlen);//����ID
				type_addr_offset++;
				break;

			default:
				eZCL_GetAttributeTypeSize((teZCL_ZCLAttributeType)*(indata+1+type_addr_offset) , &u8stringlen);	//��ȡ����
				if((u8stringlen==0)||(u8stringlen>UART_RX_DATA_MAX_NUM)){
					return CJP_ERROR;
				}
				break;
		}
		for(j=0;j<u8stringlen;j++){
			u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",*(indata+1+type_addr_offset+1+j));//����ID
		}
		type_addr_offset+=u8stringlen+3;
	}
	PDUM_eAPduInstanceSetPayloadSize(hAPduInst, u16PayloadSize);
	ZPS_eAplAfApsdeDataReq(hAPduInst,(ZPS_tsAfProfileDataReq*)&psProfileDataReq1,&sqen);
	return CJP_SUCCESS;
}


/*
 * �����Դ�����
 * mac ��Ŀ�ĵ�ַ
 * cluster��Ŀ��clusterID
 * indata ��������ָ�루��ʱû�ã�
 */
PUBLIC CJP_Status fEnd_Alarm_ReportResp( uYcl ycl,uint16 cluster ,uint8 len , uint8 * indata )
{
	//֪ͨ�ն˲�Ҫ���ͱ��������ˡ�
	fEnd_Write_AttrsReq( ycl , cluster ,  len , indata );
	return CJP_SUCCESS;
}

/*
 * CJP�������ͨ�����ݺϳɺ���
 * tAttr_Charact[]:�������ݱ������������͡�ƫ�Ƶ�ַ�����ȵ�
 *  *tarray       :Ҫ���������
 *  * sourcedata  :Դ����
  */

PUBLIC uint8 fCJP_Attr_Handle(uint8 *tarray , uint8 * sourcedata ,sAttr_Charact *tAttr_Charact , uint8 attrnum)
{
	uint8  num=0, i=0,len=0;
	uint8 *dst;
	uint8 *srt;
	dst = tarray;
	srt = sourcedata;
	num = attrnum;
	DBG_vPrintf(TRACE_APP_UART," num = %x" , num);
	*dst = num;
	len++;
	for(i=0;i<num;i++)
	{
		*(dst+len) = tAttr_Charact[i].data_type;
		len++;
		if(tAttr_Charact[i].data_type == E_ZCL_OSTRING)//�������͵����ݣ��������ݳ���
		{
			*(dst+len) = tAttr_Charact[i].datalenth;
			len++;
		}
		memcpy(  dst+len , srt+tAttr_Charact[i].Offsetlength ,   *(srt+tAttr_Charact[i].Offsetlength));
		len +=  tAttr_Charact[i].datalenth;
	}
	return len;
}






