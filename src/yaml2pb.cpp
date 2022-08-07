#include <_types/_uint32_t.h>
#include <_types/_uint64_t.h>
#include <exception>
#include <sstream>
#include <string>
#include <vector>

#include "google/protobuf/message.h"
#include "google/protobuf/descriptor.h"
#include "yaml-cpp/yaml.h"

#include "yaml2pb/yaml2pb.h"
#include "base64.h"

namespace yaml2pb
{
    class exception : public std::exception
    {
        std::string _error;

    public:
        exception(const std::string &e)
            : _error(e)
        {
        }
        exception(const google::protobuf::FieldDescriptor *field, const std::string &e)
            : _error(field->name() + ": " + e)
        {
        }
        virtual ~exception() throw(){};

        virtual const char *what() const throw() { return _error.c_str(); };
    };

    static void yaml2pb(google::protobuf::Message &message, const YAML::Node &node);
    static void pb2yaml(YAML::Node &node, const google::protobuf::Message &message);

    static void yaml2field(google::protobuf::Message &msg, const google::protobuf::FieldDescriptor *field, const YAML::Node &node)
    {
        const google::protobuf::Reflection *ref = msg.GetReflection();
        const bool repeated = field->is_repeated();

        switch (field->cpp_type())
        {
#define _SET_OR_ADD(setfunc, addfunc, __value)  \
    do                                          \
    {                                           \
        if (repeated)                           \
            ref->addfunc(&msg, field, __value); \
        else                                    \
            ref->setfunc(&msg, field, __value); \
    } while (0)

#define _CONVERT(pbtype, ctype, setfunc, addfunc)          \
    case google::protobuf::FieldDescriptor::pbtype:        \
        _SET_OR_ADD(setfunc, addfunc, node.as<ctype>());   \
        break;

            _CONVERT(CPPTYPE_DOUBLE, double, SetDouble, AddDouble);
            _CONVERT(CPPTYPE_FLOAT, float, SetFloat, AddFloat);
            _CONVERT(CPPTYPE_INT64, int64_t, SetInt64, AddInt64);
            _CONVERT(CPPTYPE_UINT64, uint64_t, SetUInt64, AddUInt64);
            _CONVERT(CPPTYPE_INT32, int32_t, SetInt32, AddInt32);
            _CONVERT(CPPTYPE_UINT32, uint32_t, SetUInt32, AddUInt32);
            _CONVERT(CPPTYPE_BOOL, bool, SetBool, AddBool);

#undef _CONVERT

        case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
            std::string value = node.as<std::string>();
            if (field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES)
            {
                std::vector<BYTE> data = base64_decode(value);
                std::string str(data.begin(), data.end());
                _SET_OR_ADD(SetString, AddString, str);
            }
            else
            {
                _SET_OR_ADD(SetString, AddString, value);
            }
            break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
            google::protobuf::Message *mf = (repeated) ? ref->AddMessage(&msg, field) : ref->MutableMessage(&msg, field);
            yaml2pb(*mf, node);
            break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: {
            const google::protobuf::EnumDescriptor *ed = field->enum_type();
            const google::protobuf::EnumValueDescriptor *ev = 0;

            try
            {
                ev = ed->FindValueByNumber(node.as<int>());
            }
            catch (...)
            {
                try
                {
                    ev = ed->FindValueByName(node.as<std::string>());
                }
                catch (...)
                {
                    throw exception("invalid enum type");
                }
            }
            if (!ev)
                throw exception(field, "Enum value not found");
            _SET_OR_ADD(SetEnum, AddEnum, ev);
            break;
        }
        default:
            break;
        }
    }

    static void yaml2pb(google::protobuf::Message &message, const YAML::Node &node)
    {
        const google::protobuf::Descriptor *d = message.GetDescriptor();
        const google::protobuf::Reflection *ref = message.GetReflection();
        if (!d || !ref)
            throw exception("No descriptor or reflection");

        for (YAML::const_iterator it = node.begin(); it != node.end(); it++)
        {
            std::string name = it->first.as<std::string>();
            const YAML::Node &value = it->second;

            const google::protobuf::FieldDescriptor *field = d->FindFieldByName(name);
            if (!field)
                field = ref->FindKnownExtensionByName(name);
            if (!field)
                throw exception("unknown field " + name);

            if (field->is_repeated())
            {
                if (!value.IsSequence())
                    throw exception(field, "invalid array");

                for (YAML::const_iterator it2 = value.begin(); it2 != value.end(); it2++)
                    yaml2field(message, field, *it2);
            }
            else
            {
                yaml2field(message, field, value);
            }
        }
    }

