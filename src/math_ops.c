#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "math_ops.h"
#include "net.h"

/*
 * 矩阵乘法：C = A * B
 * 其中 A 是 m×k 矩阵，B 是 k×n 矩阵，C 是 m×n 矩阵（行优先存储）
 * @param A 左矩阵，大小为 m×k
 * @param B 右矩阵，大小为 k×n
 * @param C 输出矩阵，大小为 m×n，函数执行前不必初始化
 * @param m 矩阵 A 的行数，矩阵 C 的行数
 * @param n 矩阵 B 的列数，矩阵 C 的列数
 * @param k 矩阵 A 的列数，矩阵 B 的行数
 */
void mat_mul(const float *A, const float *B, float *C, int m, int n, int k)
{
    for (int i = 0; i < m; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            C[i * n + j] = 0.0f;
        }
    }

    for (int i = 0; i < m; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            float sum = 0.0f;
            for (int p = 0; p < k; ++p)
            {
                sum += A[i * k + p] * B[p * n + j];
            }
            C[i * n + j] = sum;
        }
    }
}

/*
 * 向量加法：out = a + b （逐元素相加）
 * @param a 输入向量，长度为 n
 * @param b 输入向量，长度为 n
 * @param out 输出向量，长度为 n，与 a、b 可重叠
 * @param n 向量的长度
 */
void vec_add(const float *a, const float *b, float *out, int n)
{
    for (int i = 0; i < n; ++i)
    {
        out[i] = a[i] + b[i];
    }
}

/*
 * ReLU 激活函数（原地操作）：x = max(0, x)
 * 将输入向量 x 中所有负值元素置为 0，正值保持不变
 * @param x 输入/输出向量，长度为 n，函数执行后会被修改
 * @param n 向量的长度
 */
void relu(float *x, int n)
{
    for (int i = 0; i < n; ++i)
    {
        if (x[i] < 0.0f)
        {
            x[i] = 0.0f;
        }
    }
}

/*
 * Softmax 函数（原地操作）：将输入向量 x 转换为概率分布
 * 计算过程：先减去最大值防止溢出，再取指数并归一化
 * @param x 输入/输出向量，长度为 n，函数执行后得到 softmax 结果
 * @param n 向量的长度
 */
void softmax(float *x, int n)
{
    float max_val = x[0];
    for (int i = 1; i < n; ++i)
    {
        if (x[i] > max_val)
        {
            max_val = x[i];
        }
    }

    float sum = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        x[i] = expf(x[i] - max_val);
        sum += x[i];
    }

    for (int i = 0; i < n; ++i)
    {
        x[i] /= sum;
    }
}

/*
 * 初始化权重数组：生成在 [-scale, scale] 范围内均匀分布的随机数
 * @param w 输出权重数组，长度为 size
 * @param size 数组长度
 * @param scale 缩放因子，决定随机数的范围
 */
void init_weights(float *w, int size, float scale)
{
    for (int i = 0; i < size; ++i)
    {
        w[i] = (rand() / (float)RAND_MAX) * 2.0f * scale - scale;
    }
}

/*
 * 外积：计算向量 a 与向量 b 的外积，结果存入矩阵 c
 * 即 c[i][j] = a[i] * b[j]，其中 c 是 m×n 矩阵（行优先）
 * @param a 左向量，长度为 m
 * @param b 右向量，长度为 n
 * @param c 输出矩阵，大小为 m×n，需预先分配
 * @param m 向量 a 的长度，矩阵 c 的行数
 * @param n 向量 b 的长度，矩阵 c 的列数
 */
void outer_product(const float *a, const float *b, float *c, int m, int n)
{
    for (int i = 0; i < m; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            c[i * n + j] = a[i] * b[j];
        }
    }
}

/*
 * 矩阵（转置）乘向量：计算 A^T * v，结果存入 out
 * 其中 A 是 rows_A × cols_A 矩阵（行优先），v 是长度为 rows_A 的向量
 * 结果 out 是长度为 cols_A 的向量，out[j] = Σ_i A[i][j] * v[i]
 * @param A 输入矩阵，大小为 rows_A × cols_A
 * @param v 输入向量，长度为 rows_A
 * @param out 输出向量，长度为 cols_A
 * @param rows_A 矩阵 A 的行数
 * @param cols_A 矩阵 A 的列数
 */
void mat_vec_mul_transpose(const float *A, const float *v, float *out, int rows_A, int cols_A)
{
    for (int j = 0; j < cols_A; ++j)
    {
        float sum = 0.0f;
        for (int i = 0; i < rows_A; ++i)
        {
            sum += A[i * cols_A + j] * v[i];
        }
        out[j] = sum;
    }
}

/*
 * 交叉熵损失函数：计算单个样本的交叉熵损失
 * 对于真实标签 label，损失为 -log(y_pred[label])，并添加微小常数防止 log(0)
 * @param y_pred 预测的概率分布向量（softmax 输出），长度为类别数
 * @param label 真实类别索引（从 0 开始）
 * @return 交叉熵损失值
 */
float cross_entropy(const float *y_pred, int label)
{
    return -logf(y_pred[label] + 1e-7f);
}