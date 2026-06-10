//
// A builder with C++26 reflection.
// Copyright by stefano.marocco@gmail.com
//
#include <meta>
#include <iostream>
#include <memory>
#include <vector>

namespace refl_builder {


enum class OutputType {
    Unique,
    Shared,
    Value
};

template<typename H, typename B, typename T, OutputType Output, auto Fn, auto BitMask, uint64_t RequiredMask>
struct WithMethod;

struct BuilderExpose {
    bool required;
};
struct BuilderValidate {};

template<typename H, typename B, typename T, OutputType Output, uint64_t RequiredMask>
consteval std::vector<std::meta::info> defineBuilderMembers();

template<typename T>
static void callValidate(T& object);

template<typename H, typename T, OutputType Output, uint64_t RequiredMask>
auto createBuilder(H holder);

template<typename H, typename B, typename T, OutputType Output, uint64_t RequiredMask>
class FinalBuilder : public B
{

public:
    FinalBuilder(H holder) 
    {
        setHolder(std::move(holder));
    }

    FinalBuilder(const FinalBuilder&) = delete;
    FinalBuilder& operator=(const FinalBuilder&) = delete;

    FinalBuilder(FinalBuilder&&) = default;
    FinalBuilder& operator=(FinalBuilder&&) = default;
    
    auto build() 
        requires (RequiredMask == 0)
    {
        if constexpr (Output == OutputType::Value)
        {
            callValidate(this->holder_);
        }
        else
        {
            callValidate(*this->holder_);
        }
        return std::move(this->holder_);
    }

private:
    static consteval auto getMembers()
    {
        return std::define_static_array(
            std::meta::members_of(
                ^^B,
                std::meta::access_context::unchecked()
            )
        );
    }

    constexpr void setHolder(H holder)
    {
        this->holder_ = std::move(holder);
        
        template for (constexpr auto m : getMembers())
        {
            if constexpr (
                std::meta::is_class_member(m) && 
                !std::meta::is_constructor(m) && 
                !is_special_member_function(m) && 
                !is_destructor(m)) 
            {
                constexpr auto type = type_of(m);

                if constexpr (std::meta::has_template_arguments(type))
                {
                    static constexpr auto args = std::define_static_array(
                        std::meta::template_arguments_of(type)
                    );
                    
                    if constexpr (std::meta::template_of(type) == ^^WithMethod)
                    {
                        this->[:m:].holder_ = &this->holder_;
                    }          
                }  
            }
        }
    }

};

template<auto V>
struct PrintValue;

template<typename H, typename B, typename T, OutputType Output>
B initBuilder(H holder) {
    return B(std::move(holder));
}


template<typename B, typename T, typename S, OutputType Output, uint64_t RequiredMask>
consteval void processClass(std::vector<std::meta::info>& builderMembers);


template<typename H, typename B, typename T, OutputType Output, auto WithFn, auto BitMask, uint64_t RequiredMask>
struct WithMethod {
    H* holder_;
    auto operator()(auto... args) {

        if constexpr (Output == OutputType::Value)
        {
            WithFn(*holder_, std::forward<decltype(args)>(args)...);
        }
        else
        {
            WithFn(*holder_->get(), std::forward<decltype(args)>(args)...);
        }
         
        return createBuilder<H, T, Output, RequiredMask & ~BitMask>(std::move(*holder_));
    }
};


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

enum class MemberType
{
    Data,
    Method,
    Validate
};

struct MemberInfo
{
    int index;
    bool required;
    std::meta::info member;
    MemberType type;
};

template<typename T>
consteval auto collectClassMemberInfo()
{
    std::vector<MemberInfo> memberInfos;

    auto visit = [&]<typename C>(auto&& self) consteval
    {
        static constexpr auto members =
            std::define_static_array(
                std::meta::members_of(^^C,
                    std::meta::access_context::unchecked())
            );

        template for (constexpr auto m : members)
        {
            if constexpr (std::meta::is_function(m))
            {
                constexpr auto anns = std::define_static_array(
                    std::meta::annotations_of_with_type(m, ^^BuilderExpose)
                );

                if constexpr (anns.size() == 1)
                {
                    using A = typename[:std::meta::type_of(anns[0]):];
                    constexpr auto cfg = std::meta::extract<A>(anns[0]);

                    int index = memberInfos.size();
                    memberInfos.push_back({
                        .index = index,
                        .required = cfg.required,
                        .member = m,
                        .type = MemberType::Method
                    });
                }
                else if (std::meta::annotations_of_with_type(m, ^^BuilderValidate).size() == 1)
                {
                    int index = memberInfos.size();
                    memberInfos.push_back({
                        .index = index,
                        .required = false,
                        .member = m,
                        .type = MemberType::Validate
                    });
                }
            }
        }

        static constexpr auto nonstatic_members =
            std::define_static_array(
                std::meta::nonstatic_data_members_of(^^C,
                    std::meta::access_context::unchecked())
            );

        template for (constexpr auto m : nonstatic_members)
        {
            if constexpr (!std::meta::is_function(m))
            {
                constexpr auto anns = std::define_static_array(
                    std::meta::annotations_of_with_type(m, ^^BuilderExpose)
                );

                if constexpr (anns.size() == 1)
                {
                    using A = typename[:std::meta::type_of(anns[0]):];
                    constexpr auto cfg = std::meta::extract<A>(anns[0]);

                    int index = memberInfos.size();
                    memberInfos.push_back({
                        .index = index,
                        .required = cfg.required,
                        .member = m,
                        .type = MemberType::Data
                    });
                }
            }
        }

        static constexpr auto bases =
            std::define_static_array(
                std::meta::bases_of(^^C,
                    std::meta::access_context::unchecked())
            );

        template for (constexpr auto b : bases)
        {
            using Base = [: std::meta::type_of(b) :];
            self.template operator()<Base>(self);
        }
    };

    visit.template operator()<T>(visit);

    return std::define_static_array(memberInfos);
}


template<typename T>
struct ClassRegistry
{
    static constexpr auto classMemberInfos = collectClassMemberInfo<T>();

