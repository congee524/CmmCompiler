make -f GDBMakefile clean
make -f GDBMakefile parser

# ./parser /data/compilers-tests/tests/ext1.cmm
# ./parser ../Test/test1.cmm
gdb --args ./parser /data/compilers-tests/tests/yzy18.cmm ./out.ir

# gdb --args ./parser ../Test/ttt1.cmm ./out.ir

# ser args "/data/compilers-tests/tests/ext5.cmm"