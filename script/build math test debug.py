#!/usr/bin/env python
import helper

script = "test/math/SConscript"
is_debug = 1
helper.exec_cmd(f'scons -j4 -Q debug={is_debug} -Q script={script}')
helper.exec_cmd('build/HPW')
