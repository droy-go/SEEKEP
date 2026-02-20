/*
 * SEEKEP - Smart Expressive Efficient Knowledge Execution Platform
 * المحلل اللغوي (Parser) - Parser
 * 
 * يقوم بتحويل الرموز إلى شجرة البنية المجردة (AST)
 */

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "seekep.h"

/* أنواع عقد شجرة البنية المجردة */
typedef enum {
    /* عقد التعبيرات */
    AST_NUMBER,           /* رقم */
    AST_STRING,           /* نص */
    AST_BOOLEAN,          /* قيمة منطقية */
    AST_NULL,             /* فارغ */
    AST_IDENTIFIER,       /* معرف */
    AST_BINARY_OP,        /* عملية ثنائية */
    AST_UNARY_OP,         /* عملية أحادية */
    AST_ASSIGNMENT,       /* تعيين */
    AST_CALL,             /* استدعاء دالة */
    AST_MEMBER_ACCESS,    /* الوصول للعضو */
    AST_INDEX_ACCESS,     /* الوصول بالفهرس */
    AST_LIST_LITERAL,     /* قائمة حرفية */
    AST_DICT_LITERAL,     /* قاموس حرفي */
    AST_LAMBDA,           /* دالة lambda */
    AST_TERNARY,          /* عملية ثلاثية */
    
    /* عقد التصريحات */
    AST_VAR_DECL,         /* تصريح متغير */
    AST_CONST_DECL,       /* تصريح ثابت */
    AST_FUNC_DECL,        /* تصريح دالة */
    AST_CLASS_DECL,       /* تصريح صنف */
    AST_RETURN,           /* إرجاع */
    AST_IF,               /* إذا */
    AST_WHILE,            /* أثناء */
    AST_FOR,              /* لكل */
    AST_FOREACH,          /* لكل-في */
    AST_BREAK,            /* توقف */
    AST_CONTINUE,         /* استمر */
    AST_BLOCK,            /* كتلة */
    AST_EXPRESSION_STMT,  /* عبارة تعبير */
    AST_IMPORT,           /* استيراد */
    AST_EXPORT,           /* تصدير */
    
    /* عقد خاصة */
    AST_PROGRAM           /* البرنامج الكامل */
} ast_node_type_t;

/* أنواع العمليات الثنائية */
typedef enum {
    BINOP_ADD,      /* + */
    BINOP_SUB,      /* - */
    BINOP_MUL,      /* * */
    BINOP_DIV,      /* / */
    BINOP_MOD,      /* % */
    BINOP_POW,      /* ^ */
    BINOP_EQ,       /* == */
    BINOP_NE,       /* != */
    BINOP_LT,       /* < */
    BINOP_GT,       /* > */
    BINOP_LE,       /* <= */
    BINOP_GE,       /* >= */
    BINOP_AND,      /* && */
    BINOP_OR,       /* || */
    BINOP_BIT_AND,  /* & */
    BINOP_BIT_OR,   /* | */
    BINOP_BIT_XOR,  /* ~ */
    BINOP_SHL,      /* << */
    BINOP_SHR       /* >> */
} binop_type_t;

/* أنواع العمليات الأحادية */
typedef enum {
    UNOP_NEG,       /* - */
    UNOP_NOT,       /* ! */
    UNOP_BIT_NOT,   /* ~ */
    UNOP_INC,       /* ++ */
    UNOP_DEC        /* -- */
} unop_type_t;

