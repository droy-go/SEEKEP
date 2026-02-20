/*
 * SEEKEP - Smart Expressive Efficient Knowledge Execution Platform
 * الجهاز الافتراضي (VM) - Virtual Machine Implementation
 * 
 * ينفذ بايتكود SEEKEP
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>
#include "vm.h"

/* ========== إنشاء وإتلاف الجهاز الافتراضي ========== */

skp_vm_t* vm_create(void) {
    skp_vm_t* vm = (skp_vm_t*)malloc(sizeof(skp_vm_t));
    if (!vm) return NULL;
    
    vm->stack_top = vm->stack;
    vm->frame_count = 0;
    vm->globals = skp_dict_create();
    vm->objects = NULL;
    vm->bytes_allocated = 0;
    vm->next_gc = SKP_GC_THRESHOLD;
    vm->open_upvalues = NULL;
    vm->gray_stack = NULL;
    vm->gray_count = 0;
    vm->gray_capacity = 0;
    vm->running = 0;
    vm->had_error = 0;
    vm->error_message = NULL;
    
    /* تسجيل الدوال المدمجة */
    vm_register_natives(vm);
    
    return vm;
}

void vm_destroy(skp_vm_t* vm) {
    if (!vm) return;
    
    /* تحرير جميع الكائنات */
    skp_object_t* object = vm->objects;
    while (object) {
        skp_object_t* next = object->next;
        skp_free(object);
        object = next;
    }
    
    /* تحرير القاموس العام */
    skp_dict_destroy(vm->globals);
    
    /* تحرير المكدس الرمادي */
    free(vm->gray_stack);
    
    /* تحرير رسالة الخطأ */
    free(vm->error_message);
    
    free(vm);
}

void vm_reset(skp_vm_t* vm) {
    vm->stack_top = vm->stack;
    vm->frame_count = 0;
    vm->had_error = 0;
    free(vm->error_message);
    vm->error_message = NULL;
}

/* ========== المكدس ========== */

void vm_push(skp_vm_t* vm, skp_object_t* value) {
    *vm->stack_top = value;
    vm->stack_top++;
}

skp_object_t* vm_pop(skp_vm_t* vm) {
    vm->stack_top--;
    return *vm->stack_top;
}

skp_object_t* vm_peek(skp_vm_t* vm, int distance) {
    return vm->stack_top[-1 - distance];
}

void vm_popn(skp_vm_t* vm, int n) {
    vm->stack_top -= n;
}

/* ========== الأخطاء ========== */

void vm_runtime_error(skp_vm_t* vm, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    va_end(args);
    
    vm->had_error = 1;
    free(vm->error_message);
    vm->error_message = strdup(buffer);
    
    /* طباعة تتبع المكدس */
    fprintf(stderr, "خطأ زمني: %s\n", buffer);
    
    for (int i = vm->frame_count - 1; i >= 0; i--) {
        call_frame_t* frame = &vm->frames[i];
        int instruction = (int)(frame->ip - frame->chunk->code - 1);
        int line = frame->chunk->lines[instruction];
        fprintf(stderr, "[سطر %d] في الدالة\n", line);
    }
}

/* ========== الاستدعاء ========== */

int vm_call(skp_vm_t* vm, skp_object_t* callee, int arg_count) {
    if (vm->frame_count >= SKP_FRAMES_MAX) {
        vm_runtime_error(vm, "تجاوز الحد الأقصى لعمق الاستدعاء");
        return 0;
    }
    
    call_frame_t* frame = &vm->frames[vm->frame_count++];
    frame->chunk = &callee->data.v_func.closure->data.v_closure.function->chunk;
    frame->ip = frame->chunk->code;
    frame->slots = vm->stack_top - arg_count - 1;
    
    return 1;
}

int vm_call_value(skp_vm_t* vm, skp_object_t* callee, int arg_count) {
    if (callee == NULL) {
        vm_runtime_error(vm, "لا يمكن استدعاء قيمة فارغة");
        return 0;
    }
    
    switch (callee->type) {
        case SKP_TYPE_FUNCTION:
            return vm_call(vm, callee, arg_count);
            
        case SKP_TYPE_NATIVE: {
            skp_native_func_t native = callee->data.v_native.func;
            skp_object_t* result = native(vm, arg_count, vm->stack_top - arg_count);
            vm->stack_top -= arg_count + 1;
            vm_push(vm, result);
            return 1;
        }
            
        case SKP_TYPE_CLASS: {
            skp_object_t* instance = skp_new_instance(callee->data.v_class.klass);
            vm->stack_top[-arg_count - 1] = instance;
            
            /* استدعاء المُنشئ إذا وجد */
            skp_object_t* initializer;
            if (skp_dict_get(callee->data.v_class.klass->methods, "init", &initializer)) {
                return vm_call(vm, initializer, arg_count);
            } else if (arg_count != 0) {
                vm_runtime_error(vm, "توقع 0 معاملات لكن تم تمرير %d", arg_count);
                return 0;
            }
            
            return 1;
        }
            
        case SKP_TYPE_BOUND_METHOD: {
            skp_bound_method_t* bound = callee->data.v_bound_method.bound;
            vm->stack_top[-arg_count - 1] = bound->receiver;
            return vm_call(vm, bound->method, arg_count);
        }
            
        default:
            vm_runtime_error(vm, "لا يمكن استدعاء هذا النوع");
            return 0;
    }
}

int vm_invoke(skp_vm_t* vm, skp_object_t* receiver, skp_string name, int arg_count) {
    skp_type_t type = skp_get_type(receiver);
    
    if (type == SKP_TYPE_INSTANCE) {
        skp_object_t* value;
        if (skp_dict_get(receiver->data.v_object.fields, name, &value)) {
            vm->stack_top[-arg_count - 1] = value;
            return vm_call_value(vm, value, arg_count);
        }
        
        return vm_invoke_from_class(vm, receiver->data.v_object.klass, name, arg_count);
    }
    
    /* دوال مدمجة للأنواع الأساسية */
    vm_runtime_error(vm, "لا يمكن استدعاء طريقة على هذا النوع");
    return 0;
}

