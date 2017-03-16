# PIL
重读Programming in Lua，把不太了解或重要或难理解的知识点记录下，日后查看。


lua调用c库：
1. 编写代码
2. gcc mylib.c -fPIC -shared -o mylib.so -I /usr/include/lua5.3/		(用的lua5.3，在/usr/include目录下)
3. 切换到lua目录下，运行 lua c.lua