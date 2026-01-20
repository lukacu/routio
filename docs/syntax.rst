==================================================
Message Description Language
==================================================

While the messages can be read and written using the low-level API, it is more convenient to define message types using the Routio message description language. A generator tool then generates the necessary C++ and Python code to serialize and deserialize these message types.

This document describes the syntax of the message description language. The parser generates an Abstract Syntax Tree (AST) which is then mapped to C++ and Python code.

Overview
========

A description file consists of an optional namespace declaration followed by a series of declarations (enums, structures, messages, imports, includes, and external types).

.. code-block:: text

    Description = [Namespace] ZeroOrMore(Declaration)

----

Namespace Declaration
=====================

Declare a namespace for all types defined in the file.

Syntax
------

.. code-block:: text

    namespace <name>;

Rules
-----

- Namespace names consist of alphanumeric characters and dots (``.``)
- Dots separate namespace segments
- Each segment becomes a nested namespace in generated code

Example
-------

.. code-block:: text

    namespace my.package.messages;

Generated C++ namespace:

.. code-block:: cpp

    namespace my {
      namespace package {
        namespace messages {
          // types here
        }
      }
    }

Generated Python package:

.. code-block:: python

    my.package.messages  # as module path

----

Enumeration Declaration
========================

Define an enumeration type with named values.

Syntax
------

.. code-block:: text

    enumerate <name> {
        value1, value2, value3, ...
    }

Rules
-----

- Enumeration names are identifiers (alphanumeric + underscore)
- Values are comma-separated identifiers
- Values are automatically numbered starting from 0
- Trailing comma is optional

Example
-------

.. code-block:: text

    enumerate Color {
        Red, Green, Blue
    }

Generated C++:

.. code-block:: cpp

    enum Color { COLOR_Red, COLOR_Green, COLOR_Blue };

Generated Python:

.. code-block:: python

    Color = enum("Color", { 'Red' : 0, 'Green' : 1, 'Blue' : 2 })

----

Structure Declaration
======================

Define a data structure with typed fields.

Syntax
------

.. code-block:: text

    structure <name> {
        <type> <fieldname>;
        <type> <fieldname>;
        ...
    }

Rules
-----

- Structures are defined with curly braces
- Each field ends with a semicolon
- Fields consist of: type, optional array specifier, name, optional properties, optional default value

Example
-------

.. code-block:: text

    structure Point {
        float x;
        float y;
        float z = 0.0;
    }

Generated C++:

.. code-block:: cpp

    class Point {
    public:
        Point(float x = 0.0f, float y = 0.0f, float z = 0.0f) {
            this->x = x;
            this->y = y;
            this->z = z;
        };
        
        virtual ~Point() {};
        float x;
        float y;
        float z;
    };

----

Message Declaration
====================

Define a message type that can be sent over the message bus.

Syntax
------

.. code-block:: text

    message <name> {
        <type> <fieldname>;
        ...
    }

Rules
-----

- Messages have the same structure as structures
- Messages are registered with a unique hash identifier for serialization
- Each message can have subscriber and publisher classes in Python

Example
-------

.. code-block:: text

    message LocationUpdate {
        string device_id;
        float latitude;
        float longitude;
        int32 timestamp;
    }

Generated C++:

.. code-block:: cpp

    class LocationUpdate {
    public:
        LocationUpdate(string device_id = "", float latitude = 0.0f, 
                       float longitude = 0.0f, int32 timestamp = 0) {
            this->device_id = device_id;
            this->latitude = latitude;
            this->longitude = longitude;
            this->timestamp = timestamp;
        };
        
        virtual ~LocationUpdate() {};
        string device_id;
        float latitude;
        float longitude;
        int32 timestamp;
    };

Generated Python:

