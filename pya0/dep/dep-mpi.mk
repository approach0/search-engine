CFLAGS  += $(shell mpicc --showme:compile)
LDFLAGS += $(shell mpicc --showme:link)
