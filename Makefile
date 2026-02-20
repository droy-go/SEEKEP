#
# SEEKEP - Smart Expressive Efficient Knowledge Execution Platform
# ملف البناء - Makefile
#

# المترجم والخيارات
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -fPIC
DEBUG_CFLAGS = -g -O0 -DDEBUG_TRACE_EXECUTION
LDFLAGS = -lm

# الأسماء
TARGET = seekep
LIBRARY = libseekep.a
SHARED = libseekep.so

# المجلدات
SRCDIR = المصدر
OBJDIR = كائنات
BINDIR = bin
LIBDIR = lib
EXAMPLEDIR = أمثلة
TESTDIR = اختبارات

# الملفات المصدرية
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

# رؤوس
HEADERS = $(wildcard $(SRCDIR)/*.h)

# الأهداف الافتراضية
.PHONY: all clean debug install uninstall test examples

all: directories $(BINDIR)/$(TARGET) $(LIBDIR)/$(LIBRARY)

directories:
	@mkdir -p $(OBJDIR) $(BINDIR) $(LIBDIR) $(EXAMPLEDIR) $(TESTDIR)

# بناء الملف التنفيذي
$(BINDIR)/$(TARGET): $(OBJECTS)
	@echo "بناء المفسر: $@"
	@$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# بناء المكتبة الثابتة
$(LIBDIR)/$(LIBRARY): $(OBJECTS)
	@echo "بناء المكتبة الثابتة: $@"
	@ar rcs $@ $(OBJECTS)

# بناء المكتبة المشتركة
shared: $(LIBDIR)/$(SHARED)

$(LIBDIR)/$(SHARED): $(OBJECTS)
	@echo "بناء المكتبة المشتركة: $@"
	@$(CC) -shared $(OBJECTS) -o $@ $(LDFLAGS)

# تجميع الملفات المصدرية
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@echo "تجميع: $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# وضع التصحيح
debug: CFLAGS = $(DEBUG_CFLAGS)
debug: clean all

# التنظيف
clean:
	@echo "تنظيف..."
	@rm -rf $(OBJDIR) $(BINDIR) $(LIBDIR)
	@rm -f *.skpbc

# التثبيت
PREFIX ?= /usr/local
INSTALL_BINDIR = $(PREFIX)/bin
INSTALL_LIBDIR = $(PREFIX)/lib
INSTALL_INCDIR = $(PREFIX)/include/seekep

install: all
	@echo "تثبيت SEEKEP..."
	@mkdir -p $(INSTALL_BINDIR) $(INSTALL_LIBDIR) $(INSTALL_INCDIR)
	@cp $(BINDIR)/$(TARGET) $(INSTALL_BINDIR)/
	@cp $(LIBDIR)/$(LIBRARY) $(INSTALL_LIBDIR)/
	@cp $(SRCDIR)/*.h $(INSTALL_INCDIR)/
	@echo "تم التثبيت بنجاح!"

uninstall:
	@echo "إلغاء تثبيت SEEKEP..."
	@rm -f $(INSTALL_BINDIR)/$(TARGET)
	@rm -f $(INSTALL_LIBDIR)/$(LIBRARY)
	@rm -rf $(INSTALL_INCDIR)
	@echo "تم إلغاء التثبيت!"

# تشغيل الاختبارات
test: all
	@echo "تشغيل الاختبارات..."
	@for test in $(TESTDIR)/*.سكيب; do \
		if [ -f "$$test" ]; then \
			echo "اختبار: $$test"; \
			$(BINDIR)/$(TARGET) "$$test"; \
		fi \
	done

# بناء الأمثلة
examples: all
	@echo "بناء الأمثلة..."
	@for example in $(EXAMPLEDIR)/*.سكيب; do \
		if [ -f "$$example" ]; then \
			echo "بناء: $$example"; \
			$(BINDIR)/$(TARGET) -c "$$example" -o "$$example.bc"; \
		fi \
	done

# معلومات البناء
info:
	@echo "SEEKEP Build Information"
	@echo "========================"
	@echo "CC: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "SOURCES: $(SOURCES)"
	@echo "OBJECTS: $(OBJECTS)"

# مساعدة
help:
	@echo "SEEKEP - نظام البناء"
	@echo "===================="
	@echo ""
	@echo "الأهداف المتاحة:"
	@echo "  all       - بناء المفسر والمكتبة (افتراضي)"
	@echo "  debug     - بناء مع معلومات التصحيح"
	@echo "  shared    - بناء المكتبة المشتركة"
	@echo "  clean     - تنظيف ملفات البناء"
	@echo "  install   - تثبيت SEEKEP على النظام"
	@echo "  uninstall - إلغاء تثبيت SEEKEP"
	@echo "  test      - تشغيل الاختبارات"
	@echo "  examples  - بناء الأمثلة"
	@echo "  info      - عرض معلومات البناء"
	@echo "  help      - عرض هذه المساعدة"
	@echo ""
	@echo "المتغيرات:"
	@echo "  PREFIX    - مسار التثبيت (افتراضي: /usr/local)"
	@echo "  CC        - المترجم (افتراضي: gcc)"
	@echo "  CFLAGS    - خيارات الترجمة"
	@echo "  LDFLAGS   - خيارات الربط"
