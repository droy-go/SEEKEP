/*
 * SEEKEP - Smart Expressive Efficient Knowledge Execution Platform
 * المترجم (Compiler) - Compiler Implementation
 * 
 * يقوم بتحويل شجرة البنية المجردة إلى بايتكود
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"

/* ========== إدارة الكتلة ========== */

void chunk_init(chunk_t* chunk) {
    chunk->code = NULL;
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->lines = NULL;
    chunk->constants = NULL;
    chunk->constant_count = 0;
    chunk->constant_capacity = 0;
}

void chunk_free(chunk_t* chunk) {
    free(chunk->code);
    free(chunk->lines);
    
    for (size_t i = 0; i < chunk->constant_count; i++) {
        if (chunk->constants[i].type == SKP_TYPE_STRING && 
            chunk->constants[i].value.string_val) {
            free(chunk->constants[i].value.string_val);
        }
    }
    free(chunk->constants);
    
    chunk_init(chunk);
}

void chunk_write(chunk_t* chunk, uint8_t byte, int line) {
    if (chunk->count >= chunk->capacity) {
        chunk->capacity = chunk->capacity == 0 ? 8 : chunk->capacity * 2;
        chunk->code = (uint8_t*)realloc(chunk->code, chunk->capacity);
        chunk->lines = (int*)realloc(chunk->lines, chunk->capacity * sizeof(int));
    }
    
    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

int chunk_add_constant(chunk_t* chunk, constant_t constant) {
    if (chunk->constant_count >= chunk->constant_capacity) {
        chunk->constant_capacity = chunk->constant_capacity == 0 ? 8 : chunk->constant_capacity * 2;
        chunk->constants = (constant_t*)realloc(chunk->constants, 
                                                  chunk->constant_capacity * sizeof(constant_t));
    }
    
    chunk->constants[chunk->constant_count] = constant;
    return chunk->constant_count++;
}

/* ========== إنشاء وإتلاف المترجم ========== */

static compiler_t* create_compiler(compiler_t* enclosing, function_type_t type, const char* name) {
    compiler_t* compiler = (compiler_t*)malloc(sizeof(compiler_t));
    if (!compiler) return NULL;
    
    compiler->enclosing = enclosing;
    compiler->type = type;
    compiler->name = name ? strdup(name) : NULL;
    compiler->local_count = 0;
    compiler->scope_depth = 0;
    compiler->upvalue_count = 0;
    compiler->function_arity = 0;
    
    chunk_init(&compiler->chunk);
    
    /* المتغير المحلي الأول هو دائماً 'هذا' في الأساليب */
    if (type != TYPE_SCRIPT) {
        compiler->locals[0].name = strdup("هذا");
        compiler->locals[0].depth = 0;
        compiler->locals[0].is_captured = 0;
        compiler->local_count = 1;
    }
    
    return compiler;
}

static void destroy_compiler(compiler_t* compiler) {
    if (!compiler) return;
    
    for (int i = 0; i < compiler->local_count; i++) {
        free(compiler->locals[i].name);
    }
    
    chunk_free(&compiler->chunk);
    free(compiler->name);
    free(compiler);
}

skp_compiler_t* compiler_create(parser_t* parser) {
    skp_compiler_t* compiler = (skp_compiler_t*)malloc(sizeof(skp_compiler_t));
    if (!compiler) return NULL;
    
    compiler->parser = parser;
    compiler->ast = NULL;
    compiler->had_error = 0;
    compiler->current = create_compiler(NULL, TYPE_SCRIPT, NULL);
    
    return compiler;
}

void compiler_destroy(skp_compiler_t* compiler) {
    if (!compiler) return;
    
    destroy_compiler(compiler->current);
    free(compiler);
}

chunk_t* compiler_compile(skp_compiler_t* compiler, ast_node_t* ast) {
    if (!compiler || !ast) return NULL;
    
    compiler->ast = ast;
    compile_node(compiler, ast);
    
    /* إضافة إيقاف في النهاية */
    emit_opcode(compiler, OP_HALT);
    
    if (compiler->had_error) {
        return NULL;
    }
    
    /* نسخ الكتلة للإرجاع */
    chunk_t* result = (chunk_t*)malloc(sizeof(chunk_t));
    if (result) {
        *result = compiler->current->chunk;
        /* إعادة تعيين الكتلة الأصلية حتى لا يتم تحريرها */
        compiler->current->chunk.code = NULL;
        compiler->current->chunk.lines = NULL;
        compiler->current->chunk.constants = NULL;
    }
    
    return result;
}

/* ========== كتابة أكواد العمليات ========== */

void emit_byte(skp_compiler_t* compiler, uint8_t byte) {
    chunk_write(&compiler->current->chunk, byte, 
                compiler->ast ? compiler->ast->line : 0);
}

void emit_bytes(skp_compiler_t* compiler, uint8_t byte1, uint8_t byte2) {
    emit_byte(compiler, byte1);
    emit_byte(compiler, byte2);
}

void emit_opcode(skp_compiler_t* compiler, opcode_t op) {
    emit_byte(compiler, (uint8_t)op);
}

void emit_constant(skp_compiler_t* compiler, constant_t constant) {
    int index = chunk_add_constant(&compiler->current->chunk, constant);
    
    if (index > 255) {
        fprintf(stderr, "خطأ: عدد كبير جداً من الثوابت\n");
        compiler->had_error = 1;
        return;
    }
    
    emit_bytes(compiler, OP_CONST_INT, (uint8_t)index);
}

void emit_jump(skp_compiler_t* compiler, opcode_t op, int offset) {
    emit_opcode(compiler, op);
    /* نترك مكاناً للإزاحة (2 بايت) */
    emit_byte(compiler, 0xFF);
    emit_byte(compiler, 0xFF);
}

void patch_jump(skp_compiler_t* compiler, int offset) {
    int jump = compiler->current->chunk.count - offset - 2;
    
    if (jump > 65535) {
        fprintf(stderr, "خطأ: قفزة كبيرة جداً\n");
        compiler->had_error = 1;
        return;
    }
    
    compiler->current->chunk.code[offset] = (jump >> 8) & 0xFF;
    compiler->current->chunk.code[offset + 1] = jump & 0xFF;
}

void emit_loop(skp_compiler_t* compiler, int loop_start) {
    emit_opcode(compiler, OP_LOOP);
    
    int offset = compiler->current->chunk.count - loop_start + 2;
    if (offset > 65535) {
        fprintf(stderr, "خطأ: حلقة كبيرة جداً\n");
        compiler->had_error = 1;
        return;
    }
    
    emit_byte(compiler, (offset >> 8) & 0xFF);
    emit_byte(compiler, offset & 0xFF);
}

/* ========== التحكم في النطاق ========== */

void begin_scope(skp_compiler_t* compiler) {
    compiler->current->scope_depth++;
}

void end_scope(skp_compiler_t* compiler) {
    compiler->current->scope_depth--;
    
    /* إزالة المتغيرات المحلية التي خرجت من النطاق */
    while (compiler->current->local_count > 0 &&
           compiler->current->locals[compiler->current->local_count - 1].depth > 
           compiler->current->scope_depth) {
        
        if (compiler->current->locals[compiler->current->local_count - 1].is_captured) {
            emit_opcode(compiler, OP_CLOSE_UPVALUE);
        } else {
            emit_opcode(compiler, OP_POP);
        }
        
        compiler->current->local_count--;
    }
}

/* ========== المتغيرات ========== */

int resolve_local(skp_compiler_t* compiler, const char* name) {
    for (int i = compiler->current->local_count - 1; i >= 0; i--) {
        if (strcmp(compiler->current->locals[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int add_upvalue(skp_compiler_t* compiler, uint8_t index, int is_local) {
    int upvalue_count = compiler->current->upvalue_count;
    
    /* التحقق من وجود upvalue مسبقاً */
    for (int i = 0; i < upvalue_count; i++) {
        if (compiler->current->upvalues[i].index == index &&
            compiler->current->upvalues[i].is_local == is_local) {
            return i;
        }
    }
    
    if (upvalue_count >= 256) {
        fprintf(stderr, "خطأ: عدد كبير جداً من upvalues\n");
        return 0;
    }
    
    compiler->current->upvalues[upvalue_count].index = index;
    compiler->current->upvalues[upvalue_count].is_local = is_local;
    return compiler->current->upvalue_count++;
}

int resolve_upvalue(skp_compiler_t* compiler, const char* name) {
    if (compiler->current->enclosing == NULL) return -1;
    
    int local = resolve_local(compiler->current->enclosing, name);
    if (local != -1) {
        compiler->current->enclosing->locals[local].is_captured = 1;
        return add_upvalue(compiler, (uint8_t)local, 1);
    }
    
    int upvalue = resolve_upvalue(compiler->current->enclosing, name);
    if (upvalue != -1) {
        return add_upvalue(compiler, (uint8_t)upvalue, 0);
    }
    
    return -1;
}

void add_local(skp_compiler_t* compiler, const char* name) {
    if (compiler->current->local_count >= 256) {
        fprintf(stderr, "خطأ: عدد كبير جداً من المتغيرات المحلية\n");
        compiler->had_error = 1;
        return;
    }
    
    compiler->current->locals[compiler->current->local_count].name = strdup(name);
    compiler->current->locals[compiler->current->local_count].depth = -1; /* سيتم تعيينه لاحقاً */
    compiler->current->locals[compiler->current->local_count].is_captured = 0;
    compiler->current->local_count++;
}

void declare_variable(skp_compiler_t* compiler, const char* name) {
    if (compiler->current->scope_depth == 0) return;
    
    /* التحقق من عدم وجود متغير بنفس الاسم في نفس النطاق */
    for (int i = compiler->current->local_count - 1; i >= 0; i--) {
        if (compiler->current->locals[i].depth != -1 &&
            compiler->current->locals[i].depth < compiler->current->scope_depth) {
            break;
        }
        
        if (strcmp(compiler->current->locals[i].name, name) == 0) {
            fprintf(stderr, "خطأ: متغير '%s' معرف مسبقاً في هذا النطاق\n", name);
            compiler->had_error = 1;
            return;
        }
    }
    
    add_local(compiler, name);
}

uint8_t identifier_constant(skp_compiler_t* compiler, const char* name) {
    constant_t constant;
    constant.type = SKP_TYPE_STRING;
    constant.value.string_val = strdup(name);
    return (uint8_t)chunk_add_constant(&compiler->current->chunk, constant);
}

void define_variable(skp_compiler_t* compiler, uint8_t global) {
    if (compiler->current->scope_depth > 0) {
        /* متغير محلي */
        compiler->current->locals[compiler->current->local_count - 1].depth = 
            compiler->current->scope_depth;
        return;
    }
    
    emit_bytes(compiler, OP_DEFINE_GLOBAL, global);
}

void named_variable(skp_compiler_t* compiler, const char* name, int can_assign) {
    uint8_t get_op, set_op;
    int arg = resolve_local(compiler->current, name);
    
    if (arg != -1) {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    } else if ((arg = resolve_upvalue(compiler, name)) != -1) {
        get_op = OP_GET_UPVALUE;
        set_op = OP_SET_UPVALUE;
    } else {
        arg = identifier_constant(compiler, name);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }
    
    if (can_assign) {
        emit_bytes(compiler, set_op, (uint8_t)arg);
    } else {
        emit_bytes(compiler, get_op, (uint8_t)arg);
    }
}

/* ========== الترجمة الرئيسية ========== */

void compile_node(skp_compiler_t* compiler, ast_node_t* node) {
    if (!node) return;
    
    switch (node->type) {
        /* التعبيرات */
        case AST_NUMBER:
        case AST_STRING:
        case AST_BOOLEAN:
        case AST_NULL:
        case AST_IDENTIFIER:
        case AST_BINARY_OP:
        case AST_UNARY_OP:
        case AST_ASSIGNMENT:
        case AST_CALL:
        case AST_MEMBER_ACCESS:
        case AST_INDEX_ACCESS:
        case AST_LIST_LITERAL:
        case AST_DICT_LITERAL:
        case AST_LAMBDA:
        case AST_TERNARY:
            compile_expression(compiler, node);
            break;
            
        /* التصريحات */
        case AST_VAR_DECL:
        case AST_CONST_DECL:
        case AST_FUNC_DECL:
        case AST_CLASS_DECL:
        case AST_IMPORT:
        case AST_EXPORT:
            compile_declaration(compiler, node);
            break;
            
        /* عبارات التحكم */
        case AST_IF:
        case AST_WHILE:
        case AST_FOR:
        case AST_FOREACH:
        case AST_RETURN:
        case AST_BREAK:
        case AST_CONTINUE:
        case AST_BLOCK:
        case AST_EXPRESSION_STMT:
            compile_statement(compiler, node);
            break;
            
        case AST_PROGRAM:
            compile_program(compiler, node);
            break;
            
        default:
            fprintf(stderr, "خطأ: نوع عقدة غير معروف\n");
            compiler->had_error = 1;
            break;
    }
}

/* ========== ترجمة التعبيرات ========== */

void compile_expression(skp_compiler_t* compiler, ast_node_t* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_NUMBER: {
            constant_t constant;
            /* التحقق إذا كان عدد صحيح أو عشري */
            if (strchr(node->data.number.value, '.') || 
                strchr(node->data.number.value, 'e') ||
                strchr(node->data.number.value, 'E')) {
                constant.type = SKP_TYPE_FLOAT;
                constant.value.float_val = strtod(node->data.number.value, NULL);
                emit_constant(compiler, constant);
            } else {
                constant.type = SKP_TYPE_INT;
                constant.value.int_val = strtoll(node->data.number.value, NULL, 10);
                emit_constant(compiler, constant);
            }
            break;
        }
            
        case AST_STRING: {
            constant_t constant;
            constant.type = SKP_TYPE_STRING;
            constant.value.string_val = strdup(node->data.string.value);
            emit_constant(compiler, constant);
            break;
        }
            
        case AST_BOOLEAN:
            emit_opcode(compiler, node->data.boolean.value ? OP_CONST_TRUE : OP_CONST_FALSE);
            break;
            
        case AST_NULL:
            emit_opcode(compiler, OP_CONST_NULL);
            break;
            
        case AST_IDENTIFIER:
            named_variable(compiler, node->data.identifier.name, 1);
            break;
            
        case AST_BINARY_OP:
            compile_binary(compiler, node);
            break;
            
        case AST_UNARY_OP:
            compile_unary(compiler, node);
            break;
            
        case AST_ASSIGNMENT:
            compile_assignment(compiler, node);
            break;
            
        case AST_CALL:
            compile_call(compiler, node);
            break;
            
        case AST_MEMBER_ACCESS:
            compile_member_access(compiler, node);
            break;
            
        case AST_INDEX_ACCESS:
            compile_index_access(compiler, node);
            break;
            
        case AST_LIST_LITERAL:
            compile_list_literal(compiler, node);
            break;
            
        case AST_DICT_LITERAL:
            compile_dict_literal(compiler, node);
            break;
            
        case AST_LAMBDA:
            compile_lambda(compiler, node);
            break;
            
        case AST_TERNARY:
            compile_ternary(compiler, node);
            break;
            
        default:
            fprintf(stderr, "خطأ: تعبير غير معروف\n");
            compiler->had_error = 1;
            break;
    }
}

void compile_binary(skp_compiler_t* compiler, ast_node_t* node) {
    compile_expression(compiler, node->data.binary_op.left);
    compile_expression(compiler, node->data.binary_op.right);
    
    switch (node->data.binary_op.op) {
        case BINOP_ADD: emit_opcode(compiler, OP_ADD); break;
        case BINOP_SUB: emit_opcode(compiler, OP_SUB); break;
        case BINOP_MUL: emit_opcode(compiler, OP_MUL); break;
        case BINOP_DIV: emit_opcode(compiler, OP_DIV); break;
        case BINOP_MOD: emit_opcode(compiler, OP_MOD); break;
        case BINOP_POW: emit_opcode(compiler, OP_POW); break;
        case BINOP_EQ: emit_opcode(compiler, OP_EQ); break;
        case BINOP_NE: emit_opcode(compiler, OP_NE); break;
        case BINOP_LT: emit_opcode(compiler, OP_LT); break;
        case BINOP_GT: emit_opcode(compiler, OP_GT); break;
        case BINOP_LE: emit_opcode(compiler, OP_LE); break;
        case BINOP_GE: emit_opcode(compiler, OP_GE); break;
        case BINOP_AND: emit_opcode(compiler, OP_AND); break;
        case BINOP_OR: emit_opcode(compiler, OP_OR); break;
        case BINOP_BIT_AND: emit_opcode(compiler, OP_BIT_AND); break;
        case BINOP_BIT_OR: emit_opcode(compiler, OP_BIT_OR); break;
        case BINOP_BIT_XOR: emit_opcode(compiler, OP_BIT_XOR); break;
        case BINOP_SHL: emit_opcode(compiler, OP_SHL); break;
        case BINOP_SHR: emit_opcode(compiler, OP_SHR); break;
        default:
            fprintf(stderr, "خطأ: عملية ثنائية غير معروفة\n");
            compiler->had_error = 1;
            break;
    }
}

void compile_unary(skp_compiler_t* compiler, ast_node_t* node) {
    compile_expression(compiler, node->data.unary_op.operand);
    
    switch (node->data.unary_op.op) {
        case UNOP_NEG: emit_opcode(compiler, OP_NEG); break;
        case UNOP_NOT: emit_opcode(compiler, OP_NOT); break;
        case UNOP_BIT_NOT: emit_opcode(compiler, OP_BIT_NOT); break;
        case UNOP_INC:
            /* ++x => x = x + 1 */
            emit_byte(compiler, 1);
            emit_opcode(compiler, OP_ADD);
            break;
        case UNOP_DEC:
            /* --x => x = x - 1 */
            emit_byte(compiler, 1);
            emit_opcode(compiler, OP_SUB);
            break;
        default:
            fprintf(stderr, "خطأ: عملية أحادية غير معروفة\n");
            compiler->had_error = 1;
            break;
    }
}

void compile_assignment(skp_compiler_t* compiler, ast_node_t* node) {
    /* تجميع القيمة أولاً */
    compile_expression(compiler, node->data.assignment.value);
    
    /* ثم تعيينها للهدف */
    ast_node_t* target = node->data.assignment.target;
    
    if (target->type == AST_IDENTIFIER) {
        named_variable(compiler, target->data.identifier.name, 1);
    } else if (target->type == AST_MEMBER_ACCESS) {
        compile_expression(compiler, target->data.member_access.object);
        uint8_t name = identifier_constant(compiler, target->data.member_access.member);
        emit_bytes(compiler, OP_SET_FIELD, name);
    } else if (target->type == AST_INDEX_ACCESS) {
        compile_expression(compiler, target->data.index_access.object);
        compile_expression(compiler, target->data.index_access.index);
        emit_opcode(compiler, OP_SET_INDEX);
    } else {
        fprintf(stderr, "خطأ: هدف تعيين غير صالح\n");
        compiler->had_error = 1;
    }
}

void compile_call(skp_compiler_t* compiler, ast_node_t* node) {
    /* تجميع المعاملات أولاً */
    for (size_t i = 0; i < node->data.call.arg_count; i++) {
        compile_expression(compiler, node->data.call.args[i]);
    }
    
    /* ثم تجميع الدالة */
    compile_expression(compiler, node->data.call.callee);
    
    /* استدعاء الدالة */
    emit_bytes(compiler, OP_CALL, (uint8_t)node->data.call.arg_count);
}

void compile_member_access(skp_compiler_t* compiler, ast_node_t* node) {
    compile_expression(compiler, node->data.member_access.object);
    uint8_t name = identifier_constant(compiler, node->data.member_access.member);
    emit_bytes(compiler, OP_GET_FIELD, name);
}

void compile_index_access(skp_compiler_t* compiler, ast_node_t* node) {
    compile_expression(compiler, node->data.index_access.object);
    compile_expression(compiler, node->data.index_access.index);
    emit_opcode(compiler, OP_GET_INDEX);
}

void compile_list_literal(skp_compiler_t* compiler, ast_node_t* node) {
    /* تجميع العناصر */
    for (size_t i = 0; i < node->data.list_literal.element_count; i++) {
        compile_expression(compiler, node->data.list_literal.elements[i]);
    }
    
    /* إنشاء القائمة */
    emit_bytes(compiler, OP_CONST_LIST, (uint8_t)node->data.list_literal.element_count);
}

void compile_dict_literal(skp_compiler_t* compiler, ast_node_t* node) {
    /* تجميع الأزواج مفتاح-قيمة */
    for (size_t i = 0; i < node->data.dict_literal.entry_count; i++) {
        compile_expression(compiler, node->data.dict_literal.keys[i]);
        compile_expression(compiler, node->data.dict_literal.values[i]);
    }
    
    /* إنشاء القاموس */
    emit_bytes(compiler, OP_CONST_DICT, (uint8_t)node->data.dict_literal.entry_count);
}

void compile_lambda(skp_compiler_t* compiler, ast_node_t* node) {
    /* إنشاء مترجم فرعي للدالة */
    compiler_t* enclosing = compiler->current;
    compiler->current = create_compiler(enclosing, TYPE_FUNCTION, "<lambda>");
    compiler->current->function_arity = (int)node->data.lambda.param_count;
    
    /* إضافة المعاملات كمتغيرات محلية */
    for (size_t i = 0; i < node->data.lambda.param_count; i++) {
        declare_variable(compiler, node->data.lambda.params[i]);
        define_variable(compiler, 0);
    }
    
    /* تجميع جسم الدالة */
    compile_node(compiler, node->data.lambda.body);
    
    /* إرجاع تلقائي */
    emit_opcode(compiler, OP_RETURN_VOID);
    
    /* استعادة المترجم السابق */
    chunk_t* function_chunk = (chunk_t*)malloc(sizeof(chunk_t));
    *function_chunk = compiler->current->chunk;
    compiler->current->chunk.code = NULL;
    compiler->current->chunk.lines = NULL;
    compiler->current->chunk.constants = NULL;
    
    int upvalue_count = compiler->current->upvalue_count;
    upvalue_t upvalues[256];
    memcpy(upvalues, compiler->current->upvalues, sizeof(upvalue_t) * upvalue_count);
    
    destroy_compiler(compiler->current);
    compiler->current = enclosing;
    
    /* إنشاء closure */
    emit_opcode(compiler, OP_CLOSURE);
    emit_byte(compiler, (uint8_t)upvalue_count);
    
    for (int i = 0; i < upvalue_count; i++) {
        emit_byte(compiler, upvalues[i].is_local ? 1 : 0);
        emit_byte(compiler, upvalues[i].index);
    }
}

void compile_ternary(skp_compiler_t* compiler, ast_node_t* node) {
    compile_expression(compiler, node->data.ternary.condition);
    
    int else_jump = compiler->current->chunk.count;
    emit_jump(compiler, OP_JUMP_IF_FALSE, 0);
    
    compile_expression(compiler, node->data.ternary.true_expr);
    
    int end_jump = compiler->current->chunk.count;
    emit_jump(compiler, OP_JUMP, 0);
    
    patch_jump(compiler, else_jump);
    
    compile_expression(compiler, node->data.ternary.false_expr);
    
    patch_jump(compiler, end_jump);
}

/* ========== ترجمة التصريحات ========== */

void compile_declaration(skp_compiler_t* compiler, ast_node_t* node) {
    switch (node->type) {
        case AST_VAR_DECL:
        case AST_CONST_DECL:
            compile_var_decl(compiler, node);
            break;
        case AST_FUNC_DECL:
            compile_func_decl(compiler, node);
            break;
        case AST_CLASS_DECL:
            compile_class_decl(compiler, node);
            break;
        case AST_IMPORT:
            compile_import(compiler, node);
            break;
        case AST_EXPORT:
            compile_export(compiler, node);
            break;
        default:
            compile_statement(compiler, node);
            break;
    }
}

void compile_var_decl(skp_compiler_t* compiler, ast_node_t* node) {
    /* الإعلان عن المتغير */
    declare_variable(compiler, node->data.var_decl.name);
    
    /* تجميع القيمة الأولية إذا وجدت */
    if (node->data.var_decl.initializer) {
        compile_expression(compiler, node->data.var_decl.initializer);
    } else {
        emit_opcode(compiler, OP_CONST_NULL);
    }
    
    /* تعريف المتغير */
    define_variable(compiler, identifier_constant(compiler, node->data.var_decl.name));
}

void compile_func_decl(skp_compiler_t* compiler, ast_node_t* node) {
    /* إنشاء مترجم فرعي */
    compiler_t* enclosing = compiler->current;
    compiler->current = create_compiler(enclosing, TYPE_FUNCTION, node->data.func_decl.name);
    compiler->current->function_arity = (int)node->data.func_decl.param_count;
    
    begin_scope(compiler);
    
    /* إضافة المعاملات */
    for (size_t i = 0; i < node->data.func_decl.param_count; i++) {
        declare_variable(compiler, node->data.func_decl.params[i]);
        define_variable(compiler, 0);
    }
    
    /* تجميع الجسم */
    compile_node(compiler, node->data.func_decl.body);
    
    /* إرجاع تلقائي */
    emit_opcode(compiler, OP_RETURN_VOID);
    
    end_scope(compiler);
    
    /* استعادة المترجم السابق */
    int upvalue_count = compiler->current->upvalue_count;
    upvalue_t upvalues[256];
    memcpy(upvalues, compiler->current->upvalues, sizeof(upvalue_t) * upvalue_count);
    
    destroy_compiler(compiler->current);
    compiler->current = enclosing;
    
    /* تعريف الدالة */
    uint8_t global = identifier_constant(compiler, node->data.func_decl.name);
    
    emit_opcode(compiler, OP_CLOSURE);
    emit_byte(compiler, (uint8_t)upvalue_count);
    
    for (int i = 0; i < upvalue_count; i++) {
        emit_byte(compiler, upvalues[i].is_local ? 1 : 0);
        emit_byte(compiler, upvalues[i].index);
    }
    
    define_variable(compiler, global);
}

void compile_class_decl(skp_compiler_t* compiler, ast_node_t* node) {
    uint8_t name_constant = identifier_constant(compiler, node->data.class_decl.name);
    declare_variable(compiler, node->data.class_decl.name);
    
    emit_bytes(compiler, OP_CLASS, name_constant);
    define_variable(compiler, name_constant);
    
    named_variable(compiler, node->data.class_decl.name, 0);
    
    /* وراثة */
    if (node->data.class_decl.parent) {
        named_variable(compiler, node->data.class_decl.parent, 0);
        emit_opcode(compiler, OP_INHERIT);
    }
    
    /* تجميع الأعضاء */
    for (size_t i = 0; i < node->data.class_decl.member_count; i++) {
        if (node->data.class_decl.members[i]->type == AST_FUNC_DECL) {
            ast_node_t* method = node->data.class_decl.members[i];
            uint8_t method_name = identifier_constant(compiler, method->data.func_decl.name);
            
            /* تجميع الدالة */
            compiler_t* enclosing = compiler->current;
            compiler->current = create_compiler(enclosing, TYPE_METHOD, method->data.func_decl.name);
            compiler->current->function_arity = (int)method->data.func_decl.param_count;
            
            begin_scope(compiler);
            
            /* إضافة 'هذا' ومعاملات */
            for (size_t j = 0; j < method->data.func_decl.param_count; j++) {
                declare_variable(compiler, method->data.func_decl.params[j]);
                define_variable(compiler, 0);
            }
            
            compile_node(compiler, method->data.func_decl.body);
            emit_opcode(compiler, OP_RETURN_VOID);
            
            end_scope(compiler);
            
            int upvalue_count = compiler->current->upvalue_count;
            destroy_compiler(compiler->current);
            compiler->current = enclosing;
            
            emit_bytes(compiler, OP_METHOD, method_name);
        }
    }
    
    emit_opcode(compiler, OP_POP);
}

/* ========== ترجمة العبارات ========== */

void compile_statement(skp_compiler_t* compiler, ast_node_t* node) {
    switch (node->type) {
        case AST_IF:
            compile_if(compiler, node);
            break;
        case AST_WHILE:
            compile_while(compiler, node);
            break;
        case AST_FOR:
            compile_for(compiler, node);
            break;
        case AST_FOREACH:
            compile_foreach(compiler, node);
            break;
        case AST_RETURN:
            compile_return(compiler, node);
            break;
        case AST_BREAK:
            /* TODO: تنفيذ التوقف */
            break;
        case AST_CONTINUE:
            /* TODO: تنفيذ الاستمرار */
            break;
        case AST_BLOCK:
            compile_block(compiler, node);
            break;
        case AST_EXPRESSION_STMT:
            compile_expression(compiler, node->data.expression_stmt.expression);
            emit_opcode(compiler, OP_POP);
            break;
        default:
            compile_expression(compiler, node);
            emit_opcode(compiler, OP_POP);
            break;
    }
}

void compile_if(skp_compiler_t* compiler, ast_node_t* node) {
    compile_expression(compiler, node->data.if_stmt.condition);
    
    int then_jump = compiler->current->chunk.count;
    emit_jump(compiler, OP_JUMP_IF_FALSE, 0);
    
    compile_node(compiler, node->data.if_stmt.then_branch);
    
    int else_jump = compiler->current->chunk.count;
    emit_jump(compiler, OP_JUMP, 0);
    
    patch_jump(compiler, then_jump);
    
    if (node->data.if_stmt.else_branch) {
        compile_node(compiler, node->data.if_stmt.else_branch);
    }
    
    patch_jump(compiler, else_jump);
}

void compile_while(skp_compiler_t* compiler, ast_node_t* node) {
    int loop_start = compiler->current->chunk.count;
    
    compile_expression(compiler, node->data.while_stmt.condition);
    
    int exit_jump = compiler->current->chunk.count;
    emit_jump(compiler, OP_JUMP_IF_FALSE, 0);
    
    compile_node(compiler, node->data.while_stmt.body);
    
    emit_loop(compiler, loop_start);
    
    patch_jump(compiler, exit_jump);
}

void compile_for(skp_compiler_t* compiler, ast_node_t* node) {
    begin_scope(compiler);
    
    /* التهيئة */
    if (node->data.for_stmt.init_var) {
        declare_variable(compiler, node->data.for_stmt.init_var);
        if (node->data.for_stmt.init_expr) {
            compile_expression(compiler, node->data.for_stmt.init_expr);
        } else {
            emit_opcode(compiler, OP_CONST_NULL);
        }
        define_variable(compiler, 0);
    } else if (node->data.for_stmt.init_expr) {
        compile_expression(compiler, node->data.for_stmt.init_expr);
        emit_opcode(compiler, OP_POP);
    }
    
    int loop_start = compiler->current->chunk.count;
    int exit_jump = -1;
    
    /* الشرط */
    if (node->data.for_stmt.condition) {
        compile_expression(compiler, node->data.for_stmt.condition);
        exit_jump = compiler->current->chunk.count;
        emit_jump(compiler, OP_JUMP_IF_FALSE, 0);
    }
    
    /* الجسم */
    compile_node(compiler, node->data.for_stmt.body);
    
    /* الزيادة */
    if (node->data.for_stmt.increment) {
        compile_expression(compiler, node->data.for_stmt.increment);
        emit_opcode(compiler, OP_POP);
    }
    
    emit_loop(compiler, loop_start);
    
    if (exit_jump != -1) {
        patch_jump(compiler, exit_jump);
    }
    
    end_scope(compiler);
}

void compile_foreach(skp_compiler_t* compiler, ast_node_t* node) {
    begin_scope(compiler);
    
    /* المتغير المؤقت للمجموعة */
    const char* iter_name = "@iter";
    declare_variable(compiler, iter_name);
    compile_expression(compiler, node->data.foreach_stmt.iterable);
    define_variable(compiler, 0);
    
    /* متغير الحلقة */
    declare_variable(compiler, node->data.foreach_stmt.var);
    
    int loop_start = compiler->current->chunk.count;
    
    /* الحصول على العنصر التالي */
    named_variable(compiler, iter_name, 0);
    emit_opcode(compiler, OP_CONST_NULL); /* TODO: استدعاء التالي */
    
    /* التحقق من النهاية */
    emit_opcode(compiler, OP_CONST_NULL);
    emit_opcode(compiler, OP_EQ);
    
    int exit_jump = compiler->current->chunk.count;
    emit_jump(compiler, OP_JUMP_IF_TRUE, 0);
    
    /* تعيين العنصر للمتغير */
    named_variable(compiler, node->data.foreach_stmt.var, 1);
    
    /* الجسم */
    compile_node(compiler, node->data.foreach_stmt.body);
    
    emit_loop(compiler, loop_start);
    
    patch_jump(compiler, exit_jump);
    
    end_scope(compiler);
}

void compile_return(skp_compiler_t* compiler, ast_node_t* node) {
    if (node->data.return_stmt.value) {
        compile_expression(compiler, node->data.return_stmt.value);
        emit_opcode(compiler, OP_RETURN);
    } else {
        emit_opcode(compiler, OP_RETURN_VOID);
    }
}

void compile_block(skp_compiler_t* compiler, ast_node_t* node) {
    begin_scope(compiler);
    
    for (size_t i = 0; i < node->data.block.statement_count; i++) {
        compile_node(compiler, node->data.block.statements[i]);
    }
    
    end_scope(compiler);
}

void compile_import(skp_compiler_t* compiler, ast_node_t* node) {
    uint8_t module = identifier_constant(compiler, node->data.import.module);
    emit_bytes(compiler, OP_IMPORT, module);
}

void compile_export(skp_compiler_t* compiler, ast_node_t* node) {
    for (size_t i = 0; i < node->data.export_stmt.name_count; i++) {
        uint8_t name = identifier_constant(compiler, node->data.export_stmt.names[i]);
        emit_bytes(compiler, OP_EXPORT, name);
    }
}

/* ========== ترجمة البرنامج ========== */

void compile_program(skp_compiler_t* compiler, ast_node_t* node) {
    for (size_t i = 0; i < node->data.program.statement_count; i++) {
        compile_node(compiler, node->data.program.statements[i]);
    }
}

/* ========== دوال مساعدة ========== */

int get_line(ast_node_t* node) {
    return node ? node->line : 0;
}

const char* opcode_name(opcode_t op) {
    switch (op) {
        case OP_CONST_INT: return "CONST_INT";
        case OP_CONST_FLOAT: return "CONST_FLOAT";
        case OP_CONST_STRING: return "CONST_STRING";
        case OP_CONST_TRUE: return "CONST_TRUE";
        case OP_CONST_FALSE: return "CONST_FALSE";
        case OP_CONST_NULL: return "CONST_NULL";
        case OP_CONST_LIST: return "CONST_LIST";
        case OP_CONST_DICT: return "CONST_DICT";
        case OP_GET_LOCAL: return "GET_LOCAL";
        case OP_SET_LOCAL: return "SET_LOCAL";
        case OP_GET_GLOBAL: return "GET_GLOBAL";
        case OP_SET_GLOBAL: return "SET_GLOBAL";
        case OP_GET_UPVALUE: return "GET_UPVALUE";
        case OP_SET_UPVALUE: return "SET_UPVALUE";
        case OP_GET_FIELD: return "GET_FIELD";
        case OP_SET_FIELD: return "SET_FIELD";
        case OP_GET_INDEX: return "GET_INDEX";
        case OP_SET_INDEX: return "SET_INDEX";
        case OP_DEFINE_GLOBAL: return "DEFINE_GLOBAL";
        case OP_ADD: return "ADD";
        case OP_SUB: return "SUB";
        case OP_MUL: return "MUL";
        case OP_DIV: return "DIV";
        case OP_MOD: return "MOD";
        case OP_POW: return "POW";
        case OP_NEG: return "NEG";
        case OP_NOT: return "NOT";
        case OP_AND: return "AND";
        case OP_OR: return "OR";
        case OP_EQ: return "EQ";
        case OP_NE: return "NE";
        case OP_LT: return "LT";
        case OP_GT: return "GT";
        case OP_LE: return "LE";
        case OP_GE: return "GE";
        case OP_BIT_AND: return "BIT_AND";
        case OP_BIT_OR: return "BIT_OR";
        case OP_BIT_XOR: return "BIT_XOR";
        case OP_BIT_NOT: return "BIT_NOT";
        case OP_SHL: return "SHL";
        case OP_SHR: return "SHR";
        case OP_JUMP: return "JUMP";
        case OP_JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case OP_JUMP_IF_TRUE: return "JUMP_IF_TRUE";
        case OP_LOOP: return "LOOP";
        case OP_CALL: return "CALL";
        case OP_RETURN: return "RETURN";
        case OP_RETURN_VOID: return "RETURN_VOID";
        case OP_CLOSURE: return "CLOSURE";
        case OP_CLOSE_UPVALUE: return "CLOSE_UPVALUE";
        case OP_CLASS: return "CLASS";
        case OP_METHOD: return "METHOD";
        case OP_INHERIT: return "INHERIT";
        case OP_GET_SUPER: return "GET_SUPER";
        case OP_SUPER_INVOKE: return "SUPER_INVOKE";
        case OP_THROW: return "THROW";
        case OP_TRY_START: return "TRY_START";
        case OP_TRY_END: return "TRY_END";
        case OP_CATCH: return "CATCH";
        case OP_FINALLY: return "FINALLY";
        case OP_POP: return "POP";
        case OP_DUP: return "DUP";
        case OP_SWAP: return "SWAP";
        case OP_PRINT: return "PRINT";
        case OP_IMPORT: return "IMPORT";
        case OP_EXPORT: return "EXPORT";
        case OP_HALT: return "HALT";
        default: return "UNKNOWN";
    }
}

void chunk_disassemble(chunk_t* chunk, const char* name) {
    printf("== %s ==\n", name);
    
    for (size_t offset = 0; offset < chunk->count;) {
        offset = disassemble_instruction(chunk, offset);
    }
}

static int constant_instruction(const char* name, chunk_t* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    
    constant_t c = chunk->constants[constant];
    switch (c.type) {
        case SKP_TYPE_INT:
            printf("%lld", c.value.int_val);
            break;
        case SKP_TYPE_FLOAT:
            printf("%f", c.value.float_val);
            break;
        case SKP_TYPE_STRING:
            printf("%s", c.value.string_val);
            break;
        default:
            printf("?");
    }
    printf("'\n");
    
    return offset + 2;
}

static int byte_instruction(const char* name, chunk_t* chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

static int jump_instruction(const char* name, int sign, chunk_t* chunk, int offset) {
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

int disassemble_instruction(chunk_t* chunk, int offset) {
    printf("%04d ", offset);
    
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }
    
    uint8_t instruction = chunk->code[offset];
    
    switch (instruction) {
        case OP_CONST_INT:
        case OP_CONST_FLOAT:
        case OP_CONST_STRING:
            return constant_instruction(opcode_name((opcode_t)instruction), chunk, offset);
            
        case OP_GET_LOCAL:
        case OP_SET_LOCAL:
        case OP_GET_GLOBAL:
        case OP_SET_GLOBAL:
        case OP_GET_UPVALUE:
        case OP_SET_UPVALUE:
        case OP_DEFINE_GLOBAL:
        case OP_GET_FIELD:
        case OP_SET_FIELD:
        case OP_CALL:
        case OP_CONST_LIST:
        case OP_CONST_DICT:
        case OP_CLASS:
        case OP_METHOD:
            return byte_instruction(opcode_name((opcode_t)instruction), chunk, offset);
            
        case OP_JUMP:
        case OP_JUMP_IF_FALSE:
        case OP_JUMP_IF_TRUE:
            return jump_instruction(opcode_name((opcode_t)instruction), 1, chunk, offset);
            
        case OP_LOOP:
            return jump_instruction(opcode_name((opcode_t)instruction), -1, chunk, offset);
            
        case OP_CONST_TRUE:
        case OP_CONST_FALSE:
        case OP_CONST_NULL:
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_MOD:
        case OP_POW:
        case OP_NEG:
        case OP_NOT:
        case OP_AND:
        case OP_OR:
        case OP_EQ:
        case OP_NE:
        case OP_LT:
        case OP_GT:
        case OP_LE:
        case OP_GE:
        case OP_BIT_AND:
        case OP_BIT_OR:
        case OP_BIT_XOR:
        case OP_BIT_NOT:
        case OP_SHL:
        case OP_SHR:
        case OP_GET_INDEX:
        case OP_SET_INDEX:
        case OP_RETURN:
        case OP_RETURN_VOID:
        case OP_CLOSE_UPVALUE:
        case OP_INHERIT:
        case OP_GET_SUPER:
        case OP_SUPER_INVOKE:
        case OP_THROW:
        case OP_TRY_START:
        case OP_TRY_END:
        case OP_CATCH:
        case OP_FINALLY:
        case OP_POP:
        case OP_DUP:
        case OP_SWAP:
        case OP_PRINT:
        case OP_IMPORT:
        case OP_EXPORT:
        case OP_HALT:
            printf("%s\n", opcode_name((opcode_t)instruction));
            return offset + 1;
            
        case OP_CLOSURE: {
            offset++;
            uint8_t upvalue_count = chunk->code[offset++];
            printf("%-16s %4d\n", "CLOSURE", upvalue_count);
            for (int i = 0; i < upvalue_count; i++) {
                int is_local = chunk->code[offset++];
                int index = chunk->code[offset++];
                printf("%04d      |                     %s %d\n",
                       offset - 2, is_local ? "local" : "upvalue", index);
            }
            return offset;
        }
            
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