int vm_invoke_from_class(skp_vm_t* vm, skp_class_t* klass, skp_string name, int arg_count) {
    skp_object_t* method;
    if (!skp_dict_get(klass->methods, name, &method)) {
        vm_runtime_error(vm, "الطريقة غير معرفة: %s", name);
        return 0;
    }
    
    return vm_call(vm, method, arg_count);
}

/* ========== Upvalues ========== */

skp_upvalue_t* vm_capture_upvalue(skp_vm_t* vm, skp_object_t** local) {
    /* البحث عن upvalue موجود */
    skp_upvalue_t* prev_upvalue = NULL;
    skp_upvalue_t* upvalue = vm->open_upvalues;
    
    while (upvalue && upvalue->location > local) {
        prev_upvalue = upvalue;
        upvalue = upvalue->next;
    }
    
    if (upvalue && upvalue->location == local) {
        return upvalue;
    }
    
    /* إنشاء upvalue جديد */
    skp_upvalue_t* created = skp_new_upvalue(local);
    created->next = upvalue;
    
    if (prev_upvalue) {
        prev_upvalue->next = created;
    } else {
        vm->open_upvalues = created;
    }
    
    return created;
}

void vm_close_upvalues(skp_vm_t* vm, skp_object_t** last) {
    while (vm->open_upvalues && vm->open_upvalues->location >= last) {
        skp_upvalue_t* upvalue = vm->open_upvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm->open_upvalues = upvalue->next;
    }
}

/* ========== التنفيذ ========== */

