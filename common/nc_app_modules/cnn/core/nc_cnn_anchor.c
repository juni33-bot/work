/**
********************************************************************************
* Copyright (C) 2022 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************
*/
/**
********************************************************************************
* @file    : nc_cnn_anchor.c
*
* @brief   : nc_cnn_anchor source
*
* @author  : SoC SW team.  NextChip Inc.
*
* @date    : 2022.11.10
*
* @version : 1.0.0
********************************************************************************
* @note
* 2022.11.10 / 1.0.0 / Initial released.
*
********************************************************************************
*/

#include <math.h>

#include "nc_cnn_anchor.h"

float Box_iou(stBBox *a, stBBox *b);
float Box_intersection(stBBox *a, stBBox *b);
float Box_union(stBBox *a, stBBox *b);
float Overlap(float x1, float w1, float x2, float w2);

float nc_Logistic_activate(float x)
{
    return (float)(1. / (1. + exp(-x)));
}

void nc_Nms_box_yolov8(detection *dets, int *total, int classes, float thresh)
{
    int i, j, k;

     for (k = 0; k < classes; ++k) {
        for (i = 0; i < *total; ++i) {
            if (dets[i].prob[k] == 0) continue;
            stBBox *a = &dets[i].bbox;
            for (j = i + 1; j < *total; ++j) {
            stBBox *b = &dets[j].bbox;
                if (Box_iou(a, b) > thresh) {
                    for(int cls = 0; cls < classes ; ++cls){
                        //모든 class index의 score 비교하여 1개의 box만 남도록 nms
                        if (dets[i].prob[k] >= dets[j].prob[cls])
                        {
                            dets[j].prob[cls] = 0;
                        }
                        else
                        {
                            dets[i].prob[k] = 0;
                        }
                    }
                }
            }
        }
    }
}

void nc_Nms_box(detection *dets, int *total, int classes, float thresh)
{
    int i, j, k, m;

    for (k = 0; k < classes; ++k) {
        for (i = 0; i < *total; ++i) {
            if (dets[i].prob[k] == 0 && dets[i].objectness == 0) continue;
            stBBox *a = &dets[i].bbox;
            for (j = i + 1; j < *total; ++j) {
                stBBox *b = &dets[j].bbox;
                if (Box_iou(a, b) >= thresh) {
                    if (dets[i].objectness >= dets[j].objectness)
                    {
                        dets[j].objectness = 0;
                        for (m = 0; m < classes; ++m) {
                            dets[j].prob[k] = 0;
                        }
                    }
                    else
                    {
                        dets[i].objectness = 0;
                        for (m = 0; m < classes; ++m) {
                            dets[i].prob[k] = 0;
                        }
                        continue;
                    }
                }
            }
        }
    }
}

//class별 score비교
void nc_Nms_box_by_class(detection *dets, int *total, int classes, float thresh)
{
    int i, j, k;
    int pred_class = 0;
    float best_score = 0;
    int class_index = 0;
    int class_index_overlap = 0;
    float curr_score = 0;

    for (int i = 0; i < *total; ++i)
    {
        //pull out best score
        for (class_index = 0; class_index < classes; ++class_index) {
            curr_score = dets[i].prob[class_index];
            if (curr_score >= best_score) {
                if (curr_score == best_score) {
                    class_index_overlap = 1;
                }
                else{
                    class_index_overlap = 0;
                }
                best_score = curr_score;
                pred_class = class_index;
            }
        }

        //best score인 클래스 이외의 클래스 score들은 모두 0으로 만듦
        for (class_index = 0; class_index < classes; ++class_index) {
            if(class_index_overlap == 1){
                dets[i].prob[class_index] = 0; //다른 class끼리 best score가 겹치면 detection결과에서 제외
            }
            else
            {
                if(class_index != pred_class){
                    dets[i].prob[class_index] = 0;
                }
            }
        }

        //다른 class끼리 best score가 겹치면 detection결과에서 제외
        if(class_index_overlap == 1) dets[i].objectness = 0;

        best_score = 0;
        pred_class = 0;
    }

    //nms비교
    for (k = 0; k < classes; ++k) {
        for (i = 0; i < *total; ++i) {
            if (dets[i].prob[k] == 0) continue;
            stBBox *a = &dets[i].bbox;
            for (j = i + 1; j < *total; ++j) {
                if(dets[j].prob[k] != 0) {
                    stBBox *b = &dets[j].bbox;
                    if (Box_iou(a, b) >= thresh) {
                        if (dets[i].prob[k] >= dets[j].prob[k])
                        {
                            dets[j].prob[k] = 0;
                        }
                        else
                        {
                            dets[i].prob[k] = 0;
                        }
                    }
                    else //iou값이 nms threshoid값보다는 낮지만 박스 끼리 겹쳤을때 제거
                    {
                        if(Box_iou(a, b) > 0)
                        {
                            // b박스가 a박스 내부에 있을때 b박스 제거
                            if(((a->x - a->w / 2) <= b->x))
                            {
                                if((b->x <= (a->x + a->w / 2)))
                                {
                                    if(((a->y - a->h / 2) <= b->y))
                                    {
                                        if((b->y <= (a->y + a->h / 2)))
                                        {
                                            if (dets[i].prob[k] >= dets[j].prob[k])
                                            {
                                                dets[j].prob[k] = 0;
                                            }
                                            else
                                            {
                                                dets[i].prob[k] = 0;
                                            }
                                        }
                                    }
                                }
                            }
                            // a박스가 b박스 내부에 있을때 a박스 제거
                            else if(((b->x - b->w/2) <= a->x))
                            {
                                if((a->x <= (b->x + b->w/2)))
                                {
                                    if(((b->y - b->h/2) <= a->y))
                                    {
                                        if((a->y <= (b->y + b->h/2)))
                                        {
                                            if (dets[i].prob[k] >= dets[j].prob[k])
                                            {
                                                dets[j].prob[k] = 0;
                                            }
                                            else
                                            {
                                                dets[i].prob[k] = 0;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

float Box_intersection(stBBox *a, stBBox *b)
{
    float w = Overlap(a->x, a->w, b->x, b->w);
    if (w <= 0) return 0;
    float h = Overlap(a->y, a->h, b->y, b->h);
    if (h <= 0) return 0;
    float area = w * h;
    return area;
}

float Overlap(float x1, float w1, float x2, float w2)
{
    float l1 = x1 - w1 / 2;
    float l2 = x2 - w2 / 2;
    float left = l1 > l2 ? l1 : l2;
    float r1 = x1 + w1 / 2;
    float r2 = x2 + w2 / 2;
    float right = r1 < r2 ? r1 : r2;
    return right - left;
}

float Box_union(stBBox *a, stBBox *b)
{
    float i = Box_intersection(a, b);
    float u = a->w*a->h + b->w*b->h - i;
    return u;
}

float Box_iou(stBBox *a, stBBox *b)
{
    float I = Box_intersection(a, b);
    if (I <= 0) return 0;
    float U = Box_union(a, b);
    if (U <= 0) return 0;
    return I / U;
}
