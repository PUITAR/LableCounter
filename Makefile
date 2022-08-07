include ../make_config.mk

ifndef DISABLE_JEMALLOC
	ifdef JEMALLOC
		PLATFORM_CXXFLAGS += -DROCKSDB_JEMALLOC -DJEMALLOC_NO_DEMANGLE
	endif
	EXEC_LDFLAGS := $(JEMALLOC_LIB) $(EXEC_LDFLAGS) -lpthread
	PLATFORM_CXXFLAGS += $(JEMALLOC_INCLUDE)
endif

ifneq ($(USE_RTTI), 1)
	CXXFLAGS += -fno-rtti
endif

CFLAGS += -Wstrict-prototypes

.PHONY: clean librocksdb

src_main = src/main.cc src/lable_db_engine.cc
src_lable = src/lable_generator.cc src/lable_db_engine.cc

main: librocksdb
	$(CXX) $(CXXFLAGS) $(src_main) -o main ../librocksdb.a -I../include -I./inc -O2 -std=c++17 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS) -g

lable: librocksdb
	$(CXX) $(CXXFLAGS) $(src_lable) -o lables ../librocksdb.a -I../include -I./inc -O2 -std=c++17 $(PLATFORM_LDFLAGS) $(PLATFORM_CXXFLAGS) $(EXEC_LDFLAGS) -g

librocksdb:
	cd .. && $(MAKE) static_lib

wiki = wiki-topcats.txt wiki-topcats-lable.txt


ready:
	./lables $(wiki)


do:
	./main $(wiki) > record/wiki-topcats-record.txt
	



