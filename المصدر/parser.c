/*
 * SEEKEP - Smart Expressive Efficient Knowledge Execution Platform
 * المحلل اللغوي (Parser) - Parser Implementation
 * 
 * يقوم بتحويل الرموز إلى شجرة البنية المجردة (AST)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

/* ========== إنشاء وإتلاف العقد ========== */

ast_node_t* ast_create_node(ast_node_type_t type, int line, int column) {
    ast_node_t* node = (ast_node_t*)malloc(sizeof(ast_node_t));
    if (!node) {
        fprintf(stderr, "خطأ: فشل في تخصيص الذاكرة للعقدة\n");
        return NULL;
    }
    
    node->type = type;
    node->line = line;
    node->column = column;
    memset(&node->data, 0, sizeof(node->data));
    
    return node;
}

void ast_destroy_node(ast_node_t* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_NUMBER:
            free(node->data.number.value);
            break;
            
        case AST_STRING:
            free(node->data.string.value);
            break;
            
        case AST_IDENTIFIER:
            free(node->data.identifier.name);
            break;
            
        case AST_BINARY_OP:
            ast_destroy_node(node->data.binary_op.left);
            ast_destroy_node(node->data.binary_op.right);
            break;
            
        case AST_UNARY_OP:
            ast_destroy_node(node->data.unary_op.operand);
            break;
            
        case AST_ASSIGNMENT:
            ast_destroy_node(node->data.assignment.target);
            ast_destroy_node(node->data.assignment.value);
            break;
            
        case AST_CALL:
            ast_destroy_node(node->data.call.callee);
            for (size_t i = 0; i < node->data.call.arg_count; i++) {
                ast_destroy_node(node->data.call.args[i]);
            }
            free(node->data.call.args);
            break;
            
        case AST_MEMBER_ACCESS:
            ast_destroy_node(node->data.member_access.object);
            free(node->data.member_access.member);
            break;
            
        case AST_INDEX_ACCESS:
            ast_destroy_node(node->data.index_access.object);
            ast_destroy_node(node->data.index_access.index);
            break;
            
        case AST_LIST_LITERAL:
            for (size_t i = 0; i < node->data.list_literal.element_count; i++) {
                ast_destroy_node(node->data.list_literal.elements[i]);
            }
            free(node->data.list_literal.elements);
            break;
            
        case AST_DICT_LITERAL:
            for (size_t i = 0; i < node->data.dict_literal.entry_count; i++) {
                ast_destroy_node(node->data.dict_literal.keys[i]);
                ast_destroy_node(node->data.dict_literal.values[i]);
            }
            free(node->data.dict_literal.keys);
            free(node->data.dict_literal.values);
            break;
            
        case AST_LAMBDA:
            for (size_t i = 0; i < node->data.lambda.param_count; i++) {
                free(node->data.lambda.params[i]);
            }
            free(node->data.lambda.params);
            ast_destroy_node(node->data.lambda.body);
            break;
            
        case AST_TERNARY:
            ast_destroy_node(node->data.ternary.condition);
            ast_destroy_node(node->data.ternary.true_expr);
            ast_destroy_node(node->data.ternary.false_expr);
            break;
            
        case AST_VAR_DECL:
        case AST_CONST_DECL:
            free(node->data.var_decl.name);
            ast_destroy_node(node->data.var_decl.initializer);
            break;
            
        case AST_FUNC_DECL:
            free(node->data.func_decl.name);
            for (size_t i = 0; i < node->data.func_decl.param_count; i++) {
                free(node->data.func_decl.params[i]);
            }
            free(node->data.func_decl.params);
            if (node->data.func_decl.defaults) {
                for (size_t i = 0; i < node->data.func_decl.param_count; i++) {
                    ast_destroy_node(node->data.func_decl.defaults[i]);
                }
                free(node->data.func_decl.defaults);
            }
            ast_destroy_node(node->data.func_decl.body);
            break;
            
        case AST_CLASS_DECL:
            free(node->data.class_decl.name);
            if (node->data.class_decl.parent) {
                free(node->data.class_decl.parent);
            }
            for (size_t i = 0; i < node->data.class_decl.member_count; i++) {
                ast_destroy_node(node->data.class_decl.members[i]);
            }
            free(node->data.class_decl.members);
            break;
            
        case AST_RETURN:
            ast_destroy_node(node->data.return_stmt.value);
            break;
            
        case AST_IF:
            ast_destroy_node(node->data.if_stmt.condition);
            ast_destroy_node(node->data.if_stmt.then_branch);
            ast_destroy_node(node->data.if_stmt.else_branch);
            break;
            
        case AST_WHILE:
            ast_destroy_node(node->data.while_stmt.condition);
            ast_destroy_node(node->data.while_stmt.body);
            break;
            
        case AST_FOR:
            free(node->data.for_stmt.init_var);
            ast_destroy_node(node->data.for_stmt.init_expr);
            ast_destroy_node(node->data.for_stmt.condition);
            ast_destroy_node(node->data.for_stmt.increment);
            ast_destroy_node(node->data.for_stmt.body);
            break;
            
        case AST_FOREACH:
            free(node->data.foreach_stmt.var);
            ast_destroy_node(node->data.foreach_stmt.iterable);
            ast_destroy_node(node->data.foreach_stmt.body);
            break;
            
        case AST_BLOCK:
        case AST_PROGRAM:
            for (size_t i = 0; i < node->data.block.statement_count; i++) {
                ast_destroy_node(node->data.block.statements[i]);
            }
            free(node->data.block.statements);
            break;
            
        case AST_EXPRESSION_STMT:
            ast_destroy_node(node->data.expression_stmt.expression);
            break;
            
        case AST_IMPORT:
            free(node->data.import.module);
            for (size_t i = 0; i < node->data.import.name_count; i++) {
                free(node->data.import.names[i]);
            }
            free(node->data.import.names);
            break;
            
        case AST_EXPORT:
            for (size_t i = 0; i < node->data.export_stmt.name_count; i++) {
                free(node->data.export_stmt.names[i]);
            }
            free(node->data.export_stmt.names);
            break;
            
        default:
            break;
    }
    
    free(node);
}

ast_node_t* ast_create_number(const char* value, int line, int column) {
    ast_node_t* node = ast_create_node(AST_NUMBER, line, column);
    if (node) {
        node->data.number.value = strdup(value);
    }
    return node;
}

ast_node_t* ast_create_string(const char* value, int line, int column) {
    ast_node_t* node = ast_create_node(AST_STRING, line, column);
    if (node) {
        node->data.string.value = strdup(value);
    }
    return node;
}

ast_node_t* ast_create_boolean(int value, int line, int column) {
    ast_node_t* node = ast_create_node(AST_BOOLEAN, line, column);
    if (node) {
        node->data.boolean.value = value;
    }
    return node;
}

ast_node_t* ast_create_null(int line, int column) {
    return ast_create_node(AST_NULL, line, column);
}

ast_node_t* ast_create_identifier(const char* name, int line, int column) {
    ast_node_t* node = ast_create_node(AST_IDENTIFIER, line, column);
    if (node) {
        node->data.identifier.name = strdup(name);
    }
    return node;
}

