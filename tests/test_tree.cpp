#include <EagleLib/Nodes/NodeManager.h>
#include "EagleLib/Nodes/Node.h"

int main()
{
    auto node = EagleLib::NodeManager::getInstance().addNode("SerialStack");
    return 0;
}
