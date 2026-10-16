# Host (Linux/WSL) build of the calculator core and the upstream test suites.
# Verifies the ported core against mate-calc's own expected results before it
# is cross-compiled for HarmonyOS.
#
#   make DEPS=$HOME/hmdeps/root test
#
# DEPS points to a root containing gmp/mpfr/mpc headers and libraries
# (default: system paths).

DEPS ?=
CC ?= gcc
CORE = ../core
COMPAT = ../compat
ENGINE = ../engine

CPPFLAGS = -I$(COMPAT) -I$(CORE) -I$(ENGINE)
ifneq ($(DEPS),)
CPPFLAGS += -I$(DEPS)/usr/include -I$(DEPS)/usr/include/x86_64-linux-gnu
LDFLAGS += -L$(DEPS)/usr/lib/x86_64-linux-gnu
endif
CFLAGS ?= -O1 -g -std=gnu11 -Wall -Wno-unused-parameter
LDLIBS = -Wl,-Bstatic -lmpc -lmpfr -lgmp -Wl,-Bdynamic -lm

CORE_SRC = $(addprefix $(CORE)/, mp.c mp-binary.c mp-convert.c mp-trigonometric.c \
	mp-equation.c mp-serializer.c parser.c parserfunc.c lexer.c prelexer.c \
	financial.c unit.c unit-category.c unit-manager.c currency.c currency-manager.c \
	math-variables.c) $(COMPAT)/glib-compat.c $(ENGINE)/calc-engine.c

BUILD = build
CORE_OBJ = $(patsubst %.c,$(BUILD)/%.o,$(notdir $(CORE_SRC)))

vpath %.c $(CORE) $(COMPAT) $(ENGINE) .

all: $(BUILD)/test-mp $(BUILD)/test-mp-equation $(BUILD)/test-port $(BUILD)/test-engine

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/test-mp: $(BUILD)/test-mp.o $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/test-mp-equation: $(BUILD)/test-mp-equation.o $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/test-port: $(BUILD)/test-port.o $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/test-engine: $(BUILD)/test-engine.o $(CORE_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

test: all
	./$(BUILD)/test-mp
	./$(BUILD)/test-mp-equation
	./$(BUILD)/test-port
	./$(BUILD)/test-engine

clean:
	rm -rf $(BUILD)

.PHONY: all test clean
