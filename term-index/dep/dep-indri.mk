CFLAGS += -I "$(INDRI)/include/" -DP_NEEDS_GNU_CXX_NAMESPACE=1
LDFLAGS += -L "$(INDRI)/obj/"
OTHER_MERGE_AR += "$(INDRI)/obj/libindri.a"
