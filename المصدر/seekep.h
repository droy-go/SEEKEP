/* ============================================
 * SEEKEP Programming Language
 * High-Level, Powerful, Arabic-Based
 * Version 1.0.0
 * ============================================
 * 
 * SEEKEP: Smart Expressive Efficient Knowledge 
 *         Execution Platform
 */

#ifndef SEEKEP_H
#define SEEKEP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <assert.h>

/* ============================================
 * النسخة والمعلومات
 * ============================================ */
#define SEEKEP_VERSION_MAJOR 1
#define SEEKEP_VERSION_MINOR 0
#define SEEKEP_VERSION_PATCH 0
#define SEEKEP_VERSION_STRING "1.0.0"

/* ============================================
 * أنواع البيانات الأساسية
 * ============================================ */

typedef int64_t    skp_int;      /* عدد صحيح */
typedef double     skp_float;    /* عدد عشري */
typedef bool       skp_bool;     /* منطقي */
typedef char*      skp_string;   /* نص */
typedef void*      skp_any;      /* أي نوع */

/* القيم المنطقية */
#define SKP_TRUE  true
#define SKP_FALSE false
#define SKP_NULL   NULL

/* ============================================
 * أنواع الكائنات
 * ============================================ */

typedef enum {
    SKP_TYPE_INT,
    SKP_TYPE_FLOAT,
    SKP_TYPE_BOOL,
    SKP_TYPE_STRING,
    SKP_TYPE_LIST,
    SKP_TYPE_DICT,
    SKP_TYPE_FUNC,
    SKP_TYPE_OBJECT,
    SKP_TYPE_NULL,
    SKP_TYPE_ANY
} skp_type_t;

/* ============================================
 * هيكل الكائن الأساسي
 * ============================================ */

typedef struct skp_object {
    skp_type_t type;
    int refcount;
    
    union {
        skp_int    v_int;
        skp_float  v_float;
        skp_bool   v_bool;
        skp_string v_string;
        
        struct {
            struct skp_object** items;
            size_t count;
            size_t capacity;
        } v_list;
        
        struct {
            struct skp_dict_entry** entries;
            size_t count;
            size_t capacity;
        } v_dict;
        
        struct {
            struct skp_object* (*func)(struct skp_object** args, size_t argc);
            struct skp_object* closure;
        } v_func;
        
        struct {
            struct skp_class* klass;
            struct skp_dict_entry** fields;
            size_t field_count;
        } v_object;
    } data;
} skp_object_t;

/* مدخل القاموس */
typedef struct skp_dict_entry {
    char* key;
    skp_object_t* value;
} skp_dict_entry_t;

/* الصنف */
typedef struct skp_class {
    char* name;
    struct skp_class* parent;
    skp_dict_entry_t** methods;
    size_t method_count;
} skp_class_t;

/* ============================================
 * إدارة الذاكرة
 * ============================================ */

skp_object_t* skp_new_int(skp_int value);
skp_object_t* skp_new_float(skp_float value);
skp_object_t* skp_new_bool(skp_bool value);
skp_object_t* skp_new_string(const char* value);
skp_object_t* skp_new_list(void);
skp_object_t* skp_new_dict(void);
skp_object_t* skp_new_null(void);
skp_object_t* skp_new_func(skp_object_t* (*func)(skp_object_t** args, size_t argc));
skp_object_t* skp_new_object(skp_class_t* klass);

void skp_incref(skp_object_t* obj);
void skp_decref(skp_object_t* obj);
void skp_free(skp_object_t* obj);

/* ============================================
 * عمليات على القوائم
 * ============================================ */

void skp_list_append(skp_object_t* list, skp_object_t* item);
skp_object_t* skp_list_get(skp_object_t* list, size_t index);
void skp_list_set(skp_object_t* list, size_t index, skp_object_t* item);
size_t skp_list_len(skp_object_t* list);
skp_object_t* skp_list_slice(skp_object_t* list, skp_int start, skp_int end);
void skp_list_sort(skp_object_t* list);

