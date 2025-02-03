//////////////////////////////////////////////////////////////// Configurations ////////////////////////////////////////////////////////////////

/* #meta GPIOS : GPIO
/*
	GPIOS = (
		GPIO('buildin_led', 'B5', 'output'),
	)
*/

//////////////////////////////////////////////////////////////// Primitives ////////////////////////////////////////////////////////////////

#define false                   0
#define true                    1
#define STRINGIFY_(X)           #X
#define STRINGIFY(X)            STRINGIFY_(X)
#define CONCAT_(X, Y)           X##Y
#define CONCAT(X, Y)            CONCAT_(X, Y)
#define sizeof(...)             ((signed) sizeof(__VA_ARGS__))
#define countof(...)            (sizeof(__VA_ARGS__) / sizeof((__VA_ARGS__)[0]))
#define bitsof(...)             (sizeof(__VA_ARGS__) * 8)
#define useret                  __attribute__((warn_unused_result))
#define noret                   __attribute__((noreturn))
#define static_assert(...)      _Static_assert(__VA_ARGS__, STRINGIFY(__VA_ARGS__))

#include "primitives.meta"
/*
	for name, underlying, size in (
		('u8'  , 'unsigned char'     , 1),
		('u16' , 'unsigned short'    , 2),
		('u32' , 'unsigned long'     , 4),
		('u64' , 'unsigned long long', 8),
		('i8'  , 'signed char'       , 1),
		('i16' , 'signed short'      , 2),
		('i32' , 'signed long'       , 4),
		('i64' , 'signed long long'  , 8),
		('b8'  , 'signed char'       , 1),
		('b16' , 'signed short'      , 2),
		('b32' , 'signed long'       , 4),
		('b64' , 'signed long long'  , 8),
		('f32' , 'float'             , 4),
	):
		Meta.line(f'''
			typedef {underlying} {name};
			static_assert(sizeof({name}) == {size});
		''')
*/
