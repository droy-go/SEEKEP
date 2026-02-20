/* ============================================
 * SEEKEP Programming Language - Core Implementation
 * High-Level, Powerful, Arabic-Based
 * Version 1.0.0
 * ============================================ */

#include "seekep.h"

/* ============================================
 * إنشاء كائنات جديدة
 * ============================================ */

skp_object_t* skp_new_int(skp_int value) {
    skp_object_t* obj = (skp_object_t*)malloc(sizeof(skp_object_t));
    if (!obj) return NULL;
    
    obj->type = SKP_TYPE_INT;
    obj->refcount = 1;
    obj->data.v_int = value;
    
    return obj;
}

skp_object_t* skp_new_float(skp_float value) {
    skp_object_t* obj = (skp_object_t*)malloc(sizeof(skp_object_t));
    if (!obj) return NULL;
    
    obj->type = SKP_TYPE_FLOAT;
    obj->refcount = 1;
    obj->data.v_float = value;
    
    return obj;
}

skp_object_t* skp_new_bool(skp_bool value) {
    skp_object_t* obj = (skp_object_t*)malloc(sizeof(skp_object_t));
    if (!obj) return NULL;
    
    obj->type = SKP_TYPE_BOOL;
    obj->refcount = 1;
    obj->data.v_bool = value;
    
    return obj;
}

skp_object_t* skp_new_string(const char* value) {
    skp_object_t* obj = (skp_object_t*)malloc(sizeof(skp_object_t));
    if (!obj) return NULL;
    
    obj->type = SKP_TYPE_STRING;
    obj->refcount = 1;
    obj->data.v_string = strdup(value);
    
    return obj;
}

skp_object_t* skp_new_list(void) {
    skp_object_t* obj = (skp_object_t*)malloc(sizeof(skp_object_t));
    if (!obj) return NULL;
    
    obj->type = SKP_TYPE_LIST;
    obj->refcount = 1;
    obj->data.v_list.items = NULL;
    obj->data.v_list.count = 0;
    obj->data.v_list.capacity = 0;
    
    return obj;
}

skp_object_t* skp_new_dict(void) {
    skp_object_t* obj = (skp_object_t*)malloc(sizeof(skp_object_t));
    if (!obj) return NULL;
    
    obj->type = SKP_TYPE_DICT;
    obj->refcount = 1;
    obj->data.v_dict.entries = NULL;
    obj->data.v_dict.count = 0;
    obj->data.v_dict.capacity = 0;
    
    return obj;
}

skp_object_t* skp_new_null(void) {
    skp_object_t* obj = (skp_object_t*)malloc(sizeof(skp_object_t));
    if (!obj) return NULL;
    
    obj->type = SKP_TYPE_NULL;
    obj->refcount = 1;
    
    return obj;
}

skp_object_t* skp_new_func(skp_object_t* (*func)(skp_object_t** args, size_t argc)) {
    skp_object_t* obj = (skp_object_t*)malloc(sizeof(skp_object_t));
    if (!obj) return NULL;
    
    obj->type = SKP_TYPE_FUNC;
    obj->refcount = 1;
    obj->data.v_func.func = func;
    obj->data.v_func.closure = NULL;
    
    return obj;
}

/* ============================================
 * إدارة الذاكرة
 * ============================================ */

void skp_incref(skp_object_t* obj) {
    if (obj) obj->refcount++;
}

void skp_decref(skp_object_t* obj) {
    if (obj && --obj->refcount <= 0) {
        skp_free(obj);
    }
}

void skp_free(skp_object_t* obj) {
    if (!obj) return;
    
    switch (obj->type) {
        case SKP_TYPE_STRING:
            if (obj->data.v_string) free(obj->data.v_string);
            break;
            
        case SKP_TYPE_LIST:
            for (size_t i = 0; i < obj->data.v_list.count; i++) {
                skp_decref(obj->data.v_list.items[i]);
            }
            free(obj->data.v_list.items);
            break;
            
        case SKP_TYPE_DICT:
            for (size_t i = 0; i < obj->data.v_dict.count; i++) {
                free(obj->data.v_dict.entries[i]->key);
                skp_decref(obj->data.v_dict.entries[i]->value);
                free(obj->data.v_dict.entries[i]);
            }
            free(obj->data.v_dict.entries);
            break;
            
        default:
            break;
    }
    
    free(obj);
}