/* ============================================
 * عمليات على القواميس
 * ============================================ */

void skp_dict_set(skp_object_t* dict, const char* key, skp_object_t* value);
skp_object_t* skp_dict_get(skp_object_t* dict, const char* key);
skp_bool skp_dict_has(skp_object_t* dict, const char* key);
void skp_dict_remove(skp_object_t* dict, const char* key);
size_t skp_dict_len(skp_object_t* dict);

/* ============================================
 * العمليات الحسابية
 * ============================================ */

skp_object_t* skp_add(skp_object_t* a, skp_object_t* b);
skp_object_t* skp_sub(skp_object_t* a, skp_object_t* b);
skp_object_t* skp_mul(skp_object_t* a, skp_object_t* b);
skp_object_t* skp_div(skp_object_t* a, skp_object_t* b);
skp_object_t* skp_mod(skp_object_t* a, skp_object_t* b);
skp_object_t* skp_pow(skp_object_t* a, skp_object_t* b);

/* ============================================
 * العمليات المنطقية
 * ============================================ */

skp_bool skp_eq(skp_object_t* a, skp_object_t* b);
skp_bool skp_ne(skp_object_t* a, skp_object_t* b);
skp_bool skp_lt(skp_object_t* a, skp_object_t* b);
skp_bool skp_gt(skp_object_t* a, skp_object_t* b);
skp_bool skp_le(skp_object_t* a, skp_object_t* b);
skp_bool skp_ge(skp_object_t* a, skp_object_t* b);

skp_bool skp_and(skp_bool a, skp_bool b);
skp_bool skp_or(skp_bool a, skp_bool b);
skp_bool skp_not(skp_bool a);

/* ============================================
 * التحويل بين الأنواع
 * ============================================ */

skp_object_t* skp_to_string(skp_object_t* obj);
skp_object_t* skp_to_int(skp_object_t* obj);
skp_object_t* skp_to_float(skp_object_t* obj);
skp_bool skp_to_bool(skp_object_t* obj);

/* ============================================
 * المكتبة القياسية
 * ============================================ */

/* النصوص */
skp_object_t* skp_str_length(skp_object_t* str);
skp_object_t* skp_str_concat(skp_object_t* a, skp_object_t* b);
skp_object_t* skp_str_substring(skp_object_t* str, skp_int start, skp_int end);
skp_object_t* skp_str_contains(skp_object_t* str, skp_object_t* substr);
skp_object_t* skp_str_split(skp_object_t* str, skp_object_t* delim);
skp_object_t* skp_str_replace(skp_object_t* str, skp_object_t* old, skp_object_t* new);
skp_object_t* skp_str_upper(skp_object_t* str);
skp_object_t* skp_str_lower(skp_object_t* str);
skp_object_t* skp_str_trim(skp_object_t* str);

/* الرياضيات */
skp_object_t* skp_math_abs(skp_object_t* x);
skp_object_t* skp_math_sqrt(skp_object_t* x);
skp_object_t* skp_math_sin(skp_object_t* x);
skp_object_t* skp_math_cos(skp_object_t* x);
skp_object_t* skp_math_tan(skp_object_t* x);
skp_object_t* skp_math_log(skp_object_t* x);
skp_object_t* skp_math_exp(skp_object_t* x);
skp_object_t* skp_math_floor(skp_object_t* x);
skp_object_t* skp_math_ceil(skp_object_t* x);
skp_object_t* skp_math_round(skp_object_t* x);
skp_object_t* skp_math_random(void);

/* النظام */
void skp_print(skp_object_t* obj);
void skp_println(skp_object_t* obj);
skp_object_t* skp_input(void);
void skp_exit(skp_int code);

/* الوقت */
skp_object_t* skp_time_now(void);
skp_object_t* skp_time_sleep(skp_object_t* seconds);

/* ============================================
 * معلومات
 * ============================================ */

const char* skp_type_name(skp_type_t type);
skp_type_t skp_get_type(skp_object_t* obj);
const char* skp_version(void);
void skp_info(void);

#endif /* SEEKEP_H */
