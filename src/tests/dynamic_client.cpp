/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

#include <iostream>
#include <algorithm>
#include <string>
#include <thread>
#include <chrono>

#include <routio/client.h>
#include <routio/datatypes.h>

using namespace std;
using namespace routio;

int main(int argc, char** argv) {

    SharedIOLoop loop = make_shared<IOLoop>();

    for (int i = 0; i < 100; i++) {

        SharedClient client = make_shared<routio::Client>();

        cout << "Adding handler" << endl;

        loop->add_handler(client);
        
        loop->wait(100);

        cout << "Removing handler" << endl;

        loop->remove_handler(client);

        loop->wait(100);

    }

    exit(0);
}
