#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "net.h"
#include "math_ops.h"

/**
 * 创建一个新的网络层。
 *
 * 该函数一次性分配所有所需内存，包括 Layer 结构体本身以及权重、偏置、
 * 线性输出缓冲区 (buf_z) 和激活输出缓冲区 (buf_a)。所有权重和偏置被初
 * 始化为 [-0.1, 0.1] 范围内的随机值。buf_z 和 buf_a 未初始化，将在前向
 * 传播时被填充。
 *
 * @param in_dim  输入维度（前一层的神经元数量）。
 * @param out_dim 输出维度（本层神经元数量）。
 * @param is_relu 若为非零，表示本层使用 ReLU 激活函数；否则视为无激活（线性层）。
 * @return 指向新创建的 Layer 的指针，失败时返回 NULL（例如 malloc 失败）。
 *
 * @note 返回的 Layer 必须通过 free() 释放，但建议使用 net_destroy 统一释放。
 *       本函数不建立链表关系，需调用 net_add_layer 将层添加到网络中。
 */
Layer *create_layer(int in_dim, int out_dim, int is_relu)
{
    char *data = (char *)malloc((in_dim * out_dim + out_dim * 3) * sizeof(float) + sizeof(Layer));
    Layer *out = (Layer *)data;
    out->w = (float *)(data + sizeof(Layer));
    out->b = out->w + in_dim * out_dim;
    out->buf_z = out->b + out_dim;
    out->buf_a = out->buf_z + out_dim;
    init_weights(out->w, in_dim * out_dim, 0.1f);
    init_weights(out->b, out_dim, 0.1f);
    out->in_dim = in_dim;
    out->out_dim = out_dim;
    out->is_relu = is_relu;
    return out;
}

/**
 * 将一个已创建的层添加到网络的尾部。
 *
 * 该函数将 new_layer 链接到 net 的当前尾部之后，更新 net 的 tail 指针。
 * 如果网络当前为空（head == NULL），则将 new_layer 同时设为头尾。
 *
 * @param net       指向网络结构体的指针（必须有效）。
 * @param new_layer 指向要添加的层的指针（必须由 create_layer 创建且未被添加过）。
 *
 * @note 该函数不检查 new_layer 是否已属于其他网络，调用者需保证正确性。
 *       添加后，new_layer 的生命周期由 net 管理，最终应通过 net_destroy 释放。
 */
void net_add_layer(Net *net, Layer *new_layer)
{
    new_layer->next = NULL;
    if (!net->tail)
    {
        net->head = net->tail = new_layer;
        new_layer->prev = NULL;
        return;
    }
    net->tail->next = new_layer;
    new_layer->prev = net->tail;
    net->tail = new_layer;
}

/**
 * 销毁整个网络，释放所有层占用的内存。
 *
 * 遍历网络中的每一层，调用 free() 释放层内存（因为层是通过单次 malloc 分配的）。
 * 注意：该函数不释放 Net 结构体本身，因为 Net 通常为栈上分配。调用者需自行管理
 * Net 变量的生命周期（例如在栈上定义 Net 变量，无需释放）。
 *
 * @param net 指向要销毁的网络的指针。函数执行后，net 的 head 和 tail 将被置为 NULL，
 *            但 net 指针本身仍有效，可重新用于构建新网络。
 *
 * @warning 确保在调用本函数后不再使用已释放的层指针。
 *          若 Net 是通过动态分配（如 malloc）获得的，调用者需在 net_destroy 后
 *          额外 free(net)。
 */
void net_destroy(Net *net)
{
    Layer *curr = net->head;
    while (curr)
    {
        Layer *next = curr->next;
        free(curr);
        curr = next;
    }
    net->head = net->tail = NULL;
}

