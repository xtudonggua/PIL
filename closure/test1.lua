
-- 测试upvalue
--[[
	debug.getupvalue顺序由函数体中用到决定的(如果没用到，则不会存在)，而不是变量声明顺序
]]
function foo()
	local count = 0
	local a = {}
	local b = {}
	local c = "abc"
	return function ()
		b[count] = count ^ 3
		a[count] = count ^ 2
		count = count + 1
	end
end

local f = foo()
f()
f()
f()

local i = 1
while true do
	local n, v = debug.getupvalue(f, i)
	if not n then break end
	print(i, n,	v, ";")		-- 1 b table; 2	count 3; 3 a table
	i = i + 1
end