include ../rules.mk
include ../module.mk

CFLAGS += $(shell python3-config --cflags)
LDFLAGS += $(shell python3-config --ldflags)

all: jieba-wrap.py

jieba-wrap.py: ../txt-seg/jieba-wrap.py
	ln -sf $<

clean:
	@ echo 'clean'