/**
 * 前向传播函数。
 *
 * 逐层计算网络输出。每层先计算线性变换 z = W * prev_a + b 并存入该层的 buf_z，
 * 然后复制到 buf_a，若该层使用 ReLU 激活，则对 buf_a 进行 in-place ReLU 操作。
 * 最终若 y 非空，将最后一层的 buf_a 复制到 y 中。
 *
 * @param net 指向网络的指针，网络必须至少包含一层且各层的 buf_z 和 buf_a 已分配好内存。
 * @param x   输入数据指针，长度为输入层的 in_dim（通常为 784），应为 float 数组。
 * @param y   输出缓冲区指针，若非 NULL，则长度必须至少为输出层的 out_dim（通常为 10）。
 *            函数会将最后一层的激活输出复制到 y 中。若为 NULL，则不复制。
 *
 * @note 该函数假设网络已经通过 net_add_layer 正确构建，且各层的权重、偏置已初始化。
 *       前向过程中，每层的 buf_z 和 buf_a 会被覆盖，因此调用者无需预先初始化它们。
 *       如果网络为空（head == NULL），函数将直接返回（无操作）。
 */
void forward(const Net *net, const float *x, float *y)
{
    Layer *curr = net->head;
    const float *prev_a = x;
    while (curr)
    {
        mat_mul(curr->w, prev_a, curr->buf_z, curr->out_dim, 1, curr->in_dim);
        vec_add(curr->buf_z, curr->b, curr->buf_z, curr->out_dim);
        memcpy(curr->buf_a, curr->buf_z, curr->out_dim * sizeof(float));
        if (curr->is_relu)
            relu(curr->buf_a, curr->out_dim);
        prev_a = curr->buf_a;
        curr = curr->next;
    }
    if (y)
        memcpy(y, net->tail->buf_a, net->tail->out_dim * sizeof(float));
}

/**
 * 反向传播函数，执行梯度计算并更新网络参数。
 *
 * 从输出层开始逐层向前传播误差，计算权重梯度并更新权重和偏置。
 * 需要调用者提供临时缓冲区 delta_buf 和 dw_buf，以避免重复分配内存。
 *
 * @param net       指向网络的指针，网络必须已通过前向传播计算好各层的 buf_z 和 buf_a。
 * @param input     输入数据指针，与 forward 中使用的 x 相同，长度为输入层 in_dim。
 * @param loss      输出层的误差梯度（即 delta），长度为输出层 out_dim。
 *                  通常为 softmax 输出减去 one-hot 标签向量：y_pred - y_true。
 * @param lr        学习率，用于参数更新。
 * @param delta_buf 临时缓冲区，用于存储传播给上一层的梯度（未乘激活导数前的原始梯度）。
 *                  大小至少为网络中最大层的输出维度（max_out_dim）。该缓冲区的内容会在
 *                  函数执行过程中被覆盖，调用者无需初始化。
 * @param dw_buf    临时缓冲区，用于存储当前层的权重梯度。大小至少为最大层的
 *                  in_dim * out_dim（即最大权重矩阵元素个数）。该缓冲区也会被覆盖。
 */
void backward(Net *net, const float *input, const float *loss, float lr,
              float *delta_buf, float *dw_buf)
{
    Layer *curr = net->tail;
    const float *last_loss = loss;

    while (curr)
    {
        const float *prev_a = (curr == net->head) ? input : curr->prev->buf_a;
        outer_product(last_loss, prev_a, dw_buf, curr->out_dim, curr->in_dim);

        for (int i = 0; i < curr->out_dim; i++)
            curr->b[i] -= lr * last_loss[i];

        if (curr->prev)
        {
            mat_vec_mul_transpose(curr->w, last_loss, delta_buf,
                                  curr->out_dim, curr->in_dim);
            if (curr->prev->is_relu)
            {
                for (int i = 0; i < curr->prev->out_dim; i++)
                {
                    if (curr->prev->buf_z[i] <= 0)
                        delta_buf[i] = 0.0f;
                }
            }
            last_loss = delta_buf;
        }
        for (int i = 0; i < curr->out_dim * curr->in_dim; i++)
            curr->w[i] -= lr * dw_buf[i];

        curr = curr->prev;
    }
}