/* ========== إنشاء عقد التعبيرات ========== */

ast_node_t* ast_create_binary_op(binop_type_t op, ast_node_t* left, ast_node_t* right, int line, int column) {
    ast_node_t* node = ast_create_node(AST_BINARY_OP, line, column);
    if (node) {
        node->data.binary_op.op = op;
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
    }
    return node;
}

ast_node_t* ast_create_unary_op(unop_type_t op, ast_node_t* operand, int line, int column) {
    ast_node_t* node = ast_create_node(AST_UNARY_OP, line, column);
    if (node) {
        node->data.unary_op.op = op;
        node->data.unary_op.operand = operand;
    }
    return node;
}

ast_node_t* ast_create_assignment(ast_node_t* target, ast_node_t* value, int line, int column) {
    ast_node_t* node = ast_create_node(AST_ASSIGNMENT, line, column);
    if (node) {
        node->data.assignment.target = target;
        node->data.assignment.value = value;
    }
    return node;
}

ast_node_t* ast_create_call(ast_node_t* callee, ast_node_t** args, size_t arg_count, int line, int column) {
    ast_node_t* node = ast_create_node(AST_CALL, line, column);
    if (node) {
        node->data.call.callee = callee;
        node->data.call.args = args;
        node->data.call.arg_count = arg_count;
    }
    return node;
}

ast_node_t* ast_create_member_access(ast_node_t* object, const char* member, int line, int column) {
    ast_node_t* node = ast_create_node(AST_MEMBER_ACCESS, line, column);
    if (node) {
        node->data.member_access.object = object;
        node->data.member_access.member = strdup(member);
    }
    return node;
}

ast_node_t* ast_create_index_access(ast_node_t* object, ast_node_t* index, int line, int column) {
    ast_node_t* node = ast_create_node(AST_INDEX_ACCESS, line, column);
    if (node) {
        node->data.index_access.object = object;
        node->data.index_access.index = index;
    }
    return node;
}

ast_node_t* ast_create_list_literal(ast_node_t** elements, size_t count, int line, int column) {
    ast_node_t* node = ast_create_node(AST_LIST_LITERAL, line, column);
    if (node) {
        node->data.list_literal.elements = elements;
        node->data.list_literal.element_count = count;
    }
    return node;
}

ast_node_t* ast_create_dict_literal(ast_node_t** keys, ast_node_t** values, size_t count, int line, int column) {
    ast_node_t* node = ast_create_node(AST_DICT_LITERAL, line, column);
    if (node) {
        node->data.dict_literal.keys = keys;
        node->data.dict_literal.values = values;
        node->data.dict_literal.entry_count = count;
    }
    return node;
}

ast_node_t* ast_create_lambda(char** params, size_t param_count, ast_node_t* body, int line, int column) {
    ast_node_t* node = ast_create_node(AST_LAMBDA, line, column);
    if (node) {
        node->data.lambda.params = params;
        node->data.lambda.param_count = param_count;
        node->data.lambda.body = body;
    }
    return node;
}

ast_node_t* ast_create_ternary(ast_node_t* cond, ast_node_t* true_expr, ast_node_t* false_expr, int line, int column) {
    ast_node_t* node = ast_create_node(AST_TERNARY, line, column);
    if (node) {
        node->data.ternary.condition = cond;
        node->data.ternary.true_expr = true_expr;
        node->data.ternary.false_expr = false_expr;
    }
    return node;
}

/* ========== إنشاء عقد التصريحات ========== */

ast_node_t* ast_create_var_decl(const char* name, ast_node_t* init, int is_mutable, int line, int column) {
    ast_node_t* node = ast_create_node(is_mutable ? AST_VAR_DECL : AST_CONST_DECL, line, column);
    if (node) {
        node->data.var_decl.name = strdup(name);
        node->data.var_decl.initializer = init;
        node->data.var_decl.is_mutable = is_mutable;
    }
    return node;
}

ast_node_t* ast_create_const_decl(const char* name, ast_node_t* init, int line, int column) {
    return ast_create_var_decl(name, init, 0, line, column);
}

ast_node_t* ast_create_func_decl(const char* name, char** params, size_t param_count,
                                   ast_node_t** defaults, ast_node_t* body, int line, int column) {
    ast_node_t* node = ast_create_node(AST_FUNC_DECL, line, column);
    if (node) {
        node->data.func_decl.name = strdup(name);
        node->data.func_decl.params = params;
        node->data.func_decl.param_count = param_count;
        node->data.func_decl.defaults = defaults;
        node->data.func_decl.body = body;
    }
    return node;
}

ast_node_t* ast_create_class_decl(const char* name, const char* parent,
                                    ast_node_t** members, size_t member_count, int line, int column) {
    ast_node_t* node = ast_create_node(AST_CLASS_DECL, line, column);
    if (node) {
        node->data.class_decl.name = strdup(name);
        node->data.class_decl.parent = parent ? strdup(parent) : NULL;
        node->data.class_decl.members = members;
        node->data.class_decl.member_count = member_count;
    }
    return node;
}

ast_node_t* ast_create_return(ast_node_t* value, int line, int column) {
    ast_node_t* node = ast_create_node(AST_RETURN, line, column);
    if (node) {
        node->data.return_stmt.value = value;
    }
    return node;
}

ast_node_t* ast_create_if(ast_node_t* cond, ast_node_t* then_branch, ast_node_t* else_branch, int line, int column) {
    ast_node_t* node = ast_create_node(AST_IF, line, column);
    if (node) {
        node->data.if_stmt.condition = cond;
        node->data.if_stmt.then_branch = then_branch;
        node->data.if_stmt.else_branch = else_branch;
    }
    return node;
}

ast_node_t* ast_create_while(ast_node_t* cond, ast_node_t* body, int line, int column) {
    ast_node_t* node = ast_create_node(AST_WHILE, line, column);
    if (node) {
        node->data.while_stmt.condition = cond;
        node->data.while_stmt.body = body;
    }
    return node;
}

ast_node_t* ast_create_for(const char* var, ast_node_t* init, ast_node_t* cond,
                             ast_node_t* inc, ast_node_t* body, int line, int column) {
    ast_node_t* node = ast_create_node(AST_FOR, line, column);
    if (node) {
        node->data.for_stmt.init_var = var ? strdup(var) : NULL;
        node->data.for_stmt.init_expr = init;
        node->data.for_stmt.condition = cond;
        node->data.for_stmt.increment = inc;
        node->data.for_stmt.body = body;
    }
    return node;
}

ast_node_t* ast_create_foreach(const char* var, ast_node_t* iterable, ast_node_t* body, int line, int column) {
    ast_node_t* node = ast_create_node(AST_FOREACH, line, column);
    if (node) {
        node->data.foreach_stmt.var = strdup(var);
        node->data.foreach_stmt.iterable = iterable;
        node->data.foreach_stmt.body = body;
    }
    return node;
}

ast_node_t* ast_create_break(int line, int column) {
    return ast_create_node(AST_BREAK, line, column);
}

ast_node_t* ast_create_continue(int line, int column) {
    return ast_create_node(AST_CONTINUE, line, column);
}

