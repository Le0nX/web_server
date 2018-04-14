include web_server/dirs.mk
include build_toolzzz/compile.mk

SOURCES :=  $(subst ./,,$(shell find web_server -name "*.cpp"))

all: $(OBJS)