skp_result_t vm_interpret(skp_vm_t* vm, const char* source) {
    /* إنشاء الليكسر */
    lexer_t* lexer = lexer_create(source);
    if (!lexer) {
        return SKP_COMPILE_ERROR;
    }
    
    /* إنشاء المحلل */
    parser_t* parser = parser_create(lexer);
    if (!parser) {
        lexer_destroy(lexer);
        return SKP_COMPILE_ERROR;
    }
    
    /* تحليل البرنامج */
    ast_node_t* ast = parser_parse(parser);
    
    if (parser->had_error) {
        ast_destroy_node(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        return SKP_COMPILE_ERROR;
    }
    
    /* إنشاء المترجم */
    skp_compiler_t* compiler = compiler_create(parser);
    if (!compiler) {
        ast_destroy_node(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        return SKP_COMPILE_ERROR;
    }
    
    /* الترجمة */
    chunk_t* chunk = compiler_compile(compiler, ast);
    
    if (!chunk || compiler->had_error) {
        if (chunk) {
            chunk_free(chunk);
            free(chunk);
        }
        compiler_destroy(compiler);
        ast_destroy_node(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        return SKP_COMPILE_ERROR;
    }
    
    /* تنظيف */
    compiler_destroy(compiler);
    ast_destroy_node(ast);
    parser_destroy(parser);
    lexer_destroy(lexer);
    
    /* التشغيل */
    skp_result_t result = vm_run(vm, chunk);
    
    /* تحرير الكتلة */
    chunk_free(chunk);
    free(chunk);
    
    return result;
}

skp_result_t vm_run(skp_vm_t* vm, chunk_t* chunk) {
    call_frame_t* frame = &vm->frames[vm->frame_count++];
    frame->chunk = chunk;
    frame->ip = chunk->code;
    frame->slots = vm->stack;
    
    vm->running = 1;
    
#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->chunk->constants[READ_BYTE()])
#define BINARY_OP(value_type, op) \
    do { \
        skp_object_t* b = vm_pop(vm); \
        skp_object_t* a = vm_pop(vm); \
        skp_double result = skp_to_float(a) op skp_to_float(b); \
        vm_push(vm, skp_new_float(result)); \
    } while (false)
    
    while (vm->running) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (skp_object_t** slot = vm->stack; slot < vm->stack_top; slot++) {
            printf("[ ");
            skp_print(*slot);
            printf(" ]");
        }
        printf("\n");
        disassemble_instruction(frame->chunk, (int)(frame->ip - frame->chunk->code));
#endif
        
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONST_INT: {
                constant_t constant = READ_CONSTANT();
                vm_push(vm, skp_new_int(constant.value.int_val));
                break;
            }
                
            case OP_CONST_FLOAT: {
                constant_t constant = READ_CONSTANT();
                vm_push(vm, skp_new_float(constant.value.float_val));
                break;
            }
                
            case OP_CONST_STRING: {
                constant_t constant = READ_CONSTANT();
                vm_push(vm, skp_new_string(constant.value.string_val));
                break;
            }
                
            case OP_CONST_TRUE:
                vm_push(vm, skp_new_bool(1));
                break;
                
            case OP_CONST_FALSE:
                vm_push(vm, skp_new_bool(0));
                break;
                
            case OP_CONST_NULL:
                vm_push(vm, skp_new_null());
                break;
                
            case OP_CONST_LIST: {
                uint8_t count = READ_BYTE();
                skp_object_t* list = skp_list_create();
                for (int i = count - 1; i >= 0; i--) {
                    skp_list_append(list, vm_peek(vm, i));
                }
                vm_popn(vm, count);
                vm_push(vm, list);
                break;
            }
                
            case OP_CONST_DICT: {
                uint8_t count = READ_BYTE();
                skp_object_t* dict = skp_dict_create();
                for (int i = count - 1; i >= 0; i--) {
                    skp_object_t* value = vm_pop(vm);
                    skp_object_t* key = vm_pop(vm);
                    if (skp_get_type(key) != SKP_TYPE_STRING) {
                        vm_runtime_error(vm, "المفتاح يجب أن يكون نصاً");
                        return SKP_RUNTIME_ERROR;
                    }
                    skp_dict_set(dict, skp_to_string(key), value);
                }
                vm_push(vm, dict);
                break;
            }
                
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                vm_push(vm, frame->slots[slot]);
                break;
            }
                
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = vm_peek(vm, 0);
                break;
            }
                
            case OP_GET_GLOBAL: {
                constant_t name = READ_CONSTANT();
                skp_object_t* value;
                if (!skp_dict_get(vm->globals, name.value.string_val, &value)) {
                    vm_runtime_error(vm, "متغير غير معرف: %s", name.value.string_val);
                    return SKP_RUNTIME_ERROR;
                }
                vm_push(vm, value);
                break;
            }
                
            case OP_SET_GLOBAL: {
                constant_t name = READ_CONSTANT();
                skp_dict_set(vm->globals, name.value.string_val, vm_peek(vm, 0));
                break;
            }
                
            case OP_DEFINE_GLOBAL: {
                constant_t name = READ_CONSTANT();
                skp_dict_set(vm->globals, name.value.string_val, vm_peek(vm, 0));
                vm_pop(vm);
                break;
            }
                
            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                vm_push(vm, *frame->closure->upvalues[slot]->location);
                break;
            }
                
            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = vm_peek(vm, 0);
                break;
            }
                
            case OP_GET_FIELD: {
                constant_t name = READ_CONSTANT();
                skp_object_t* receiver = vm_peek(vm, 0);
                
                if (skp_get_type(receiver) == SKP_TYPE_INSTANCE) {
                    skp_object_t* value;
                    if (skp_dict_get(receiver->data.v_object.fields, 
                                     name.value.string_val, &value)) {
                        vm_pop(vm);
                        vm_push(vm, value);
                        break;
                    }
                }
                
                if (!vm_bind_method(vm, receiver, name.value.string_val)) {
                    return SKP_RUNTIME_ERROR;
                }
                break;
            }
                
            case OP_SET_FIELD: {
                constant_t name = READ_CONSTANT();
                skp_object_t* receiver = vm_peek(vm, 1);
                
                if (skp_get_type(receiver) != SKP_TYPE_INSTANCE) {
                    vm_runtime_error(vm, "يمكن تعيين الحقول فقط للكائنات");
                    return SKP_RUNTIME_ERROR;
                }
                
                skp_dict_set(receiver->data.v_object.fields, 
                            name.value.string_val, vm_peek(vm, 0));
                skp_object_t* value = vm_pop(vm);
                vm_pop(vm);
                vm_push(vm, value);
                break;
            }
                
            case OP_GET_INDEX: {
                skp_object_t* index = vm_pop(vm);
                skp_object_t* object = vm_pop(vm);
                skp_object_t* result = skp_get_index(object, index);
                if (!result) {
                    vm_runtime_error(vm, "فهرس غير صالح");
                    return SKP_RUNTIME_ERROR;
                }
                vm_push(vm, result);
                break;
            }
                
            case OP_SET_INDEX: {
                skp_object_t* value = vm_pop(vm);
                skp_object_t* index = vm_pop(vm);
                skp_object_t* object = vm_pop(vm);
                if (!skp_set_index(object, index, value)) {
                    vm_runtime_error(vm, "فهرس غير صالح");
                    return SKP_RUNTIME_ERROR;
                }
                vm_push(vm, value);
                break;
            }
                
            case OP_ADD: {
                skp_object_t* b = vm_pop(vm);
                skp_object_t* a = vm_pop(vm);
                
                if (skp_get_type(a) == SKP_TYPE_STRING && skp_get_type(b) == SKP_TYPE_STRING) {
                    vm_push(vm, skp_string_concat(a, b));
                } else {
                    vm_push(vm, skp_add(a, b));
                }
                break;
            }
                
            case OP_SUB:
                BINARY_OP(SKP_TYPE_FLOAT, -);
                break;
                
            case OP_MUL:
                BINARY_OP(SKP_TYPE_FLOAT, *);
                break;
                
            case OP_DIV:
                BINARY_OP(SKP_TYPE_FLOAT, /);
                break;
                
            case OP_MOD: {
                skp_object_t* b = vm_pop(vm);
                skp_object_t* a = vm_pop(vm);
                vm_push(vm, skp_mod(a, b));
                break;
            }
                
            case OP_POW: {
                skp_object_t* b = vm_pop(vm);
                skp_object_t* a = vm_pop(vm);
                vm_push(vm, skp_pow(a, b));
                break;
            }
                
            case OP_NEG: {
                skp_object_t* value = vm_pop(vm);
                vm_push(vm, skp_neg(value));
                break;
            }
                
            case OP_NOT: {
                skp_object_t* value = vm_pop(vm);
                vm_push(vm, skp_new_bool(!skp_is_truthy(value)));
                break;
            }
                
            case OP_AND: {
                skp_object_t* b = vm_pop(vm);
                skp_object_t* a = vm_pop(vm);
                vm_push(vm, skp_new_bool(skp_is_truthy(a) && skp_is_truthy(b)));
                break;
            }
                
            case OP_OR: {
                skp_object_t* b = vm_pop(vm);
                skp_object_t* a = vm_pop(vm);
                vm_push(vm, skp_new_bool(skp_is_truthy(a) || skp_is_truthy(b)));
                break;
            }
                
            case OP_EQ: {
                skp_object_t* b = vm_pop(vm);
                skp_object_t* a = vm_pop(vm);
                vm_push(vm, skp_new_bool(skp_eq(a, b)));
                break;
            }
                
            case OP_NE: {
                skp_object_t* b = vm_pop(vm);
                skp_object_t* a = vm_pop(vm);
                vm_push(vm, skp_new_bool(!skp_eq(a, b)));
                break;
            }
                
            case OP_LT:
                BINARY_OP(SKP_TYPE_BOOL, <);
                break;
                
            case OP_GT:
                BINARY_OP(SKP_TYPE_BOOL, >);
                break;
                
            case OP_LE:
                BINARY_OP(SKP_TYPE_BOOL, <=);
                break;
                
            case OP_GE:
                BINARY_OP(SKP_TYPE_BOOL, >=);
                break;
                
            case OP_BIT_AND: {
                skp_object_t* b = vm_pop(vm);
                skp_object_t* a = vm_pop(vm);
                vm_push(vm, skp_new_int(skp_to_int(a) & skp_to_int(b)));
                break;
            }
                
            case OP_BIT_OR: {
                skp_object_t* b = vm_pop(vm);
                skp_object_t* a = vm_pop(vm);
                vm_push(vm, skp_new_int(skp_to_int(a) | skp_to_int(b)));
                break;
            }
                
            case OP_BIT_XOR: {
                skp_object_t* b = vm_pop(vm);
                skp_object_t* a = vm_pop(vm);
                vm_push(vm, skp_new_int(skp_to_int(a) ^ skp_to_int(b)));
                break;
            }
                
            case OP_BIT_NOT: {
                skp_object_t* value = vm_pop(vm);
                vm_push(vm, skp_new_int(~skp_to_int(value)));
                break;
            }
                
            case OP_SHL: {
                skp_object_t* b = vm_pop(vm);
                skp_object_t* a = vm_pop(vm);
                vm_push(vm, skp_new_int(skp_to_int(a) << skp_to_int(b)));
                break;
            }
                
            case OP_SHR: {
                skp_object_t* b = vm_pop(vm);
                skp_object_t* a = vm_pop(vm);
                vm_push(vm, skp_new_int(skp_to_int(a) >> skp_to_int(b)));
                break;
            }
                
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
                
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (!skp_is_truthy(vm_peek(vm, 0))) {
                    frame->ip += offset;
                }
                break;
            }
                
            case OP_JUMP_IF_TRUE: {
                uint16_t offset = READ_SHORT();
                if (skp_is_truthy(vm_peek(vm, 0))) {
                    frame->ip += offset;
                }
                break;
            }
                
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
                
            case OP_CALL: {
                int arg_count = READ_BYTE();
                if (!vm_call_value(vm, vm_peek(vm, arg_count), arg_count)) {
                    return SKP_RUNTIME_ERROR;
                }
                frame = &vm->frames[vm->frame_count - 1];
                break;
            }
                
            case OP_RETURN: {
                skp_object_t* result = vm_pop(vm);
                vm_close_upvalues(vm, frame->slots);
                vm->frame_count--;
                if (vm->frame_count == 0) {
                    vm_pop(vm);
                    return SKP_OK;
                }
                
                vm->stack_top = frame->slots;
                vm_push(vm, result);
                frame = &vm->frames[vm->frame_count - 1];
                break;
            }
                
            case OP_RETURN_VOID: {
                vm_close_upvalues(vm, frame->slots);
                vm->frame_count--;
                if (vm->frame_count == 0) {
                    return SKP_OK;
                }
                
                vm->stack_top = frame->slots;
                vm_push(vm, skp_new_null());
                frame = &vm->frames[vm->frame_count - 1];
                break;
            }
                
            case OP_CLOSURE: {
                constant_t constant = READ_CONSTANT();
                skp_object_t* function = skp_new_function(constant.value.string_val);
                vm_push(vm, function);
                
                skp_object_t* closure = skp_new_closure(function);
                vm_pop(vm);
                vm_push(vm, closure);
                
                uint8_t upvalue_count = READ_BYTE();
                for (int i = 0; i < upvalue_count; i++) {
                    uint8_t is_local = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (is_local) {
                        closure->data.v_closure.upvalues[i] = 
                            vm_capture_upvalue(vm, frame->slots + index);
                    } else {
                        closure->data.v_closure.upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }
                
            case OP_CLOSE_UPVALUE:
                vm_close_upvalues(vm, vm->stack_top - 1);
                vm_pop(vm);
                break;
                
            case OP_CLASS: {
                constant_t name = READ_CONSTANT();
                vm_push(vm, skp_new_class(name.value.string_val));
                break;
            }
                
            case OP_METHOD: {
                constant_t name = READ_CONSTANT();
                skp_object_t* method = vm_peek(vm, 0);
                skp_object_t* klass = vm_peek(vm, 1);
                skp_dict_set(klass->data.v_class.klass->methods, 
                            name.value.string_val, method);
                vm_pop(vm);
                break;
            }
                
            case OP_INHERIT: {
                skp_object_t* superclass = vm_peek(vm, 1);
                skp_object_t* subclass = vm_peek(vm, 0);
                
                if (skp_get_type(superclass) != SKP_TYPE_CLASS) {
                    vm_runtime_error(vm, "الصنف الأب يجب أن يكون صنفاً");
                    return SKP_RUNTIME_ERROR;
                }
                
                /* نسخ الطرق من الأب */
                skp_dict_merge(subclass->data.v_class.klass->methods,
                              superclass->data.v_class.klass->methods);
                
                vm_pop(vm);
                break;
            }
                
            case OP_POP:
                vm_pop(vm);
                break;
                
            case OP_DUP:
                vm_push(vm, vm_peek(vm, 0));
                break;
                
            case OP_SWAP: {
                skp_object_t* a = vm_pop(vm);
                skp_object_t* b = vm_pop(vm);
                vm_push(vm, a);
                vm_push(vm, b);
                break;
            }
                
            case OP_PRINT: {
                skp_object_t* value = vm_pop(vm);
                skp_print(value);
                printf("\n");
                break;
            }
                
            case OP_IMPORT: {
                constant_t name = READ_CONSTANT();
                /* TODO: تنفيذ الاستيراد */
                break;
            }
                
            case OP_EXPORT: {
                constant_t name = READ_CONSTANT();
                /* TODO: تنفيذ التصدير */
                break;
            }
                
            case OP_HALT:
                vm->running = 0;
                return SKP_OK;
                
            default:
                vm_runtime_error(vm, "كود عملية غير معروف: %d", instruction);
                return SKP_RUNTIME_ERROR;
        }
    }
    
