include web_server/dirs.mk
include build_toolzzz/compile.mk

SOURCES :=  $(subst ./,,$(shell find src -name "*.cpp"))

all: $(OBJS)