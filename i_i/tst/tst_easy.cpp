#include <QtTest>
#include <QDebug>

#include "a_a.hpp"

// add necessary includes here

class easy : public QObject
{
    Q_OBJECT

public:
    easy();
    ~easy();

private slots:
    void test_case1();
    void test_case2();

};

easy::easy()
{

}

easy::~easy()
{

}

enum EM_PARAM
{
    ONE=1,
    TWO=2
};

EXPLICIT(void,execParam,2)
void execParam(int a, bool bPrint)
{
    Q_UNUSED(bPrint)
    qDebug()<< "param: " << a;
}

void easy::test_case1()
{
    int a = 1;
    execParam(a,true);
}

void easy::test_case2()
{
    float a = 1.5f;
    execParam(a,true);
}

QTEST_APPLESS_MAIN(easy)

#include "tst_easy.moc"