#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef BINARY_OP
    
    return SKP_OK;
}

/* ========== دوال native مدمجة ========== */

void vm_define_native(skp_vm_t* vm, const char* name, skp_native_func_t func) {
    skp_dict_set(vm->globals, name, skp_new_native(func));
}

/* دوال الإدخال/الإخراج */
skp_object_t* native_print(skp_vm_t* vm, int argc, skp_object_t** argv) {
    for (int i = 0; i < argc; i++) {
        skp_print(argv[i]);
        if (i < argc - 1) printf(" ");
    }
    printf("\n");
    return skp_new_null();
}

skp_object_t* native_input(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc > 0) {
        skp_print(argv[0]);
    }
    
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), stdin)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
        return skp_new_string(buffer);
    }
    
    return skp_new_null();
}

skp_object_t* native_clock(skp_vm_t* vm, int argc, skp_object_t** argv) {
    return skp_new_float((skp_double)clock() / CLOCKS_PER_SEC);
}

skp_object_t* native_type(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_null();
    return skp_new_string(skp_type_name(skp_get_type(argv[0])));
}

skp_object_t* native_len(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_int(0);
    
    skp_type_t type = skp_get_type(argv[0]);
    if (type == SKP_TYPE_STRING) {
        return skp_new_int(strlen(argv[0]->data.v_string.chars));
    } else if (type == SKP_TYPE_LIST) {
        return skp_new_int(argv[0]->data.v_list.count);
    } else if (type == SKP_TYPE_DICT) {
        return skp_new_int(argv[0]->data.v_dict.count);
    }
    
    return skp_new_int(0);
}

