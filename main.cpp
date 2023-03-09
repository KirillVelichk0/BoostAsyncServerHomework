#include <iostream>
#include "Server.h"
using namespace std;

int main()
{
    auto demon = ServerDemon::Create();
    auto demonHelper = demon->GetPtr();
    std::cout << "instance created" << std::endl;
    demonHelper->RunDemon();
}
