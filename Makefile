FUZZ_DB_SRCS := fuzz_raw_db_format.c logger.c mode_desc.c
FUZZ_DB_OBJS := $(addprefix obj/fuzz/,$(FUZZ_DB_SRCS:.c=.o))

FUZZ_API_SRCS_C   := logger.c mode_desc.c
FUZZ_API_SRCS_CXX := fuzz_api.cc ops.cc
FUZZ_API_OBJS     := $(addprefix obj/fuzz/,$(FUZZ_API_SRCS_C:.c=.o)) \
                     $(addprefix obj/fuzz/,$(FUZZ_API_SRCS_CXX:.cc=.o))

DB_GEN_SRCS := utils/raw_db_gen.c utils/common.c mode_desc.c
DB_GEN_OBJS := $(addprefix obj/dbgen/,$(DB_GEN_SRCS:.c=.o))

API_GEN_SRCS := utils/api_seed_gen.c utils/common.c mode_desc.c
API_GEN_OBJS := $(addprefix obj/dbgen/,$(API_GEN_SRCS:.c=.o))

LIBMDBX        := ./libmdbx
LIBMDBX_NORMAL := obj/libmdbx.normal.a
LIBMDBX_FUZZ   := obj/libmdbx.fuzz.a

CC          := clang
CXX         := clang++
CFLAGS      := -Wall -Wextra
CXXFLAGS    := -Wall -Wextra
LIBFUZZER   := -fsanitize=fuzzer,address -fno-omit-frame-pointer -ggdb
INCLUDE     := -I. -I$(LIBMDBX)

LPM_SRC      := libprotobuf-mutator
LPM_BUILD    := obj/lpm-build
LPM_PROTO    := $(LPM_BUILD)/external.protobuf
LPM_FUZZER_A := $(LPM_BUILD)/src/libfuzzer/libprotobuf-mutator-libfuzzer.a

PROTOC     := $(LPM_PROTO)/bin/protoc
PROTO_SRC  := fuzz_api.proto
PROTO_OUT  := obj/proto
PROTO_CC   := $(PROTO_OUT)/fuzz_api.pb.cc
PROTO_OBJ  := obj/fuzz/fuzz_api.pb.o
PROTO_INC  := -I$(LPM_PROTO)/include

LPM_INCLUDE := -I$(LPM_SRC)
LPM_LIBS    := $(LPM_FUZZER_A) \
               $(LPM_BUILD)/src/libprotobuf-mutator.a

PROTO_LIBS  := -Wl,--start-group \
                 $(LPM_PROTO)/lib/libprotobuf.a \
                 $(LPM_PROTO)/lib/libutf8_range.a \
                 $(LPM_PROTO)/lib/libutf8_validity.a \
                 $(wildcard $(LPM_PROTO)/lib/libabsl_*.a) \
               -Wl,--end-group

all: fuzz_raw_db_format db_seed_gen fuzz_api

$(LIBMDBX_NORMAL):
	@mkdir -p $(dir $@)
	$(MAKE) -C $(LIBMDBX) clean
	$(MAKE) -C $(LIBMDBX) libmdbx.a \
		CC="$(CC)" CFLAGS="$(CFLAGS)" \
		CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)"
	cp $(LIBMDBX)/libmdbx.a $@

$(LIBMDBX_FUZZ):
	@mkdir -p $(dir $@)
	$(MAKE) -C $(LIBMDBX) clean
	$(MAKE) -C $(LIBMDBX) libmdbx.a \
		CC="$(CC)" CFLAGS="$(CFLAGS) $(LIBFUZZER)" \
		CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS) $(LIBFUZZER)"
	cp $(LIBMDBX)/libmdbx.a $@

$(LPM_FUZZER_A):
	@mkdir -p $(LPM_BUILD)
	cmake -S $(LPM_SRC) -B $(LPM_BUILD)\
		-DLIB_PROTO_MUTATOR_TESTING=OFF \
		-DLIB_PROTO_MUTATOR_DOWNLOAD_PROTOBUF=ON \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_COMPILER="$(CC)" \
		-DCMAKE_CXX_COMPILER="$(CXX)" \
		-GNinja
	cd $(LPM_BUILD) && ninja

fuzz_raw_db_format: CFLAGS   += $(LIBFUZZER)
fuzz_raw_db_format: CXXFLAGS += $(LIBFUZZER)
fuzz_raw_db_format: $(LIBMDBX_FUZZ) $(FUZZ_DB_OBJS)
	$(CC) $(CFLAGS) -o $@ $(FUZZ_DB_OBJS) $(LIBMDBX_FUZZ)

db_seed_gen: $(LIBMDBX_NORMAL) $(DB_GEN_OBJS)
	$(CC) $(CFLAGS) -o $@ $(DB_GEN_OBJS) $(LIBMDBX_NORMAL)

$(PROTO_CC): $(PROTO_SRC) | $(LPM_FUZZER_A)
	@mkdir -p $(PROTO_OUT)
	$(PROTOC) --cpp_out=$(PROTO_OUT) $<

$(PROTO_OBJ): $(PROTO_CC) | $(LPM_FUZZER_A)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(PROTO_INC) -I$(PROTO_OUT) -I$(LIBMDBX) -c $< -o $@

$(FUZZ_API_OBJS): $(PROTO_CC) | $(LPM_FUZZER_A)

fuzz_api: CFLAGS   += $(LIBFUZZER)
fuzz_api: CXXFLAGS += $(LIBFUZZER)
fuzz_api: INCLUDE  += -I$(PROTO_OUT) $(LPM_INCLUDE) $(PROTO_INC)
fuzz_api: $(LIBMDBX_FUZZ) $(FUZZ_API_OBJS) $(PROTO_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(FUZZ_API_OBJS) $(PROTO_OBJ) \
		$(LPM_LIBS) $(PROTO_LIBS) $(LIBMDBX_FUZZ)

obj/fuzz/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

obj/fuzz/%.o: %.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

obj/dbgen/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -rf obj/fuzz obj/dbgen obj/proto obj/libmdbx.*.a \
		fuzz_raw_db_format db_seed_gen fuzz_api 

distclean: clean
	rm -rf obj

.PHONY: all clean distclean
