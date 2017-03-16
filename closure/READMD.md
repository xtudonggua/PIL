
我所理解的闭包:
在运行时，每当Lua执行一个function() ... end时，会创建一个新的数据对象，包含函数原型的引用及一个由所有upvalue引用组成的数组，
这个数据对象就是闭包。所以函数是编译器概念(静态的)，闭包是运行期概念(动态的)。
严格来说，lua没有函数，只有闭包。每个闭包包含函数和上值(upvalue)，没有upvalue的闭包称函数。
upvalue既不是局部变量，也不是全局变量。多个闭包如果upvalue相同，会共用同一份堆栈上的upvalue，所以改变一个，另一个也会改变

闭包的应用：
1. 作为高阶函数的参数，比如table.sort()
2. 创建其他函数的函数，即函数返回一个闭包
3. 实现迭代器
4. 最重要的一点，创建一个安全的运行环境，即所谓的"沙盒"
	do
	  local oldOpen = io.open
	  local accessOk = function(filename, mode)
	      <权限访问检查>
	        end
	 
	  io.open = function (filename, mode)
	          if accessOk(filename, mode) then
	              return oldOpen(filename, mode)
	          else
	              return nil, access denied
	          end
	     end
	end



参考: http://www.2cto.com/kf/201503/382691.html