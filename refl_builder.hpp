//
// A builder with C++26 reflection.
// Copyright by stefano.marocco@gmail.com
//
#include <meta>
#include <iostream>
#include <memory>
#include <vector>

namespace refl_builder {

struct Unique {};
struct Shared {};
struct Value {};

template<typename Tag, typename T>
struct BuilderPolicy;

template<typename T>
struct BuilderPolicy<Unique, T>
{
    using HolderType = std::unique_ptr<T>;
    using ObjectType = T;

    static HolderType create(auto... args)
    {
        return std::make_unique<T>(std::forward<decltype(args)>(args)...);
    }

    static T& getReference(HolderType& holder)
    {
        return *holder;
    }
};

template<typename T>
struct BuilderPolicy<Value, T>
{
    using HolderType = T;
    using ObjectType = T;

    static HolderType create(auto... args)
    {
        return T(std::forward<decltype(args)>(args)...);
    }

    static T& getReference(HolderType& holder)
    {
        return holder;
    }
};

template<typename T>
struct BuilderPolicy<Shared, T>
{
    using HolderType = std::shared_ptr<T>;
    using ObjectType = T;

    static HolderType create(auto... args)
    {
        return std::make_shared<T>(std::forward<decltype(args)>(args)...);
    }

    static T& getReference(HolderType& holder)
    {
        return *holder;
    }
};


template<typename P, auto Fn, auto BitMask, uint64_t RequiredMask>
struct WithMethod;

struct BuilderExpose {
    bool required;
};
struct BuilderValidate {};

template<typename P, uint64_t RequiredMask>
consteval std::vector<std::meta::info> defineBuilderMembers();

template<typename T>
static void callValidate(T& object);

template<typename P, uint64_t RequiredMask>
auto createBuilder(typename P::HolderType holder);

template<typename P, typename B, uint64_t RequiredMask>
class FinalBuilder : public B
{

public:
    FinalBuilder(typename P::HolderType holder) 
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
        callValidate(P::getReference(this->holder_));

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

    constexpr void setHolder(P::HolderType holder)
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

template<typename Policy, auto WithFn, auto BitMask, uint64_t RequiredMask>
struct WithMethod {
    Policy::HolderType* holder_;
    auto operator()(auto... args) {

        WithFn(Policy::getReference(*holder_), std::forward<decltype(args)>(args)...);

        return createBuilder<Policy, RequiredMask & ~BitMask>(std::move(*holder_));
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
consteval void collectClassMemberInfo(std::vector<MemberInfo>& memberInfos)
{
    static constexpr auto members =
        std::define_static_array(
            std::meta::members_of(^^T,
                std::meta::access_context::unchecked())
        );

    template for (constexpr auto m : members)
    {
        if constexpr (
            std::meta::is_class_member(m) && 
            !std::meta::is_constructor(m) && 
            !is_special_member_function(m) && 
            !is_destructor(m)) 
        {
            if constexpr (std::meta::is_function(m))
            {
                static constexpr auto anns = std::define_static_array(
                    std::meta::annotations_of_with_type(m, ^^BuilderExpose)
                );

                if constexpr (anns.size() == 1)
                {
                    using A = typename[:std::meta::type_of(anns[0]):];
                    static constexpr auto cfg = std::meta::extract<A>(anns[0]);

                    int index = memberInfos.size();
                    memberInfos.push_back({
                        .index = index,
                        .required = cfg.required,
                        .member = m,
                        .type = MemberType::Method
                    });
                }
                else if constexpr (std::meta::annotations_of_with_type(m, ^^BuilderValidate).size() == 1)
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
            else if constexpr (std::meta::is_nonstatic_data_member(m)) 
            {
                static constexpr auto anns = std::define_static_array(
                    std::meta::annotations_of_with_type(m, ^^BuilderExpose)
                );

                if constexpr (anns.size() == 1)
                {
                    using A = typename[:std::meta::type_of(anns[0]):];
                    static constexpr auto cfg = std::meta::extract<A>(anns[0]);

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
    }

    static constexpr auto bases =
        std::define_static_array(
            std::meta::bases_of(^^T,
                std::meta::access_context::unchecked())
        );

    template for (constexpr auto b : bases)
    {
        using Base = [:std::meta::type_of(b):];
        collectClassMemberInfo<Base>(memberInfos);
    }
}

template<typename T>
consteval auto collectClassMemberInfo()
{
    std::vector<MemberInfo> memberInfos;
    collectClassMemberInfo<T>(memberInfos);
    return std::define_static_array(memberInfos);
}

template<typename T>
struct ClassRegistry
{
    static constexpr auto classMemberInfos = collectClassMemberInfo<T>();

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

template<typename P, uint64_t RequiredMask>
consteval std::vector<std::meta::info> defineBuilderMembers()
{
    using HolderType = typename P::HolderType;

    std::vector<std::meta::info> builderMembers = { 
        std::meta::data_member_spec(
            ^^HolderType, 
            {.name = "holder_"} 
        )
    };

    template for (constexpr auto m : ClassRegistry<typename P::ObjectType>::classMemberInfos)
    {
        constexpr auto BitMask = m.required ? (1ull << m.index) : 0ull;

        if constexpr (m.type == MemberType::Method)
        {
            auto builderMethod = [](P::ObjectType& obj, auto...args){
                constexpr auto memptr = &[: m.member :];  // workaround for private access
                (obj.*memptr)(std::forward<decltype(args)>(args)...);
            };

            builderMembers.push_back(
                std::meta::data_member_spec(
                    ^^WithMethod<P, builderMethod, BitMask, RequiredMask>,  
                    {.name = std::meta::identifier_of(m.member)} 
                )
            );
        }
        else if constexpr (m.type == MemberType::Data)
        {
            using Arg = typename[:std::meta::type_of(m.member):]; 
            auto builderMethod = [](P::ObjectType& obj, Arg arg){
                obj.[:m.member:] = std::move(arg);
            };

            builderMembers.push_back(
                std::meta::data_member_spec(
                    ^^WithMethod<P, builderMethod, BitMask, RequiredMask>, 
                    {.name = makeWithName(std::meta::identifier_of(m.member))} 
                )
            );
        }
    }

    return builderMembers;
}

template<typename P, uint64_t RequiredMask>
auto createBuilder(typename P::HolderType holder) 
{
    struct B;
    consteval {
        auto builderMembers = defineBuilderMembers<P, RequiredMask>();
        std::meta::define_aggregate(^^B, builderMembers);
    } 
    return FinalBuilder<P, B, RequiredMask>(std::move(holder));
}

template<typename T, typename Tag>
auto makeBuilder(auto... args) 
{
    static constexpr auto requiredMask = ClassRegistry<T>::requiredMask();
    static_assert(requiredMask == requiredMask);

    using Policy = BuilderPolicy<Tag, T>;
    auto holder = Policy::create(std::forward<decltype(args)>(args)...);

    return createBuilder<Policy, requiredMask>(holder);
}


template<typename T>
auto makeSharedBuilder(auto... args)
{
    return makeBuilder<T, Shared>(std::forward<decltype(args)>(args)...);
}

template<typename T>
auto makeUniqueBuilder(auto... args)
{
    return makeBuilder<T, Unique>(std::forward<decltype(args)>(args)...);
}

template<typename T>
auto makeValueBuilder(auto... args)
{
    return makeBuilder<T, Value>(std::forward<decltype(args)>(args)...);
}

}