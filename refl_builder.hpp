//
// A builder with C++26 reflection.
// Copyright by stefano.marocco@gmail.com
//
#include <meta>
#include <iostream>
#include <memory>

namespace refl_builder {

enum class OutputType {
    Unique,
    Shared,
    Value
};

template<typename B, typename T, OutputType Output, auto Fn, auto MethodTag, auto... UsedMethods>
struct WithMethod;

struct BuilderParamT {};
struct BuilderMethodT {};
struct BuilderValidateT {};
struct RequiredT {};

static inline constexpr BuilderParamT BuilderParam;
static inline constexpr BuilderMethodT BuilderMethod;
static inline constexpr BuilderValidateT BuilderValidate;
static inline constexpr RequiredT Required;

template<typename B, typename T, OutputType Output, auto... UsedMethods>
consteval std::vector<std::meta::info> defineBuilderMembers();

template<typename T>
static void callValidateIfExists(T& object);

template<typename T>
struct ExtractBaseBuilder;


template<typename T, auto... UsedMethods>
constexpr bool AllMethodsCalled = [] consteval {

    std::vector<std::meta::info> requiredMethods;

    auto collectRequiredMethods = [&requiredMethods]<typename C>(auto&& self) consteval {
        constexpr auto members = std::define_static_array(
            std::meta::members_of(^^C, std::meta::access_context::unchecked())
        );
        template for (constexpr auto m : auto(members))
        {
            if (std::meta::annotations_of_with_type(m, ^^RequiredT).size() == 1)
            {
                requiredMethods.push_back(m);
            }
        }

        constexpr auto bases = std::define_static_array(
            std::meta::bases_of(^^C, std::meta::access_context::unchecked())
        );
        template for (constexpr auto base : auto(bases))
        {
            using Base = [: std::meta::type_of(base) :];
            self.template operator()<Base>(self);
        }
    };

    collectRequiredMethods.template operator()<T>(collectRequiredMethods);

    int requiredCount = requiredMethods.size();
    auto checkMethod = [&requiredMethods, &requiredCount]<auto M>()
    {
        if (std::find(requiredMethods.begin(), requiredMethods.end(), M) != requiredMethods.end())
        {
            requiredCount--;
        }
    };
    (checkMethod.template operator()<UsedMethods>(), ...);
    return requiredCount == 0;
}();

template<typename B, typename T, OutputType Output>
B& initBuilder();

template<typename B, typename T, OutputType Output, auto... UsedMethods>
class FinalBuilder final : public B
{
public:
    auto build() 
        requires AllMethodsCalled<T, UsedMethods...>
    {
        if constexpr (Output == OutputType::Value)
        {
            callValidateIfExists(this->object_);
        }
        else
        {
            callValidateIfExists(*this->object_);
        }

        auto object = std::move(this->object_);
        delete this;
        return object;
    }
};

template<typename B, typename T, OutputType Output>
struct ExtractBaseBuilder<FinalBuilder<B, T, Output>>
{
    using type = B;
};

template<typename B, typename T, OutputType Output>
B& initBuilder() {
    B* builder = new B;

    if constexpr (Output == OutputType::Unique)
    {
        builder->object_ = std::make_unique<T>();
    }
    else if constexpr (Output == OutputType::Shared)
    {
        builder->object_ = std::make_shared<T>();
    }

    using BB = typename ExtractBaseBuilder<B>::type;
    constexpr auto members = std::define_static_array(
        std::meta::nonstatic_data_members_of(^^BB, std::meta::access_context::unchecked())
    );
    template for (constexpr auto m : auto(members))
    {
        constexpr auto type = type_of(m);
        if constexpr (std::meta::is_class_member(m) && 
            std::meta::has_template_arguments(type))
        {
            constexpr auto args = std::define_static_array(
                std::meta::template_arguments_of(type)
            );
            if constexpr (std::meta::is_same_type(args[0], ^^BB))
            {
                if constexpr (std::meta::template_of(type) == ^^WithMethod)
                {
                    builder->[:m:].builder_ = builder;
                }
            }
        }
    }
    return *builder;
}

template<typename B, typename T, typename S, OutputType Output, auto... UsedMethods>
consteval void processClass(std::vector<std::meta::info>& builderMembers);


template<typename T, OutputType Output, auto... UsedMethods>
auto& reinterpretBuilder(auto& builder) {
    struct NB;
    consteval {
        auto builderMembers = defineBuilderMembers<NB, T, Output, UsedMethods...>();
        std::meta::define_aggregate(^^NB, builderMembers);
    } 
    return reinterpret_cast<FinalBuilder<NB, T, Output, UsedMethods...>&>(builder);
}

template<typename B, typename T, OutputType Output, auto WithFn, auto MethodTag, auto... UsedMethods>
struct WithMethod {
    FinalBuilder<B, T, Output, UsedMethods...>* builder_;
    auto& operator()(auto... args) {
        if constexpr (Output == OutputType::Value)
        {
            WithFn(builder_->object_, std::forward<decltype(args)>(args)...);
        }
        else
        {
            WithFn(*builder_->object_, std::forward<decltype(args)>(args)...);
        }

        constexpr bool alreadyUsed = ((MethodTag == UsedMethods) || ...);
        if constexpr (alreadyUsed)
        {
            return *builder_;
        }
        else
        {
            return reinterpretBuilder<T, Output, UsedMethods..., MethodTag>(*builder_);
        }
    }
};

template<typename T>
static void callValidateIfExists(T& object)
{
    constexpr auto members = std::define_static_array(
        std::meta::members_of(^^T, std::meta::access_context::unchecked())
    );
    template for (constexpr auto m : auto(members))
    {
        if constexpr (
            std::meta::is_function(m) && 
            std::meta::annotations_of_with_type(m, ^^BuilderValidateT).size() == 1)
        {
            constexpr auto memptr = &[: m :];
            (object.*memptr)();
        }
    }

    constexpr auto bases = std::define_static_array(
        std::meta::bases_of(^^T, std::meta::access_context::unchecked())
    );
    template for (constexpr auto base : auto(bases))
    {
        using Base = [: std::meta::type_of(base) :];

        callValidateIfExists<Base>(object);
    }
}

consteval char toUpperAscii(char c)
{
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 'A';

    return c;
}

consteval auto makeWithName(std::string_view field)
{
    std::string result = "with";

    bool upper = true;

    for (char c : field)
    {
        if (c == '_')
            continue;

        if (upper) {
            result += static_cast<char>(toUpperAscii(c));
            upper = false;
        }
        else {
            result += c;
        }
    }

    return result;
}

template<typename B, typename T, typename S, OutputType Output, auto... UsedMethods>
consteval void processClass(std::vector<std::meta::info>& builderMembers)
{
    constexpr auto members = std::define_static_array(
        std::meta::members_of(^^T, std::meta::access_context::unchecked())
    );

    template for (constexpr auto m : auto(members))
    {
        if constexpr (
            std::meta::is_function(m) && 
            std::meta::annotations_of_with_type(m, ^^BuilderMethodT).size() == 1 &&
            !std::meta::is_special_member_function(m) &&
            !std::meta::is_constructor(m))
        {
            auto builderMethod = [](T& obj, auto...args){
                constexpr auto memptr = &[: m :];  // workaround for private access
                (obj.*memptr)(std::forward<decltype(args)>(args)...);
            };

            builderMembers.push_back(
                std::meta::data_member_spec(
                    ^^WithMethod<B, S, Output, builderMethod, m, UsedMethods...>,  
                    {.name = std::meta::identifier_of(m)} 
                )
            );
        }
        else if constexpr (std::meta::annotations_of_with_type(m, ^^BuilderParamT).size() == 1)
        {
            using P = typename[:std::meta::type_of(m):]; 
            auto builderMethod = [](T& obj, P arg){
                obj.[:m:] = std::move(arg);
            };

            builderMembers.push_back(
                std::meta::data_member_spec(
                    ^^WithMethod<B, S, Output, builderMethod, m, UsedMethods...>, 
                    {.name = makeWithName(std::meta::identifier_of(m))} 
                )
            );
        }
    }

    constexpr auto bases = std::define_static_array(
        std::meta::bases_of(^^T, std::meta::access_context::unchecked())
    );
    template for (constexpr auto base : auto(bases))
    {
        using Base = [: std::meta::type_of(base) :];

        processClass<B, Base, S, Output, UsedMethods...>(builderMembers);
    }
}

template<typename B, typename T, OutputType Output, auto... UsedMethods>
consteval std::vector<std::meta::info> defineBuilderMembers()
{
    constexpr auto members = std::define_static_array(
        std::meta::members_of(^^T, std::meta::access_context::unchecked())
    );

    auto getObjectTypeInfo = []() consteval {
        if (Output == OutputType::Unique) return ^^std::unique_ptr<T>;
        else if (Output == OutputType::Shared) return ^^std::shared_ptr<T>;
        return ^^T;
    };

    constexpr auto objectMember = std::meta::data_member_spec(
        getObjectTypeInfo(), 
        {.name = "object_"} 
    );

    std::vector<std::meta::info> builderMembers = { 
        objectMember
    };

    processClass<B, T, T, Output, UsedMethods...>(builderMembers);

    return builderMembers;
}

template<typename T, OutputType Output>
auto& makeBuilder() 
{
    struct B;
    consteval {
        auto builderMembers = defineBuilderMembers<B, T, Output>();
        std::meta::define_aggregate(^^B, builderMembers);
    } 
    return initBuilder<FinalBuilder<B, T, Output>, T, Output>();
}

template<typename T>
auto& makeSharedBuilder()
{
    return makeBuilder<T, OutputType::Shared>();
}

template<typename T>
auto& makeUniqueBuilder()
{
    return makeBuilder<T, OutputType::Unique>();
}

template<typename T>
auto& makeValueBuilder()
{
    return makeBuilder<T, OutputType::Value>();
}

}