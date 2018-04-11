/*
 * Array_list.h
 *
 *  Created on: 2018��3��23��
 *      Author: 1
 *
 */

#ifndef ARRAY_LIST_H_
#define ARRAY_LIST_H_


#include "app_CIE_uart.h"
#include "app_events.h"

//���ò���
typedef sEnddev_BasicInf ElemType;


typedef struct {
    ElemType *Array;//ʵ�ʴ��Ԫ�ص�����
    uint8 current_num;//���е�Ԫ�ظ���
    uint8 size;      //������������
}sarrayList;

extern sarrayList Galist;
PUBLIC bool Array_init(sarrayList *alist,ElemType * arrayhead , uint8 totalsize ,uint8 num);
PUBLIC void DeleteArray(sarrayList  * alist);
PUBLIC bool IsEmpty(sarrayList * alist);
PUBLIC uint8 ArrayLength(sarrayList *alist);
PUBLIC bool GetElem(sarrayList* alist,uint8 index,ElemType *e);
PUBLIC void PrintfArray(sarrayList *alist);
PUBLIC uint8 LocateElem(sarrayList *alist,uYcl ycl);
PUBLIC bool AddElem(sarrayList *alist,ElemType e);
PUBLIC bool DelElem(sarrayList *alist,uint8 index);


#endif /* ARRAY_LIST_H_ */
