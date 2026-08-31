test_cases = [
    'testFileMenu.py',
    'testEditMenu.py',
    'testHelpMenu.py',
    'testZoom.py',
    'testGoMenu.py',
    'testBookmarksMenu.py',
    'testEncryptedFile.py',
    'testFileReloading.py',
    'testWrongFileExtension.py'
]

/* xvfb-run is used to run the Python E2E tests under a virtual
 * X server.  It is required in CI (where xvfb is installed by
 * the workflow) but optional in dev environments.  When xvfb-run
 * is not available, the Python tests are silently skipped -- this
 * matches the existing behaviour so local builds without xvfb
 * continue to work. */
xvfb_run = find_program('xvfb-run', required: false)

foreach case : test_cases
    test_script = find_program(case)

    if xvfb_run.found()
        test_exe = xvfb_run
        test_args = ['-a', test_script.full_path(), xreader.full_path()]
    else
        test_exe = test_script
        test_args = [xreader.full_path()]
    endif

    test(
        case,
        test_exe,
        args: test_args,
        is_parallel: false,
        depends: xreader,
        workdir: join_paths(prefix, bindir),
        timeout: 120,
    )
endforeach
