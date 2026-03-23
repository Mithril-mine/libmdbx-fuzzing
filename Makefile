FUZZ_DB_SRCS := fuzz_raw_db_format.c logger.c mode_desc.c
FUZZ_DB_OBJS := $(addprefix obj/fuzz/,$(FUZZ_DB_SRCS:.c=.o))

DB_GEN_SRCS := utils/raw_db_gen.c mode_desc.c
DB_GEN_OBJS := $(addprefix obj/dbgen/,$(DB_GEN_SRCS:.c=.o))

LIBMDBX := ./libmdbx
LIBMDBX_NORMAL := obj/libmdbx.normal.a
LIBMDBX_FUZZ := obj/libmdbx.fuzz.a

CC := clang
CXX := clang++
CFLAGS := -Wall -Wextra
CXXFLAGS := -Wall -Wextra
LIBFUZZER := -fsanitize=fuzzer -fno-omit-frame-pointer -ggdb
INCLUDE := -I. -I$(LIBMDBX)

all: fuzz_raw_db_format db_seed_gen

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

fuzz_raw_db_format: CFLAGS += $(LIBFUZZER)
fuzz_raw_db_format: CXXFLAGS += $(LIBFUZZER)
fuzz_raw_db_format: $(LIBMDBX_FUZZ) $(FUZZ_DB_OBJS)
	$(CC) $(CFLAGS) -o $@ $(FUZZ_DB_OBJS) $(LIBMDBX_FUZZ)

db_seed_gen: $(LIBMDBX_NORMAL) $(DB_GEN_OBJS)
	$(CC) $(CFLAGS) -o $@ $(DB_GEN_OBJS) $(LIBMDBX_NORMAL)

obj/fuzz/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

obj/dbgen/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -rf obj fuzz_raw_db_format db_seed_gen

.PHONY: all clean
