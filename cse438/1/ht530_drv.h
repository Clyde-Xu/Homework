#ifndef HT530_DRV_H
#define HT530_DRV_H

typedef struct ht_object {
    int key;
    int data;
} ht_object_t;

typedef struct dump_arg {
    int n;
    ht_object_t object_array[8];
} dump_arg_t;

#define DUMP _IOR('q', 1, dump_arg_t *)

#endif