/* ============================================
 * عمليات على القوائم
 * ============================================ */

void skp_list_append(skp_object_t* list, skp_object_t* item) {
    if (!list || list->type != SKP_TYPE_LIST || !item) return;
    
    if (list->data.v_list.count >= list->data.v_list.capacity) {
        list->data.v_list.capacity = list->data.v_list.capacity == 0 ? 8 : list->data.v_list.capacity * 2;
        list->data.v_list.items = (skp_object_t**)realloc(
            list->data.v_list.items,
            sizeof(skp_object_t*) * list->data.v_list.capacity
        );
    }
    
    skp_incref(item);
    list->data.v_list.items[list->data.v_list.count++] = item;
}

skp_object_t* skp_list_get(skp_object_t* list, size_t index) {
    if (!list || list->type != SKP_TYPE_LIST) return NULL;
    if (index >= list->data.v_list.count) return NULL;
    
    return list->data.v_list.items[index];
}

void skp_list_set(skp_object_t* list, size_t index, skp_object_t* item) {
    if (!list || list->type != SKP_TYPE_LIST || !item) return;
    if (index >= list->data.v_list.count) return;
    
    skp_decref(list->data.v_list.items[index]);
    skp_incref(item);
    list->data.v_list.items[index] = item;
}

size_t skp_list_len(skp_object_t* list) {
    if (!list || list->type != SKP_TYPE_LIST) return 0;
    return list->data.v_list.count;
}

skp_object_t* skp_list_slice(skp_object_t* list, skp_int start, skp_int end) {
    if (!list || list->type != SKP_TYPE_LIST) return NULL;
    
    size_t len = list->data.v_list.count;
    
    if (start < 0) start = len + start;
    if (end < 0) end = len + end;
    if (start < 0) start = 0;
    if (end > (skp_int)len) end = len;
    if (start >= end) return skp_new_list();
    
    skp_object_t* result = skp_new_list();
    for (skp_int i = start; i < end; i++) {
        skp_list_append(result, list->data.v_list.items[i]);
    }
    
    return result;
}

/* ============================================
 * عمليات على القواميس
 * ============================================ */

void skp_dict_set(skp_object_t* dict, const char* key, skp_object_t* value) {
    if (!dict || dict->type != SKP_TYPE_DICT || !key || !value) return;
    
    /* البحث عن المفتاح */
    for (size_t i = 0; i < dict->data.v_dict.count; i++) {
        if (strcmp(dict->data.v_dict.entries[i]->key, key) == 0) {
            skp_decref(dict->data.v_dict.entries[i]->value);
            skp_incref(value);
            dict->data.v_dict.entries[i]->value = value;
            return;
        }
    }
    
    /* إضافة مفتاح جديد */
    if (dict->data.v_dict.count >= dict->data.v_dict.capacity) {
        dict->data.v_dict.capacity = dict->data.v_dict.capacity == 0 ? 8 : dict->data.v_dict.capacity * 2;
        dict->data.v_dict.entries = (skp_dict_entry_t**)realloc(
            dict->data.v_dict.entries,
            sizeof(skp_dict_entry_t*) * dict->data.v_dict.capacity
        );
    }
    
    skp_dict_entry_t* entry = (skp_dict_entry_t*)malloc(sizeof(skp_dict_entry_t));
    entry->key = strdup(key);
    skp_incref(value);
    entry->value = value;
    
    dict->data.v_dict.entries[dict->data.v_dict.count++] = entry;
}

