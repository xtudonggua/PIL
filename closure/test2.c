
// 引用skynet中lua-profile.c，说明upvalue的妙处。
// 以lresume(c函数)为例，将lresume注册给lua调用，然后设置3个upavalue，第一个是table {[co] = start_time}(协程的开始时间)
// 第二个是table {[co] = total_time}(协程运行已经消耗的累计时间)，第三个是function，对应lua自带的coroutine.resume
// lresume执行过程：设置start_time，然后调用lua的coroutine.resume

#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>

#include <time.h>

#if defined(__APPLE__)
#include <mach/task.h>
#include <mach/mach.h>
#endif

#define NANOSEC 1000000000
#define MICROSEC 1000000

// #define DEBUG_LOG

static double
get_time() {	// 获取时间
#if  !defined(__APPLE__)	// 不是苹果平台
	struct timespec ti;
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ti);

	int sec = ti.tv_sec & 0xffff;
	int nsec = ti.tv_nsec;

	return (double)sec + (double)nsec / NANOSEC;	
#else
	struct task_thread_times_info aTaskInfo;
	mach_msg_type_number_t aTaskInfoCount = TASK_THREAD_TIMES_INFO_COUNT;
	if (KERN_SUCCESS != task_info(mach_task_self(), TASK_THREAD_TIMES_INFO, (task_info_t )&aTaskInfo, &aTaskInfoCount)) {
		return 0;
	}

	int sec = aTaskInfo.user_time.seconds & 0xffff;
	int msec = aTaskInfo.user_time.microseconds;

	return (double)sec + (double)msec / MICROSEC;
#endif
}

static inline double 
diff_time(double start) {	// 时间差
	double now = get_time();
	if (now < start) {
		return now + 0x10000 - start;
	} else {
		return now - start;
	}
}

// edit by lxd
static int
ltime(lua_State *L) {
	double t;
#if !defined(__APPLE__)
	struct timespec ti;
	clock_gettime(CLOCK_REALTIME, &ti);
	t = (double)ti.tv_sec;
	t += (double)ti.tv_nsec / NANOSEC;
#endif
	lua_pushnumber(L, t);
	return 1;
}
////

// 当前协程计时开始
// 初始化启动时间(start time)为当前时间
// 运行累计时间(total time)为0
static int
lstart(lua_State *L) {
	if (lua_gettop(L) != 0) {
		lua_settop(L,1);
		luaL_checktype(L, 1, LUA_TTHREAD);
	} else {
		lua_pushthread(L);		// 当前协程co压入栈
	}
	lua_pushvalue(L, 1);	// 作为一个副本入栈，push coroutine 确保co在栈顶
	lua_rawget(L, lua_upvalueindex(2));		// 获取第2个upvalue，即B[co]，运行累计时间
	if (!lua_isnil(L, -1)) {	// 每个co，只能profile start一次
		return luaL_error(L, "Thread %p start profile more than once", lua_topointer(L, 1));
	}
	lua_pushvalue(L, 1);	// push coroutine 确保co在栈顶
	lua_pushnumber(L, 0); 	// 入栈
	lua_rawset(L, lua_upvalueindex(2));	// B[co] = 0

	lua_pushvalue(L, 1);	// push coroutine
	double ti = get_time();	// 当前时间
#ifdef DEBUG_LOG
	fprintf(stderr, "PROFILE [%p] start\n", L);
#endif
	lua_pushnumber(L, ti);	// 入栈
	lua_rawset(L, lua_upvalueindex(1));	// A[co] = 当前时间

	return 0;
}

// 当前co计时结束
// ti = B[co] + now_time - A[co]
// A[co] = nil
// B[co] = nil
// return ti
static int
lstop(lua_State *L) {
	if (lua_gettop(L) != 0) {
		lua_settop(L,1);
		luaL_checktype(L, 1, LUA_TTHREAD);
	} else {
		lua_pushthread(L);	// 当前co入栈
	}
	lua_pushvalue(L, 1);	// push coroutine 确保co在栈顶
	lua_rawget(L, lua_upvalueindex(1));
	if (lua_type(L, -1) != LUA_TNUMBER) {	// 没有启动时间
		return luaL_error(L, "Call profile.start() before profile.stop()");
	} 
	double ti = diff_time(lua_tonumber(L, -1));		// ti = now_time - A[co]
	lua_pushvalue(L, 1);	// push coroutine
	lua_rawget(L, lua_upvalueindex(2));	// 获取B[co]
	double total_time = lua_tonumber(L, -1);

	lua_pushvalue(L, 1);	// push coroutine
	lua_pushnil(L);
	lua_rawset(L, lua_upvalueindex(1));		// A[co] = nil

	lua_pushvalue(L, 1);	// push coroutine
	lua_pushnil(L);
	lua_rawset(L, lua_upvalueindex(2));		// B[co] = nil

	total_time += ti;
	lua_pushnumber(L, total_time);	// 压入返回结果
#ifdef DEBUG_LOG
	fprintf(stderr, "PROFILE [%p] stop (%lf / %lf)\n", L, ti, total_time);
#endif

	return 1;
}

// 恢复协程
// 需要重置启动时间
static int
timing_resume(lua_State *L) {
#ifdef DEBUG_LOG
	lua_State *from = lua_tothread(L, -1);
#endif
	lua_rawget(L, lua_upvalueindex(2));		// 获取B[co]，然后将B[co]入栈
	if (lua_isnil(L, -1)) {		// 如果没有total_time, 代表该co没有使用profile记录时间
		lua_pop(L,1);
	} else {
		lua_pop(L,1);	// 将B[co]出栈
		lua_pushvalue(L,1);
		double ti = get_time();	// 当前时间
#ifdef DEBUG_LOG
		fprintf(stderr, "PROFILE [%p] resume\n", from);
#endif
		lua_pushnumber(L, ti);
		lua_rawset(L, lua_upvalueindex(1));	// set start time 设置A[co] = ti
	}

	lua_CFunction co_resume = lua_tocfunction(L, lua_upvalueindex(3));	// 获取Lua原生的resume

	return co_resume(L);	// 调用coroutine.resume
}

