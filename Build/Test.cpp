#include <cstdio>
#include <iostream>
#include <vector>
#include <any>

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
};
class Class {
	
};
struct SelfReflTest {
    int ThisIsAField;
    float Test() {
        auto members = $TypeOf(SelfReflTest{}).$Members;
        for (const auto& [name, mem] : members) {
            std::cout << mem.$Name << std::endl;
        }
        return 0.03;
    }
    void Test2() {
        auto members = $TypeOf(Struct{}).$Members;
        auto ret_ptr = (ReturnStruct*) members["Fn2"].$Invoke(nullptr, std::vector<std::any> {});
        std::cout << (*ret_ptr).TestVar << " " << (*ret_ptr).Test2Var << std::endl;
        ret_ptr->~ReturnStruct();
        delete[] ret_ptr;
    }
    void Test3() {
        auto members = $TypeOf(Struct{}).$Members;
        auto obj = Struct{};
        std::cout << "Before invocation: " << obj.PublicField << std::endl;
        auto ret_ptr = (double*) members["Fn3"].$Invoke((void*) &obj, std::vector<std::any> {std::any(5)});
        std::cout << "After invocation: " << obj.PublicField << std::endl;
        (*ret_ptr) += 10.66;
        std::cout << "After changing: " << obj.PublicField << std::endl;
    }
};

int main() {
	//SelfReflTest{}.Test();
    //SelfReflTest{}.Test2();
    SelfReflTest{}.Test3();
}

#pragma Xfw_TUnitEnd