skp_object_t* native_range(skp_vm_t* vm, int argc, skp_object_t** argv) {
    skp_int start = 0, end = 0, step = 1;
    
    if (argc == 1) {
        end = skp_to_int(argv[0]);
    } else if (argc >= 2) {
        start = skp_to_int(argv[0]);
        end = skp_to_int(argv[1]);
        if (argc >= 3) {
            step = skp_to_int(argv[2]);
        }
    }
    
    skp_object_t* list = skp_list_create();
    if (step > 0) {
        for (skp_int i = start; i < end; i += step) {
            skp_list_append(list, skp_new_int(i));
        }
    } else if (step < 0) {
        for (skp_int i = start; i > end; i += step) {
            skp_list_append(list, skp_new_int(i));
        }
    }
    
    return list;
}

skp_object_t* native_int(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_int(0);
    return skp_new_int(skp_to_int(argv[0]));
}

skp_object_t* native_float(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_float(0.0);
    return skp_new_float(skp_to_float(argv[0]));
}

skp_object_t* native_str(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_string("");
    
    char buffer[256];
    skp_type_t type = skp_get_type(argv[0]);
    
    switch (type) {
        case SKP_TYPE_INT:
            snprintf(buffer, sizeof(buffer), "%lld", argv[0]->data.v_int.value);
            return skp_new_string(buffer);
        case SKP_TYPE_FLOAT:
            snprintf(buffer, sizeof(buffer), "%g", argv[0]->data.v_float.value);
            return skp_new_string(buffer);
        case SKP_TYPE_BOOL:
            return skp_new_string(argv[0]->data.v_bool.value ? "صحيح" : "خطأ");
        case SKP_TYPE_NULL:
            return skp_new_string("فارغ");
        case SKP_TYPE_STRING:
            return argv[0];
        default:
            snprintf(buffer, sizeof(buffer), "<%s>", skp_type_name(type));
            return skp_new_string(buffer);
    }
}

skp_object_t* native_exit(skp_vm_t* vm, int argc, skp_object_t** argv) {
    int code = 0;
    if (argc > 0) {
        code = (int)skp_to_int(argv[0]);
    }
    exit(code);
    return skp_new_null();
}

/* دوال الرياضيات */
skp_object_t* native_abs(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_int(0);
    skp_double val = skp_to_float(argv[0]);
    return skp_new_float(val < 0 ? -val : val);
}

skp_object_t* native_sqrt(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_float(0);
    return skp_new_float(sqrt(skp_to_float(argv[0])));
}

skp_object_t* native_pow(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 2) return skp_new_float(0);
    return skp_new_float(pow(skp_to_float(argv[0]), skp_to_float(argv[1])));
}

skp_object_t* native_sin(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_float(0);
    return skp_new_float(sin(skp_to_float(argv[0])));
}

skp_object_t* native_cos(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_float(0);
    return skp_new_float(cos(skp_to_float(argv[0])));
}

skp_object_t* native_tan(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_float(0);
    return skp_new_float(tan(skp_to_float(argv[0])));
}

skp_object_t* native_log(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_float(0);
    return skp_new_float(log(skp_to_float(argv[0])));
}

skp_object_t* native_log10(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_float(0);
    return skp_new_float(log10(skp_to_float(argv[0])));
}

skp_object_t* native_exp(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_float(0);
    return skp_new_float(exp(skp_to_float(argv[0])));
}

skp_object_t* native_floor(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_float(0);
    return skp_new_float(floor(skp_to_float(argv[0])));
}

skp_object_t* native_ceil(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_float(0);
    return skp_new_float(ceil(skp_to_float(argv[0])));
}

skp_object_t* native_round(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_float(0);
    return skp_new_float(round(skp_to_float(argv[0])));
}

skp_object_t* native_min(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_int(0);
    skp_object_t* min = argv[0];
    for (int i = 1; i < argc; i++) {
        if (skp_to_float(argv[i]) < skp_to_float(min)) {
            min = argv[i];
        }
    }
    return min;
}