ast_node_t* ast_create_block(ast_node_t** stmts, size_t count, int line, int column) {
    ast_node_t* node = ast_create_node(AST_BLOCK, line, column);
    if (node) {
        node->data.block.statements = stmts;
        node->data.block.statement_count = count;
    }
    return node;
}

ast_node_t* ast_create_expression_stmt(ast_node_t* expr, int line, int column) {
    ast_node_t* node = ast_create_node(AST_EXPRESSION_STMT, line, column);
    if (node) {
        node->data.expression_stmt.expression = expr;
    }
    return node;
}

ast_node_t* ast_create_import(const char* module, char** names, size_t count, int line, int column) {
    ast_node_t* node = ast_create_node(AST_IMPORT, line, column);
    if (node) {
        node->data.import.module = strdup(module);
        node->data.import.names = names;
        node->data.import.name_count = count;
    }
    return node;
}

ast_node_t* ast_create_export(char** names, size_t count, int line, int column) {
    ast_node_t* node = ast_create_node(AST_EXPORT, line, column);
    if (node) {
        node->data.export_stmt.names = names;
        node->data.export_stmt.name_count = count;
    }
    return node;
}

ast_node_t* ast_create_program(ast_node_t** stmts, size_t count, int line, int column) {
    ast_node_t* node = ast_create_node(AST_PROGRAM, line, column);
    if (node) {
        node->data.program.statements = stmts;
        node->data.program.statement_count = count;
    }
    return node;
}

/* ========== دوال المحلل اللغوي ========== */

parser_t* parser_create(lexer_t* lexer) {
    parser_t* parser = (parser_t*)malloc(sizeof(parser_t));
    if (!parser) {
        fprintf(stderr, "خطأ: فشل في تخصيص ذاكرة المحلل\n");
        return NULL;
    }
    
    parser->lexer = lexer;
    parser->current = NULL;
    parser->previous = NULL;
    parser->had_error = 0;
    parser->panic_mode = 0;
    
    /* الحصول على أول رمز */
    parser->current = lexer_next_token(lexer);
    
    return parser;
}

void parser_destroy(parser_t* parser) {
    if (!parser) return;
    
    if (parser->current) {
        token_destroy(parser->current);
    }
    if (parser->previous) {
        token_destroy(parser->previous);
    }
    
    free(parser);
}

ast_node_t* parser_parse(parser_t* parser) {
    return parse_program(parser);
}

/* ========== دوال المساعدة ========== */

int parser_match(parser_t* parser, token_type_t type) {
    if (parser_check(parser, type)) {
        if (parser->previous) {
            token_destroy(parser->previous);
        }
        parser->previous = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        return 1;
    }
    return 0;
}

int parser_check(parser_t* parser, token_type_t type) {
    return parser->current && parser->current->type == type;
}

token_t* parser_advance(parser_t* parser) {
    if (parser->previous) {
        token_destroy(parser->previous);
    }
    parser->previous = parser->current;
    parser->current = lexer_next_token(parser->lexer);
    return parser->previous;
}

token_t* parser_consume(parser_t* parser, token_type_t type, const char* message) {
    if (parser_check(parser, type)) {
        return parser_advance(parser);
    }
    
    fprintf(stderr, "خطأ في السطر %d، العمود %d: %s\n",
            parser->current ? parser->current->line : 0,
            parser->current ? parser->current->column : 0,
            message);
    parser->had_error = 1;
    parser->panic_mode = 1;
    return NULL;
}

void parser_synchronize(parser_t* parser) {
    parser->panic_mode = 0;
    
    while (parser->current->type != TOKEN_EOF) {
        if (parser->previous && parser->previous->type == TOKEN_SEMICOLON) return;
        
        switch (parser->current->type) {
            case TOKEN_VAR:
            case TOKEN_CONST:
            case TOKEN_FUNC:
            case TOKEN_CLASS:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_FOR:
            case TOKEN_RETURN:
            case TOKEN_IMPORT:
            case TOKEN_EXPORT:
                return;
            default:
                break;
        }
        
        parser_advance(parser);
    }
}

/* ========== تحليل القواعد - البرنامج ========== */

ast_node_t* parse_program(parser_t* parser) {
    ast_node_t** statements = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    int line = parser->current ? parser->current->line : 1;
    int column = parser->current ? parser->current->column : 1;
    
    while (!parser_check(parser, TOKEN_EOF)) {
        if (count >= capacity) {
            capacity = capacity == 0 ? 8 : capacity * 2;
            statements = (ast_node_t**)realloc(statements, capacity * sizeof(ast_node_t*));
        }
        
        ast_node_t* decl = parse_declaration(parser);
        if (decl) {
            statements[count++] = decl;
        }
        
        if (parser->panic_mode) {
            parser_synchronize(parser);
        }
    }
    
    return ast_create_program(statements, count, line, column);
}

ast_node_t* parse_declaration(parser_t* parser) {
    if (parser_match(parser, TOKEN_VAR)) {
        return parse_var_declaration(parser, 1);
    }
    if (parser_match(parser, TOKEN_CONST)) {
        return parse_var_declaration(parser, 0);
    }
    if (parser_match(parser, TOKEN_FUNC)) {
        return parse_func_declaration(parser);
    }
    if (parser_match(parser, TOKEN_CLASS)) {
        return parse_class_declaration(parser);
    }
    if (parser_match(parser, TOKEN_IMPORT)) {
        return parse_import_statement(parser);
    }
    if (parser_match(parser, TOKEN_EXPORT)) {
        return parse_export_statement(parser);
    }
    
    return parse_statement(parser);
}

ast_node_t* parse_var_declaration(parser_t* parser, int is_mutable) {
    int line = parser->previous->line;
    int column = parser->previous->column;
    
    token_t* name = parser_consume(parser, TOKEN_IDENTIFIER, "متوقع اسم المتغير");
    if (!name) return NULL;
    
    ast_node_t* initializer = NULL;
    if (parser_match(parser, TOKEN_ASSIGN)) {
        initializer = parse_expression(parser);
    }
    
    parser_match(parser, TOKEN_SEMICOLON);
    
    ast_node_t* node = ast_create_var_decl(name->value, initializer, is_mutable, line, column);
    token_destroy(name);
    return node;
}

