# OSS-Fuzz Integration - Local Testing Guide

This guide explains how to build, run, and test fuzzers locally using OSS-Fuzz.  
Fuzzers and seed generator can be built using `make all` in this repository.

## Prerequisites

- Docker
- Python 3
- Git
- Clang
- LLVM

~~~ sh
git clone https://github.com/Segwaz/oss-fuzz.git -b libdbmx
cd oss-fuzz
~~~

## 1. Build image

~~~sh
python3 infra/helper.py build_image libmdbx
~~~
## 1. Build fuzzers

~~~ sh
python3 infra/helper.py build_fuzzers libmdbx
~~~
## 2. Run a fuzzer

~~~ sh
python3 infra/helper.py run_fuzzer libmdbx fuzz_raw_db_format
~~~

## 3. Reproduce

Run the fuzzer in this directory with the testcase:

~~~sh
./fuzz_raw_db_format crash-XXXXXX
~~~

You may need to adjust sanitizers in the Makefile.

Debug output can be seen by setting MDBX_FUZZ_DEBUG in fuzz.h.

## Generate seeds

Run the seed generator in this directory:


~~~sh
./db_seed_gen [corpus_pathname]
~~~
Seeds are in ./corpus by default. To integrate with OSS-Fuzz they must be placed in a zip file named <fuzzer_name>_seed_corpus.zip placed in ./fuzz/seed directory.