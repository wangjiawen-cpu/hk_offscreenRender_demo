#ifndef FSBASEPROTOCOLABSTRACT_H
#define FSBASEPROTOCOLABSTRACT_H


//协议层抽象基类
class fsBaseProtocolAbstract
{
public:

    fsBaseProtocolAbstract()
    {
        m_bIsInit = false;
    }
    virtual ~fsBaseProtocolAbstract(){};

    virtual int init(const void *para) = 0;
    virtual int unInit() = 0;
    virtual int send_data(unsigned char *buff, unsigned int length) = 0;
    virtual int recvive_data(unsigned char *buff, unsigned int length,unsigned int *ip) = 0;

protected:
    bool m_bIsInit;
};

#endif // FSBASEPROTOCOLABSTRACT_H
