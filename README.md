# PIL
重读Programming in Lua，把不太了解或重要或难理解的知识点记录下，日后查看。


lua调用c库：
1. 编写代码
2. 切换的c目录下，将mylib.c编译成.o文件： gcc -c -fPIC -o mylib.o mylib.c -I /usr/include/lua5.3/		(用的lua5.3，在/usr/include目录下)
3. 将mylib.o编译成.so文件： gcc -shared mylib.so mylib.o
4. 切换到lua目录下，运行 lua c.lua