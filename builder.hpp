//
// A builder with C++26 reflection.
// Copyright by stefano.marocco@gmail.com
//
#include <meta>
#include <iostream>
#include <memory>

namespace refl_builder {

template<typename B, typename T, bool Unique, auto Fn, auto MethodTag, auto... UsedMethods>
struct WithMethod;

struct BuilderParamT {};
struct BuilderMethodT {};
struct BuilderValidateT {};
struct RequiredT {};

static inline constexpr BuilderParamT BuilderParam;
static inline constexpr BuilderMethodT BuilderMethod;
static inline constexpr BuilderValidateT BuilderValidate;
static inline constexpr RequiredT Required;

template<typename B, typename T, bool Unique = true, auto... UsedMethods>
consteval std::vector<std::meta::info> defineBuilderMembers();



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

template<typename B, typename T, bool Unique>
B& initBuilder();

template<typename B, typename T, auto... UsedMethods>
class FinalBuilder final : public B
{
public:
    auto build() 
        requires AllMethodsCalled<T, UsedMethods...>
    {
        callValidateIfExists(*this->object_);

        auto object = std::move(this->object_);
        delete this;
        return object;
    }
};

template<typename B, typename T>
struct ExtractBaseBuilder<FinalBuilder<B, T>>
{
    using type = B;
};

template<typename B, typename T, bool Unique>
B& initBuilder() {
    B* builder = new B;

    if constexpr (Unique)
    {
        builder->object_ = std::make_unique<T>();
    }
    else
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

template<typename B, typename T, typename S, bool Unique, auto... UsedMethods>
consteval void processClass(std::vector<std::meta::info>& builderMembers);


template<typename T, bool Unique, auto... UsedMethods>
auto& reinterpretBuilder(auto& builder) {
    struct NB;
    consteval {
        auto builderMembers = defineBuilderMembers<NB, T, Unique, UsedMethods...>();
        std::meta::define_aggregate(^^NB, builderMembers);
    } 
    return reinterpret_cast<FinalBuilder<NB, T, UsedMethods...>&>(builder);
}

template<typename B, typename T, bool Unique, auto WithFn, auto MethodTag, auto... UsedMethods>
struct WithMethod {
    FinalBuilder<B, T, UsedMethods...>* builder_;
    auto& operator()(auto... args) {
        WithFn(*builder_->object_, std::forward<decltype(args)>(args)...);

        constexpr bool alreadyUsed = ((MethodTag == UsedMethods) || ...);
        if constexpr (alreadyUsed)
        {
            return *builder_;
        }
        else
        {
            return reinterpretBuilder<T, Unique, UsedMethods..., MethodTag>(*builder_);
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

template<typename B, typename T, typename S, bool Unique, auto... UsedMethods>
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
                    ^^WithMethod<B, S, Unique, builderMethod, m, UsedMethods...>,  
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
                    ^^WithMethod<B, S, Unique, builderMethod, m, UsedMethods...>, 
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

        processClass<B, Base, S, Unique, UsedMethods...>(builderMembers);
    }
}

template<typename B, typename T, bool Unique, auto... UsedMethods>
consteval std::vector<std::meta::info> defineBuilderMembers()
{
    constexpr auto members = std::define_static_array(
        std::meta::members_of(^^T, std::meta::access_context::unchecked())
    );

    auto getPointerTypeInfo = []() consteval {
        return Unique ? ^^std::unique_ptr<T> : ^^std::shared_ptr<T>;
    };

    constexpr auto objectMember = std::meta::data_member_spec(
        getPointerTypeInfo(), 
        {.name = "object_"} 
    );

    std::vector<std::meta::info> builderMembers = { 
        objectMember
    };

    processClass<B, T, T, Unique, UsedMethods...>(builderMembers);

    return builderMembers;
}

template<typename T, bool Unique = true>
auto& makeBuilder() 
{
    struct B;
    consteval {
        auto builderMembers = defineBuilderMembers<B, T, Unique>();
        std::meta::define_aggregate(^^B, builderMembers);
    } 
    return initBuilder<FinalBuilder<B, T>, T, Unique>();
}

template<typename T>
auto& makeSharedBuilder()
{
    return makeBuilder<T, false>();
}

template<typename T>
auto& makeUniqueBuilder()
{
    return makeBuilder<T, true>();
}

}