skp_object_t* native_max(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_int(0);
    skp_object_t* max = argv[0];
    for (int i = 1; i < argc; i++) {
        if (skp_to_float(argv[i]) > skp_to_float(max)) {
            max = argv[i];
        }
    }
    return max;
}

skp_object_t* native_random(skp_vm_t* vm, int argc, skp_object_t** argv) {
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned)time(NULL));
        seeded = 1;
    }
    
    if (argc >= 2) {
        skp_int min = skp_to_int(argv[0]);
        skp_int max = skp_to_int(argv[1]);
        return skp_new_int(min + rand() % (max - min + 1));
    }
    
    return skp_new_float((skp_double)rand() / RAND_MAX);
}

/* دوال النصوص */
skp_object_t* native_chr(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_string("");
    char buffer[2] = {(char)skp_to_int(argv[0]), '\0'};
    return skp_new_string(buffer);
}

skp_object_t* native_ord(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1 || skp_get_type(argv[0]) != SKP_TYPE_STRING) {
        return skp_new_int(0);
    }
    return skp_new_int((skp_int)argv[0]->data.v_string.chars[0]);
}

skp_object_t* native_split(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 2 || skp_get_type(argv[0]) != SKP_TYPE_STRING || 
        skp_get_type(argv[1]) != SKP_TYPE_STRING) {
        return skp_list_create();
    }
    
    skp_string str = argv[0]->data.v_string.chars;
    skp_string sep = argv[1]->data.v_string.chars;
    
    skp_object_t* list = skp_list_create();
    char* copy = strdup(str);
    char* token = strtok(copy, sep);
    
    while (token) {
        skp_list_append(list, skp_new_string(token));
        token = strtok(NULL, sep);
    }
    
    free(copy);
    return list;
}

skp_object_t* native_join(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 2 || skp_get_type(argv[0]) != SKP_TYPE_LIST || 
        skp_get_type(argv[1]) != SKP_TYPE_STRING) {
        return skp_new_string("");
    }
    
    skp_object_t* list = argv[0];
    skp_string sep = argv[1]->data.v_string.chars;
    
    char buffer[4096] = {0};
    size_t pos = 0;
    
    for (size_t i = 0; i < list->data.v_list.count; i++) {
        skp_object_t* item = list->data.v_list.items[i];
        skp_string str = skp_to_string(item);
        
        if (i > 0) {
            pos += snprintf(buffer + pos, sizeof(buffer) - pos, "%s", sep);
        }
        pos += snprintf(buffer + pos, sizeof(buffer) - pos, "%s", str);
    }
    
    return skp_new_string(buffer);
}

skp_object_t* native_upper(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1 || skp_get_type(argv[0]) != SKP_TYPE_STRING) {
        return skp_new_string("");
    }
    
    skp_string str = argv[0]->data.v_string.chars;
    char* result = strdup(str);
    for (char* p = result; *p; p++) {
        *p = toupper(*p);
    }
    
    skp_object_t* obj = skp_new_string(result);
    free(result);
    return obj;
}

skp_object_t* native_lower(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1 || skp_get_type(argv[0]) != SKP_TYPE_STRING) {
        return skp_new_string("");
    }
    
    skp_string str = argv[0]->data.v_string.chars;
    char* result = strdup(str);
    for (char* p = result; *p; p++) {
        *p = tolower(*p);
    }
    
    skp_object_t* obj = skp_new_string(result);
    free(result);
    return obj;
}

skp_object_t* native_strip(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1 || skp_get_type(argv[0]) != SKP_TYPE_STRING) {
        return skp_new_string("");
    }
    
    skp_string str = argv[0]->data.v_string.chars;
    while (isspace(*str)) str++;
    
    char* end = (char*)str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    end[1] = '\0';
    
    return skp_new_string(str);
}

skp_object_t* native_replace(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 3 || skp_get_type(argv[0]) != SKP_TYPE_STRING || 
        skp_get_type(argv[1]) != SKP_TYPE_STRING || 
        skp_get_type(argv[2]) != SKP_TYPE_STRING) {
        return skp_new_string("");
    }
    
    skp_string str = argv[0]->data.v_string.chars;
    skp_string old = argv[1]->data.v_string.chars;
    skp_string new_str = argv[2]->data.v_string.chars;
    
    char buffer[4096] = {0};
    char* pos = (char*)str;
    char* found;
    size_t buf_pos = 0;
    size_t old_len = strlen(old);
    
    while ((found = strstr(pos, old)) != NULL) {
        size_t len = found - pos;
        memcpy(buffer + buf_pos, pos, len);
        buf_pos += len;
        memcpy(buffer + buf_pos, new_str, strlen(new_str));
        buf_pos += strlen(new_str);
        pos = found + old_len;
    }
    
    strcpy(buffer + buf_pos, pos);
    return skp_new_string(buffer);
}

skp_object_t* native_find(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 2 || skp_get_type(argv[0]) != SKP_TYPE_STRING || 
        skp_get_type(argv[1]) != SKP_TYPE_STRING) {
        return skp_new_int(-1);
    }
    
    skp_string str = argv[0]->data.v_string.chars;
    skp_string substr = argv[1]->data.v_string.chars;
    
    char* found = strstr(str, substr);
    if (found) {
        return skp_new_int(found - str);
    }
    return skp_new_int(-1);
}

skp_object_t* native_startswith(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 2 || skp_get_type(argv[0]) != SKP_TYPE_STRING || 
        skp_get_type(argv[1]) != SKP_TYPE_STRING) {
        return skp_new_bool(0);
    }
    
    skp_string str = argv[0]->data.v_string.chars;
    skp_string prefix = argv[1]->data.v_string.chars;
    
    return skp_new_bool(strncmp(str, prefix, strlen(prefix)) == 0);
}

