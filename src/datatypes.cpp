

#include <echolib/datatypes.h>

namespace echolib {

Header::Header(std::string source, std::chrono::system_clock::time_point timestamp): source(source), timestamp(timestamp) {

}

Header::~Header() {

}

// Explicit template instantiations to ensure symbols are exported in shared library

template<>
shared_ptr<Dictionary> Message::unpack(SharedMessage message) {

    MessageReader reader(message);

    shared_ptr<echolib::Dictionary> dictionary(new echolib::Dictionary);

    read(reader, *dictionary);

    return dictionary;
}

template<>
shared_ptr<Message> Message::pack(const Dictionary &data) {
    std::map<std::string, std::string>::const_iterator iter;

    ssize_t length = message_length(data);

    MessageWriter writer(length);

    write(writer, data);

    return make_shared<BufferedMessage>(writer);
}

template<>
shared_ptr<Header> Message::unpack(SharedMessage message) {

    MessageReader reader(message);

    shared_ptr<echolib::Header> header(new echolib::Header);

    read(reader, *header);

    return header;
}

template<>
shared_ptr<Message> Message::pack(const Header &data) {

    MessageWriter writer(sizeof(size_t) + data.source.size() + sizeof(time_point));

    write(writer, data.source);
    write(writer, data.timestamp);

    return make_shared<BufferedMessage>(writer);
}

template <> string get_type_identifier<Dictionary>() { return string("dictionary"); }

}
