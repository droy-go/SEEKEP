/*
 * SEEKEP - Smart Expressive Efficient Knowledge Execution Platform
 * الملف الرئيسي - Main Entry Point
 * 
 * نقطة الدخول الرئيسية لمفسر SEEKEP
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vm.h"
#include "lexer.h"
#include "parser.h"
#include "compiler.h"

#define VERSION "1.0.0"
#define MAX_INPUT_SIZE 65536

/* عرض المساعدة */
static void print_help(const char* program) {
    printf("SEEKEP - لغة برمجة عربية عالية المستوى\n");
    printf("الإصدار: %s\n\n", VERSION);
    printf("الاستخدام: %s [خيارات] [ملف]\n\n", program);
    printf("الخيارات:\n");
    printf("  -h, --help        عرض هذه المساعدة\n");
    printf("  -v, --version     عرض الإصدار\n");
    printf("  -c, --compile     ترجمة فقط (بدون تنفيذ)\n");
    printf("  -d, --debug       وضع التصحيح\n");
    printf("  -i, --interactive وضع تفاعلي\n");
    printf("  -o, --output      ملف الإخراج\n");
    printf("  -a, --ast         طباعة شجرة البنية المجردة\n");
    printf("  -b, --bytecode    طباعة البايتكود\n");
    printf("\n");
    printf("الأمثلة:\n");
    printf("  %s برنامج.سكيب          تشغيل ملف SEEKEP\n", program);
    printf("  %s -i                   وضع تفاعلي\n", program);
    printf("  %s -c برنامج.سكيب       ترجمة فقط\n", program);
    printf("  %s -a برنامج.سكيب       طباعة AST\n", program);
    printf("  %s -b برنامج.سكيب       طباعة البايتكود\n", program);
}

/* عرض الإصدار */
static void print_version(void) {
    printf("SEEKEP %s\n", VERSION);
    printf("لغة برمجة عربية عالية المستوى\n");
    printf("Smart Expressive Efficient Knowledge Execution Platform\n");
    printf("Copyright (c) 2024 SEEKEP Project\n");
    printf("مفتوحة المصدر تحت رخصة MIT\n");
}

/* قراءة ملف */
static char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "خطأ: لا يمكن فتح الملف '%s'\n", path);
        return NULL;
    }
    
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);
    
    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fprintf(stderr, "خطأ: فشل في تخصيص الذاكرة\n");
        fclose(file);
        return NULL;
    }
    
    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        fprintf(stderr, "خطأ: فشل في قراءة الملف\n");
        free(buffer);
        fclose(file);
        return NULL;
    }
    
    buffer[bytes_read] = '\0';
    fclose(file);
    return buffer;
}

/* كتابة بايتكود إلى ملف */
static int write_bytecode(chunk_t* chunk, const char* path) {
    FILE* file = fopen(path, "wb");
    if (!file) {
        fprintf(stderr, "خطأ: لا يمكن إنشاء الملف '%s'\n", path);
        return 0;
    }
    
    /* كتابة التوقيع والإصدار */
    fwrite("SKPB", 1, 4, file);
    uint8_t version[3] = {1, 0, 0};
    fwrite(version, 1, 3, file);
    
    /* كتابة حجم البايتكود */
    fwrite(&chunk->count, sizeof(size_t), 1, file);
    fwrite(chunk->code, 1, chunk->count, file);
    
    /* كتابة الثوابت */
    fwrite(&chunk->constant_count, sizeof(size_t), 1, file);
    for (size_t i = 0; i < chunk->constant_count; i++) {
        constant_t* c = &chunk->constants[i];
        fwrite(&c->type, sizeof(skp_type_t), 1, file);
        
        switch (c->type) {
            case SKP_TYPE_INT:
                fwrite(&c->value.int_val, sizeof(skp_int), 1, file);
                break;
            case SKP_TYPE_FLOAT:
                fwrite(&c->value.float_val, sizeof(skp_float), 1, file);
                break;
            case SKP_TYPE_STRING:
                size_t len = strlen(c->value.string_val);
                fwrite(&len, sizeof(size_t), 1, file);
                fwrite(c->value.string_val, 1, len, file);
                break;
            default:
                break;
        }
    }
    
    fclose(file);
    return 1;
}