.. code-block:: python

    class LocationUpdateSubscriber(routio.Subscriber):
        def __init__(self, client, alias, callback):
            def _read(message):
                reader = routio.MessageReader(message)
                return LocationUpdate.read(reader)
            super(LocationUpdateSubscriber, self).__init__(
                client, alias, "<hash>", lambda x: callback(_read(x)))

    class LocationUpdatePublisher(routio.Publisher):
        def __init__(self, client, alias):
            super(LocationUpdatePublisher, self).__init__(client, alias, "<hash>")
        
        def send(self, obj):
            writer = routio.MessageWriter()
            LocationUpdate.write(writer, obj)
            super(LocationUpdatePublisher, self).send(writer)

----

Field Specification
====================

Fields in structures and messages have the following syntax:

.. code-block:: text

    <type> [<array>] <name> [<properties>] [= <default>];

Field Type
----------

Syntax
~~~~~~

.. code-block:: text

    <type>

Built-in Types
~~~~~~~~~~~~~~

- ``int8``, ``int16``, ``int32``, ``int64`` — Signed integers
- ``uint8``, ``uint16``, ``uint32``, ``uint64`` — Unsigned integers
- ``float32``, ``float64`` — Floating-point numbers
- ``float``, ``double`` — Floating-point aliases
- ``int`` — 32-bit integer alias
- ``bool`` — Boolean
- ``string`` — Text string

Custom Types
~~~~~~~~~~~~

- Any previously defined enumeration
- Any previously defined structure or message
- Any external type

Array Specifier
---------------

Syntax
~~~~~~

.. code-block:: text

    [<length>]          # Fixed-size array
    []                  # Dynamic array

Rules
~~~~~

- Square brackets indicate an array type
- Optional numeric length for fixed-size arrays
- Empty brackets for dynamic arrays
- Array length must be a non-negative integer

Examples
~~~~~~~~

.. code-block:: text

    structure Image {
        int32 width;
        int32 height;
        uint8[1024] raw_data;           # Fixed array of 1024 bytes
        float[] histogram;               # Dynamic array of floats
    }

Properties
----------

Syntax
~~~~~~

.. code-block:: text

    (<prop1> : <prop2> : <prop3> : ...)

or with keyword arguments:

.. code-block:: text

    (key1=value1 : key2=value2 : ...)

or mixed:

.. code-block:: text

    (value1 : value2 : key1=value1 : key2=value2)

Rules
~~~~~

- Properties are enclosed in parentheses
- Properties are separated by colons
- Properties can be positional values or keyword arguments
- Keyword properties use ``name=value`` syntax

Property Values
~~~~~~~~~~~~~~~

- Numeric literals: ``42``, ``3.14``, ``-10``, ``1.5e-3``
- String literals: ``"hello"``, ``"path/to/file"``
- Boolean literals: ``true``, ``false``

Example
~~~~~~~

.. code-block:: text

    structure Sensor {
        string name;
        float value (min=0.0 : max=100.0);
        int32 id;
    }

Default Values
---------------

Syntax
~~~~~~

.. code-block:: text

    = <value>

Rules
~~~~~

- Default values are assigned with the equals sign
- Must be one of: number, string, or boolean
- Default values are optional

Example
~~~~~~~

.. code-block:: text

    structure Config {
        string name = "default";
        int32 retry_count = 3;
        bool enabled = true;
    }

----

Include Declaration
====================

Include field definitions from another file.

Syntax
------

.. code-block:: text

    include "<filename>" [(<properties>)];

Rules
-----

- Filename is a string literal
- Optional properties can follow
- Includes allow composition of definitions

Example
-------

.. code-block:: text

    include "common.msg";

----

Import Declaration
===================

Import type definitions from another file.

Syntax
------

.. code-block:: text

    import "<filename>";

Rules
-----

- Filename is a string literal
- Imports make types from another file available for use

Example
-------

.. code-block:: text

    import "geometry.msg";

----

External Type Declaration
===========================

Define mappings for types in specific programming languages.

Syntax
------

.. code-block:: text

    external <typename> (
        language <langname> "<container>" 
            [from "<source1>" "<source2>" ...]
            [default "<default_expr>"]
            [read "<read_func>" write "<write_func>"];
        ...
    );

Rules
-----

External types map custom language-specific types to the serialization framework. This is achieved using the `external` declaration that has the following schema:

