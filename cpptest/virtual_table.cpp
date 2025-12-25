#include <iostream>
#include <cstddef>
#include <sys/mman.h>
#include <unistd.h>
using namespace std;

class Animal{public:virtual void say(){cout<<"...."<<endl;}};
class Fish:public Animal{public:void say() override{cout<<"Fish."<<endl;}};
class Cat:public Animal{public:void say() override{cout<<"Cat."<<endl;}};

int main() {
	Fish fishobj;
	Cat catobj;
        Animal* fish = &fishobj;
	Animal* cat=&catobj;
	void** ptrfish= *reinterpret_cast<void***>(fish);
	void** ptrcat = *reinterpret_cast<void***>(cat);
	cout<<"fish:"<<ptrfish[0]<<endl;
	cout<<"cat:"<<ptrcat[0]<<endl;
	size_t pageSize = sysconf(_SC_PAGESIZE);
	mprotect(reinterpret_cast<void*>((reinterpret_cast<size_t>(ptrfish)& ~(pageSize-1))),pageSize,PROT_READ|PROT_WRITE);
	mprotect(reinterpret_cast<void*>((reinterpret_cast<size_t>(ptrcat)& ~(pageSize-1))),pageSize,PROT_READ|PROT_WRITE);
	void* temp = ptrfish[0];
	ptrfish[0]=ptrcat[0];
	ptrcat[0]=temp;
	fish->say();
	cat->say();
        return 0;
}

