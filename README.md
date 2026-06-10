# Dynamic class builder with C++26 Reflection

Author: Stefano Marocco <stefano.marocco@gmail.com><br>

License: GNU GENERAL PUBLIC LICENSE v3<br>

This is a C++26 reflection builder implementation. <br>

Main features are:
- data member and function annotations
- derived class support
- annotation for required members (disabling the build method at compile-time)
- unique, shared and value object creation

## Functions

- refl_builder::makeSharedBuilder<br>

  create a shared_ptr object

- refl_builder::makeUniqueBuilder<br>

  create a unique_ptr object

- refl_builder::makeValueBuilder<br>

  create a value object

## Annotations

- refl_builder::BuilderExpose{.required=true | false}<br>

  Applied to data members and functions, will expose the method to the builder class. 

- refl_builder::BuilderValidate{}<br>

  Called in the build method for allow consistency check of the object.

## Usage

Annotate the data members and function and use the make*Builder for obtain the builder:

```C++
refl_builder::makeValueBuilder<Class>();
```

## Examples

Look at [example.cpp](https://github.com/steumarok/cpp_reflection_builder/blob/main/example.cpp)

Feel free to write me an email if you have some questions.