ast_node_t* parse_func_declaration(parser_t* parser) {
    int line = parser->previous->line;
    int column = parser->previous->column;
    
    token_t* name = parser_consume(parser, TOKEN_IDENTIFIER, "متوقع اسم الدالة");
    if (!name) return NULL;
    
    parser_consume(parser, TOKEN_LPAREN, "متوقع '(' بعد اسم الدالة");
    
    char** params = NULL;
    ast_node_t** defaults = NULL;
    size_t param_count = 0;
    size_t capacity = 0;
    
    if (!parser_check(parser, TOKEN_RPAREN)) {
        do {
            if (param_count >= capacity) {
                capacity = capacity == 0 ? 4 : capacity * 2;
                params = (char**)realloc(params, capacity * sizeof(char*));
                defaults = (ast_node_t**)realloc(defaults, capacity * sizeof(ast_node_t*));
            }
            
            token_t* param = parser_consume(parser, TOKEN_IDENTIFIER, "متوقع اسم المعامل");
            if (!param) {
                /* تنظيف */
                for (size_t i = 0; i < param_count; i++) {
                    free(params[i]);
                    ast_destroy_node(defaults[i]);
                }
                free(params);
                free(defaults);
                token_destroy(name);
                return NULL;
            }
            
            params[param_count] = strdup(param->value);
            
            if (parser_match(parser, TOKEN_ASSIGN)) {
                defaults[param_count] = parse_expression(parser);
            } else {
                defaults[param_count] = NULL;
            }
            
            param_count++;
            token_destroy(param);
        } while (parser_match(parser, TOKEN_COMMA));
    }
    
    parser_consume(parser, TOKEN_RPAREN, "متوقع ')' بعد المعاملات");
    
    ast_node_t* body = parse_block(parser);
    
    ast_node_t* node = ast_create_func_decl(name->value, params, param_count, defaults, body, line, column);
    token_destroy(name);
    return node;
}

ast_node_t* parse_class_declaration(parser_t* parser) {
    int line = parser->previous->line;
    int column = parser->previous->column;
    
    token_t* name = parser_consume(parser, TOKEN_IDENTIFIER, "متوقع اسم الصنف");
    if (!name) return NULL;
    
    char* parent = NULL;
    if (parser_match(parser, TOKEN_COLON)) {
        token_t* parent_token = parser_consume(parser, TOKEN_IDENTIFIER, "متوقع اسم الصنف الأب");
        if (parent_token) {
            parent = strdup(parent_token->value);
            token_destroy(parent_token);
        }
    }
    
    parser_consume(parser, TOKEN_LBRACE, "متوقع '{' قبل أعضاء الصنف");
    
    ast_node_t** members = NULL;
    size_t member_count = 0;
    size_t capacity = 0;
    
    while (!parser_check(parser, TOKEN_RBRACE) && !parser_check(parser, TOKEN_EOF)) {
        if (member_count >= capacity) {
            capacity = capacity == 0 ? 4 : capacity * 2;
            members = (ast_node_t**)realloc(members, capacity * sizeof(ast_node_t*));
        }
        
        members[member_count++] = parse_declaration(parser);
    }
    
    parser_consume(parser, TOKEN_RBRACE, "متوقع '}' بعد أعضاء الصنف");
    
    ast_node_t* node = ast_create_class_decl(name->value, parent, members, member_count, line, column);
    token_destroy(name);
    free(parent);
    return node;
}

/* ========== تحليل التصريحات ========== */

ast_node_t* parse_statement(parser_t* parser) {
    if (parser_match(parser, TOKEN_IF)) {
        return parse_if_statement(parser);
    }
    if (parser_match(parser, TOKEN_WHILE)) {
        return parse_while_statement(parser);
    }
    if (parser_match(parser, TOKEN_FOR)) {
        return parse_for_statement(parser);
    }
    if (parser_match(parser, TOKEN_RETURN)) {
        return parse_return_statement(parser);
    }
    if (parser_match(parser, TOKEN_BREAK)) {
        return parse_break_statement(parser);
    }
    if (parser_match(parser, TOKEN_CONTINUE)) {
        return parse_continue_statement(parser);
    }
    if (parser_match(parser, TOKEN_LBRACE)) {
        return parse_block(parser);
    }
    
    return parse_expression_statement(parser);
}

ast_node_t* parse_if_statement(parser_t* parser) {
    int line = parser->previous->line;
    int column = parser->previous->column;
    
    parser_consume(parser, TOKEN_LPAREN, "متوقع '(' بعد 'إذا'");
    ast_node_t* condition = parse_expression(parser);
    parser_consume(parser, TOKEN_RPAREN, "متوقع ')' بعد الشرط");
    
    ast_node_t* then_branch = parse_statement(parser);
    ast_node_t* else_branch = NULL;
    
    if (parser_match(parser, TOKEN_ELSE)) {
        else_branch = parse_statement(parser);
    }
    
    return ast_create_if(condition, then_branch, else_branch, line, column);
}

ast_node_t* parse_while_statement(parser_t* parser) {
    int line = parser->previous->line;
    int column = parser->previous->column;
    
    parser_consume(parser, TOKEN_LPAREN, "متوقع '(' بعد 'أثناء'");
    ast_node_t* condition = parse_expression(parser);
    parser_consume(parser, TOKEN_RPAREN, "متوقع ')' بعد الشرط");
    
    ast_node_t* body = parse_statement(parser);
    
    return ast_create_while(condition, body, line, column);
}

ast_node_t* parse_for_statement(parser_t* parser) {
    int line = parser->previous->line;
    int column = parser->previous->column;
    
    parser_consume(parser, TOKEN_LPAREN, "متوقع '(' بعد 'لكل'");
    
    /* التحقق مما إذا كان لكل-في */
    if (parser_check(parser, TOKEN_IDENTIFIER)) {
        token_t* lookahead = lexer_next_token(parser->lexer);
        if (lookahead->type == TOKEN_IN) {
            token_destroy(lookahead);
            return parse_foreach_statement(parser);
        }
        /* إرجاع الرمز */
        /* ملاحظة: هذا يتطلب دعم إعادة الرموز في الليكسر */
    }
    
    /* لكل التقليدية */
    ast_node_t* init = NULL;
    char* init_var = NULL;
    
    if (!parser_check(parser, TOKEN_SEMICOLON)) {
        if (parser_match(parser, TOKEN_VAR)) {
            token_t* var = parser_consume(parser, TOKEN_IDENTIFIER, "متوقع اسم المتغير");
            init_var = strdup(var->value);
            token_destroy(var);
            if (parser_match(parser, TOKEN_ASSIGN)) {
                init = parse_expression(parser);
            }
        } else {
            init = parse_expression(parser);
        }
    }
    parser_consume(parser, TOKEN_SEMICOLON, "متوقع ';' بعد التهيئة");
    
    ast_node_t* condition = NULL;
    if (!parser_check(parser, TOKEN_SEMICOLON)) {
        condition = parse_expression(parser);
    }
    parser_consume(parser, TOKEN_SEMICOLON, "متوقع ';' بعد الشرط");
    
    ast_node_t* increment = NULL;
    if (!parser_check(parser, TOKEN_RPAREN)) {
        increment = parse_expression(parser);
    }
    parser_consume(parser, TOKEN_RPAREN, "متوقع ')' بعد الزيادة");
    
    ast_node_t* body = parse_statement(parser);
    
    return ast_create_for(init_var, init, condition, increment, body, line, column);
}

ast_node_t* parse_foreach_statement(parser_t* parser) {
    int line = parser->previous->line;
    int column = parser->previous->column;
    
    token_t* var = parser_consume(parser, TOKEN_IDENTIFIER, "متوقع اسم المتغير");
    if (!var) return NULL;
    
    parser_consume(parser, TOKEN_IN, "متوقع 'في' بعد اسم المتغير");
    
    ast_node_t* iterable = parse_expression(parser);
    parser_consume(parser, TOKEN_RPAREN, "متوقع ')' بعد المجموعة");
    
    ast_node_t* body = parse_statement(parser);
    
    ast_node_t* node = ast_create_foreach(var->value, iterable, body, line, column);
    token_destroy(var);
    return node;
}

