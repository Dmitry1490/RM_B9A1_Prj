#include "status.h"

Status::Status(QObject *parent) : QObject(parent)
{
    regUprPLIS          = 0;

}

Status::~Status(){}

// Регистр управления ПЛИС (получение)
unsigned char Status::getRegUprPLIS() const
{
    return regUprPLIS;
}





