#include <iostream>
#include <vector>

using namespace std;

struct A;
using AVec = vector<A*>;

struct A
{
	A(int a, int b) : a(a), b(b){}
	void test(AVec& av) {
		av.emplace_back(this);
	}
	int a;
	int b;
	~A() { cout << "destructed" << endl;}
};


int main() {
	A a(1, 2);
	AVec av;
	a.test(av);

	cout << av[0]->a << " " << av[0]->b << endl;
	/*cout destructed
		1 0*/
	return 0;
}