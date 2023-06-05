#include <cstdio>
#include <iostream>

struct Struct {
private:
	int PrivateField;	
protected:
	int ProtectedField;
public:
	double PublicField;
	static int StaticField;
	void Fn1() { }
	struct NestedClass { struct NestedAgain {}; };
};
class Class {
	
};
struct SelfReflTest {
    int ThisIsAField;
    void Test() {
        auto members = $TypeOf(SelfReflTest{}).$Members;
        for (const auto& [name, mem] : members) {
            std::cout << mem.$Name << std::endl;
        }
    }
};

int main() {
	SelfReflTest{}.Test();
}

#pragma Xfw_TUnitEnd
