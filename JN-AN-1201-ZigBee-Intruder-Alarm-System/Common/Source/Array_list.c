/*
 * Array_list.c
 *
 *  Created on: 2018��3��23��
 *      Author: 1
 *      ���ļ�ʵ�����Ա��ʵ��
 */

//������ʵ�����Ա�
#include "jendefs.h"
#include "Array_list.h"


//��ʼ��˳���������ʼ������
bool Array_init(sarrayList *alist,ElemType * arrayhead , uint8 totalsize )
{

	alist->Array = arrayhead;
	alist->current_num=0;
	alist->size=totalsize;
    if(NULL==alist->Array)
        return FALSE;
    else
        return TRUE;
}

//ɾ��˳���
void DeleteArray(sarrayList  * alist)
{
	alist->Array = NULL;
	alist->current_num = 0;
	alist->size = 0;

}



//�ж�˳���б�Ϊ��
bool IsEmpty(sarrayList * alist)
{
    if(0==alist->current_num)
    {
        printf("the arrayList is empty!\n");
        return TRUE;
    }
    else
    {
        printf("the arrayList is not empty!\n");
        return FALSE;
    }

}

//�б����ж��ٸ�Ԫ��
uint8 ArrayLength(sarrayList *alist)
{
    return alist->current_num;
}

//ȡ��ĳ��Ԫ��
bool GetElem(sarrayList* alist,uint8 index,ElemType *e)
{
    if((index>ArrayLength(alist))||(index==0))
    {
        printf("��Ч�ĵ�ַ!\n");
        return FALSE;
    }
    else
    {
        *e=alist->Array[index-1];
        return TRUE;
    }
}

//����˳�������ӡ
void PrintfArray(sarrayList *alist)
{
	uint8 i=0;
    printf("array elements are: ");
    for(i=0;i<alist->current_num;i++)
    {
        printf("%d\t",alist->Array[i]);
    }
    printf("\n");
}

//�ж�ĳ��Ԫ�ص�λ��
//ͨ��YCL���ж�Ԫ�ص�λ��
uint8 LocateElem(sarrayList *alist,uYcl ycl)
{
	uint8 i=0;
    for(i=0;i<ArrayLength(alist);i++)
    {
    	if(memcmp(alist->Array[i].ycl,ycl,sizeof(uYcl))==0)//�ڴ�ȽϺ���
    	{
    		return i+1;
    	}
    }
    return 255;//�����ش���
}

//���ĳ��Ԫ��
bool AddElem(sarrayList *alist,ElemType e)
{
	uint8 index=0;
     //���˳���洢�ռ�����������Ҫ���·����ڴ�
    if(alist->current_num==alist->size)
    {
    	return FALSE;//�б�����
    }
    index=ArrayLength(alist);
    alist->Array[index]=e;
    ++alist->current_num;
    return TRUE;
}

//ɾ��ĳ��Ԫ��
bool DelElem(sarrayList *alist,uint8 index)
{
	uint8 i=0;
    //���жϲ���λ���Ƿ�Ϸ�
    if((index==0)||(index>ArrayLength(alist)))
    {
        return FALSE;
    }
    for(i = index; i < ArrayLength(alist); i++)
    {
        alist->Array[i-1]=alist->Array[i];
    }
    --alist->current_num;

    return TRUE;
}
