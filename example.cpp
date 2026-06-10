#include "refl_builder.hpp"

namespace rb = refl_builder;

class Base
{
protected:
    [[=rb::BuilderExpose{}]] 
    int c_ = 0;

private:
    [[=rb::BuilderExpose{.required=true}]] 
    void withBar(int bar) {
        c_ = bar * 2;
    }

    [[=rb::BuilderValidate{}]] 
    void validate() {
        if (c_ == 0) {
            throw std::runtime_error("c_ not setted");
        }
    }
    
};

class Derived : public Base
{
private:
    [[=rb::BuilderParam]] 
    int b_ = 10;

public:
    int getB() const {
        return b_;
    }

private:
    [[=rb::BuilderExpose{.required=true}]] 
    void withFoo(int a) {
        b_ += a;
    }
    
    
};


int main() {

    auto a = rb::makeUniqueBuilder<Derived>()
        .withFoo(10)
        .withBar(10)
        .withB(19)
        .withC(20)
        .build();

    std::cout << a->getB() << std::endl;

    return 1978;
}