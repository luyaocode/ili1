#include <iostream>
using namespace std;
int main()
{
	int a=0xaa;
	char c ='C';
	int b= 0xff;
	cout<<&a<<endl<<(void*)(&c)<<endl<<&b<<endl;
}
