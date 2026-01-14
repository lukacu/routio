
#pragma once

#include <echolib/message.h>
#include <echolib/server.h>
#include <map>
#include <vector>
#include <set>

namespace echolib {

  class Service : public Server
  {
    friend ClientConnection;

  public:
    Service(SharedIOLoop loop, const std::string &address = std::string());
    ~Service();

    static bool comparator(const SharedClientConnection &lhs, const SharedClientConnection &rhs);

  private:
    virtual void handle_message(SharedClientConnection client, SharedMessage message);

    virtual void handle_disconnect(SharedClientConnection client);

    virtual void handle_connect(SharedClientConnection client);

    SharedClientConnection find(int fid);

    ClientSet clients;

    int64_t received_messages_size;

  };


}

