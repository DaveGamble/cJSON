if get_option('enable_cjson_tests')
	unity_flags = []

	if (compiler.get_id() == 'clang') or (compiler.get_id() == 'gcc')
		unity_flags += [
			'-Wno-switch-enum',
			'-Wno-error',
			'-fvisibility=default',
			'-fno-sanitize=float-divide-by-zero'
		]
	endif

	unity_c_args = []
	foreach flag : unity_flags
		if compiler.has_argument(flag)
			unity_c_args += flag
		endif
	endforeach

	unity = library('unity', 'unity/src/unity.c', c_args: unity_c_args)
	common = library('test_common', 'common.c')

	cjson_tests = [
		'parse_examples',
		'parse_number',
		'parse_hex4',
		'parse_string',
		'parse_array',
		'parse_object',
		'parse_value',
		'print_string',
		'print_number',
		'print_array',
		'print_object',
		'print_value',
		'misc_tests',
		'parse_with_opts',
		'compare_tests'
	]

	foreach cjson_test : cjson_tests
		exe = executable(cjson_test, cjson_test + '.c', link_with: [common, cjson, unity])
		test(cjson_test, exe, workdir: meson.current_source_dir())
	endforeach

	if get_option('enable_cjson_utils')
		cjson_utils_tests = [
			'json_patch_tests',
			'old_utils_tests',
			'misc_utils_tests'
		]

		foreach cjson_utils_test : cjson_utils_tests
			exe = executable(cjson_utils_test, cjson_utils_test + '.c', link_with: [common, cjson_utils, unity, cjson])
			test(cjson_utils_test, exe, workdir: meson.current_source_dir())
		endforeach
	endif
endif
