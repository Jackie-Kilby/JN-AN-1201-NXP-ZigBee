/******************************************************************
 * app_data_handle.c
 *
 * ���ļ���Ҫ�Ƕ����ݵĴ��������ݵ���ȡ�����ݵ����ϵȵ�
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






/*
 * ����Ҫ���͸�JNI�����Ϣ���и�ʽת����
 * ��������ID��Ҫ���͵�������ṹ�壬�������ͳ�����Ҫ��ŵĵ�ַ��
 *
 */

PUBLIC CJP_Status  CoortoJNI_DataSw ( CJP_CmdID tCJP_CmdID )
{


}


/*
 * ����YCLѰ��ClusterID
 */
PUBLIC uint16 ClusterID_Search(uYcl ycl)
{
	uint8 loca=0;
	sEnddev_BasicInf  BasicInf;
	loca=LocateElem(&Galist,ycl);
	if(loca==0)
	{
		return 0;
	}

	if(GetElem(&Galist,loca,&BasicInf))
	{
		return BasicInf.clusterID;
	}
	return 0;
}


/*
 * ���ϲ����Ա��ת��ΪZigBee��׼������ID
 *
 */
PUBLIC uint16 AttrID_CJP_to_Zigbee(uYcl ycl , uint8 attrid)
{
	//���ٸ�cluster
	uint8 i=0,j=0;
	uint16 tclusterid;
	tclusterid = ClusterID_Search(ycl);
	if(tclusterid == 0)
	{
		return 0;
	}
	for(i=0;i<Coor_Dev_manage.model_num;i++)
	{
		if(Attr_Model_Array[i].Attr_Model[0].head.clusterID == tclusterid)
		{
			for(j=0;j<Attr_Model_Array[i].Attr_Model[0].head.attrnum;j++)
			{
				if(Attr_Model_Array[i].Attr_Model[j+1].attr.CattrID == attrid)
				{
					return Attr_Model_Array[i].Attr_Model[j+1].attr.zattrID;
				}
			}
		}

	}
	return 0;//û�д�����ID


}


/*
 * ��ZigBee����IDת��Ϊ�ϲ�����Ա��
 */
PUBLIC uint8 AttrID_Zigbee_to_CJP(uint16 clusterid , uint16 attrid)
{
	uint8 i=0 , j=0;
	for(i=0;i<Coor_Dev_manage.model_num;i++)
	{
		if(Attr_Model_Array[i].Attr_Model[0].head.clusterID == clusterid)
		{
			for(j=0;j<Attr_Model_Array[i].Attr_Model[0].head.attrnum;j++)
			{
				if(Attr_Model_Array[i].Attr_Model[j+1].attr.zattrID== attrid)
				{
					return Attr_Model_Array[i].Attr_Model[j+1].attr.CattrID;
				}
			}
			return 0;
		}

	}
	return 0;//û�д�����ID

}


/*
 * ����
 */

PUBLIC usLinkKey Linkkey_Calculate( uYcl Ycl)
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

	linkkey_tmp.sLinkkey.LinkKey_YCL=ycl_tmp;
	linkkey_tmp.sLinkkey.LinkKey_Last4By=((uint32)*(&(ycl_tmp.YCL_Array[11]))+(uint32)*(&(ycl_tmp.YCL_Array[7]))+(uint32)*(&(ycl_tmp.YCL_Array[3])))/0xFFFF;
	return linkkey_tmp;
}

PUBLIC uint8  test1(void)
{
	return 1;
}




