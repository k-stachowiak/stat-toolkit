#include <iostream>
#include <sstream>
#include <string>
using namespace std;

int main() {
	stringstream ss;

	ss.seekg(0);
	ss.seekp(0);

	string s("2");
	ss << s;

	double dbl;
	ss >> dbl;

	if(!ss) 
		cout << "Conversion failed" << endl;
	else
		cout << "Conversion succeeded" << endl;

	ss.seekg(0);
	ss.seekp(0);

	ss << s;
	ss >> dbl;

	if(!ss) 
		cout << "2nd Conversion failed" << endl;
	else
		cout << "2nd Conversion succeeded" << endl;

	return 0;
}
