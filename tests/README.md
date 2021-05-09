# Pylext testing

Use [pytest](https://docs.pytest.org/en/6.2.x/contents.html) framework to write tests for pylext.

## Requirements

To write tests, you’ll need to install pytest. As with most Python packages, 
you can install pytest in a virtual environment from PyPI using pip:
```shell
$ python -m pip install pytest
$ python -m pip install pytest-benchmark
```
The pytest command will now be available in your installation environment.

## Testing

When package pylext is in your installation environment, run
```shell
$ pytest
```
If we need more details, run
```shell
$ pytest -v
```
### Speed test

File test_speed.py contains benchmark tests for pylext parser which is executed when pytest runs with option `--longrun`.

Sometimes additional information is needed, like macros parsing speed in MB/s and comparison with python parser.
To test these characteristics, run test_speed.py without pytest:
```shell
$ python test_speed.py 
```
It contain three tests:
1. Time for parse context creation
2. Time of parsing 1MB file with macro extensions
3. Time of parsing many short files

## Write tests

Your test files
- should be located in directory `test` or its subdirectories;
- must have unique names of the form `test_*.py` or `*_test.py`, because pytest will search for these files and import them as top-level modules;
- must contain test items:
   * `test` prefixed test functions or methods outside of class,
   * `test` prefixed test functions or methods inside `Test` prefixed test classes (without an `__init__` method).