ast_node_t* parse_return_statement(parser_t* parser) {
    int line = parser->previous->line;
    int column = parser->previous->column;
    
    ast_node_t* value = NULL;
    if (!parser_check(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_RBRACE)) {
        value = parse_expression(parser);
    }
    
    parser_match(parser, TOKEN_SEMICOLON);
    
    return ast_create_return(value, line, column);
}

ast_node_t* parse_break_statement(parser_t* parser) {
    int line = parser->previous->line;
    int column = parser->previous->column;
    parser_match(parser, TOKEN_SEMICOLON);
    return ast_create_break(line, column);
}

ast_node_t* parse_continue_statement(parser_t* parser) {
    int line = parser->previous->line;
    int column = parser->previous->column;
    parser_match(parser, TOKEN_SEMICOLON);
    return ast_create_continue(line, column);
}

ast_node_t* parse_import_statement(parser_t* parser) {
    int line = parser->previous->line;
    int column = parser->previous->column;
    
    token_t* module = parser_consume(parser, TOKEN_STRING, "متوقع اسم الوحدة");
    if (!module) return NULL;
    
    char** names = NULL;
    size_t name_count = 0;
    
    if (parser_match(parser, TOKEN_IMPORT)) {
        /* استيراد محدد */
        parser_consume(parser, TOKEN_LBRACE, "متوقع '{' بعد 'استورد'");
        
        do {
            token_t* name = parser_consume(parser, TOKEN_IDENTIFIER, "متوقع اسم للاستيراد");
            if (name) {
                names = (char**)realloc(names, (name_count + 1) * sizeof(char*));
                names[name_count++] = strdup(name->value);
                token_destroy(name);
            }
        } while (parser_match(parser, TOKEN_COMMA));
        
        parser_consume(parser, TOKEN_RBRACE, "متوقع '}' بعد الأسماء");
    }
    
    parser_match(parser, TOKEN_SEMICOLON);
    
    ast_node_t* node = ast_create_import(module->value, names, name_count, line, column);
    token_destroy(module);
    return node;
}

ast_node_t* parse_export_statement(parser_t* parser) {
    int line = parser->previous->line;
    int column = parser->previous->column;
    
    char** names = NULL;
    size_t name_count = 0;
    
    if (parser_match(parser, TOKEN_LBRACE)) {
        do {
            token_t* name = parser_consume(parser, TOKEN_IDENTIFIER, "متوقع اسم للتصدير");
            if (name) {
                names = (char**)realloc(names, (name_count + 1) * sizeof(char*));
                names[name_count++] = strdup(name->value);
                token_destroy(name);
            }
        } while (parser_match(parser, TOKEN_COMMA));
        
        parser_consume(parser, TOKEN_RBRACE, "متوقع '}' بعد الأسماء");
    }
    
    parser_match(parser, TOKEN_SEMICOLON);
    
    return ast_create_export(names, name_count, line, column);
}

ast_node_t* parse_block(parser_t* parser) {
    int line = parser->previous->line;
    int column = parser->previous->column;
    
    ast_node_t** statements = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    while (!parser_check(parser, TOKEN_RBRACE) && !parser_check(parser, TOKEN_EOF)) {
        if (count >= capacity) {
            capacity = capacity == 0 ? 8 : capacity * 2;
            statements = (ast_node_t**)realloc(statements, capacity * sizeof(ast_node_t*));
        }
        
        ast_node_t* stmt = parse_declaration(parser);
        if (stmt) {
            statements[count++] = stmt;
        }
        
        if (parser->panic_mode) {
            parser_synchronize(parser);
        }
    }
    
    parser_consume(parser, TOKEN_RBRACE, "متوقع '}' في نهاية الكتلة");
    
    return ast_create_block(statements, count, line, column);
}

ast_node_t* parse_expression_statement(parser_t* parser) {
    int line = parser->current ? parser->current->line : 1;
    int column = parser->current ? parser->current->column : 1;
    
    ast_node_t* expr = parse_expression(parser);
    parser_match(parser, TOKEN_SEMICOLON);
    
    return ast_create_expression_stmt(expr, line, column);
}

/* ========== تحليل التعبيرات ========== */

ast_node_t* parse_expression(parser_t* parser) {
    return parse_assignment(parser);
}

ast_node_t* parse_assignment(parser_t* parser) {
    ast_node_t* expr = parse_ternary(parser);
    
    if (parser_match(parser, TOKEN_ASSIGN)) {
        ast_node_t* value = parse_assignment(parser);
        return ast_create_assignment(expr, value, expr->line, expr->column);
    }
    
    /* تعيينات مركبة */
    if (parser_match(parser, TOKEN_PLUS_ASSIGN)) {
        ast_node_t* value = parse_assignment(parser);
        ast_node_t* add = ast_create_binary_op(BINOP_ADD, expr, value, expr->line, expr->column);
        return ast_create_assignment(expr, add, expr->line, expr->column);
    }
    if (parser_match(parser, TOKEN_MINUS_ASSIGN)) {
        ast_node_t* value = parse_assignment(parser);
        ast_node_t* sub = ast_create_binary_op(BINOP_SUB, expr, value, expr->line, expr->column);
        return ast_create_assignment(expr, sub, expr->line, expr->column);
    }
    if (parser_match(parser, TOKEN_MUL_ASSIGN)) {
        ast_node_t* value = parse_assignment(parser);
        ast_node_t* mul = ast_create_binary_op(BINOP_MUL, expr, value, expr->line, expr->column);
        return ast_create_assignment(expr, mul, expr->line, expr->column);
    }
    if (parser_match(parser, TOKEN_DIV_ASSIGN)) {
        ast_node_t* value = parse_assignment(parser);
        ast_node_t* div = ast_create_binary_op(BINOP_DIV, expr, value, expr->line, expr->column);
        return ast_create_assignment(expr, div, expr->line, expr->column);
    }
    
    return expr;
}

ast_node_t* parse_ternary(parser_t* parser) {
    ast_node_t* condition = parse_or(parser);
    
    if (parser_match(parser, TOKEN_QUESTION)) {
        ast_node_t* true_expr = parse_expression(parser);
        parser_consume(parser, TOKEN_COLON, "متوقع ':' في العملية الثلاثية");
        ast_node_t* false_expr = parse_ternary(parser);
        return ast_create_ternary(condition, true_expr, false_expr, condition->line, condition->column);
    }
    
    return condition;
}

ast_node_t* parse_or(parser_t* parser) {
    ast_node_t* left = parse_and(parser);
    
    while (parser_match(parser, TOKEN_OR)) {
        ast_node_t* right = parse_and(parser);
        left = ast_create_binary_op(BINOP_OR, left, right, left->line, left->column);
    }
    
    return left;
}

ast_node_t* parse_and(parser_t* parser) {
    ast_node_t* left = parse_equality(parser);
    
    while (parser_match(parser, TOKEN_AND)) {
        ast_node_t* right = parse_equality(parser);
        left = ast_create_binary_op(BINOP_AND, left, right, left->line, left->column);
    }
    
    return left;
}