skp_object_t* skp_dict_get(skp_object_t* dict, const char* key) {
    if (!dict || dict->type != SKP_TYPE_DICT || !key) return NULL;
    
    for (size_t i = 0; i < dict->data.v_dict.count; i++) {
        if (strcmp(dict->data.v_dict.entries[i]->key, key) == 0) {
            return dict->data.v_dict.entries[i]->value;
        }
    }
    
    return NULL;
}

skp_bool skp_dict_has(skp_object_t* dict, const char* key) {
    return skp_dict_get(dict, key) != NULL;
}

void skp_dict_remove(skp_object_t* dict, const char* key) {
    if (!dict || dict->type != SKP_TYPE_DICT || !key) return;
    
    for (size_t i = 0; i < dict->data.v_dict.count; i++) {
        if (strcmp(dict->data.v_dict.entries[i]->key, key) == 0) {
            free(dict->data.v_dict.entries[i]->key);
            skp_decref(dict->data.v_dict.entries[i]->value);
            free(dict->data.v_dict.entries[i]);
            
            /* إزالة العنصر */
            for (size_t j = i; j < dict->data.v_dict.count - 1; j++) {
                dict->data.v_dict.entries[j] = dict->data.v_dict.entries[j + 1];
            }
            dict->data.v_dict.count--;
            return;
        }
    }
}

size_t skp_dict_len(skp_object_t* dict) {
    if (!dict || dict->type != SKP_TYPE_DICT) return 0;
    return dict->data.v_dict.count;
}

/* ============================================
 * العمليات الحسابية
 * ============================================ */

skp_object_t* skp_add(skp_object_t* a, skp_object_t* b) {
    if (!a || !b) return skp_new_null();
    
    /* جمع أعداد */
    if (a->type == SKP_TYPE_INT && b->type == SKP_TYPE_INT) {
        return skp_new_int(a->data.v_int + b->data.v_int);
    }
    if (a->type == SKP_TYPE_FLOAT && b->type == SKP_TYPE_FLOAT) {
        return skp_new_float(a->data.v_float + b->data.v_float);
    }
    if (a->type == SKP_TYPE_INT && b->type == SKP_TYPE_FLOAT) {
        return skp_new_float(a->data.v_int + b->data.v_float);
    }
    if (a->type == SKP_TYPE_FLOAT && b->type == SKP_TYPE_INT) {
        return skp_new_float(a->data.v_float + b->data.v_int);
    }
    
    /* جمع نصوص */
    if (a->type == SKP_TYPE_STRING && b->type == SKP_TYPE_STRING) {
        size_t len = strlen(a->data.v_string) + strlen(b->data.v_string) + 1;
        char* result = (char*)malloc(len);
        strcpy(result, a->data.v_string);
        strcat(result, b->data.v_string);
        skp_object_t* obj = skp_new_string(result);
        free(result);
        return obj;
    }
    
    /* جمع قوائم */
    if (a->type == SKP_TYPE_LIST && b->type == SKP_TYPE_LIST) {
        skp_object_t* result = skp_new_list();
        for (size_t i = 0; i < a->data.v_list.count; i++) {
            skp_list_append(result, a->data.v_list.items[i]);
        }
        for (size_t i = 0; i < b->data.v_list.count; i++) {
            skp_list_append(result, b->data.v_list.items[i]);
        }
        return result;
    }
    
    return skp_new_null();
}

skp_object_t* skp_sub(skp_object_t* a, skp_object_t* b) {
    if (!a || !b) return skp_new_null();
    
    if (a->type == SKP_TYPE_INT && b->type == SKP_TYPE_INT) {
        return skp_new_int(a->data.v_int - b->data.v_int);
    }
    if (a->type == SKP_TYPE_FLOAT && b->type == SKP_TYPE_FLOAT) {
        return skp_new_float(a->data.v_float - b->data.v_float);
    }
    if (a->type == SKP_TYPE_INT && b->type == SKP_TYPE_FLOAT) {
        return skp_new_float(a->data.v_int - b->data.v_float);
    }
    if (a->type == SKP_TYPE_FLOAT && b->type == SKP_TYPE_INT) {
        return skp_new_float(a->data.v_float - b->data.v_int);
    }
    
    return skp_new_null();
}

