# Makefile 规则
# -fPIC -shared 编译成位置无关代码(Position-Independent Code)的共享库
# -I 指定头文件目录
# -o 输出文件名称
#  
# -L 指定动态库目录,注意在-o之后,被依赖库应当在后面
# -l 指定动态库文件,注意在-o之后,且省略lib前缀和.so后缀,例如链接当前库需要写为:-Llib -lijk,或者lib/libijk.so指定全路径
# -Wl 表示将逗号后面的参数传递给链接器(linker)，多个参数用,分开
#

