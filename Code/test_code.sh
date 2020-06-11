make -f GDBMakefile clean
make -f GDBMakefile parser

# ./parser /data/compilers-tests/tests/ext1.cmm
# ./parser ../Test/test1.cmm
# gdb --args ./parser /data/compilers-tests/tests/impossible1.cmm ./out.s

gdb --args ./parser ../Test/ttt1.cmm ./out.s

# ser args "/data/compilers-tests/tests/ext5.cmm"