/* عقدة شجرة البنية المجردة */
typedef struct ast_node {
    ast_node_type_t type;
    int line;
    int column;
    
    union {
        /* قيم حرفية */
        struct {
            char* value;
        } number;
        
        struct {
            char* value;
        } string;
        
        struct {
            int value;
        } boolean;
        
        struct {
            char* name;
        } identifier;
        
        /* عمليات */
        struct {
            binop_type_t op;
            struct ast_node* left;
            struct ast_node* right;
        } binary_op;
        
        struct {
            unop_type_t op;
            struct ast_node* operand;
        } unary_op;
        
        struct {
            struct ast_node* target;
            struct ast_node* value;
        } assignment;
        
        struct {
            struct ast_node* callee;
            struct ast_node** args;
            size_t arg_count;
        } call;
        
        struct {
            struct ast_node* object;
            char* member;
        } member_access;
        
        struct {
            struct ast_node* object;
            struct ast_node* index;
        } index_access;
        
        struct {
            struct ast_node** elements;
            size_t element_count;
        } list_literal;
        
        struct {
            struct ast_node** keys;
            struct ast_node** values;
            size_t entry_count;
        } dict_literal;
        
        struct {
            char** params;
            size_t param_count;
            struct ast_node* body;
        } lambda;
        
        struct {
            struct ast_node* condition;
            struct ast_node* true_expr;
            struct ast_node* false_expr;
        } ternary;
        
        /* تصريحات */
        struct {
            char* name;
            struct ast_node* initializer;
            int is_mutable;
        } var_decl;
        
        struct {
            char* name;
            char** params;
            size_t param_count;
            struct ast_node** defaults;
            struct ast_node* body;
        } func_decl;
        
        struct {
            char* name;
            char* parent;
            struct ast_node** members;
            size_t member_count;
        } class_decl;
        
        struct {
            struct ast_node* value;
        } return_stmt;
        
        struct {
            struct ast_node* condition;
            struct ast_node* then_branch;
            struct ast_node* else_branch;
        } if_stmt;
        
        struct {
            struct ast_node* condition;
            struct ast_node* body;
        } while_stmt;
        
        struct {
            char* init_var;
            struct ast_node* init_expr;
            struct ast_node* condition;
            struct ast_node* increment;
            struct ast_node* body;
        } for_stmt;
        
        struct {
            char* var;
            struct ast_node* iterable;
            struct ast_node* body;
        } foreach_stmt;
        
        struct {
            struct ast_node** statements;
            size_t statement_count;
        } block;
        
        struct {
            struct ast_node* expression;
        } expression_stmt;
        
        struct {
            char* module;
            char** names;
            size_t name_count;
        } import;
        
        struct {
            char** names;
            size_t name_count;
        } export_stmt;
        
        struct {
            struct ast_node** statements;
            size_t statement_count;
        } program;
    } data;
} ast_node_t;

/* هيكل المحلل اللغوي */
typedef struct {
    lexer_t* lexer;
    token_t* current;
    token_t* previous;
    int had_error;
    int panic_mode;
} parser_t;

/* إنشاء وإتلاف العقد */
ast_node_t* ast_create_node(ast_node_type_t type, int line, int column);
void ast_destroy_node(ast_node_t* node);
ast_node_t* ast_create_number(const char* value, int line, int column);
ast_node_t* ast_create_string(const char* value, int line, int column);
ast_node_t* ast_create_boolean(int value, int line, int column);
ast_node_t* ast_create_null(int line, int column);
ast_node_t* ast_create_identifier(const char* name, int line, int column);

/* إنشاء عقد التعبيرات */
ast_node_t* ast_create_binary_op(binop_type_t op, ast_node_t* left, ast_node_t* right, int line, int column);
ast_node_t* ast_create_unary_op(unop_type_t op, ast_node_t* operand, int line, int column);
ast_node_t* ast_create_assignment(ast_node_t* target, ast_node_t* value, int line, int column);
ast_node_t* ast_create_call(ast_node_t* callee, ast_node_t** args, size_t arg_count, int line, int column);
ast_node_t* ast_create_member_access(ast_node_t* object, const char* member, int line, int column);
ast_node_t* ast_create_index_access(ast_node_t* object, ast_node_t* index, int line, int column);
ast_node_t* ast_create_list_literal(ast_node_t** elements, size_t count, int line, int column);
ast_node_t* ast_create_dict_literal(ast_node_t** keys, ast_node_t** values, size_t count, int line, int column);
ast_node_t* ast_create_lambda(char** params, size_t param_count, ast_node_t* body, int line, int column);
ast_node_t* ast_create_ternary(ast_node_t* cond, ast_node_t* true_expr, ast_node_t* false_expr, int line, int column);

/* إنشاء عقد التصريحات */
ast_node_t* ast_create_var_decl(const char* name, ast_node_t* init, int is_mutable, int line, int column);
ast_node_t* ast_create_const_decl(const char* name, ast_node_t* init, int line, int column);
ast_node_t* ast_create_func_decl(const char* name, char** params, size_t param_count, 
                                   ast_node_t** defaults, ast_node_t* body, int line, int column);
ast_node_t* ast_create_class_decl(const char* name, const char* parent, 
                                    ast_node_t** members, size_t member_count, int line, int column);