skp_object_t* skp_mul(skp_object_t* a, skp_object_t* b) {
    if (!a || !b) return skp_new_null();
    
    if (a->type == SKP_TYPE_INT && b->type == SKP_TYPE_INT) {
        return skp_new_int(a->data.v_int * b->data.v_int);
    }
    if (a->type == SKP_TYPE_FLOAT && b->type == SKP_TYPE_FLOAT) {
        return skp_new_float(a->data.v_float * b->data.v_float);
    }
    if (a->type == SKP_TYPE_INT && b->type == SKP_TYPE_FLOAT) {
        return skp_new_float(a->data.v_int * b->data.v_float);
    }
    if (a->type == SKP_TYPE_FLOAT && b->type == SKP_TYPE_INT) {
        return skp_new_float(a->data.v_float * b->data.v_int);
    }
    
    /* تكرار نص */
    if (a->type == SKP_TYPE_STRING && b->type == SKP_TYPE_INT) {
        size_t len = strlen(a->data.v_string) * b->data.v_int + 1;
        char* result = (char*)malloc(len);
        result[0] = '\0';
        for (skp_int i = 0; i < b->data.v_int; i++) {
            strcat(result, a->data.v_string);
        }
        skp_object_t* obj = skp_new_string(result);
        free(result);
        return obj;
    }
    
    return skp_new_null();
}

skp_object_t* skp_div(skp_object_t* a, skp_object_t* b) {
    if (!a || !b) return skp_new_null();
    
    skp_float divisor = 0;
    if (b->type == SKP_TYPE_INT) divisor = b->data.v_int;
    else if (b->type == SKP_TYPE_FLOAT) divisor = b->data.v_float;
    else return skp_new_null();
    
    if (divisor == 0) {
        fprintf(stderr, "خطأ: قسمة على صفر\n");
        return skp_new_null();
    }
    
    if (a->type == SKP_TYPE_INT && b->type == SKP_TYPE_INT) {
        return skp_new_float((skp_float)a->data.v_int / b->data.v_int);
    }
    if (a->type == SKP_TYPE_FLOAT && b->type == SKP_TYPE_FLOAT) {
        return skp_new_float(a->data.v_float / b->data.v_float);
    }
    if (a->type == SKP_TYPE_INT && b->type == SKP_TYPE_FLOAT) {
        return skp_new_float(a->data.v_int / b->data.v_float);
    }
    if (a->type == SKP_TYPE_FLOAT && b->type == SKP_TYPE_INT) {
        return skp_new_float(a->data.v_float / b->data.v_int);
    }
    
    return skp_new_null();
}

skp_object_t* skp_mod(skp_object_t* a, skp_object_t* b) {
    if (!a || !b) return skp_new_null();
    
    if (a->type == SKP_TYPE_INT && b->type == SKP_TYPE_INT) {
        if (b->data.v_int == 0) {
            fprintf(stderr, "خطأ: قسمة على صفر\n");
            return skp_new_null();
        }
        return skp_new_int(a->data.v_int % b->data.v_int);
    }
    
    return skp_new_null();
}

skp_object_t* skp_pow(skp_object_t* a, skp_object_t* b) {
    if (!a || !b) return skp_new_null();
    
    skp_float base = 0, exp = 0;
    
    if (a->type == SKP_TYPE_INT) base = a->data.v_int;
    else if (a->type == SKP_TYPE_FLOAT) base = a->data.v_float;
    else return skp_new_null();
    
    if (b->type == SKP_TYPE_INT) exp = b->data.v_int;
    else if (b->type == SKP_TYPE_FLOAT) exp = b->data.v_float;
    else return skp_new_null();
    
    return skp_new_float(pow(base, exp));
}

/* ============================================
 * العمليات المنطقية
 * ============================================ */