ast_node_t* parse_equality(parser_t* parser) {
    ast_node_t* left = parse_comparison(parser);
    
    while (1) {
        if (parser_match(parser, TOKEN_EQ)) {
            ast_node_t* right = parse_comparison(parser);
            left = ast_create_binary_op(BINOP_EQ, left, right, left->line, left->column);
        } else if (parser_match(parser, TOKEN_NE)) {
            ast_node_t* right = parse_comparison(parser);
            left = ast_create_binary_op(BINOP_NE, left, right, left->line, left->column);
        } else {
            break;
        }
    }
    
    return left;
}

ast_node_t* parse_comparison(parser_t* parser) {
    ast_node_t* left = parse_bitwise_or(parser);
    
    while (1) {
        if (parser_match(parser, TOKEN_LT)) {
            ast_node_t* right = parse_bitwise_or(parser);
            left = ast_create_binary_op(BINOP_LT, left, right, left->line, left->column);
        } else if (parser_match(parser, TOKEN_GT)) {
            ast_node_t* right = parse_bitwise_or(parser);
            left = ast_create_binary_op(BINOP_GT, left, right, left->line, left->column);
        } else if (parser_match(parser, TOKEN_LE)) {
            ast_node_t* right = parse_bitwise_or(parser);
            left = ast_create_binary_op(BINOP_LE, left, right, left->line, left->column);
        } else if (parser_match(parser, TOKEN_GE)) {
            ast_node_t* right = parse_bitwise_or(parser);
            left = ast_create_binary_op(BINOP_GE, left, right, left->line, left->column);
        } else {
            break;
        }
    }
    
    return left;
}

ast_node_t* parse_bitwise_or(parser_t* parser) {
    ast_node_t* left = parse_bitwise_xor(parser);
    
    while (parser_match(parser, TOKEN_BIT_OR)) {
        ast_node_t* right = parse_bitwise_xor(parser);
        left = ast_create_binary_op(BINOP_BIT_OR, left, right, left->line, left->column);
    }
    
    return left;
}

ast_node_t* parse_bitwise_xor(parser_t* parser) {
    ast_node_t* left = parse_bitwise_and(parser);
    
    while (parser_match(parser, TOKEN_BIT_XOR)) {
        ast_node_t* right = parse_bitwise_and(parser);
        left = ast_create_binary_op(BINOP_BIT_XOR, left, right, left->line, left->column);
    }
    
    return left;
}

ast_node_t* parse_bitwise_and(parser_t* parser) {
    ast_node_t* left = parse_shift(parser);
    
    while (parser_match(parser, TOKEN_BIT_AND)) {
        ast_node_t* right = parse_shift(parser);
        left = ast_create_binary_op(BINOP_BIT_AND, left, right, left->line, left->column);
    }
    
    return left;
}

ast_node_t* parse_shift(parser_t* parser) {
    ast_node_t* left = parse_additive(parser);
    
    while (1) {
        if (parser_match(parser, TOKEN_SHL)) {
            ast_node_t* right = parse_additive(parser);
            left = ast_create_binary_op(BINOP_SHL, left, right, left->line, left->column);
        } else if (parser_match(parser, TOKEN_SHR)) {
            ast_node_t* right = parse_additive(parser);
            left = ast_create_binary_op(BINOP_SHR, left, right, left->line, left->column);
        } else {
            break;
        }
    }
    
    return left;
}

ast_node_t* parse_additive(parser_t* parser) {
    ast_node_t* left = parse_multiplicative(parser);
    
    while (1) {
        if (parser_match(parser, TOKEN_PLUS)) {
            ast_node_t* right = parse_multiplicative(parser);
            left = ast_create_binary_op(BINOP_ADD, left, right, left->line, left->column);
        } else if (parser_match(parser, TOKEN_MINUS)) {
            ast_node_t* right = parse_multiplicative(parser);
            left = ast_create_binary_op(BINOP_SUB, left, right, left->line, left->column);
        } else {
            break;
        }
    }
    
    return left;
}

ast_node_t* parse_multiplicative(parser_t* parser) {
    ast_node_t* left = parse_unary(parser);
    
    while (1) {
        if (parser_match(parser, TOKEN_MUL)) {
            ast_node_t* right = parse_unary(parser);
            left = ast_create_binary_op(BINOP_MUL, left, right, left->line, left->column);
        } else if (parser_match(parser, TOKEN_DIV)) {
            ast_node_t* right = parse_unary(parser);
            left = ast_create_binary_op(BINOP_DIV, left, right, left->line, left->column);
        } else if (parser_match(parser, TOKEN_MOD)) {
            ast_node_t* right = parse_unary(parser);
            left = ast_create_binary_op(BINOP_MOD, left, right, left->line, left->column);
        } else if (parser_match(parser, TOKEN_POW)) {
            ast_node_t* right = parse_unary(parser);
            left = ast_create_binary_op(BINOP_POW, left, right, left->line, left->column);
        } else {
            break;
        }
    }
    
    return left;
}

ast_node_t* parse_unary(parser_t* parser) {
    if (parser_match(parser, TOKEN_NOT)) {
        ast_node_t* operand = parse_unary(parser);
        return ast_create_unary_op(UNOP_NOT, operand, parser->previous->line, parser->previous->column);
    }
    if (parser_match(parser, TOKEN_MINUS)) {
        ast_node_t* operand = parse_unary(parser);
        return ast_create_unary_op(UNOP_NEG, operand, parser->previous->line, parser->previous->column);
    }
    if (parser_match(parser, TOKEN_BIT_NOT)) {
        ast_node_t* operand = parse_unary(parser);
        return ast_create_unary_op(UNOP_BIT_NOT, operand, parser->previous->line, parser->previous->column);
    }
    if (parser_match(parser, TOKEN_INC)) {
        ast_node_t* operand = parse_unary(parser);
        return ast_create_unary_op(UNOP_INC, operand, parser->previous->line, parser->previous->column);
    }
    if (parser_match(parser, TOKEN_DEC)) {
        ast_node_t* operand = parse_unary(parser);
        return ast_create_unary_op(UNOP_DEC, operand, parser->previous->line, parser->previous->column);
    }
    
    return parse_postfix(parser);
}

ast_node_t* parse_postfix(parser_t* parser) {
    ast_node_t* expr = parse_primary(parser);
    
    while (1) {
        if (parser_match(parser, TOKEN_LPAREN)) {
            expr = parse_call(parser, expr);
        } else if (parser_match(parser, TOKEN_DOT)) {
            token_t* member = parser_consume(parser, TOKEN_IDENTIFIER, "متوقع اسم العضو بعد '.'");
            if (member) {
                expr = ast_create_member_access(expr, member->value, expr->line, expr->column);
                token_destroy(member);
            }
        } else if (parser_match(parser, TOKEN_LBRACKET)) {
            ast_node_t* index = parse_expression(parser);
            parser_consume(parser, TOKEN_RBRACKET, "متوقع ']' بعد الفهرس");
            expr = ast_create_index_access(expr, index, expr->line, expr->column);
        } else if (parser_match(parser, TOKEN_INC)) {
            expr = ast_create_unary_op(UNOP_INC, expr, expr->line, expr->column);
        } else if (parser_match(parser, TOKEN_DEC)) {
            expr = ast_create_unary_op(UNOP_DEC, expr, expr->line, expr->column);
        } else {
            break;
        }
    }
    
    return expr;
}