skp_object_t* native_endswith(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 2 || skp_get_type(argv[0]) != SKP_TYPE_STRING || 
        skp_get_type(argv[1]) != SKP_TYPE_STRING) {
        return skp_new_bool(0);
    }
    
    skp_string str = argv[0]->data.v_string.chars;
    skp_string suffix = argv[1]->data.v_string.chars;
    
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    
    if (suffix_len > str_len) return skp_new_bool(0);
    
    return skp_new_bool(strcmp(str + str_len - suffix_len, suffix) == 0);
}

/* دوال القوائم والقواميس */
skp_object_t* native_append(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 2 || skp_get_type(argv[0]) != SKP_TYPE_LIST) {
        return skp_new_null();
    }
    
    skp_list_append(argv[0], argv[1]);
    return argv[0];
}

skp_object_t* native_insert(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 3 || skp_get_type(argv[0]) != SKP_TYPE_LIST) {
        return skp_new_null();
    }
    
    skp_int index = skp_to_int(argv[1]);
    skp_list_insert(argv[0], index, argv[2]);
    return argv[0];
}

skp_object_t* native_remove(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 2) return skp_new_null();
    
    skp_type_t type = skp_get_type(argv[0]);
    if (type == SKP_TYPE_LIST) {
        skp_int index = skp_to_int(argv[1]);
        skp_list_remove(argv[0], index);
    } else if (type == SKP_TYPE_DICT) {
        skp_dict_remove(argv[0], skp_to_string(argv[1]));
    }
    
    return argv[0];
}

skp_object_t* native_pop(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1 || skp_get_type(argv[0]) != SKP_TYPE_LIST) {
        return skp_new_null();
    }
    
    skp_object_t* list = argv[0];
    if (list->data.v_list.count == 0) return skp_new_null();
    
    skp_int index = list->data.v_list.count - 1;
    if (argc >= 2) {
        index = skp_to_int(argv[1]);
    }
    
    skp_object_t* result = skp_list_get(list, index);
    skp_list_remove(list, index);
    return result;
}

skp_object_t* native_clear(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_null();
    
    skp_type_t type = skp_get_type(argv[0]);
    if (type == SKP_TYPE_LIST) {
        skp_list_clear(argv[0]);
    } else if (type == SKP_TYPE_DICT) {
        skp_dict_clear(argv[0]);
    }
    
    return argv[0];
}

skp_object_t* native_sort(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1 || skp_get_type(argv[0]) != SKP_TYPE_LIST) {
        return skp_new_null();
    }
    
    /* TODO: تنفيذ الفرز */
    return argv[0];
}

skp_object_t* native_reverse(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1 || skp_get_type(argv[0]) != SKP_TYPE_LIST) {
        return skp_new_null();
    }
    
    skp_object_t* list = argv[0];
    size_t count = list->data.v_list.count;
    
    for (size_t i = 0; i < count / 2; i++) {
        skp_object_t* temp = list->data.v_list.items[i];
        list->data.v_list.items[i] = list->data.v_list.items[count - 1 - i];
        list->data.v_list.items[count - 1 - i] = temp;
    }
    
    return list;
}

skp_object_t* native_copy(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1) return skp_new_null();
    
    skp_type_t type = skp_get_type(argv[0]);
    if (type == SKP_TYPE_LIST) {
        return skp_list_copy(argv[0]);
    } else if (type == SKP_TYPE_DICT) {
        return skp_dict_copy(argv[0]);
    }
    
    return argv[0];
}

skp_object_t* native_keys(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1 || skp_get_type(argv[0]) != SKP_TYPE_DICT) {
        return skp_list_create();
    }
    
    return skp_dict_keys(argv[0]);
}

skp_object_t* native_values(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1 || skp_get_type(argv[0]) != SKP_TYPE_DICT) {
        return skp_list_create();
    }
    
    return skp_dict_values(argv[0]);
}

skp_object_t* native_items(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1 || skp_get_type(argv[0]) != SKP_TYPE_DICT) {
        return skp_list_create();
    }
    
    return skp_dict_items(argv[0]);
}

/* دوال الملفات */
skp_object_t* native_open(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 2 || skp_get_type(argv[0]) != SKP_TYPE_STRING || 
        skp_get_type(argv[1]) != SKP_TYPE_STRING) {
        return skp_new_null();
    }
    
    skp_string path = argv[0]->data.v_string.chars;
    skp_string mode = argv[1]->data.v_string.chars;
    
    FILE* file = fopen(path, mode);
    if (!file) return skp_new_null();
    
    /* TODO: إرجاع كائن ملف */
    fclose(file);
    return skp_new_null();
}

skp_object_t* native_read(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1 || skp_get_type(argv[0]) != SKP_TYPE_STRING) {
        return skp_new_null();
    }
    
    skp_string path = argv[0]->data.v_string.chars;
    FILE* file = fopen(path, "r");
    if (!file) return skp_new_null();
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = malloc(size + 1);
    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    
    skp_object_t* result = skp_new_string(buffer);
    free(buffer);
    return result;
}

skp_object_t* native_write(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 2 || skp_get_type(argv[0]) != SKP_TYPE_STRING || 
        skp_get_type(argv[1]) != SKP_TYPE_STRING) {
        return skp_new_bool(0);
    }
    
    skp_string path = argv[0]->data.v_string.chars;
    skp_string content = argv[1]->data.v_string.chars;
    
    FILE* file = fopen(path, "w");
    if (!file) return skp_new_bool(0);
    
    fwrite(content, 1, strlen(content), file);
    fclose(file);
    
    return skp_new_bool(1);
}

skp_object_t* native_close(skp_vm_t* vm, int argc, skp_object_t** argv) {
    /* TODO: إغلاق كائن ملف */
    return skp_new_null();
}

skp_object_t* native_exists(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1 || skp_get_type(argv[0]) != SKP_TYPE_STRING) {
        return skp_new_bool(0);
    }
    
    FILE* file = fopen(argv[0]->data.v_string.chars, "r");
    if (file) {
        fclose(file);
        return skp_new_bool(1);
    }
    return skp_new_bool(0);
}

