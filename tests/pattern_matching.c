
#define NULL ((void *)0)
#define UNUSED(x) ((void)x)

#define RANGE(x, from, to) ((x) >= (from) && (x) <= (to))

typedef struct {
    const char *chars;
    int length, cap;
} lavender_string;

typedef struct untyped_array_t {
    void *data;
    int member_size;
    int size, cap;
} Untyped_Array;

typedef int len_return_type;
len_return_type len(Untyped_Array x) { return x.size; }

typedef int lavender_main_return_type;
lavender_main_return_type lavender_main(Untyped_Array args) {
    lavender_main_return_type ___ret_value;
    {
        len_return_type __switch_condition = len(args);
        if (__switch_condition < 0) ___ret_value = 0;
        else if (__switch_condition == 0) ___ret_value = 1;
        else if (RANGE(__switch_condition, 1, 5)) ___ret_value = 2;
        else if (__switch_condition > 10) ___ret_value = 3;
        else ___ret_value = 4;
    }
    return ___ret_value;
}

int main(int argc, char *argv[]) {
    Untyped_Array args = {NULL, sizeof(lavender_string), argc, argc};
    for (int i = 0; i < argc; i++) {
    }
    lavender_main_return_type result = lavender_main(args);
    return result;
}