ast_node_t* parse_call(parser_t* parser, ast_node_t* callee) {
    ast_node_t** args = NULL;
    size_t arg_count = 0;
    
    if (!parser_check(parser, TOKEN_RPAREN)) {
        do {
            args = (ast_node_t**)realloc(args, (arg_count + 1) * sizeof(ast_node_t*));
            args[arg_count++] = parse_expression(parser);
        } while (parser_match(parser, TOKEN_COMMA));
    }
    
    parser_consume(parser, TOKEN_RPAREN, "متوقع ')' بعد المعاملات");
    
    return ast_create_call(callee, args, arg_count, callee->line, callee->column);
}

ast_node_t* parse_primary(parser_t* parser) {
    if (parser_match(parser, TOKEN_NUMBER)) {
        return ast_create_number(parser->previous->value, parser->previous->line, parser->previous->column);
    }
    
    if (parser_match(parser, TOKEN_STRING)) {
        return ast_create_string(parser->previous->value, parser->previous->line, parser->previous->column);
    }
    
    if (parser_match(parser, TOKEN_TRUE)) {
        return ast_create_boolean(1, parser->previous->line, parser->previous->column);
    }
    
    if (parser_match(parser, TOKEN_FALSE)) {
        return ast_create_boolean(0, parser->previous->line, parser->previous->column);
    }
    
    if (parser_match(parser, TOKEN_NULL)) {
        return ast_create_null(parser->previous->line, parser->previous->column);
    }
    
    if (parser_match(parser, TOKEN_IDENTIFIER)) {
        return ast_create_identifier(parser->previous->value, parser->previous->line, parser->previous->column);
    }
    
    if (parser_match(parser, TOKEN_LPAREN)) {
        ast_node_t* expr = parse_expression(parser);
        parser_consume(parser, TOKEN_RPAREN, "متوقع ')' بعد التعبير");
        return expr;
    }
    
    if (parser_match(parser, TOKEN_LBRACKET)) {
        return parse_list_literal(parser);
    }
    
    if (parser_match(parser, TOKEN_LBRACE)) {
        return parse_dict_literal(parser);
    }
    
    if (parser_match(parser, TOKEN_FUNC)) {
        return parse_lambda(parser);
    }
    
    if (parser_match(parser, TOKEN_THIS)) {
        return ast_create_identifier("هذا", parser->previous->line, parser->previous->column);
    }
    
    fprintf(stderr, "خطأ في السطر %d، العمود %d: تعبير غير متوقع\n",
            parser->current->line, parser->current->column);
    parser->had_error = 1;
    return NULL;
}

ast_node_t* parse_list_literal(parser_t* parser) {
    int line = parser->previous->line;
    int column = parser->previous->column;
    
    ast_node_t** elements = NULL;
    size_t count = 0;
    
    if (!parser_check(parser, TOKEN_RBRACKET)) {
        do {
            elements = (ast_node_t**)realloc(elements, (count + 1) * sizeof(ast_node_t*));
            elements[count++] = parse_expression(parser);
        } while (parser_match(parser, TOKEN_COMMA));
    }
    
    parser_consume(parser, TOKEN_RBRACKET, "متوقع ']' في نهاية القائمة");
    
    return ast_create_list_literal(elements, count, line, column);
}

ast_node_t* parse_dict_literal(parser_t* parser) {
    int line = parser->previous->line;
    int column = parser->previous->column;
    
    ast_node_t** keys = NULL;
    ast_node_t** values = NULL;
    size_t count = 0;
    
    if (!parser_check(parser, TOKEN_RBRACE)) {
        do {
            ast_node_t* key = parse_expression(parser);
            parser_consume(parser, TOKEN_COLON, "متوقع ':' بين المفتاح والقيمة");
            ast_node_t* value = parse_expression(parser);
            
            keys = (ast_node_t**)realloc(keys, (count + 1) * sizeof(ast_node_t*));
            values = (ast_node_t**)realloc(values, (count + 1) * sizeof(ast_node_t*));
            keys[count] = key;
            values[count] = value;
            count++;
        } while (parser_match(parser, TOKEN_COMMA));
    }
    
    parser_consume(parser, TOKEN_RBRACE, "متوقع '}' في نهاية القاموس");
    
    return ast_create_dict_literal(keys, values, count, line, column);
}

ast_node_t* parse_lambda(parser_t* parser) {
    int line = parser->previous->line;
    int column = parser->previous->column;
    
    parser_consume(parser, TOKEN_LPAREN, "متوقع '(' بعد 'دالة'");
    
    char** params = NULL;
    size_t param_count = 0;
    
    if (!parser_check(parser, TOKEN_RPAREN)) {
        do {
            token_t* param = parser_consume(parser, TOKEN_IDENTIFIER, "متوقع اسم المعامل");
            if (param) {
                params = (char**)realloc(params, (param_count + 1) * sizeof(char*));
                params[param_count++] = strdup(param->value);
                token_destroy(param);
            }
        } while (parser_match(parser, TOKEN_COMMA));
    }
    
    parser_consume(parser, TOKEN_RPAREN, "متوقع ')' بعد المعاملات");
    
    ast_node_t* body;
    if (parser_match(parser, TOKEN_LBRACE)) {
        body = parse_block(parser);
    } else {
        parser_consume(parser, TOKEN_ARROW, "متوقع '=>' قبل جسم الدالة");
        body = parse_expression(parser);
    }
    
    return ast_create_lambda(params, param_count, body, line, column);
}

/* ========== دوال الطباعة والتصحيح ========== */

const char* ast_node_type_name(ast_node_type_t type) {
    switch (type) {
        case AST_NUMBER: return "رقم";
        case AST_STRING: return "نص";
        case AST_BOOLEAN: return "منطقي";
        case AST_NULL: return "فارغ";
        case AST_IDENTIFIER: return "معرف";
        case AST_BINARY_OP: return "عملية-ثنائية";
        case AST_UNARY_OP: return "عملية-أحادية";
        case AST_ASSIGNMENT: return "تعيين";
        case AST_CALL: return "استدعاء";
        case AST_MEMBER_ACCESS: return "وصول-عضو";
        case AST_INDEX_ACCESS: return "وصول-فهرس";
        case AST_LIST_LITERAL: return "قائمة-حرفية";
        case AST_DICT_LITERAL: return "قاموس-حرفي";
        case AST_LAMBDA: return "lambda";
        case AST_TERNARY: return "ثلاثي";
        case AST_VAR_DECL: return "تصريح-متغير";
        case AST_CONST_DECL: return "تصريح-ثابت";
        case AST_FUNC_DECL: return "تصريح-دالة";
        case AST_CLASS_DECL: return "تصريح-صنف";
        case AST_RETURN: return "إرجاع";
        case AST_IF: return "إذا";
        case AST_WHILE: return "أثناء";
        case AST_FOR: return "لكل";
        case AST_FOREACH: return "لكل-في";
        case AST_BREAK: return "توقف";
        case AST_CONTINUE: return "استمر";
        case AST_BLOCK: return "كتلة";
        case AST_EXPRESSION_STMT: return "عبارة-تعبير";
        case AST_IMPORT: return "استيراد";
        case AST_EXPORT: return "تصدير";
        case AST_PROGRAM: return "برنامج";
        default: return "غير-معروف";
    }
}

