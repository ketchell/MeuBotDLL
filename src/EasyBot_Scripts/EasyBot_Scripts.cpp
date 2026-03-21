#include <iostream>

#include "../../proto_functions_client.h"
#include "../../const.h"
//python -m grpc_tools.protoc -IC:\Users\Wojciech\Desktop\Projects\EasyBot86 --python_out=. --grpc_python_out=. C:\Users\Wojciech\Desktop\Projects\EasyBot86\bot.proto
int main() {
    auto item = proto->findItemInContainers(3265, -1, 0);
    std::cout << proto->getItemId(item) << std::endl;
    proto->equipItem(item);
    return 0;
}
