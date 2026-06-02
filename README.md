# Dynamic class builder with C++26 Reflection

Author: Stefano Marocco <stefano.marocco@gmail.com><br>

License: GNU GENERAL PUBLIC LICENSE v3<br>

This is a C++26 reflection builder implementation. <br>

Main features are:
- data member and function annotations
- derived class support
- annotation for required member
- unique, shared and value object creation

## Functions

- refl_builder::makeSharedBuilder<br>

  create a shared_ptr object

- refl_builder::makeUniqueBuilder<br>

  create a unique_ptr object

- refl_builder::makeValueBuilder<br>

  create a value object

## Annotations

- refl_builder::BuilderParam<br>

  Applied to data members, will generate a method called "with" + name. 

- refl_builder::BuilderMethod<br>

  Applied to functions, will generate a method called like the function name. 

- refl_builder::Required<br>

  Disable the build method if not all required methods are called.

## Usage

Annotate the data members and function and implement a static builder() function:

```C++
static auto& builder()
{
    return refl_builder::makeValueBuilder<Class>();
}
```

The return by reference is need because the builder object is self contained. It will be destroy when the build() function is called.

## Examples

Simple unique object builder:

```C++
#include "refl_builder.hpp"

class A
{
    [[=refl_builder::BuilderParam, =refl_builder::Required]] 
    int c_ = 10;

    [[=refl_builder::BuilderMethod]] 
    void withBar(int bar) 
    {
        c_ = bar * 2;
    }

public:
    static auto& builder()
    {
        return refl_builder::makeUniqueBuilder<A>();
    }

    int getC() const
    {
        return c_;
    }
};

int main() 
{
    auto a = A::builder()
        .withC(20)
        .withBar(10)
        .build();

    std::cout << a->getC() << std::endl;

    return 0;
}
```

Derived class builder:

```C++
#include "refl_builder.hpp"

class A
{
protected:
    [[=refl_builder::BuilderParam, =refl_builder::Required]] 
    int c_ = 10;

public:
    [[=refl_builder::BuilderMethod]] 
    void withBar(int bar) 
    {
        c_ = bar * 2;
    }

};

class B : public A
{
    [[=refl_builder::BuilderParam, =refl_builder::Required]] 
    int x_ = 10;

    [[=refl_builder::BuilderMethod, =refl_builder::Required]] 
    void withFoo(int foo) 
    {
        x_ = c_ * foo;
    }

public:
    static auto& builder()
    {
        return refl_builder::makeValueBuilder<B>();
    }

    int getX() const
    {
        return x_;
    }
};

int main() 
{
    auto b = B::builder()
        .withC(20)
        .withX(20)
        .withBar(10)
        .withFoo(10)
        .build();

    std::cout << b.getX() << std::endl;

    return 0;
}
```

Feel free to write me an email if you have some questions.