const char* binop_type_name(binop_type_t op) {
    switch (op) {
        case BINOP_ADD: return "+";
        case BINOP_SUB: return "-";
        case BINOP_MUL: return "*";
        case BINOP_DIV: return "/";
        case BINOP_MOD: return "%";
        case BINOP_POW: return "^";
        case BINOP_EQ: return "==";
        case BINOP_NE: return "!=";
        case BINOP_LT: return "<";
        case BINOP_GT: return ">";
        case BINOP_LE: return "<=";
        case BINOP_GE: return ">=";
        case BINOP_AND: return "&&";
        case BINOP_OR: return "||";
        case BINOP_BIT_AND: return "&";
        case BINOP_BIT_OR: return "|";
        case BINOP_BIT_XOR: return "~";
        case BINOP_SHL: return "<<";
        case BINOP_SHR: return ">>";
        default: return "?";
    }
}

const char* unop_type_name(unop_type_t op) {
    switch (op) {
        case UNOP_NEG: return "-";
        case UNOP_NOT: return "!";
        case UNOP_BIT_NOT: return "~";
        case UNOP_INC: return "++";
        case UNOP_DEC: return "--";
        default: return "?";
    }
}

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

void ast_print(ast_node_t* node, int indent) {
    if (!node) {
        print_indent(indent);
        printf("(فارغ)\n");
        return;
    }
    
    print_indent(indent);
    printf("%s [%d:%d]\n", ast_node_type_name(node->type), node->line, node->column);
    
    switch (node->type) {
        case AST_NUMBER:
            print_indent(indent + 1);
            printf("قيمة: %s\n", node->data.number.value);
            break;
            
        case AST_STRING:
            print_indent(indent + 1);
            printf("قيمة: \"%s\"\n", node->data.string.value);
            break;
            
        case AST_BOOLEAN:
            print_indent(indent + 1);
            printf("قيمة: %s\n", node->data.boolean.value ? "صحيح" : "خطأ");
            break;
            
        case AST_IDENTIFIER:
            print_indent(indent + 1);
            printf("اسم: %s\n", node->data.identifier.name);
            break;
            
        case AST_BINARY_OP:
            print_indent(indent + 1);
            printf("عملية: %s\n", binop_type_name(node->data.binary_op.op));
            ast_print(node->data.binary_op.left, indent + 1);
            ast_print(node->data.binary_op.right, indent + 1);
            break;
            
        case AST_UNARY_OP:
            print_indent(indent + 1);
            printf("عملية: %s\n", unop_type_name(node->data.unary_op.op));
            ast_print(node->data.unary_op.operand, indent + 1);
            break;
            
        case AST_ASSIGNMENT:
            print_indent(indent + 1);
            printf("هدف:\n");
            ast_print(node->data.assignment.target, indent + 2);
            print_indent(indent + 1);
            printf("قيمة:\n");
            ast_print(node->data.assignment.value, indent + 2);
            break;
            
        case AST_CALL:
            print_indent(indent + 1);
            printf("دالة:\n");
            ast_print(node->data.call.callee, indent + 2);
            print_indent(indent + 1);
            printf("معاملات (%zu):\n", node->data.call.arg_count);
            for (size_t i = 0; i < node->data.call.arg_count; i++) {
                ast_print(node->data.call.args[i], indent + 2);
            }
            break;
            
        case AST_VAR_DECL:
        case AST_CONST_DECL:
            print_indent(indent + 1);
            printf("اسم: %s\n", node->data.var_decl.name);
            print_indent(indent + 1);
            printf("قابل للتغيير: %s\n", node->data.var_decl.is_mutable ? "نعم" : "لا");
            if (node->data.var_decl.initializer) {
                print_indent(indent + 1);
                printf("قيمة أولية:\n");
                ast_print(node->data.var_decl.initializer, indent + 2);
            }
            break;
            
        case AST_FUNC_DECL:
            print_indent(indent + 1);
            printf("اسم: %s\n", node->data.func_decl.name);
            print_indent(indent + 1);
            printf("معاملات (%zu):\n", node->data.func_decl.param_count);
            for (size_t i = 0; i < node->data.func_decl.param_count; i++) {
                print_indent(indent + 2);
                printf("%s\n", node->data.func_decl.params[i]);
            }
            print_indent(indent + 1);
            printf("جسم:\n");
            ast_print(node->data.func_decl.body, indent + 2);
            break;
            
        case AST_IF:
            print_indent(indent + 1);
            printf("شرط:\n");
            ast_print(node->data.if_stmt.condition, indent + 2);
            print_indent(indent + 1);
            printf("فرع صحيح:\n");
            ast_print(node->data.if_stmt.then_branch, indent + 2);
            if (node->data.if_stmt.else_branch) {
                print_indent(indent + 1);
                printf("فرع خطأ:\n");
                ast_print(node->data.if_stmt.else_branch, indent + 2);
            }
            break;
            
        case AST_WHILE:
            print_indent(indent + 1);
            printf("شرط:\n");
            ast_print(node->data.while_stmt.condition, indent + 2);
            print_indent(indent + 1);
            printf("جسم:\n");
            ast_print(node->data.while_stmt.body, indent + 2);
            break;
            
        case AST_BLOCK:
        case AST_PROGRAM:
            for (size_t i = 0; i < node->data.block.statement_count; i++) {
                ast_print(node->data.block.statements[i], indent + 1);
            }
            break;
            
        case AST_EXPRESSION_STMT:
            ast_print(node->data.expression_stmt.expression, indent + 1);
            break;
            
        case AST_RETURN:
            if (node->data.return_stmt.value) {
                print_indent(indent + 1);
                printf("قيمة:\n");
                ast_print(node->data.return_stmt.value, indent + 2);
            }
            break;
            
        case AST_LIST_LITERAL:
            print_indent(indent + 1);
            printf("عناصر (%zu):\n", node->data.list_literal.element_count);
            for (size_t i = 0; i < node->data.list_literal.element_count; i++) {
                ast_print(node->data.list_literal.elements[i], indent + 2);
            }
            break;
            
        case AST_DICT_LITERAL:
            print_indent(indent + 1);
            printf("إدخالات (%zu):\n", node->data.dict_literal.entry_count);
            for (size_t i = 0; i < node->data.dict_literal.entry_count; i++) {
                print_indent(indent + 2);
                printf("مفتاح:\n");
                ast_print(node->data.dict_literal.keys[i], indent + 3);
                print_indent(indent + 2);
                printf("قيمة:\n");
                ast_print(node->data.dict_literal.values[i], indent + 3);
            }
            break;
            
        default:
            break;
    }
}
