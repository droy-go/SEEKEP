/* ============================================
 * SEEKEP Lexer - محلل الرموز
 * ============================================ */

#ifndef SEEKEP_LEXER_H
#define SEEKEP_LEXER_H

#include "seekep.h"

/* أنواع الرموز */
typedef enum {
    /* أنواع البيانات */
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_BOOL,
    TOKEN_NULL,
    
    /* المعرفات */
    TOKEN_IDENTIFIER,
    
    /* الكلمات المفتاحية */
    TOKEN_VAR,           /* متغير */
    TOKEN_CONST,         /* ثابت */
    TOKEN_FUNC,          /* دالة */
    TOKEN_RETURN,        /* أرجع */
    TOKEN_IF,            /* إذا */
    TOKEN_ELSE,          /* وإلا */
    TOKEN_WHILE,         /* أثناء */
    TOKEN_FOR,           /* لكل */
    TOKEN_IN,            /* في */
    TOKEN_BREAK,         /* توقف */
    TOKEN_CONTINUE,      /* استمر */
    TOKEN_CLASS,         /* صنف */
    TOKEN_NEW,           /* جديد */
    TOKEN_THIS,          /* هذا */
    TOKEN_IMPORT,        /* استورد */
    TOKEN_EXPORT,        /* صدر */
    TOKEN_TRUE,          /* صحيح */
    TOKEN_FALSE,         /* خطأ */
    TOKEN_NULL_KW,       /* فارغ */
    
    /* العمليات الحسابية */
    TOKEN_PLUS,          /* + */
    TOKEN_MINUS,         /* - */
    TOKEN_STAR,          /* * */
    TOKEN_SLASH,         /* / */
    TOKEN_PERCENT,       /* % */
    TOKEN_POWER,         /* ** */
    
    /* العمليات المنطقية */
    TOKEN_AND,           /* و */
    TOKEN_OR,            /* أو */
    TOKEN_NOT,           /* ليس */
    
    /* عمليات المقارنة */
    TOKEN_EQ,            /* == */
    TOKEN_NE,            /* != */
    TOKEN_LT,            /* < */
    TOKEN_GT,            /* > */
    TOKEN_LE,            /* <= */
    TOKEN_GE,            /* >= */
    
    /* الإسناد */
    TOKEN_ASSIGN,        /* = */
    TOKEN_PLUS_ASSIGN,   /* += */
    TOKEN_MINUS_ASSIGN,  /* -= */
    TOKEN_STAR_ASSIGN,   /* *= */
    TOKEN_SLASH_ASSIGN,  /* /= */
    
    /* الأقواس */
    TOKEN_LPAREN,        /* ( */
    TOKEN_RPAREN,        /* ) */
    TOKEN_LBRACE,        /* { */
    TOKEN_RBRACE,        /* } */
    TOKEN_LBRACKET,      /* [ */
    TOKEN_RBRACKET,      /* ] */
    
    /* الفواصل */
    TOKEN_COMMA,         /* ، */
    TOKEN_SEMICOLON,     /* ؛ */
    TOKEN_COLON,         /* : */
    TOKEN_DOT,           /* . */
    TOKEN_ARROW,         /* -> */
    
    /* نهاية الملف */
    TOKEN_EOF,
    TOKEN_ERROR
} token_type_t;

/* هيكل الرمز */
typedef struct {
    token_type_t type;
    char* value;
    int line;
    int column;
} token_t;

/* هيكل المعجم */
typedef struct {
    const char* source;
    size_t length;
    size_t position;
    int line;
    int column;
    token_t** tokens;
    size_t token_count;
    size_t token_capacity;
} lexer_t;

/* دوال المعجم */
lexer_t* lexer_create(const char* source);
void lexer_destroy(lexer_t* lexer);
token_t** lexer_tokenize(lexer_t* lexer, size_t* count);

/* دوال الرموز */
token_t* token_create(token_type_t type, const char* value, int line, int column);
void token_destroy(token_t* token);
const char* token_type_name(token_type_t type);

/* دوال مساعدة */
char lexer_current(lexer_t* lexer);
char lexer_peek(lexer_t* lexer, int offset);
void lexer_advance(lexer_t* lexer);
void lexer_skip_whitespace(lexer_t* lexer);
void lexer_skip_comment(lexer_t* lexer);

token_t* lexer_read_number(lexer_t* lexer);
token_t* lexer_read_string(lexer_t* lexer);
token_t* lexer_read_identifier(lexer_t* lexer);
token_t* lexer_read_operator(lexer_t* lexer);

token_type_t lexer_keyword_type(const char* word);

#endif /* SEEKEP_LEXER_H */
