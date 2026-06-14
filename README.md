### Dynamic Class Builder with C++26 Reflection

Author: Stefano Marocco — [stefano.marocco@gmail.com](mailto:stefano.marocco@gmail.com)

License: GNU General Public License v3

This project provides a C++26 reflection-based builder implementation.

### Main Features

1. Data member and function annotations

   Expose selected members and methods to the generated builder through annotations.

2. Derived class support

   Builders work correctly with inheritance hierarchies and derived classes.

3. Required member annotations

   Mark members as required and automatically disable the build() method at compile time when they are missing.

4. Flexible object creation

   Create objects as std::shared_ptr, std::unique_ptr, or plain value objects.

### Functions

| Function                        | Description                                             |
| ------------------------------- | ------------------------------------------------------- |
| refl_builder::makeSharedBuilder | Create a builder that returns a std::shared_ptr object. |
| refl_builder::makeUniqueBuilder | Create a builder that returns a std::unique_ptr object. |
| refl_builder::makeValueBuilder  | Create a builder that returns a value object.           |

### Annotations

refl_builder::BuilderExpose{.required=true | false}

Apply this annotation to data members or functions to expose them through the builder interface.

* required=true marks the member as mandatory.

* required=false marks the member as optional.

refl_builder::BuilderValidate{}

Apply this annotation to a validation function that will be called during build() to perform consistency checks on the object before it is created.

### Usage

Annotate the desired data members and functions, then create a builder with one of the make*Builder functions.

Example:

Create a value builder

cpp

refl_builder::makeValueBuilder<Class>();

### Examples

* [simple_example.cpp](https://github.com/steumarok/cpp_reflection_builder/blob/main/simple_example.cpp) — basic usage of the reflection builder

* [policy_example.cpp](https://github.com/steumarok/cpp_reflection_builder/blob/main/policy_example.cpp) — advanced usage with policies and validation

### Questions?

Feel free to contact me by email if you have any questions or feedback.