.. code-block:: text

    external <typename> (
        language <langname> "<container>" 
            from "<source1>" "<source2>" ...
            default "<default_expr>"
            read "<read_func>" write "<write_func>";
        ...
    );

The components are:

- ``language`` keyword specifies the target language (cpp, c++, python, py)
- ``container`` is the C++ class name or Python type name
- ``from`` lists include files or imports needed
- ``default`` specifies how to create a default instance
- ``read`` and ``write`` specify serialization functions

Example
-------

.. code-block:: text

    external Vector (
        language cpp "glm::vec3" 
            from "<glm/vec3.hpp>"
            default "glm::vec3(0.0f)"
            read "readVector" write "writeVector";
        language python "numpy.ndarray"
            from "numpy"
            read "read_array" write "write_array";
    );

----

Complete Example
=================

Here's a complete message file demonstrating all features:

.. code-block:: text

    namespace robotics.vision;

    enumerate ImageFormat {
        RGB8, RGBA8, Grayscale, Depth16
    }

    external Matrix (
        language cpp "cv::Mat"
            from "<opencv2/core.hpp>"
            read "readMat" write "writeMat";
        language python "numpy.ndarray"
            from "numpy"
            read "read_array" write "write_array";
    );

    structure CameraInfo {
        float focal_length = 525.0;
        float principal_x = 320.0;
        float principal_y = 240.0;
        int32 width = 640;
        int32 height = 480;
    }

    structure ImageData {
        string camera_id;
        ImageFormat format;
        int32 width;
        int32 height;
        uint8[] pixel_data;
        int64 timestamp;
    }

    message CameraFrame {
        CameraInfo info;
        ImageData data;
        Matrix depth_map;
    }

    message DetectionResult {
        string[] labels;
        float[] confidence;
        int32[][] bounding_boxes;
    }

----

Comments
========

The language supports Python-style comments:

Syntax
------

.. code-block:: text

    # This is a comment

Rules
-----

- Comments start with ``#`` and continue to end of line
- Comments can appear anywhere whitespace is allowed

Example
-------

.. code-block:: text

    # Camera message definitions
    namespace robotics.vision;

    # Supported image formats
    enumerate ImageFormat {
        RGB8,        # 8-bit RGB
        RGBA8,       # 8-bit RGBA with alpha channel
        Grayscale,   # Single channel grayscale
        Depth16      # 16-bit depth
    }

----

Type System
===========

Built-in Type Mappings
----------------------

======== ========== ========== ====== ==========
Language int32      float64    bool   string
======== ========== ========== ====== ==========
C++      ``int32_t`` ``double`` ``bool`` ``std::string``
Python   ``int``    ``float``  ``bool`` ``str``
======== ========== ========== ====== ==========

Type Inference
--------------

- Default constructors are automatically generated
- Arrays are converted to ``std::vector<T>`` in C++ and ``list`` in Python
- Fixed-size arrays become ``T[N]`` in C++ and ``list`` in Python

----

Error Handling
==============

The parser provides detailed error messages with:

- Filename
- Line number
- Column number
- Error description

Example error:

.. code-block:: text

    geometry.msg (line: 15, col: 8): Expected field type

----

Best Practices
===============

1. **Naming Conventions**

   - Types (structures, messages): PascalCase
   - Fields: snake_case
   - Enumerations: PascalCase
   - Enumeration values: UPPER_CASE

2. **Documentation**

   Use comments liberally:

   .. code-block:: text

       # Represents a 3D point in space
       structure Point {
           float x;
           float y;
           float z;
       }

3. **Field Organization**

   Group related fields together:

   .. code-block:: text

       structure Sensor {
           # Identity
           string id;
           string name;
           
           # Data
           float[] readings;
           int32 sample_rate;
           
           # Metadata
           int64 timestamp;
       }

4. **Default Values**

   Provide sensible defaults for optional fields:

   .. code-block:: text

       structure Config {
           int32 timeout = 5000;
           bool verbose = false;
           string log_level = "INFO";
       }
