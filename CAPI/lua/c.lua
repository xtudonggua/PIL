
-- 直接将生成.so文件，放到统一目录下，或根据自己的目录结构添加路径
package.cpath = package.cpath .. ";../c/?.so"

local mylib = require("mylib")

print(mylib.add(1.5, 2.8))
print(mylib.sub(3.18, 1.21))