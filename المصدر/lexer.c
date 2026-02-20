/* ============================================
 * SEEKEP Lexer - محلل الرموز
 * ============================================ */

#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/* إنشاء معجم جديد */
lexer_t* lexer_create(const char* source) {
    lexer_t* lexer = (lexer_t*)malloc(sizeof(lexer_t));
    if (!lexer) return NULL;
    
    lexer->source = source;
    lexer->length = strlen(source);
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->token_count = 0;
    lexer->token_capacity = 64;
    lexer->tokens = (token_t**)malloc(sizeof(token_t*) * lexer->token_capacity);
    
    return lexer;
}

/* تحرير المعجم */
void lexer_destroy(lexer_t* lexer) {
    if (!lexer) return;
    
    for (size_t i = 0; i < lexer->token_count; i++) {
        token_destroy(lexer->tokens[i]);
    }
    free(lexer->tokens);
    free(lexer);
}

/* إنشاء رمز جديد */
token_t* token_create(token_type_t type, const char* value, int line, int column) {
    token_t* token = (token_t*)malloc(sizeof(token_t));
    if (!token) return NULL;
    
    token->type = type;
    token->value = value ? strdup(value) : NULL;
    token->line = line;
    token->column = column;
    
    return token;
}

/* تحرير رمز */
void token_destroy(token_t* token) {
    if (!token) return;
    if (token->value) free(token->value);
    free(token);
}

/* الحصول على الحرف الحالي */
char lexer_current(lexer_t* lexer) {
    if (lexer->position >= lexer->length) return '\0';
    return lexer->source[lexer->position];
}

/* نظرة للأمام */
char lexer_peek(lexer_t* lexer, int offset) {
    size_t pos = lexer->position + offset;
    if (pos >= lexer->length) return '\0';
    return lexer->source[pos];
}

/* التقدم */
void lexer_advance(lexer_t* lexer) {
    if (lexer->position < lexer->length) {
        if (lexer->source[lexer->position] == '\n') {
            lexer->line++;
            lexer->column = 1;
        } else {
            lexer->column++;
        }
        lexer->position++;
    }
}

/* تخطي المسافات */
void lexer_skip_whitespace(lexer_t* lexer) {
    while (isspace(lexer_current(lexer))) {
        lexer_advance(lexer);
    }
}

/* تخطي التعليق */
void lexer_skip_comment(lexer_t* lexer) {
    if (lexer_current(lexer) == '#') {
        while (lexer_current(lexer) != '\n' && lexer_current(lexer) != '\0') {
            lexer_advance(lexer);
        }
    }
}

/* قراءة رقم */
token_t* lexer_read_number(lexer_t* lexer) {
    int line = lexer->line;
    int column = lexer->column;
    char buffer[64];
    int i = 0;
    
    while (isdigit(lexer_current(lexer)) || lexer_current(lexer) == '.') {
        buffer[i++] = lexer_current(lexer);
        lexer_advance(lexer);
    }
    buffer[i] = '\0';
    
    /* التحقق إذا كان عشرياً */
    if (strchr(buffer, '.')) {
        return token_create(TOKEN_FLOAT, buffer, line, column);
    }
    return token_create(TOKEN_INT, buffer, line, column);
}

/* قراءة نص */
token_t* lexer_read_string(lexer_t* lexer) {
    int line = lexer->line;
    int column = lexer->column;
    char quote = lexer_current(lexer);
    lexer_advance(lexer);
    
    char buffer[1024];
    int i = 0;
    
    while (lexer_current(lexer) != quote && lexer_current(lexer) != '\0') {
        if (lexer_current(lexer) == '\\') {
            lexer_advance(lexer);
            switch (lexer_current(lexer)) {
                case 'n': buffer[i++] = '\n'; break;
                case 't': buffer[i++] = '\t'; break;
                case '\\': buffer[i++] = '\\'; break;
                case '"': buffer[i++] = '"'; break;
                case '\'': buffer[i++] = '\''; break;
                default: buffer[i++] = lexer_current(lexer); break;
            }
        } else {
            buffer[i++] = lexer_current(lexer);
        }
        lexer_advance(lexer);
    }
    
    if (lexer_current(lexer) == quote) {
        lexer_advance(lexer);
    }
    
    buffer[i] = '\0';
    return token_create(TOKEN_STRING, buffer, line, column);
}

/* قراءة معرف */
token_t* lexer_read_identifier(lexer_t* lexer) {
    int line = lexer->line;
    int column = lexer->column;
    char buffer[256];
    int i = 0;
    
    while (isalnum(lexer_current(lexer)) || lexer_current(lexer) == '_') {
        buffer[i++] = lexer_current(lexer);
        lexer_advance(lexer);
    }
    buffer[i] = '\0';
    
    token_type_t type = lexer_keyword_type(buffer);
    return token_create(type, buffer, line, column);
}