/* تشغيل ملف */
static int run_file(skp_vm_t* vm, const char* path, int debug, int compile_only, 
                    const char* output_path, int print_ast, int print_bytecode) {
    char* source = read_file(path);
    if (!source) return 1;
    
    /* إنشاء الليكسر */
    lexer_t* lexer = lexer_create(source);
    if (!lexer) {
        free(source);
        return 1;
    }
    
    /* إنشاء المحلل */
    parser_t* parser = parser_create(lexer);
    if (!parser) {
        lexer_destroy(lexer);
        free(source);
        return 1;
    }
    
    /* تحليل البرنامج */
    ast_node_t* ast = parser_parse(parser);
    
    if (parser->had_error) {
        fprintf(stderr, "خطأ في التحليل اللغوي\n");
        ast_destroy_node(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        free(source);
        return 1;
    }
    
    /* طباعة AST إذا طُلب */
    if (print_ast) {
        printf("=== شجرة البنية المجردة ===\n");
        ast_print(ast, 0);
        printf("\n");
    }
    
    /* إنشاء المترجم */
    skp_compiler_t* compiler = compiler_create(parser);
    if (!compiler) {
        ast_destroy_node(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        free(source);
        return 1;
    }
    
    /* الترجمة */
    chunk_t* chunk = compiler_compile(compiler, ast);
    
    if (!chunk || compiler->had_error) {
        fprintf(stderr, "خطأ في الترجمة\n");
        if (chunk) {
            chunk_free(chunk);
            free(chunk);
        }
        compiler_destroy(compiler);
        ast_destroy_node(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        free(source);
        return 1;
    }
    
    /* طباعة البايتكود إذا طُلب */
    if (print_bytecode) {
        printf("=== البايتكود ===\n");
        chunk_disassemble(chunk, path);
        printf("\n");
    }
    
    /* ترجمة فقط */
    if (compile_only) {
        const char* out_path = output_path ? output_path : "output.skpbc";
        if (write_bytecode(chunk, out_path)) {
            printf("تم حفظ البايتكود في: %s\n", out_path);
        }
        
        chunk_free(chunk);
        free(chunk);
        compiler_destroy(compiler);
        ast_destroy_node(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        free(source);
        return 0;
    }
    
    /* التشغيل */
    skp_result_t result = vm_run(vm, chunk);
    
    /* تنظيف */
    chunk_free(chunk);
    free(chunk);
    compiler_destroy(compiler);
    ast_destroy_node(ast);
    parser_destroy(parser);
    lexer_destroy(lexer);
    free(source);
    
    if (result == SKP_RUNTIME_ERROR) {
        fprintf(stderr, "خطأ زمني\n");
        return 1;
    }
    
    return 0;
}

/* الوضع التفاعلي */
static void run_repl(skp_vm_t* vm) {
    printf("SEEKEP %s - الوضع التفاعلي\n", VERSION);
    printf("اكتب 'خروج' للخروج\n\n");
    
    char input[MAX_INPUT_SIZE];
    
    while (1) {
        printf("سكيب> ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            printf("\n");
            break;
        }
        
        /* إزالة السطر الجديد */
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }
        
        /* التحقق من الخروج */
        if (strcmp(input, "خروج") == 0 || strcmp(input, "exit") == 0) {
            printf("مع السلامة!\n");
            break;
        }
        
        /* تنفيذ */
        skp_result_t result = vm_interpret(vm, input);
        
        if (result == SKP_COMPILE_ERROR) {
            printf("خطأ في الترجمة\n");
        } else if (result == SKP_RUNTIME_ERROR) {
            printf("خطأ زمني\n");
        }
        
        vm_reset(vm);
    }
}

/* الدالة الرئيسية */
int main(int argc, char* argv[]) {
    /* الخيارات الافتراضية */
    int debug = 0;
    int compile_only = 0;
    int interactive = 0;
    int print_ast = 0;
    int print_bytecode = 0;
    char* output_path = NULL;
    char* input_file = NULL;
    
    /* معالجة الخيارات */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }
        
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        }
        
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            debug = 1;
            continue;
        }
        
        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--compile") == 0) {
            compile_only = 1;
            continue;
        }
        
        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interactive") == 0) {
            interactive = 1;
            continue;
        }
        
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--ast") == 0) {
            print_ast = 1;
            continue;
        }
        
        if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--bytecode") == 0) {
            print_bytecode = 1;
            continue;
        }
        
        if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) {
                output_path = argv[++i];
            }
            continue;
        }
        
        /* الملف المدخل */
        if (argv[i][0] != '-') {
            input_file = argv[i];
            continue;
        }
        
        fprintf(stderr, "خيار غير معروف: %s\n", argv[i]);
        print_help(argv[0]);
        return 1;
    }
    
    /* إنشاء الجهاز الافتراضي */
    skp_vm_t* vm = vm_create();
    if (!vm) {
        fprintf(stderr, "خطأ: فشل في إنشاء الجهاز الافتراضي\n");
        return 1;
    }
    
    int result = 0;
    
    /* تشغيل الملف أو الوضع التفاعلي */
    if (input_file) {
        result = run_file(vm, input_file, debug, compile_only, 
                         output_path, print_ast, print_bytecode);
    } else if (interactive || argc == 1) {
        run_repl(vm);
    } else {
        print_help(argv[0]);
        result = 1;
    }
    
    /* تنظيف */
    vm_destroy(vm);
    
    return result;
}
