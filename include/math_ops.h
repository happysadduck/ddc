#ifndef MATH_OPS_H
#define MATH_OPS_H

#include <stdio.h>

typedef struct
{
    float *w1; // [128][784]
    float *b1; // [128]
    float *w2; // [10][128]
    float *b2; // [10]
} Network;

void mat_mul(const float *A, const float *B, float *C, int m, int n, int k);
void vec_add(const float *a, const float *b, float *out, int n);
void relu(float *x, int n);
void softmax(float *x, int n);
void init_weights(float *w, int size, float scale);
void outer_product(const float *a, const float *b, float *c, int m, int n);
void mat_vec_mul_transpose(const float *A, const float *v, float *out, int rows_A, int cols_A);
float cross_entropy(const float *y_pred, int label);

#endif