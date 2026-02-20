/*
 * SEEKEP - Smart Expressive Efficient Knowledge Execution Platform
 * المترجم (Compiler) - Compiler
 * 
 * يقوم بتحويل شجرة البنية المجردة إلى بايتكود
 */

#ifndef COMPILER_H
#define COMPILER_H

#include "parser.h"
#include "seekep.h"

/* أكواد العمليات (Opcodes) */
typedef enum {
    /* ثوابت */
    OP_CONST_INT,       /* تحميل عدد صحيح */
    OP_CONST_FLOAT,     /* تحميل عدد عشري */
    OP_CONST_STRING,    /* تحميل نص */
    OP_CONST_TRUE,      /* تحميل صحيح */
    OP_CONST_FALSE,     /* تحميل خطأ */
    OP_CONST_NULL,      /* تحميل فارغ */
    OP_CONST_LIST,      /* تحميل قائمة */
    OP_CONST_DICT,      /* تحميل قاموس */
    
    /* المتغيرات */
    OP_GET_LOCAL,       /* قراءة متغير محلي */
    OP_SET_LOCAL,       /* كتابة متغير محلي */
    OP_GET_GLOBAL,      /* قراءة متغير عام */
    OP_SET_GLOBAL,      /* كتابة متغير عام */
    OP_GET_UPVALUE,     /* قراءة upvalue */
    OP_SET_UPVALUE,     /* كتابة upvalue */
    OP_GET_FIELD,       /* قراءة حقل */
    OP_SET_FIELD,       /* كتابة حقل */
    OP_GET_INDEX,       /* قراءة بالفهرس */
    OP_SET_INDEX,       /* كتابة بالفهرس */
    OP_DEFINE_GLOBAL,   /* تعريف متغير عام */
    
    /* العمليات الحسابية */
    OP_ADD,             /* جمع */
    OP_SUB,             /* طرح */
    OP_MUL,             /* ضرب */
    OP_DIV,             /* قسمة */
    OP_MOD,             /* باقي القسمة */
    OP_POW,             /* أس */
    OP_NEG,             /* سالب */
    
    /* العمليات المنطقية */
    OP_NOT,             /* نفي */
    OP_AND,             /* و */
    OP_OR,              /* أو */
    
    /* العمليات المقارنة */
    OP_EQ,              /* يساوي */
    OP_NE,              /* لا يساوي */
    OP_LT,              /* أصغر من */
    OP_GT,              /* أكبر من */
    OP_LE,              /* أصغر أو يساوي */
    OP_GE,              /* أكبر أو يساوي */
    
    /* العمليات على البتات */
    OP_BIT_AND,         /* و بت */
    OP_BIT_OR,          /* أو بت */
    OP_BIT_XOR,         /* XOR بت */
    OP_BIT_NOT,         /* نفي بت */
    OP_SHL,             /* إزاحة يسار */
    OP_SHR,             /* إزاحة يمين */
    
    /* التحكم في التدفق */
    OP_JUMP,            /* قفز */
    OP_JUMP_IF_FALSE,   /* قفز إذا خطأ */
    OP_JUMP_IF_TRUE,    /* قفز إذا صحيح */
    OP_LOOP,            /* تكرار حلقة */
    OP_CALL,            /* استدعاء دالة */
    OP_RETURN,          /* إرجاع */
    OP_RETURN_VOID,     /* إرجاع بدون قيمة */
    
    /* الدوال والأصناف */
    OP_CLOSURE,         /* إنشاء closure */
    OP_CLOSE_UPVALUE,   /* إغلاق upvalue */
    OP_CLASS,           /* إنشاء صنف */
    OP_METHOD,          /* تعريف طريقة */
    OP_INHERIT,         /* وراثة */
    OP_GET_SUPER,       /* قراءة من الأب */
    OP_SUPER_INVOKE,    /* استدعاء من الأب */
    
    /* التعامل مع الأخطاء */
    OP_THROW,           /* رمي استثناء */
    OP_TRY_START,       /* بداية try */
    OP_TRY_END,         /* نهاية try */
    OP_CATCH,           /* catch */
    OP_FINALLY,         /* finally */
    
    /* أخرى */
    OP_POP,             /* إزالة من المكدس */
    OP_DUP,             /* تكرار قمة المكدس */
    OP_SWAP,            /* تبديل قمتي المكدس */
    OP_PRINT,           /* طباعة */
    OP_IMPORT,          /* استيراد */
    OP_EXPORT,          /* تصدير */
    OP_HALT             /* إيقاف */
} opcode_t;

/* ثابت في تجمع الثوابت */
typedef struct {
    skp_type_t type;
    union {
        skp_int int_val;
        skp_float float_val;
        char* string_val;
        size_t count;
    } value;
} constant_t;

/* كتلة بايتكود */
typedef struct {
    uint8_t* code;          /* البايتكود */
    size_t count;           /* عدد البايتات */
    size_t capacity;        /* السعة */
    
    int* lines;             /* أرقام الأسطر لكل بايت */
    
    constant_t* constants;  /* تجمع الثوابت */
    size_t constant_count;
    size_t constant_capacity;
} chunk_t;

/* متغير محلي */
typedef struct {
    char* name;
    int depth;
    int is_captured;
} local_t;

/* upvalue */
typedef struct {
    uint8_t index;
    int is_local;
} upvalue_t;

