make -f GDBMakefile clean
make -f GDBMakefile parser

#./parser /data/compilers-tests/tests/ext1.cmm
# ./parser ../Test/ext1.cmm
gdb --args ./parser /data/compilers-tests/tests/ZZ03.cmm

# ser args "/data/compilers-tests/tests/ext5.cmm"