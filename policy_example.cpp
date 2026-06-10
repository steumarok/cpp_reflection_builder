#include "refl_builder.hpp"

namespace rb = refl_builder;

class A
{
public:
    A(int param)
    : param_(param)
    { }

    int getC() const {
        return c_;
    }

protected:
    [[=rb::BuilderExpose{}]] 
    int c_ = 0;
   
    int& param_;
};

struct MyTag {};

template<typename T>
struct rb::BuilderPolicy<MyTag, T>
{
    using HolderType = std::shared_ptr<T>;
    using ObjectType = T;

    static HolderType create()
    {
        return std::make_shared<T>(100);
    }

    static T& getReference(HolderType& holder)
    {
        return *holder;
    }
};

int main() {

    auto a = rb::makeBuilder<A, MyTag>()
        .withC(20)
        .build();

    std::cout << a->getC() << std::endl;

    return 1978;
}