    template<auto Member>
    static consteval auto memberIndex()
    {
        for (int i = 0; i < classMemberInfos.size(); ++i)
        {
            if (classMemberInfos[i].member == Member)
            {
                return i;
            }
        }

        return 0;
    }

    static consteval uint64_t requiredMask()
    {
        uint64_t mask = 0;
        for (size_t i = 0; i < classMemberInfos.size(); ++i)
        {
            if (classMemberInfos[i].required)
            {
                mask |= (1ull << classMemberInfos[i].index);
            }
        }
        return mask;
    }

};

template<typename T>
static void callValidate(T& object)
{
    template for (constexpr auto m : ClassRegistry<T>::classMemberInfos)
    {
        if constexpr (m.type == MemberType::Validate) 
        {
            constexpr auto memptr = &[: m.member :];
            (object.*memptr)();
        }
    }
}

template<typename H, typename B, typename T, OutputType Output, uint64_t RequiredMask>
consteval std::vector<std::meta::info> defineBuilderMembers()
{
    constexpr auto members = std::define_static_array(
        std::meta::members_of(^^T, std::meta::access_context::unchecked())
    );

    std::vector<std::meta::info> builderMembers = { 
        std::meta::data_member_spec(
            ^^H, 
            {.name = "holder_"} 
        )
    };

    template for (constexpr auto m : ClassRegistry<T>::classMemberInfos)
    {
        constexpr auto BitMask = m.required ? (1ull << m.index) : 0ull;

        if constexpr (m.type == MemberType::Method)
        {
            auto builderMethod = [](T& obj, auto...args){
                constexpr auto memptr = &[: m.member :];  // workaround for private access
                (obj.*memptr)(std::forward<decltype(args)>(args)...);
            };

            builderMembers.push_back(
                std::meta::data_member_spec(
                    ^^WithMethod<H, B, T, Output, builderMethod, BitMask, RequiredMask>,  
                    {.name = std::meta::identifier_of(m.member)} 
                )
            );
        }
        else if constexpr (m.type == MemberType::Data)
        {
            using P = typename[:std::meta::type_of(m.member):]; 
            auto builderMethod = [](T& obj, P arg){
                obj.[:m.member:] = std::move(arg);
            };

            builderMembers.push_back(
                std::meta::data_member_spec(
                    ^^WithMethod<H, B, T, Output, builderMethod, BitMask, RequiredMask>, 
                    {.name = makeWithName(std::meta::identifier_of(m.member))} 
                )
            );
        }
    }

    return builderMembers;
}

template<typename H, typename T, OutputType Output, uint64_t RequiredMask>
auto createBuilder(H holder) 
{
    struct B;
    consteval {
        auto builderMembers = defineBuilderMembers<H, B, T, Output, RequiredMask>();
        std::meta::define_aggregate(^^B, builderMembers);
    } 
    return FinalBuilder<H, B, T, Output, RequiredMask>(std::move(holder));
}

template<typename T, OutputType Output>
auto makeBuilder() 
{
    constexpr auto objectHolderInfo = []() consteval {
        if (Output == OutputType::Unique) return ^^std::unique_ptr<T>;
        else if (Output == OutputType::Shared) return ^^std::shared_ptr<T>;
        return ^^T;
    }();

    using H = typename[:objectHolderInfo:];

    constexpr auto requiredMask = ClassRegistry<T>::requiredMask();

    auto createHolder = [] -> H {
        if constexpr (Output == OutputType::Unique)
        {
            return std::make_unique<T>();
        }
        else if constexpr (Output == OutputType::Shared)
        {
            return std::make_shared<T>();
        }
        else
        {
            return T{};
        } 
    };

    return createBuilder<H, T, Output, requiredMask>(createHolder());
}


template<typename T>
auto makeSharedBuilder()
{
    return makeBuilder<T, OutputType::Shared>();
}

template<typename T>
auto makeUniqueBuilder()
{
    return makeBuilder<T, OutputType::Unique>();
}

template<typename T>
auto makeValueBuilder()
{
    return makeBuilder<T, OutputType::Value>();
}

}