skp_object_t* native_remove_file(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1 || skp_get_type(argv[0]) != SKP_TYPE_STRING) {
        return skp_new_bool(0);
    }
    
    return skp_new_bool(remove(argv[0]->data.v_string.chars) == 0);
}

skp_object_t* native_rename(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 2 || skp_get_type(argv[0]) != SKP_TYPE_STRING || 
        skp_get_type(argv[1]) != SKP_TYPE_STRING) {
        return skp_new_bool(0);
    }
    
    return skp_new_bool(rename(argv[0]->data.v_string.chars, 
                                argv[1]->data.v_string.chars) == 0);
}

skp_object_t* native_mkdir(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1 || skp_get_type(argv[0]) != SKP_TYPE_STRING) {
        return skp_new_bool(0);
    }
    
#ifdef _WIN32
    return skp_new_bool(_mkdir(argv[0]->data.v_string.chars) == 0);
#else
    return skp_new_bool(mkdir(argv[0]->data.v_string.chars, 0755) == 0);
#endif
}

skp_object_t* native_rmdir(skp_vm_t* vm, int argc, skp_object_t** argv) {
    if (argc < 1 || skp_get_type(argv[0]) != SKP_TYPE_STRING) {
        return skp_new_bool(0);
    }
    
    return skp_new_bool(rmdir(argv[0]->data.v_string.chars) == 0);
}

skp_object_t* native_listdir(skp_vm_t* vm, int argc, skp_object_t** argv) {
    skp_string path = ".";
    if (argc >= 1 && skp_get_type(argv[0]) == SKP_TYPE_STRING) {
        path = argv[0]->data.v_string.chars;
    }
    
    skp_object_t* list = skp_list_create();
    
#ifdef _WIN32
    WIN32_FIND_DATA findData;
    HANDLE hFind;
    char searchPath[MAX_PATH];
    snprintf(searchPath, MAX_PATH, "%s\\*", path);
    
    hFind = FindFirstFile(searchPath, &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            skp_list_append(list, skp_new_string(findData.cFileName));
        } while (FindNextFile(hFind, &findData));
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(path);
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            skp_list_append(list, skp_new_string(entry->d_name));
        }
        closedir(dir);
    }
#endif
    
    return list;
}

/* ========== تسجيل الدوال المدمجة ========== */

void vm_register_natives(skp_vm_t* vm) {
    /* الإدخال/الإخراج */
    vm_define_native(vm, "اطبع", native_print);
    vm_define_native(vm, "ادخل", native_input);
    vm_define_native(vm, "الوقت", native_clock);
    vm_define_native(vm, "النوع", native_type);
    vm_define_native(vm, "الطول", native_len);
    vm_define_native(vm, "المدى", native_range);
    vm_define_native(vm, "صحيح", native_int);
    vm_define_native(vm, "عشري", native_float);
    vm_define_native(vm, "نص", native_str);
    vm_define_native(vm, "اخرج", native_exit);
    
    /* الرياضيات */
    vm_define_native(vm, "قيمة_مطلقة", native_abs);
    vm_define_native(vm, "جذر", native_sqrt);
    vm_define_native(vm, "أس", native_pow);
    vm_define_native(vm, "جيب", native_sin);
    vm_define_native(vm, "جيب_تام", native_cos);
    vm_define_native(vm, "ظل", native_tan);
    vm_define_native(vm, "لوغاريتم", native_log);
    vm_define_native(vm, "لوغاريتم10", native_log10);
    vm_define_native(vm, "أس_الطبيعي", native_exp);
    vm_define_native(vm, "أرض", native_floor);
    vm_define_native(vm, "سقف", native_ceil);
    vm_define_native(vm, "تقريب", native_round);
    vm_define_native(vm, "أصغر", native_min);
    vm_define_native(vm, "أكبر", native_max);
    vm_define_native(vm, "عشوائي", native_random);
    
    /* النصوص */
    vm_define_native(vm, "حرف", native_chr);
    vm_define_native(vm, "ترميز", native_ord);
    vm_define_native(vm, "قسم", native_split);
    vm_define_native(vm, "اربط", native_join);
    vm_define_native(vm, "كبير", native_upper);
    vm_define_native(vm, "صغير", native_lower);
    vm_define_native(vm, "تقليم", native_strip);
    vm_define_native(vm, "استبدل", native_replace);
    vm_define_native(vm, "ابحث", native_find);
    vm_define_native(vm, "يبدأ_بـ", native_startswith);
    vm_define_native(vm, "ينتهي_بـ", native_endswith);
    
    /* القوائم والقواميس */
    vm_define_native(vm, "أضف", native_append);
    vm_define_native(vm, "أدخل", native_insert);
    vm_define_native(vm, "احذف", native_remove);
    vm_define_native(vm, "اسحب", native_pop);
    vm_define_native(vm, "امسح", native_clear);
    vm_define_native(vm, "رتب", native_sort);
    vm_define_native(vm, "اعكس", native_reverse);
    vm_define_native(vm, "انسخ", native_copy);
    vm_define_native(vm, "المفاتيح", native_keys);
    vm_define_native(vm, "القيم", native_values);
    vm_define_native(vm, "الأزواج", native_items);
    
    /* الملفات */
    vm_define_native(vm, "افتح", native_open);
    vm_define_native(vm, "اقرأ", native_read);
    vm_define_native(vm, "اكتب", native_write);
    vm_define_native(vm, "أغلق", native_close);
    vm_define_native(vm, "موجود", native_exists);
    vm_define_native(vm, "احذف_ملف", native_remove_file);
    vm_define_native(vm, "أعد_تسمية", native_rename);
    vm_define_native(vm, "أنشئ_مجلد", native_mkdir);
    vm_define_native(vm, "احذف_مجلد", native_rmdir);
    vm_define_native(vm, "المحتويات", native_listdir);
}