static int
lresume(lua_State *L) {
	lua_pushvalue(L,1);
	
	return timing_resume(L);
}

static int
lresume_co(lua_State *L) {
	luaL_checktype(L, 2, LUA_TTHREAD);
	lua_rotate(L, 2, -1);

	return timing_resume(L);
}

// 挂起协程
// 计算当前消耗的时间，然后加到total_time内
static int
timing_yield(lua_State *L) {
#ifdef DEBUG_LOG
	lua_State *from = lua_tothread(L, -1);
#endif
	lua_rawget(L, lua_upvalueindex(2));	// check total time, 获取B[co]，并入栈
	if (lua_isnil(L, -1)) {
		lua_pop(L,1);
	} else {
		double ti = lua_tonumber(L, -1);	// B[co]转化成整数
		lua_pop(L,1); // B[co]出栈

		lua_pushthread(L);
		lua_rawget(L, lua_upvalueindex(1));	// 获取A[co]，并入栈
		double starttime = lua_tonumber(L, -1);  // 启动时间转化成整数
		lua_pop(L,1); // A[co]出栈

		double diff = diff_time(starttime);	// 获取运行消耗时间
		ti += diff; // 加上原有的运行时间
#ifdef DEBUG_LOG
		fprintf(stderr, "PROFILE [%p] yield (%lf/%lf)\n", from, diff, ti);
#endif

		lua_pushthread(L);
		lua_pushnumber(L, ti);	// 压入经历过的时间
		lua_rawset(L, lua_upvalueindex(2));		// 设置到B[co] = ti
	}

	lua_CFunction co_yield = lua_tocfunction(L, lua_upvalueindex(3));  // 获取lua原生的yield

	return co_yield(L);		// 执行coroutine.yield
}

static int
lyield(lua_State *L) {
	lua_pushthread(L);

	return timing_yield(L);
}

static int
lyield_co(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTHREAD);
	lua_rotate(L, 1, -1);
	
	return timing_yield(L);
}

int
luaopen_profile(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg reg[] = {
		{ "time", ltime }, // edit by lxd
		{ "start", lstart },
		{ "stop", lstop },
		{ "resume", lresume },
		{ "yield", lyield },
		{ "resume_co", lresume_co },
		{ "yield_co", lyield_co },
		{ NULL, NULL },
	};
	luaL_newlibtable(L,l);	// 创建一个table，并注册reg中所有函数，假设为lib_table
	lua_newtable(L);	// 创建一个table, 开始时间，假设是A
	lua_newtable(L);	// 创建一个table, 运行累计时间，假设是B

	lua_newtable(L);	// 创建一个weak table
	lua_pushliteral(L, "kv");
	lua_setfield(L, -2, "__mode");

	lua_pushvalue(L, -1);	// 复制weak table
	/* 栈结构
		weak_table	-- 栈顶
		weak_table
		B
		A
		lib_table	-- 栈底
	*/

	lua_setmetatable(L, -3);	// 设置B的元表是weak table，即setmetatable(B, {__mode = "kv"})
	lua_setmetatable(L, -3);	// 设置A的元表是weak table，即setmetatable(A, {__mode = "kv"})

	lua_pushnil(L);		// 压入nil值，接下来会设置成resume或yield
	luaL_setfuncs(L,l,3);	// 将reg中的所有函数设置3个upvalue(A, B, nil)，upvalue是共享的
	/* 栈结构
		lib_table
	*/

	int libtable = lua_gettop(L);	// libtable = 1

	lua_getglobal(L, "coroutine");	// 获取Lua自带resume, yield等函数，将设置成reg中注册各个函数的第3个upvalue
	lua_getfield(L, -1, "resume"); // 获取coroutine.resume并压栈，实际是一个resume

	lua_CFunction co_resume = lua_tocfunction(L, -1);	// coroutine.resume函数转换成c函数co_resume
	if (co_resume == NULL)
		return luaL_error(L, "Can't get coroutine.resume");
	lua_pop(L,1);	// coroutine.resume出栈

	lua_getfield(L, libtable, "resume");	// 获取libtable的resume函数
	lua_pushcfunction(L, co_resume);	// co_resume入栈
	lua_setupvalue(L, -2, 3);	// 设置第3个upvalue(本来是nil)为co_resume
	lua_pop(L,1);	// co_resume出栈

	lua_getfield(L, libtable, "resume_co");	// 同resume
	lua_pushcfunction(L, co_resume);
	lua_setupvalue(L, -2, 3);
	lua_pop(L,1);

	lua_getfield(L, -1, "yield");	// 同resume

	lua_CFunction co_yield = lua_tocfunction(L, -1);
	if (co_yield == NULL)
		return luaL_error(L, "Can't get coroutine.yield");
	lua_pop(L,1);

	lua_getfield(L, libtable, "yield");
	lua_pushcfunction(L, co_yield);
	lua_setupvalue(L, -2, 3);
	lua_pop(L,1);

	lua_getfield(L, libtable, "yield_co");	// 同resume
	lua_pushcfunction(L, co_yield);
	lua_setupvalue(L, -2, 3);
	lua_pop(L,1);

	lua_settop(L, libtable);	// 设置栈顶, 栈中只有一个libtable

	return 1;
}