/* نوع الدالة المُجمَّعة */
typedef enum {
    TYPE_FUNCTION,
    TYPE_INITIALIZER,
    TYPE_METHOD,
    TYPE_SCRIPT
} function_type_t;

/* سياق المترجم للدالة الحالية */
typedef struct compiler {
    struct compiler* enclosing;  /* السياق الخارجي */
    
    function_type_t type;
    char* name;
    
    local_t locals[256];         /* المتغيرات المحلية */
    int local_count;
    int scope_depth;             /* عمق النطاق الحالي */
    
    upvalue_t upvalues[256];     /* upvalues */
    int upvalue_count;
    
    chunk_t chunk;               /* كتلة البايتكود */
    int function_arity;          /* عدد المعاملات */
} compiler_t;

/* المترجم */
typedef struct {
    compiler_t* current;         /* المترجم الحالي */
    parser_t* parser;            /* المحلل اللغوي */
    ast_node_t* ast;             /* شجرة البنية المجردة */
    int had_error;               /* هل حدث خطأ؟ */
} skp_compiler_t;

/* إنشاء وإتلاف */
skp_compiler_t* compiler_create(parser_t* parser);
void compiler_destroy(skp_compiler_t* compiler);
chunk_t* compiler_compile(skp_compiler_t* compiler, ast_node_t* ast);

/* إدارة الكتلة */
void chunk_init(chunk_t* chunk);
void chunk_free(chunk_t* chunk);
void chunk_write(chunk_t* chunk, uint8_t byte, int line);
int chunk_add_constant(chunk_t* chunk, constant_t constant);

/* كتابة أكواد العمليات */
void emit_byte(skp_compiler_t* compiler, uint8_t byte);
void emit_bytes(skp_compiler_t* compiler, uint8_t byte1, uint8_t byte2);
void emit_opcode(skp_compiler_t* compiler, opcode_t op);
void emit_constant(skp_compiler_t* compiler, constant_t constant);
void emit_jump(skp_compiler_t* compiler, opcode_t op, int offset);
void patch_jump(skp_compiler_t* compiler, int offset);
void emit_loop(skp_compiler_t* compiler, int loop_start);

/* التحكم في النطاق */
void begin_scope(skp_compiler_t* compiler);
void end_scope(skp_compiler_t* compiler);

/* المتغيرات */
int resolve_local(skp_compiler_t* compiler, const char* name);
int resolve_upvalue(skp_compiler_t* compiler, const char* name);
int add_upvalue(skp_compiler_t* compiler, uint8_t index, int is_local);
void add_local(skp_compiler_t* compiler, const char* name);
void declare_variable(skp_compiler_t* compiler, const char* name);
uint8_t identifier_constant(skp_compiler_t* compiler, const char* name);
void define_variable(skp_compiler_t* compiler, uint8_t global);
void named_variable(skp_compiler_t* compiler, const char* name, int can_assign);

/* الترجمة */
void compile_node(skp_compiler_t* compiler, ast_node_t* node);
void compile_expression(skp_compiler_t* compiler, ast_node_t* node);
void compile_statement(skp_compiler_t* compiler, ast_node_t* node);
void compile_declaration(skp_compiler_t* compiler, ast_node_t* node);

/* ترجمة التعبيرات */
void compile_binary(skp_compiler_t* compiler, ast_node_t* node);
void compile_unary(skp_compiler_t* compiler, ast_node_t* node);
void compile_assignment(skp_compiler_t* compiler, ast_node_t* node);
void compile_call(skp_compiler_t* compiler, ast_node_t* node);
void compile_member_access(skp_compiler_t* compiler, ast_node_t* node);
void compile_index_access(skp_compiler_t* compiler, ast_node_t* node);
void compile_list_literal(skp_compiler_t* compiler, ast_node_t* node);
void compile_dict_literal(skp_compiler_t* compiler, ast_node_t* node);
void compile_lambda(skp_compiler_t* compiler, ast_node_t* node);
void compile_ternary(skp_compiler_t* compiler, ast_node_t* node);

/* ترجمة التصريحات */
void compile_var_decl(skp_compiler_t* compiler, ast_node_t* node);
void compile_func_decl(skp_compiler_t* compiler, ast_node_t* node);
void compile_class_decl(skp_compiler_t* compiler, ast_node_t* node);
void compile_if(skp_compiler_t* compiler, ast_node_t* node);
void compile_while(skp_compiler_t* compiler, ast_node_t* node);
void compile_for(skp_compiler_t* compiler, ast_node_t* node);
void compile_foreach(skp_compiler_t* compiler, ast_node_t* node);
void compile_return(skp_compiler_t* compiler, ast_node_t* node);
void compile_block(skp_compiler_t* compiler, ast_node_t* node);
void compile_import(skp_compiler_t* compiler, ast_node_t* node);
void compile_export(skp_compiler_t* compiler, ast_node_t* node);

/* ترجمة البرنامج */
void compile_program(skp_compiler_t* compiler, ast_node_t* node);

/* دوال مساعدة */
int get_line(ast_node_t* node);
const char* opcode_name(opcode_t op);
void chunk_disassemble(chunk_t* chunk, const char* name);
int disassemble_instruction(chunk_t* chunk, int offset);

#endif /* COMPILER_H */
