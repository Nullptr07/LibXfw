#include <cstdio>
#include <iostream>
#include <vector>
#include <any>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/compile.h>
#include "/home/nullptr07/Xfw/Deps/dylib.hpp"

struct ReturnStruct {
    int TestVar;
    double Test2Var;

    ReturnStruct() {
        // make it non-trivial
        TestVar = 33;
        Test2Var = 0.3;
    }
    ReturnStruct (int a, double b) : TestVar (a), Test2Var(b) {}
};

struct Struct {
private:
	int PrivateField;	
protected:
	int ProtectedField;
public:
	double PublicField;
	//static int StaticField;
	void Fn1() { }
    static ReturnStruct Fn2() { std::cout << "Fn2() called!" << std::endl; return ReturnStruct {99,  0.001}; }
    double& Fn3(int q) {
        std::cout << "Fn3() called!" << std::endl;
        PublicField += q;
        return PublicField;
    }
	struct NestedClass { struct NestedAgain {}; };
    template <typename T> T Fn4() { return T{}; }
};
class Class {
	
};
struct ReflTest {
    int ThisIsAField;
    float Test() {
        std::cout << "================== SelfReflTest::Test() BEGIN ==================" << std::endl;
        auto members = $TypeOf(ReflTest{}).$Members;
        for (const auto& [name, mem] : members) {
            std::cout << mem.$Name << std::endl;
        }
        std::cout << "================== SelfReflTest::Test() END ==================" << std::endl;
        return 0.03;
    }
    void Test2() {
        std::cout << "================== SelfReflTest::Test2() BEGIN ==================" << std::endl;
        auto members = $TypeOf(Struct{}).$Members;
        auto ret = std::any_cast<ReturnStruct>(members["Fn2"].$Invoke(nullptr, std::vector<std::any> {}));
        std::cout << ret.TestVar << " " << ret.Test2Var << std::endl;
        std::cout << "================== SelfReflTest::Test2() END ==================" << std::endl;
    }
    void Test3() {
        std::cout << "================== SelfReflTest::Test3() BEGIN ==================" << std::endl;
        auto members = $TypeOf(Struct{}).$Members;
        auto obj = Struct{};
        std::cout << "Before invocation: " << obj.PublicField << std::endl;
        auto ret_ptr = std::any_cast<std::reference_wrapper<double>>(members["Fn3"].$Invoke((void*) &obj, std::vector<std::any> {std::any(5)}));
        std::cout << "After invocation: " << obj.PublicField << std::endl;
        ret_ptr.get() += 10.66;
        std::cout << "After changing: " << obj.PublicField << std::endl;
        std::cout << "================== SelfReflTest::Test3() END ==================" << std::endl;
    }
};

struct Object {
    char Type; // '0' -> VAR_DECL (variables), '1' -> FUNCTION_DECL (functions)
    unsigned long Addr;

    template <typename... T>
    Object& operator()(T...) {
        // if Type is VAR, T should be {const char*} (sizeof...(T) == 1),
        // and this should return the Object to another member (like MyField, MyMemberFn, whatever.)

        // if type is FUNC, T can be anything and this should return a
        // lambda that wraps the function invocation and return an Object
        return *this;
    }
};
Object AddrTable(const char* name) {
    return Object {.Type = '0', .Addr = 0x12345};
}

int a_global_variable = 55;
int foobar;

struct TestStruct { static void Fn() {} };
int intermediate_fn(int x) { return x + 30; }
template <int I, typename T>
void call_test2() {
    std::cout << "!! Called call_test2() !!" << std::endl;
    int here_is_a_var = intermediate_fn(a_global_variable) + 10;
    //TestStruct::Fn();
    std::cout << I << ' ' << typeid(T).name() << std::endl;
    //AddrTable("std::cout")("operator<<")(35)("operator<<")(AddrTable("std::endl"));
    //AddrTable("MyVar")("ReturnsPtrToAnotherClass")()
}
void call_test() {
    std::cout << "Called call_test()" << std::endl;
    int j = 0;
    int k = 0;
}

int main() {
	ReflTest{}.Test();
    ReflTest{}.Test2();
    ReflTest{}.Test3();
    call_test();
    call_test2$(3, "int");
}

#pragma Xfw_TUnitEnd
