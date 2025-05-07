"""Tool for generating C++ test targets efficiently using Bazel rules.
"""

load("@rules_cc//cc:defs.bzl", "cc_test")

def test_set(
        test_files = None,
        size = "small",
        srcs = [],
        file_extensions = ".c",
        **kwargs):
    """Creates C++ test targets from a list of test files.

    Args:
        test_files: List of test file paths to process. Defaults to None.
        size: Test size parameter for cc_test rule. Defaults to "small".
        srcs: Additional source files to include in all tests. Defaults to empty list.
        file_extensions: Expected extension of test files. Defaults to ".cpp".
        **kwargs: Additional arguments to pass to cc_test rule.

    Returns:
        List of test target names (e.g., [":test1", ":test2"]).

    Note:
        Only files ending with the specified file_extensions are processed.
        Each test target is created with the filename (without extension) as its name.
    """
    test_targets = []

    # Process positive tests
    for file in test_files:
        if not file.endswith(file_extensions):
            continue
        name = file[:-len(file_extensions)]
        target = ":" + name
        cc_test(
            name = name,
            size = size,
            srcs = srcs + [file],
            **kwargs
        )
        test_targets.append(target)

    return test_targets