skp_bool skp_eq(skp_object_t* a, skp_object_t* b) {
    if (!a || !b) return a == b;
    if (a->type != b->type) return SKP_FALSE;
    
    switch (a->type) {
        case SKP_TYPE_INT:
            return a->data.v_int == b->data.v_int;
        case SKP_TYPE_FLOAT:
            return a->data.v_float == b->data.v_float;
        case SKP_TYPE_BOOL:
            return a->data.v_bool == b->data.v_bool;
        case SKP_TYPE_STRING:
            return strcmp(a->data.v_string, b->data.v_string) == 0;
        case SKP_TYPE_NULL:
            return SKP_TRUE;
        default:
            return SKP_FALSE;
    }
}

skp_bool skp_ne(skp_object_t* a, skp_object_t* b) {
    return !skp_eq(a, b);
}

skp_bool skp_lt(skp_object_t* a, skp_object_t* b) {
    if (!a || !b) return SKP_FALSE;
    
    if (a->type == SKP_TYPE_INT && b->type == SKP_TYPE_INT) {
        return a->data.v_int < b->data.v_int;
    }
    if (a->type == SKP_TYPE_FLOAT && b->type == SKP_TYPE_FLOAT) {
        return a->data.v_float < b->data.v_float;
    }
    
    return SKP_FALSE;
}

skp_bool skp_gt(skp_object_t* a, skp_object_t* b) {
    if (!a || !b) return SKP_FALSE;
    
    if (a->type == SKP_TYPE_INT && b->type == SKP_TYPE_INT) {
        return a->data.v_int > b->data.v_int;
    }
    if (a->type == SKP_TYPE_FLOAT && b->type == SKP_TYPE_FLOAT) {
        return a->data.v_float > b->data.v_float;
    }
    
    return SKP_FALSE;
}

skp_bool skp_le(skp_object_t* a, skp_object_t* b) {
    return skp_lt(a, b) || skp_eq(a, b);
}

skp_bool skp_ge(skp_object_t* a, skp_object_t* b) {
    return skp_gt(a, b) || skp_eq(a, b);
}

skp_bool skp_and(skp_bool a, skp_bool b) {
    return a && b;
}

skp_bool skp_or(skp_bool a, skp_bool b) {
    return a || b;
}

skp_bool skp_not(skp_bool a) {
    return !a;
}

/* ============================================
 * التحويل بين الأنواع
 * ============================================ */

skp_object_t* skp_to_string(skp_object_t* obj) {
    if (!obj) return skp_new_string("null");
    
    char buffer[256];
    
    switch (obj->type) {
        case SKP_TYPE_INT:
            snprintf(buffer, sizeof(buffer), "%ld", obj->data.v_int);
            return skp_new_string(buffer);
        case SKP_TYPE_FLOAT:
            snprintf(buffer, sizeof(buffer), "%g", obj->data.v_float);
            return skp_new_string(buffer);
        case SKP_TYPE_BOOL:
            return skp_new_string(obj->data.v_bool ? "صحيح" : "خطأ");
        case SKP_TYPE_STRING:
            skp_incref(obj);
            return obj;
        case SKP_TYPE_NULL:
            return skp_new_string("فارغ");
        default:
            return skp_new_string("<object>");
    }
}

skp_object_t* skp_to_int(skp_object_t* obj) {
    if (!obj) return skp_new_int(0);
    
    switch (obj->type) {
        case SKP_TYPE_INT:
            skp_incref(obj);
            return obj;
        case SKP_TYPE_FLOAT:
            return skp_new_int((skp_int)obj->data.v_float);
        case SKP_TYPE_STRING:
            return skp_new_int(atoll(obj->data.v_string));
        case SKP_TYPE_BOOL:
            return skp_new_int(obj->data.v_bool ? 1 : 0);
        default:
            return skp_new_int(0);
    }
}

