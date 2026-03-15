#ifndef NET_H
#define NET_H

typedef struct Layer
{
    float *w;
    float *b;
    float *buf_z;
    float *buf_a;
    int in_dim;
    int out_dim;
    int is_relu;
    struct Layer *next;
    struct Layer *prev;
} Layer;

typedef struct
{
    Layer *head;
    Layer *tail;
} Net;

Layer *create_layer(int in_dim, int out_dim, int is_relu);
void net_add_layer(Net *net, Layer *new_layer);
void net_destroy(Net *net);
void forward(const Net *net, const float *x, float *y);
void backward(Net *net, const float *input, const float *loss, float lr,
              float *delta_buf, float *dw_buf);

#endif