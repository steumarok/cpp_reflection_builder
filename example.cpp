#include "builder.hpp"

namespace rb = refl_builder;

class Base
{
    [[=rb::BuilderParam]] 
    int c_ = 10;

private:
    [[=rb::BuilderMethod, =rb::Required]] 
    void withBar(int bar) {
        c_ = bar * 2;
    }

};

class Derived : public Base
{
private:
    [[=rb::BuilderParam]] 
    int b_ = 10;

public:
    static auto& builder()
    {
        return rb::makeUniqueBuilder<Derived>();
    }

    int getB() const {
        return b_;
    }

private:
    [[=rb::BuilderMethod, =rb::Required]] 
    void withFoo(int a) {
        b_ += a;
    }
    
    
};


int main() {

    auto a = Derived::builder()
        .withFoo(10)
        .withBar(10)
        .withB(19)
        .withC(20)
        .build();

    std::cout << a->getB() << std::endl;

    return 1978;
}