/* التحقق من الكلمة المفتاحية */
token_type_t lexer_keyword_type(const char* word) {
    if (strcmp(word, "متغير") == 0) return TOKEN_VAR;
    if (strcmp(word, "ثابت") == 0) return TOKEN_CONST;
    if (strcmp(word, "دالة") == 0) return TOKEN_FUNC;
    if (strcmp(word, "أرجع") == 0) return TOKEN_RETURN;
    if (strcmp(word, "إذا") == 0) return TOKEN_IF;
    if (strcmp(word, "وإلا") == 0) return TOKEN_ELSE;
    if (strcmp(word, "أثناء") == 0) return TOKEN_WHILE;
    if (strcmp(word, "لكل") == 0) return TOKEN_FOR;
    if (strcmp(word, "في") == 0) return TOKEN_IN;
    if (strcmp(word, "توقف") == 0) return TOKEN_BREAK;
    if (strcmp(word, "استمر") == 0) return TOKEN_CONTINUE;
    if (strcmp(word, "صنف") == 0) return TOKEN_CLASS;
    if (strcmp(word, "جديد") == 0) return TOKEN_NEW;
    if (strcmp(word, "هذا") == 0) return TOKEN_THIS;
    if (strcmp(word, "استورد") == 0) return TOKEN_IMPORT;
    if (strcmp(word, "صدر") == 0) return TOKEN_EXPORT;
    if (strcmp(word, "صحيح") == 0) return TOKEN_TRUE;
    if (strcmp(word, "خطأ") == 0) return TOKEN_FALSE;
    if (strcmp(word, "فارغ") == 0) return TOKEN_NULL_KW;
    if (strcmp(word, "و") == 0) return TOKEN_AND;
    if (strcmp(word, "أو") == 0) return TOKEN_OR;
    if (strcmp(word, "ليس") == 0) return TOKEN_NOT;
    
    return TOKEN_IDENTIFIER;
}

/* قراءة عامل */
token_t* lexer_read_operator(lexer_t* lexer) {
    int line = lexer->line;
    int column = lexer->column;
    char c = lexer_current(lexer);
    
    switch (c) {
        case '+':
            lexer_advance(lexer);
            if (lexer_current(lexer) == '=') {
                lexer_advance(lexer);
                return token_create(TOKEN_PLUS_ASSIGN, "+=", line, column);
            }
            return token_create(TOKEN_PLUS, "+", line, column);
            
        case '-':
            lexer_advance(lexer);
            if (lexer_current(lexer) == '=') {
                lexer_advance(lexer);
                return token_create(TOKEN_MINUS_ASSIGN, "-=", line, column);
            }
            if (lexer_current(lexer) == '>') {
                lexer_advance(lexer);
                return token_create(TOKEN_ARROW, "->", line, column);
            }
            return token_create(TOKEN_MINUS, "-", line, column);
            
        case '*':
            lexer_advance(lexer);
            if (lexer_current(lexer) == '*') {
                lexer_advance(lexer);
                return token_create(TOKEN_POWER, "**", line, column);
            }
            if (lexer_current(lexer) == '=') {
                lexer_advance(lexer);
                return token_create(TOKEN_STAR_ASSIGN, "*=", line, column);
            }
            return token_create(TOKEN_STAR, "*", line, column);
            
        case '/':
            lexer_advance(lexer);
            if (lexer_current(lexer) == '=') {
                lexer_advance(lexer);
                return token_create(TOKEN_SLASH_ASSIGN, "/=", line, column);
            }
            return token_create(TOKEN_SLASH, "/", line, column);
            
        case '%':
            lexer_advance(lexer);
            return token_create(TOKEN_PERCENT, "%", line, column);
            
        case '=':
            lexer_advance(lexer);
            if (lexer_current(lexer) == '=') {
                lexer_advance(lexer);
                return token_create(TOKEN_EQ, "==", line, column);
            }
            return token_create(TOKEN_ASSIGN, "=", line, column);
            
        case '!':
            lexer_advance(lexer);
            if (lexer_current(lexer) == '=') {
                lexer_advance(lexer);
                return token_create(TOKEN_NE, "!=", line, column);
            }
            return token_create(TOKEN_ERROR, "!", line, column);
            
        case '<':
            lexer_advance(lexer);
            if (lexer_current(lexer) == '=') {
                lexer_advance(lexer);
                return token_create(TOKEN_LE, "<=", line, column);
            }
            return token_create(TOKEN_LT, "<", line, column);
            
        case '>':
            lexer_advance(lexer);
            if (lexer_current(lexer) == '=') {
                lexer_advance(lexer);
                return token_create(TOKEN_GE, ">=", line, column);
            }
            return token_create(TOKEN_GT, ">", line, column);
            
        case '(':
            lexer_advance(lexer);
            return token_create(TOKEN_LPAREN, "(", line, column);
            
        case ')':
            lexer_advance(lexer);
            return token_create(TOKEN_RPAREN, ")", line, column);
            
        case '{':
            lexer_advance(lexer);
            return token_create(TOKEN_LBRACE, "{", line, column);
            
        case '}':
            lexer_advance(lexer);
            return token_create(TOKEN_RBRACE, "}", line, column);
            
        case '[':
            lexer_advance(lexer);
            return token_create(TOKEN_LBRACKET, "[", line, column);
            
        case ']':
            lexer_advance(lexer);
            return token_create(TOKEN_RBRACKET, "]", line, column);
            
        case ',':
        case '،':
            lexer_advance(lexer);
            return token_create(TOKEN_COMMA, "،", line, column);
            
        case ';':
        case '؛':
            lexer_advance(lexer);
            return token_create(TOKEN_SEMICOLON, "؛", line, column);
            
        case ':':
            lexer_advance(lexer);
            return token_create(TOKEN_COLON, ":", line, column);
            
        case '.':
            lexer_advance(lexer);
            return token_create(TOKEN_DOT, ".", line, column);
            
        default:
            lexer_advance(lexer);
            return token_create(TOKEN_ERROR, "?", line, column);
    }
}

