#include <functional>
#include <any>
#include <vector>
#include <map>
#include <utility>
#include <string>
#include <cstddef>
#define $FWD(x) std::forward<decltype(x)>(x)
#define $CONSTANT(...) \
  union { static constexpr auto value() { return __VA_ARGS__; } }
#define $CONSTANT_VALUE(...) \
  [] { using R = $CONSTANT(__VA_ARGS__); return R{}; }()
enum $MemberTypes {
    $Field = 0,
    $Method = 1,
    $Ctor = 2,
    $Dtor = 3
};
struct $Dynamic {

};
struct $MemberInfo {
private:
#define _$GetValue_$Ret void*
#define _$GetValue_$Args void* /* obj */
#define _$GetValue_$FuncSig _$GetValue_$Ret (_$GetValue_$Args)
#define _$Invoke_$Ret void*
#define _$Invoke_$Args void* /* obj */, std::vector<void*> /* args */
#define _$Invoke_$FuncSig _$Invoke_$Ret (_$Invoke_$Args)

    void _ConstructField(
            std::string _Name,
            std::function<_$GetValue_$FuncSig> _GetValue
    ) {
        $Name = _Name;
        $MemberType = $MemberTypes::$Field;
        $GetValue = _GetValue;
        $Invoke = [] (_$Invoke_$Args) -> _$Invoke_$Ret {
            throw "Calling $Invoke w/ a $MemberInfo that is holding a $Field!";
        };
    }
    void _ConstructMethod(
            std::string _Name,
            std::function<_$Invoke_$FuncSig> _Invoke
    ) {
        $Name = _Name;
        $MemberType = $MemberTypes::$Method;
        $GetValue = [] (_$GetValue_$Args) -> _$GetValue_$Ret {
            throw "Calling $GetValue w/ a $MemberInfo that is holding a $Method!";
        };
        $Invoke = _Invoke;
    }
public:
    std::string $Name;
    $MemberTypes $MemberType;
    std::function<_$GetValue_$FuncSig> $GetValue;
    std::function<_$Invoke_$FuncSig> $Invoke;
    int8_t $NumArgs;

    template <typename $>
    constexpr $MemberInfo($, auto&&... _Args) {
        if constexpr ($::value() == $MemberTypes::$Field) {
            _ConstructField($FWD(_Args)...);
        } else if constexpr ($::value() == $MemberTypes::$Method) {
            _ConstructMethod($FWD(_Args)...);
        } else {
            static_assert("Unknown $MemberTypes");
        }
    }

};
struct $Type {
    std::map<std::string, $MemberInfo> $Members;
};

// From Stackoverflow
template <typename T> auto $fetch_back(T& $t) -> typename std::remove_reference<decltype($t.back())>::type {
    typename std::remove_reference<decltype($t.back())>::type $ret = $t.back();
    $t.pop_back();
    return *$ret;
}
template <typename X> struct $any_ref_cast {
    X $do_cast(std::any $y) { return std::any_cast<X>($y); }
};
template <typename X> struct $any_ref_cast<X&> {
    X& do_cast(std::any $y) {
        std::reference_wrapper<X> $ref = std::any_cast<std::reference_wrapper<X>>($y);
        return $ref.get();
    }
};
template <typename X> struct $any_ref_cast<const X&> {
    const X& do_cast(std::any $y) {
        std::reference_wrapper<const X> $ref = std::any_cast<std::reference_wrapper<const X>>($y);
        return $ref.get();
    }
};
template <typename Ret, typename... Args>
Ret $Dyncall(Ret (*fn)(Args...), std::vector<std::any> args) {
    if (sizeof...(Args) != args.size())
        throw "Argument number mismatch!";
    std::vector<std::any*> _tmp(args.size()); for (auto& arg : args) _tmp.push_back(&arg);
    return func($any_ref_cast<Args>().do_cast($fetch_back(_tmp))...);
}
template <typename Class, typename Ret, typename... Args>
Ret $Dyncall(Class obj, Ret (Class::*fn)(Args...), std::vector<std::any> args) {
    if (sizeof...(Args) != args.size())
        throw "Argument number mismatch!";
    std::vector<std::any*> _tmp(args.size()); for (auto& arg : args) _tmp.push_back(&arg);
    return (obj.*fn)(any_ref_cast<Args>().do_cast($fetch_back(_tmp))...);
}

struct $Reflector;
$Type $TypeOf(auto Val);
#line 1