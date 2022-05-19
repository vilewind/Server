#include <memory>
#include <iostream>

using namespace std;

struct A
{
	A(int a, int b) : a(a), b(b) {}
	int a;
	int b;
};

struct B
{
	void *pa;
};

B test(shared_ptr<A> sa) {
	B b;
	cout << "test1 " << sa.use_count() << endl;
	b.pa = static_cast<void *>(sa.get());
	cout << "test2 " << sa.use_count() << endl;
	return b;
}

int main() {

	shared_ptr<A> sa(new A(1, 2));
	
	cout <<  "befor test " << sa.use_count() << endl;
/**
 * @bug 将智能指针的裸指针暴露后，不会影响shared_ptr的引用计数，因此无法达到RAII的目的
 * 
 */
	B b = test(sa);
	cout << "after test " << sa.use_count() << endl;
	A *a = static_cast<A*>(b.pa);
	cout << a->a << " " << a->b << endl;

	return 0;
}