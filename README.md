# Lavender

A general purpose programming language mainly focused around writing system software, such as operating systems, kernels, etc. Lavender is a statically-typed language, with a syntax similar to that of C++ and Rust. The language is also focused around safety, flexibility, and performance.

## Table of Contents

* [Table of Contents](#table-of-contents)
* [Features](#features)
* [Build](#build)
  * [Requirements](#requirements)

## Features

* **Safety** Lavender forces the developer to write safe code unless the developer explicitly tells the compiler that the program is mainly unsafe. This is done by using the `unsafe` keyword or passing a flag to the compiler. Lavender does this to ensure that the developer is aware of the risks of not only writing unsafe code, but also using unsafe code.
* **Flexibility** Lavender is a flexible language. Whilst it is mainly a object-oriented language, it does not force OOP on the developer. We do not believe in forcing the developer to write code in a specific way, or using a specific paradigm. For this reason, we have support for not only object-oriented programming, but also functional programming, data-oriented programming, imperative programming, etc.

## Build

> **Note**
> Only tested on Linux (Ubuntu) and Windows (WSL2)

```bash
$ mkdir build
$ cmake -B build
$ cmake --build build
$ ./build/compiler <file>
```

### Requirements

* CMake (`>=3.26`)
* GCC (`>=13.2.0`) *(Or any other C++ compiler that supports C++23 features you like)*

## Future Plans & Goals

I currently do not have any major future plans for the language, just sort of getting everything compiling and running properly. But my main goal is to get the language cross-compiling so that I'm able to write a kernel using it.