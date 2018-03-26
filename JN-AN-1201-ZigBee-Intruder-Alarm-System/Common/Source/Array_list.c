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


sarrayList     arraylist;


//��ʼ��˳���������ʼ������
bool Array_init(ElemType * arrayhead , uint8 totalsize )
{

	arraylist->Array= arrayhead;
    arraylist->current_num=0;
    arraylist->size=totalsize;
    if(NULL==arraylist->Array)
        return FALSE;
    else
        return TRUE;
}
/*
//ɾ��˳���
void deleteArray(arrayList* arrLst)
{
    arrLst->length = 0;
    arrLst->size = 0;
    free(arrLst->Array);
    arrLst->Array = NULL;
}

//���˳���
void clearArray(arrayList *arrLst)
{
    arrLst->length = 0;
}

//�ж��Ƿ�Ϊ��
bool is_empty(arrayList *arrLst)
{
    if(0==arrLst->length)
    {
        printf("the arrayList is empty!\n");
        return true;
    }
    else
    {
        printf("the arrayList is not empty!\n");
        return false;
    }

}

//���ж��ٸ�Ԫ��
int arrayLength(arrayList *arrLst)
{
    return arrLst->length;
}
//ȡ��ĳ��Ԫ��
bool getElem(arrayList* arrLst,int index,ElemType *e)
{
    if(index<0||index>arrayLength(arrLst)-1)
    {
        printf("�����߽�!\n");
        return false;
    }
    else
    {
        *e=arrLst->Array[index];
        return true;
    }
}

//����˳�������ӡ
void printfArray(arrayList *arrLst)
{
    printf("array elements are: ");
    for(int i=0;i<arrLst->length;i++)
    {
        printf("%d\t",arrayList->Array[i]);
    }
    printf("\n");
}

//�ж�ĳ��Ԫ�ص�λ��
int locateElem(arrayList *arrLst,ElemType e)
{
    for(int i=0;i<arrayLength(arrLst);i++)
    {
        if(e==arrLst->Array[i])
            return i;
    }
    return -1;
}

//��ĳ��Ԫ�ص�ǰ�������û�ҵ�����-2������ǵ�һ��Ԫ�ء�����-1�����򷵻�ǰ��Ԫ�ص��±�
int preElement(arrayList *arrLst,ElemType e,ElemType *preElem)
{
    for(int i=0;i<arrayLength(arrLst);i++)
    {
        if(e==arrLst->Array[i])
        {
            if(i==0)
            {
                return -1;
            }
            else
            {
                preElem=arrLst->Array[i-1];
                return i-1;
            }
        }
    }
    return -2;
}


//��ĳ��Ԫ�صĺ�̣����û�ҵ�������-2,��������һ��Ԫ�أ�����-1�����򷵻غ��Ԫ�ص��±�
int nextElement(arrayList *arrLst,ElemType e,ElemType *nxtElem)
{
    for(int i = 0; i < arrayLength(arrLst); ++i)
    {
        if(e == arrLst->Array[i])
        {
            if(arrayLength(arrLst) -1 == i)
            {
                return -1;
            }
            else
            {
                *nxtElem = arrLst->Array[i+1];
                return i+1;
            }
        }
    }
    return -2;
}

//��Ԫ�ز��뵽ָ��λ��
bool insertElem(arrayList *arrLst,int index,ElemType e)
{
    //���жϲ���λ���Ƿ�Ϸ�
    if(index<0||index>arrayLength(arrLst)-1)
    {
        return false;
    }
     //���˳���洢�ռ�����������Ҫ���·����ڴ�
     if(arrLst->length==arrLst->size)
     {
        arrLst->Array=(ElemType*)reolloc(arrLst->Array,2*arrLst->size*sizeof(ElemType));
        if(NULL==arrLst->Array)
            return false;//����ռ�ʧ��
        else
        {
            arrLst->size*=2;
        }
     }
    for(int i = index; i < arrayLength(arrLst); ++i)
    {
        arrLst->Array[i+1]=arrLst->Array[i];
    }
    arrLst->Array[index]=e;
    ++arrLst->length;

    return true;
}

//ɾ��ĳ��Ԫ��
bool deleteElem(arrayList *arrLst,int index ,ElemType *e)
{
    //���жϲ���λ���Ƿ�Ϸ�
    if(index<0||index>arrayLength(arrLst)-1)
    {
        return false;
    }
    *e=array->Array[index];
    for(int i = index; i < arrayLength(arrLst); ++i)
    {
        arrLst->Array[i]=arrLst->Array[i+1];
    }

    --arrLst->length;

    return true;
}
