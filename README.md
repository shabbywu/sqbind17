# sqbind17 - Headonly squirrel c++17 binding

Squirrel is a high level imperative, object-oriented programming language, designed to be a light-weight scripting language that fits in the size, memory bandwidth, and real-time requirements of applications like video games.
sqbind17 is intended to be a comprehensive binding of Squirrel 3 for C++17.

![github-stars][stars-badge]

| CI                 | status                                                                |
| ------------------ | --------------------------------------------------------------------- |
| Linux/macOS Travis | [![Travis-CI][travis-badge]][travis-link]                             |
| MSVC 2019          | [![AppVeyor][appveyor-badge]][appveyor-link]                          |
| pip builds         | [![Pip Actions Status][actions-pip-badge]][actions-pip-link]          |
| [cibuildwheel][]   | [![Wheels Actions Status][actions-wheels-badge]][actions-wheels-link] |

[cibuildwheel]: https://cibuildwheel.readthedocs.io
[stars-badge]: https://img.shields.io/github/stars/shabbywu/sqbind17?style=social
[actions-pip-link]: https://github.com/shabbywu/sqbind17/actions/workflows/pip.yml
[actions-pip-badge]: https://github.com/shabbywu/sqbind17/workflows/Pip/badge.svg
[actions-wheels-link]: https://github.com/shabbywu/sqbind17/actions/workflows/wheels.yml
[actions-wheels-badge]: https://github.com/shabbywu/sqbind17/workflows/Wheels/badge.svg
[travis-link]: https://travis-ci.org/shabbywu/sqbind17
[travis-badge]: https://travis-ci.org/shabbywu/sqbind17.svg?branch=master&status=passed
[appveyor-link]: https://ci.appveyor.com/project/shabbywu/sqbind17
[appveyor-badge]: https://ci.appveyor.com/api/projects/status/f04io15t7o63916y

## Integration

### CMake
You can also use the `sqbind17::sqbind17` or `sqbind17::sqbind17_header` interface target in CMake.  This target populates the appropriate usage requirements for `INTERFACE_INCLUDE_DIRECTORIES` to point to the appropriate include directories.


## Example
see the **\*.cpp** file in `tests_cpp` folder.
