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

//���ò���
typedef sEnddev_BasicInf ElemType;

typedef struct {
    ElemType *Array;//ʵ�ʴ��Ԫ�ص�����
    uint8 current_num;//���е�Ԫ�ظ���
    uint8 size;      //������������
}sarrayList;


#endif /* ARRAY_LIST_H_ */
