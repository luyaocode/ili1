#include <iostream>
using namespace std;
class CBase{};
class CDrived:public CBase{};
class CDrivedOneProperty:public CBase{char m_c1;};
class CDrivedTwoProperty:public CBase{int m_s2;char m_c1;};
class CDrivedDrived:public CDrived{};
class CParentOneProperty{char m_c1;};
class CSon:public CParentOneProperty{};
class CVirtual{virtual ~CVirtual(){}};
int main(){
	cout<<"Size"<<endl;
	cout<<"CBase: "<<sizeof(CBase)<<endl;
	cout<<"CDrived: "<<sizeof(CDrived)<<endl;
	cout<<"CDrivedOneProperty: "<<sizeof(CDrivedOneProperty)<<endl;
	cout<<"CDrivedTwoProperty: "<<sizeof(CDrivedTwoProperty)<<endl;
	cout<<"CDrivedDrived: "<<sizeof(CDrivedDrived)<<endl;
	cout<<"------------------"<<endl;
	cout<<"CParentOneProperty: "<<sizeof(CParentOneProperty)<<endl;
	cout<<"CSon: "<<sizeof(CSon)<<endl;
	cout<<"CVirtual: "<<sizeof(CVirtual)<<endl;
	
}