skp_object_t* skp_to_float(skp_object_t* obj) {
    if (!obj) return skp_new_float(0.0);
    
    switch (obj->type) {
        case SKP_TYPE_INT:
            return skp_new_float((skp_float)obj->data.v_int);
        case SKP_TYPE_FLOAT:
            skp_incref(obj);
            return obj;
        case SKP_TYPE_STRING:
            return skp_new_float(atof(obj->data.v_string));
        case SKP_TYPE_BOOL:
            return skp_new_float(obj->data.v_bool ? 1.0 : 0.0);
        default:
            return skp_new_float(0.0);
    }
}

skp_bool skp_to_bool(skp_object_t* obj) {
    if (!obj) return SKP_FALSE;
    
    switch (obj->type) {
        case SKP_TYPE_INT:
            return obj->data.v_int != 0;
        case SKP_TYPE_FLOAT:
            return obj->data.v_float != 0.0;
        case SKP_TYPE_BOOL:
            return obj->data.v_bool;
        case SKP_TYPE_STRING:
            return strlen(obj->data.v_string) > 0;
        case SKP_TYPE_NULL:
            return SKP_FALSE;
        default:
            return SKP_TRUE;
    }
}

/* ============================================
 * المكتبة القياسية - النصوص
 * ============================================ */

skp_object_t* skp_str_length(skp_object_t* str) {
    if (!str || str->type != SKP_TYPE_STRING) return skp_new_int(0);
    return skp_new_int(strlen(str->data.v_string));
}

skp_object_t* skp_str_concat(skp_object_t* a, skp_object_t* b) {
    return skp_add(a, b);
}

skp_object_t* skp_str_substring(skp_object_t* str, skp_int start, skp_int end) {
    if (!str || str->type != SKP_TYPE_STRING) return skp_new_string("");
    
    size_t len = strlen(str->data.v_string);
    
    if (start < 0) start = len + start;
    if (end < 0) end = len + end;
    if (start < 0) start = 0;
    if (end > (skp_int)len) end = len;
    if (start >= end) return skp_new_string("");
    
    size_t sublen = end - start;
    char* result = (char*)malloc(sublen + 1);
    strncpy(result, str->data.v_string + start, sublen);
    result[sublen] = '\0';
    
    skp_object_t* obj = skp_new_string(result);
    free(result);
    return obj;
}

skp_object_t* skp_str_contains(skp_object_t* str, skp_object_t* substr) {
    if (!str || str->type != SKP_TYPE_STRING || !substr || substr->type != SKP_TYPE_STRING) {
        return skp_new_bool(SKP_FALSE);
    }
    return skp_new_bool(strstr(str->data.v_string, substr->data.v_string) != NULL);
}

skp_object_t* skp_str_upper(skp_object_t* str) {
    if (!str || str->type != SKP_TYPE_STRING) return skp_new_string("");
    
    char* result = strdup(str->data.v_string);
    for (char* p = result; *p; p++) {
        *p = toupper(*p);
    }
    
    skp_object_t* obj = skp_new_string(result);
    free(result);
    return obj;
}

skp_object_t* skp_str_lower(skp_object_t* str) {
    if (!str || str->type != SKP_TYPE_STRING) return skp_new_string("");
    
    char* result = strdup(str->data.v_string);
    for (char* p = result; *p; p++) {
        *p = tolower(*p);
    }
    
    skp_object_t* obj = skp_new_string(result);
    free(result);
    return obj;
}

skp_object_t* skp_str_trim(skp_object_t* str) {
    if (!str || str->type != SKP_TYPE_STRING) return skp_new_string("");
    
    char* s = str->data.v_string;
    while (isspace(*s)) s++;
    
    if (*s == '\0') return skp_new_string("");
    
    char* e = s + strlen(s) - 1;
    while (e > s && isspace(*e)) e--;
    
    size_t len = e - s + 1;
    char* result = (char*)malloc(len + 1);
    strncpy(result, s, len);
    result[len] = '\0';
    
    skp_object_t* obj = skp_new_string(result);
    free(result);
    return obj;
}

/* ============================================
 * المكتبة القياسية - الرياضيات
 * ============================================ */

