Installation
================================

Supported platforms
-------------------
Currently, Routio only supports Linux platforms. Support for other POSIX based platforms is planned.

Dependencies
------------
The core Routio system with bindings for c++ requires no additional dependencies. If you want to use the included Python bindings, `PyBind11 <https://github.com/wjakob/pybind11>`_ is required. Since one of the provided examples shows video streaming with the ose of the OpenCV platform, `OpenCV <http://opencv.org/>`_ is required to build this example.  Additionally, you will need `CMake <https://cmake.org/>`_ to build the project.
Installation
------------
First, create a new directory and download the project from Github::
    
    mkdir project_directory_path
    cd project_directory_path
    git clone https://github.com/lukacu/routio

Then, use CMake to build the project::
    
    cd routio
    mkdir build
    cd build
    cmake ..
    make

Running the examples
--------------------
Routio comes with several example programs that demonstrate it's use. After building the project, they should be available in the ./bin directory. The following files should be created::
    
    routio-router 
    chat (simple chat application)
    chunked (a demonstration of splitting big messages into smaller, more optimal chunks)
    opencv (a demonstartion of sharing video using opencv)

Routio is based on a central process or router, which runs in the background and manages the communication between processes. Because of this, the router process must always be running in the background when you wish to use Routio. When starting the router process, you must prowide a socket name that other proccesses will connect to. To start the daemon, simply run::

    ./routio-router path_to_socket/socket_name

Alternativelly, if you wish to communicate over IP sockets, run the router daemon with::

    ./routio-router -i port 

With the daemon running, you can now start programs that will communicate with each other. For example, starting two instances of the chat example will allow you to send text messages between the instances.

Routio is now ready for use. Click :doc:`here <getting_started>` to start using the system.

