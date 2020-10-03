from __future__ import print_function

import sys
import subprocess
import os

def is_test_file(name, path):
    """
    Return True if the given path/file name is a test file.
    """

    if os.name == 'nt':
        return os.path.isfile(path) and path.endswith('.exe')

    return os.path.isfile(path) and name.startswith('test_')

def main(args):
    """
    Run all test files in the given test directory.
    """

    if len(args) != 2:
        raise SyntaxError(
            'Invalid syntax: python run_tests.py <path to test directory>')

    test_dir = args[1]

    test_count = 0
    success_count = 0
    for name in os.listdir(test_dir):
        f = os.path.join(test_dir, name)
        if not is_test_file(name, f):
            continue

        test_count += 1
        print('Testing {0}...'.format(name), end=' ')
        try:
            output = subprocess.check_output(f)
        except subprocess.CalledProcessError as e:
            print('Failed with return code {0}!'.format(e.returncode))
            print('Output:\n')
            print(e.output)
        else:
            print('OK!')
            success_count += 1

    print('{0} of {1} tests finished sucessfully.'.format(
        success_count, test_count))

    if test_count != success_count:
        sys.exit(1)

if __name__ == '__main__':
    main(sys.argv)