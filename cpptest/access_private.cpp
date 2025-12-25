#include <iostream>
class CPersonPrivate{
public:
	void setAddr(const char* addr){m_pszAddr=addr;}
        char* m_pszName;
        const char* m_pszAddr = "Shenzhen";
        uint8_t m_u8Age;
};
class CPerson{
public:
	CPerson():d(new CPersonPrivate){}
	~CPerson(){delete d;d=nullptr;}
	void setAge(uint8_t age){d->m_u8Age=age;}
	virtual void setName(char* name){d->m_pszName=name;}
private:
	CPersonPrivate* d;
};

int main() {
    	CPerson obj;
	char** pSome = (char**)(((char*)(*((char**)((char*)(&obj)+sizeof(void**))))+sizeof(char*)));
       	std::cout << "> " << *pSome << std::endl;
	reinterpret_cast<CPersonPrivate*>((char*)(*((char**)((char*)(&obj)+sizeof(void**)))))->setAddr("Beijing");
       	std::cout << "> " << *pSome << std::endl;
    	return 0;
}

