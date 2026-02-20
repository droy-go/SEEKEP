/*
 * SEEKEP - Smart Expressive Efficient Knowledge Execution Platform
 * الجهاز الافتراضي (VM) - Virtual Machine
 * 
 * ينفذ بايتكود SEEKEP
 */

#ifndef VM_H
#define VM_H

#include "compiler.h"
#include "seekep.h"

/* حجم المكدس */
#define SKP_STACK_MAX 65536
#define SKP_FRAMES_MAX 64
#define SKP_GC_THRESHOLD 1024 * 1024  /* 1MB */

/* إطار الاستدعاء */
typedef struct {
    chunk_t* chunk;          /* كتلة البايتكود */
    uint8_t* ip;             /* مؤشر التعليمة */
    skp_object_t** slots;    /* فتحات المكدس للدالة */
} call_frame_t;

/* الجهاز الافتراضي */
typedef struct {
    /* المكدس */
    skp_object_t* stack[SKP_STACK_MAX];
    skp_object_t** stack_top;
    
    /* إطارات الاستدعاء */
    call_frame_t frames[SKP_FRAMES_MAX];
    int frame_count;
    
    /* المتغيرات العامة */
    skp_dict_t* globals;
    
    /* الكائنات المُخصَّصة (لجمع القمامة) */
    skp_object_t* objects;
    size_t bytes_allocated;
    size_t next_gc;
    
    /* Upvalues المفتوحة */
    skp_upvalue_t* open_upvalues;
    
    /* الكائنات الرمادية (للـ GC) */
    skp_object_t** gray_stack;
    int gray_count;
    int gray_capacity;
    
    /* حالة التشغيل */
    int running;
    int had_error;
    char* error_message;
} skp_vm_t;

/* نتيجة التنفيذ */
typedef enum {
    SKP_OK,
    SKP_COMPILE_ERROR,
    SKP_RUNTIME_ERROR
} skp_result_t;

/* إنشاء وإتلاف */
skp_vm_t* vm_create(void);
void vm_destroy(skp_vm_t* vm);
void vm_reset(skp_vm_t* vm);

/* التنفيذ */
skp_result_t vm_interpret(skp_vm_t* vm, const char* source);
skp_result_t vm_run(skp_vm_t* vm, chunk_t* chunk);

/* المكدس */
void vm_push(skp_vm_t* vm, skp_object_t* value);
skp_object_t* vm_pop(skp_vm_t* vm);
skp_object_t* vm_peek(skp_vm_t* vm, int distance);
void vm_popn(skp_vm_t* vm, int n);

/* العمليات */
int vm_call(skp_vm_t* vm, skp_object_t* callee, int arg_count);
int vm_call_value(skp_vm_t* vm, skp_object_t* callee, int arg_count);
int vm_invoke(skp_vm_t* vm, skp_object_t* receiver, skp_string name, int arg_count);
int vm_invoke_from_class(skp_vm_t* vm, skp_class_t* klass, skp_string name, int arg_count);

/* Upvalues */
skp_upvalue_t* vm_capture_upvalue(skp_vm_t* vm, skp_object_t** local);
void vm_close_upvalues(skp_vm_t* vm, skp_object_t** last);

/* الأخطاء */
void vm_runtime_error(skp_vm_t* vm, const char* format, ...);
void vm_define_native(skp_vm_t* vm, const char* name, skp_native_func_t func);

/* جمع القمامة */
void vm_collect_garbage(skp_vm_t* vm);
void vm_mark_object(skp_vm_t* vm, skp_object_t* object);
void vm_mark_value(skp_vm_t* vm, skp_object_t* value);
void vm_trace_references(skp_vm_t* vm);
void vm_sweep(skp_vm_t* vm);

/* دوال native مدمجة */
skp_object_t* native_print(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_input(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_clock(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_type(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_len(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_range(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_int(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_float(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_str(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_exit(skp_vm_t* vm, int argc, skp_object_t** argv);

/* دوال الرياضيات */
skp_object_t* native_abs(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_sqrt(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_pow(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_sin(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_cos(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_tan(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_log(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_log10(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_exp(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_floor(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_ceil(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_round(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_min(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_max(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_random(skp_vm_t* vm, int argc, skp_object_t** argv);

/* دوال النصوص */
skp_object_t* native_chr(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_ord(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_split(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_join(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_upper(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_lower(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_strip(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_replace(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_find(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_startswith(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_endswith(skp_vm_t* vm, int argc, skp_object_t** argv);

/* دوال القوائم والقواميس */
skp_object_t* native_append(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_insert(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_remove(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_pop(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_clear(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_sort(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_reverse(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_copy(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_keys(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_values(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_items(skp_vm_t* vm, int argc, skp_object_t** argv);

/* دوال الملفات */
skp_object_t* native_open(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_read(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_write(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_close(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_exists(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_remove_file(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_rename(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_mkdir(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_rmdir(skp_vm_t* vm, int argc, skp_object_t** argv);
skp_object_t* native_listdir(skp_vm_t* vm, int argc, skp_object_t** argv);

/* تسجيل جميع الدوال المدمجة */
void vm_register_natives(skp_vm_t* vm);

/* قراءة بايت من البايتكود */
static inline uint8_t read_byte(call_frame_t* frame) {
    return *frame->ip++;
}

/* قراءة ثابت */
static inline constant_t read_constant(call_frame_t* frame, chunk_t* chunk) {
    return chunk->constants[read_byte(frame)];
}

/* قراءة إزاحة 16 بت */
static inline uint16_t read_short(call_frame_t* frame) {
    frame->ip += 2;
    return (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]);
}

#endif /* VM_H */