/* تحليل المصدر إلى رموز */
token_t** lexer_tokenize(lexer_t* lexer, size_t* count) {
    while (lexer->position < lexer->length) {
        lexer_skip_whitespace(lexer);
        lexer_skip_comment(lexer);
        lexer_skip_whitespace(lexer);
        
        if (lexer->position >= lexer->length) break;
        
        char c = lexer_current(lexer);
        token_t* token = NULL;
        
        if (isdigit(c)) {
            token = lexer_read_number(lexer);
        } else if (c == '"' || c == '\'') {
            token = lexer_read_string(lexer);
        } else if (isalpha(c) || c == '_') {
            token = lexer_read_identifier(lexer);
        } else {
            token = lexer_read_operator(lexer);
        }
        
        if (token) {
            if (lexer->token_count >= lexer->token_capacity) {
                lexer->token_capacity *= 2;
                lexer->tokens = (token_t**)realloc(lexer->tokens, sizeof(token_t*) * lexer->token_capacity);
            }
            lexer->tokens[lexer->token_count++] = token;
        }
    }
    
    /* إضافة رمز النهاية */
    token_t* eof = token_create(TOKEN_EOF, NULL, lexer->line, lexer->column);
    if (lexer->token_count >= lexer->token_capacity) {
        lexer->token_capacity++;
        lexer->tokens = (token_t**)realloc(lexer->tokens, sizeof(token_t*) * lexer->token_capacity);
    }
    lexer->tokens[lexer->token_count++] = eof;
    
    if (count) *count = lexer->token_count;
    return lexer->tokens;
}

/* اسم نوع الرمز */
const char* token_type_name(token_type_t type) {
    switch (type) {
        case TOKEN_INT: return "INT";
        case TOKEN_FLOAT: return "FLOAT";
        case TOKEN_STRING: return "STRING";
        case TOKEN_BOOL: return "BOOL";
        case TOKEN_NULL: return "NULL";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_VAR: return "VAR";
        case TOKEN_CONST: return "CONST";
        case TOKEN_FUNC: return "FUNC";
        case TOKEN_RETURN: return "RETURN";
        case TOKEN_IF: return "IF";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_WHILE: return "WHILE";
        case TOKEN_FOR: return "FOR";
        case TOKEN_IN: return "IN";
        case TOKEN_BREAK: return "BREAK";
        case TOKEN_CONTINUE: return "CONTINUE";
        case TOKEN_CLASS: return "CLASS";
        case TOKEN_NEW: return "NEW";
        case TOKEN_THIS: return "THIS";
        case TOKEN_IMPORT: return "IMPORT";
        case TOKEN_EXPORT: return "EXPORT";
        case TOKEN_TRUE: return "TRUE";
        case TOKEN_FALSE: return "FALSE";
        case TOKEN_NULL_KW: return "NULL_KW";
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_STAR: return "STAR";
        case TOKEN_SLASH: return "SLASH";
        case TOKEN_PERCENT: return "PERCENT";
        case TOKEN_POWER: return "POWER";
        case TOKEN_AND: return "AND";
        case TOKEN_OR: return "OR";
        case TOKEN_NOT: return "NOT";
        case TOKEN_EQ: return "EQ";
        case TOKEN_NE: return "NE";
        case TOKEN_LT: return "LT";
        case TOKEN_GT: return "GT";
        case TOKEN_LE: return "LE";
        case TOKEN_GE: return "GE";
        case TOKEN_ASSIGN: return "ASSIGN";
        case TOKEN_PLUS_ASSIGN: return "PLUS_ASSIGN";
        case TOKEN_MINUS_ASSIGN: return "MINUS_ASSIGN";
        case TOKEN_STAR_ASSIGN: return "STAR_ASSIGN";
        case TOKEN_SLASH_ASSIGN: return "SLASH_ASSIGN";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_COLON: return "COLON";
        case TOKEN_DOT: return "DOT";
        case TOKEN_ARROW: return "ARROW";
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}
