#include <string>
#include "google/protobuf/message.h"

namespace yaml2pb
{
    void yaml2pb(google::protobuf::Message &message, const std::string &buf);
    std::string pb2yaml(const google::protobuf::Message &message);
}