    void yaml2pb(google::protobuf::Message &message, const std::string &buf)
    {
        YAML::Node node = YAML::Load(buf);
        if (!node.IsMap())
            throw exception("invalid node");
        yaml2pb(message, node);
    }

    static void field2yaml(YAML::Node &node, const google::protobuf::Message &message, const google::protobuf::FieldDescriptor *field, int index)
    {
        const google::protobuf::Reflection *ref = message.GetReflection();
        const bool repeated = field->is_repeated();

        switch (field->cpp_type())
        {
#define _CONVERT(type, ctype, setfunc, addfunc)                                         \
    case google::protobuf::FieldDescriptor::type:                                       \
        node = (repeated) ? ref->addfunc(message, field, index) : ref->setfunc(message, field); \
        break;

            _CONVERT(CPPTYPE_DOUBLE, double, GetDouble, GetRepeatedDouble);
            _CONVERT(CPPTYPE_FLOAT, float, GetFloat, GetRepeatedFloat);
            _CONVERT(CPPTYPE_INT64, int64_t, GetInt64, GetRepeatedInt64);
            _CONVERT(CPPTYPE_UINT64, uint64_t, GetUInt64, GetRepeatedUInt64);
            _CONVERT(CPPTYPE_INT32, int32_t, GetInt32, GetRepeatedInt32);
            _CONVERT(CPPTYPE_UINT32, uint32_t, GetUInt32, GetRepeatedUInt32);
            _CONVERT(CPPTYPE_BOOL, bool, GetBool, GetRepeatedBool);
#undef _CONVERT

        case google::protobuf::FieldDescriptor::CPPTYPE_STRING: {
            std::string scratch;
            const std::string &value = (repeated) ? ref->GetRepeatedStringReference(message, field, index, &scratch) : ref->GetStringReference(message, field, &scratch);
            if (field->type() == google::protobuf::FieldDescriptor::TYPE_BYTES)
                node = base64_encode((const BYTE *)value.c_str(), value.size());
            else
                node = value;
            break;
        }

        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE: {
            const google::protobuf::Message &mf = (repeated) ? ref->GetRepeatedMessage(message, field, index) : ref->GetMessage(message, field);
            pb2yaml(node, mf);
            break;
        }

        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: {
            const google::protobuf::EnumValueDescriptor *ef = (repeated) ? ref->GetRepeatedEnum(message, field, index) : ref->GetEnum(message, field);
            node = ef->name();
            break;
        }

        default:
            break;
        }

        if (node.IsNull())
            throw exception(field, "Fail to convert to yaml");
    }

    static void pb2yaml(YAML::Node &node, const google::protobuf::Message &message)
    {
        const google::protobuf::Descriptor *d = message.GetDescriptor();
        const google::protobuf::Reflection *ref = message.GetReflection();
        if (!d || !ref)
            throw exception("No descriptor or reflection");

        std::vector<const google::protobuf::FieldDescriptor *> fields;
        ref->ListFields(message, &fields);

        for (std::vector<const google::protobuf::FieldDescriptor *>::iterator it = fields.begin(); it != fields.end(); it++)
        {
            const google::protobuf::FieldDescriptor *field = *it;

            const std::string &name = (field->is_extension()) ? field->full_name() : field->name();
            if (field->is_repeated())
            {
                size_t count = ref->FieldSize(message, field);
                if (!count)
                    continue;

                for (size_t j = 0; j < count; j++)
                {
                    YAML::Node item;
                    field2yaml(item, message, field, j);
                    node[name].push_back(item);
                }
            }
            else if (ref->HasField(message, field))
            {
                YAML::Node item;
                field2yaml(item, message, field, 0);
                node[name] = item;
            }
        }
    }

    std::string pb2yaml(const google::protobuf::Message &message)
    {
        YAML::Node root;
        pb2yaml(root, message);
        std::ostringstream oss;
        oss << root << std::endl;
        return oss.str();
    }
} // namespace yaml2pb