skp_object_t* skp_math_abs(skp_object_t* x) {
    if (!x) return skp_new_null();
    
    if (x->type == SKP_TYPE_INT) {
        return skp_new_int(x->data.v_int < 0 ? -x->data.v_int : x->data.v_int);
    }
    if (x->type == SKP_TYPE_FLOAT) {
        return skp_new_float(fabs(x->data.v_float));
    }
    
    return skp_new_null();
}

skp_object_t* skp_math_sqrt(skp_object_t* x) {
    if (!x) return skp_new_null();
    
    skp_float val = 0;
    if (x->type == SKP_TYPE_INT) val = x->data.v_int;
    else if (x->type == SKP_TYPE_FLOAT) val = x->data.v_float;
    else return skp_new_null();
    
    return skp_new_float(sqrt(val));
}

skp_object_t* skp_math_sin(skp_object_t* x) {
    if (!x) return skp_new_null();
    
    skp_float val = 0;
    if (x->type == SKP_TYPE_INT) val = x->data.v_int;
    else if (x->type == SKP_TYPE_FLOAT) val = x->data.v_float;
    else return skp_new_null();
    
    return skp_new_float(sin(val));
}

skp_object_t* skp_math_cos(skp_object_t* x) {
    if (!x) return skp_new_null();
    
    skp_float val = 0;
    if (x->type == SKP_TYPE_INT) val = x->data.v_int;
    else if (x->type == SKP_TYPE_FLOAT) val = x->data.v_float;
    else return skp_new_null();
    
    return skp_new_float(cos(val));
}

skp_object_t* skp_math_tan(skp_object_t* x) {
    if (!x) return skp_new_null();
    
    skp_float val = 0;
    if (x->type == SKP_TYPE_INT) val = x->data.v_int;
    else if (x->type == SKP_TYPE_FLOAT) val = x->data.v_float;
    else return skp_new_null();
    
    return skp_new_float(tan(val));
}

skp_object_t* skp_math_log(skp_object_t* x) {
    if (!x) return skp_new_null();
    
    skp_float val = 0;
    if (x->type == SKP_TYPE_INT) val = x->data.v_int;
    else if (x->type == SKP_TYPE_FLOAT) val = x->data.v_float;
    else return skp_new_null();
    
    return skp_new_float(log(val));
}

skp_object_t* skp_math_exp(skp_object_t* x) {
    if (!x) return skp_new_null();
    
    skp_float val = 0;
    if (x->type == SKP_TYPE_INT) val = x->data.v_int;
    else if (x->type == SKP_TYPE_FLOAT) val = x->data.v_float;
    else return skp_new_null();
    
    return skp_new_float(exp(val));
}

skp_object_t* skp_math_floor(skp_object_t* x) {
    if (!x) return skp_new_null();
    
    skp_float val = 0;
    if (x->type == SKP_TYPE_INT) return skp_new_int(x->data.v_int);
    else if (x->type == SKP_TYPE_FLOAT) val = x->data.v_float;
    else return skp_new_null();
    
    return skp_new_int((skp_int)floor(val));
}

skp_object_t* skp_math_ceil(skp_object_t* x) {
    if (!x) return skp_new_null();
    
    skp_float val = 0;
    if (x->type == SKP_TYPE_INT) return skp_new_int(x->data.v_int);
    else if (x->type == SKP_TYPE_FLOAT) val = x->data.v_float;
    else return skp_new_null();
    
    return skp_new_int((skp_int)ceil(val));
}

skp_object_t* skp_math_round(skp_object_t* x) {
    if (!x) return skp_new_null();
    
    skp_float val = 0;
    if (x->type == SKP_TYPE_INT) return skp_new_int(x->data.v_int);
    else if (x->type == SKP_TYPE_FLOAT) val = x->data.v_float;
    else return skp_new_null();
    
    return skp_new_int((skp_int)round(val));
}

skp_object_t* skp_math_random(void) {
    return skp_new_float((skp_float)rand() / RAND_MAX);
}

/* ============================================
 * المكتبة القياسية - النظام
 * ============================================ */