ast_node_t* ast_create_return(ast_node_t* value, int line, int column);
ast_node_t* ast_create_if(ast_node_t* cond, ast_node_t* then_branch, ast_node_t* else_branch, int line, int column);
ast_node_t* ast_create_while(ast_node_t* cond, ast_node_t* body, int line, int column);
ast_node_t* ast_create_for(const char* var, ast_node_t* init, ast_node_t* cond, 
                             ast_node_t* inc, ast_node_t* body, int line, int column);
ast_node_t* ast_create_foreach(const char* var, ast_node_t* iterable, ast_node_t* body, int line, int column);
ast_node_t* ast_create_break(int line, int column);
ast_node_t* ast_create_continue(int line, int column);
ast_node_t* ast_create_block(ast_node_t** stmts, size_t count, int line, int column);
ast_node_t* ast_create_expression_stmt(ast_node_t* expr, int line, int column);
ast_node_t* ast_create_import(const char* module, char** names, size_t count, int line, int column);
ast_node_t* ast_create_export(char** names, size_t count, int line, int column);
ast_node_t* ast_create_program(ast_node_t** stmts, size_t count, int line, int column);

/* دوال المحلل اللغوي */
parser_t* parser_create(lexer_t* lexer);
void parser_destroy(parser_t* parser);
ast_node_t* parser_parse(parser_t* parser);

/* دوال المساعدة للمحلل */
int parser_match(parser_t* parser, token_type_t type);
int parser_check(parser_t* parser, token_type_t type);
token_t* parser_advance(parser_t* parser);
token_t* parser_consume(parser_t* parser, token_type_t type, const char* message);
void parser_synchronize(parser_t* parser);

/* دوال تحليل القواعد */
ast_node_t* parse_program(parser_t* parser);
ast_node_t* parse_declaration(parser_t* parser);
ast_node_t* parse_var_declaration(parser_t* parser, int is_mutable);
ast_node_t* parse_func_declaration(parser_t* parser);
ast_node_t* parse_class_declaration(parser_t* parser);
ast_node_t* parse_statement(parser_t* parser);
ast_node_t* parse_if_statement(parser_t* parser);
ast_node_t* parse_while_statement(parser_t* parser);
ast_node_t* parse_for_statement(parser_t* parser);
ast_node_t* parse_foreach_statement(parser_t* parser);
ast_node_t* parse_return_statement(parser_t* parser);
ast_node_t* parse_break_statement(parser_t* parser);
ast_node_t* parse_continue_statement(parser_t* parser);
ast_node_t* parse_import_statement(parser_t* parser);
ast_node_t* parse_export_statement(parser_t* parser);
ast_node_t* parse_block(parser_t* parser);
ast_node_t* parse_expression_statement(parser_t* parser);

/* دوال تحليل التعبيرات */
ast_node_t* parse_expression(parser_t* parser);
ast_node_t* parse_assignment(parser_t* parser);
ast_node_t* parse_ternary(parser_t* parser);
ast_node_t* parse_or(parser_t* parser);
ast_node_t* parse_and(parser_t* parser);
ast_node_t* parse_equality(parser_t* parser);
ast_node_t* parse_comparison(parser_t* parser);
ast_node_t* parse_bitwise_or(parser_t* parser);
ast_node_t* parse_bitwise_xor(parser_t* parser);
ast_node_t* parse_bitwise_and(parser_t* parser);
ast_node_t* parse_shift(parser_t* parser);
ast_node_t* parse_additive(parser_t* parser);
ast_node_t* parse_multiplicative(parser_t* parser);
ast_node_t* parse_unary(parser_t* parser);
ast_node_t* parse_postfix(parser_t* parser);
ast_node_t* parse_call(parser_t* parser, ast_node_t* callee);
ast_node_t* parse_primary(parser_t* parser);
ast_node_t* parse_list_literal(parser_t* parser);
ast_node_t* parse_dict_literal(parser_t* parser);
ast_node_t* parse_lambda(parser_t* parser);
ast_node_t* parse_arguments(parser_t* parser, ast_node_t*** args, size_t* count);

/* دوال الطباعة والتصحيح */
void ast_print(ast_node_t* node, int indent);
const char* ast_node_type_name(ast_node_type_t type);
const char* binop_type_name(binop_type_t op);
const char* unop_type_name(unop_type_t op);

#endif /* PARSER_H */
