project('cJSON', 'c', default_options: ['c_std=c89'])
version = '1.6.0'
soversion = '0'

compiler = meson.get_compiler('c')

compiler_flags = []

if (compiler.get_id() == 'clang') or (compiler.get_id() == 'gcc')
	compiler_flags += [
		'-pedantic',
		'-Wall',
		'-Wextra',
		'-Werror',
		'-Wstrict-prototypes',
		'-Wwrite-strings',
		'-Wshadow',
		'-Winit-self',
		'-Wcast-align',
		'-Wformat=2',
		'-Wmissing-prototypes',
		'-Wstrict-overflow=2',
		'-Wcast-qual',
		'-Wundef',
		'-Wswitch-default',
		'-Wconversion',
		'-Wc++-compat',
		'-fstack-protector-strong',
		'-Wcomma',
		'-Wdouble-promotion',
		'-Wparentheses',
		'-Wformat-overflow',
		'-Wunused-macros',
		'-Wmissing-variable-declarations',
		'-Wused-but-marked-unused',
		'-Wswitch-enum'
	]
endif

foreach flag : compiler_flags
	if compiler.has_argument(flag)
		add_project_arguments(flag, language: 'c')
	endif
endforeach

math = compiler.find_library('m', required: false)

cjson = shared_library('cjson', 'cJSON.c', dependencies: math, version: version, soversion: soversion, install: true)
if get_option('enable_cjson_utils')
	cjson_utils = shared_library('cjson_utils', 'cJSON_Utils.c', link_with: cjson, version: version, soversion: soversion, install: true)
endif

if get_option('enable_cjson_tests')
	cjson_test = executable('cjson_test', 'test.c', link_with: cjson)
	test('cjson_test', cjson_test)
endif

subdir('tests')