void skp_print(skp_object_t* obj) {
    if (!obj) {
        printf("فارغ");
        return;
    }
    
    switch (obj->type) {
        case SKP_TYPE_INT:
            printf("%ld", obj->data.v_int);
            break;
        case SKP_TYPE_FLOAT:
            printf("%g", obj->data.v_float);
            break;
        case SKP_TYPE_BOOL:
            printf("%s", obj->data.v_bool ? "صحيح" : "خطأ");
            break;
        case SKP_TYPE_STRING:
            printf("%s", obj->data.v_string);
            break;
        case SKP_TYPE_LIST:
            printf("[");
            for (size_t i = 0; i < obj->data.v_list.count; i++) {
                skp_print(obj->data.v_list.items[i]);
                if (i < obj->data.v_list.count - 1) printf(", ");
            }
            printf("]");
            break;
        case SKP_TYPE_DICT:
            printf("{");
            for (size_t i = 0; i < obj->data.v_dict.count; i++) {
                printf("%s: ", obj->data.v_dict.entries[i]->key);
                skp_print(obj->data.v_dict.entries[i]->value);
                if (i < obj->data.v_dict.count - 1) printf(", ");
            }
            printf("}");
            break;
        case SKP_TYPE_NULL:
            printf("فارغ");
            break;
        default:
            printf("<object>");
            break;
    }
}

void skp_println(skp_object_t* obj) {
    skp_print(obj);
    printf("\n");
}

skp_object_t* skp_input(void) {
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), stdin)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
        return skp_new_string(buffer);
    }
    return skp_new_string("");
}

void skp_exit(skp_int code) {
    exit((int)code);
}

/* ============================================
 * المكتبة القياسية - الوقت
 * ============================================ */

skp_object_t* skp_time_now(void) {
    return skp_new_float((skp_float)time(NULL));
}

skp_object_t* skp_time_sleep(skp_object_t* seconds) {
    if (!seconds) return skp_new_null();
    
    skp_float secs = 0;
    if (seconds->type == SKP_TYPE_INT) secs = seconds->data.v_int;
    else if (seconds->type == SKP_TYPE_FLOAT) secs = seconds->data.v_float;
    else return skp_new_null();
    
    struct timespec ts;
    ts.tv_sec = (time_t)secs;
    ts.tv_nsec = (long)((secs - ts.tv_sec) * 1e9);
    nanosleep(&ts, NULL);
    
    return skp_new_null();
}

/* ============================================
 * معلومات
 * ============================================ */

const char* skp_type_name(skp_type_t type) {
    switch (type) {
        case SKP_TYPE_INT: return "عدد_صحيح";
        case SKP_TYPE_FLOAT: return "عدد_عشري";
        case SKP_TYPE_BOOL: return "منطقي";
        case SKP_TYPE_STRING: return "نص";
        case SKP_TYPE_LIST: return "قائمة";
        case SKP_TYPE_DICT: return "قاموس";
        case SKP_TYPE_FUNC: return "دالة";
        case SKP_TYPE_OBJECT: return "كائن";
        case SKP_TYPE_NULL: return "فارغ";
        default: return "غير_معروف";
    }
}

skp_type_t skp_get_type(skp_object_t* obj) {
    return obj ? obj->type : SKP_TYPE_NULL;
}

const char* skp_version(void) {
    return SEEKEP_VERSION_STRING;
}

void skp_info(void) {
    printf("╔════════════════════════════════════════════════╗\n");
    printf("║           SEEKEP Programming Language          ║\n");
    printf("║                                                ║\n");
    printf("║   Smart Expressive Efficient Knowledge         ║\n");
    printf("║   Execution Platform                           ║\n");
    printf("║                                                ║\n");
    printf("║   Version: %-10s                          ║\n", SEEKEP_VERSION_STRING);
    printf("║   Language: Arabic-Based                       ║\n");
    printf("║   Type: High-Level                             ║\n");
    printf("╚════════════════════════════════════════════════╝\n");
}
