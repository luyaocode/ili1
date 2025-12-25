#include <iostream>
using namespace std;
int main()
{
	int i=0xff;
	cout<<">"<<&i<<endl;
	cout<<">>"<<&(&i)<<endl;
	cout<<">>>"<<&(&(&i))<<endl;
	int& j=i;
	int* k=&j;
	int*& m = k;
	int** x=&m;
	int**&y = x;
	int*** z = &y;
	***z=0x00;
	cout<<i<<endl;
	return 0;
}
