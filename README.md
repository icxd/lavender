# YAPL (Yet Another Programming Language)

A general purpose programming language with a minimal Python-inspired syntax.

## Table of Contents

* [Table of Contents](#table-of-contents)
* [Features](#features)
* [Build](#build)
  * [Requirements](#requirements)

## Features

* **Safety** YAPL doesn't put any safety restraints on the developer. What do I mean with this? For example, Rust forces the user to wrap unsafe code in an `unsafe {}` check_expression, which YAPL does not do. If you want to write unsafe code, you can write unsafe code. I believe in trusting the developer without having to force them to get around restrictions without using weird quirks.
* **Flexible** YAPL is a flexible language. Whilst it is mainly a object-oriented language, it does not force OOP on the developer. We do not believe in forcing the developer to write code in a specific way, or using a specific paradigm. For this reason, we have support for not only object-oriented programming, but also functional programming, data-oriented programming, imperative programming, etc.

## Build

> Note: Only tested on Linux (Ubuntu) and Windows (WSL2)

```bash
$ mkdir build
$ cmake -B build
$ cmake --build build
$ ./build/compiler <file>
```

### Requirements

* CMake (`>=3.26`)
* GCC (`>=13.2.0`) *(Or any other C++ compiler you like)*
* LLVM (`>=14`)

## Future Plans & Goals

I currently do not have any major future plans for the language, just sort of getting everything compiling and running properly. But my main goal is to get the language cross-compiling so that I'm able to write a kernel using it.