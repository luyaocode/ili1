#include <iostream>
using namespace std;
class CBanana
{
public:
	void eat()
	{
		cout<<"This Addr: "<<this<<endl;
		cout<<"You ate 1 banana."<<endl;
	}
	void pass()
	{
		cout<<"aaaa "<<this->aaaa<<endl;
	}
	virtual void vf()
	{
		cout<<"This Addr: "<<this<<endl;
                cout<<"You ate 2 banana."<<endl;
	}
	static void sf()
	{
                cout<<"You ate static banana."<<endl;
	}
private:
	char* aaaa;
};
int main()
{
	CBanana* pBanana=new CBanana;
	pBanana = nullptr;
	pBanana->eat();
	return 0;
}
