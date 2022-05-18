#include <iostream>
#include <string>

using namespace std;

int main() {
	char* buf = "hello";
	string s{buf};
	if (s == "hello")
		cout << "bingo" << endl;
	else
		cout << "failure" << endl;

	return 0;
}