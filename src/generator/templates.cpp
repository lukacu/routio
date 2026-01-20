// codegen.cpp
// Implementation of C++ code generator for the message description language.

#include "templates.h"

#include <sstream>
#include <algorithm>
#include <iomanip>
#include <functional>
#include <cstring>

namespace routio::generator {

// --------------------------- TypeMetadata implementation ---------------------------

std::string TypeMetadata::getContainer(const std::string& language) const {
    auto it = containers.containers.find(language);
    return it != containers.containers.end() ? it->second : name;
}

std::string TypeMetadata::getDefault(const std::string& language) const {
    auto it = containers.defaults.find(language);
    return it != containers.defaults.end() ? it->second : "";
}

std::string TypeMetadata::getReader(const std::string& language) const {
    auto it = containers.readers.find(language);
    return it != containers.readers.end() ? it->second : "";
}

std::string TypeMetadata::getWriter(const std::string& language) const {
    auto it = containers.writers.find(language);
    return it != containers.writers.end() ? it->second : "";
}

// --------------------------- TypeRegistry ---------------------------

TypeRegistry::TypeRegistry() {
    registerBuiltinTypes_();
}

TypeMetadata TypeRegistry::createBuiltinType_(const std::string& name,
                                             const std::map<std::string, std::string>& containers,
                                             const std::map<std::string, std::string>& defaults) {
    TypeMetadata meta;
    meta.name = name;
    meta.is_builtin = true;
    meta.hash = computeHash(name);
    meta.containers.containers = containers;
    meta.containers.defaults = defaults;
    
    // Add standard includes for types
    if (name == "string") {
        meta.sources = {"string"};
    } else if (name == "array" || name == "tensor") {
        meta.sources = {"vector", "routio/array.h", "numpy"};
    } else if (name == "timestamp") {
        meta.sources = {"chrono", "datetime"};
    }
    
    return meta;
}

void TypeRegistry::registerBuiltinTypes_() {
    // Basic numeric types
    registerType(createBuiltinType_("int8",
        {{"cpp", "int8_t"}, {"python", "int"}},
        {{"cpp", "0"}, {"python", "0"}}));
    
    registerType(createBuiltinType_("int16",
        {{"cpp", "int16_t"}, {"python", "int"}},
        {{"cpp", "0"}, {"python", "0"}}));
    
    registerType(createBuiltinType_("int32",
        {{"cpp", "int32_t"}, {"python", "int"}},
        {{"cpp", "0"}, {"python", "0"}}));
    
    registerType(createBuiltinType_("int64",
        {{"cpp", "int64_t"}, {"python", "routio.long"}},
        {{"cpp", "0"}, {"python", "0"}}));
    
    registerType(createBuiltinType_("uint8",
        {{"cpp", "uint8_t"}, {"python", "int"}},
        {{"cpp", "0"}, {"python", "0"}}));
    
    registerType(createBuiltinType_("uint16",
        {{"cpp", "uint16_t"}, {"python", "int"}},
        {{"cpp", "0"}, {"python", "0"}}));
    
    registerType(createBuiltinType_("uint32",
        {{"cpp", "uint32_t"}, {"python", "int"}},
        {{"cpp", "0"}, {"python", "0"}}));
    
    registerType(createBuiltinType_("uint64",
        {{"cpp", "uint64_t"}, {"python", "int"}},
        {{"cpp", "0"}, {"python", "0"}}));
    
    registerType(createBuiltinType_("float32",
        {{"cpp", "float"}, {"python", "float"}},
        {{"cpp", "0.0f"}, {"python", "0.0"}}));
    
    registerType(createBuiltinType_("float64",
        {{"cpp", "double"}, {"python", "routio.double"}},
        {{"cpp", "0.0"}, {"python", "0.0"}}));
    
    registerType(createBuiltinType_("bool",
        {{"cpp", "bool"}, {"python", "bool"}},
        {{"cpp", "false"}, {"python", "False"}}));
    
    registerType(createBuiltinType_("string",
        {{"cpp", "std::string"}, {"python", "str"}},
        {{"cpp", "\"\""}, {"python", "\"\""}}));
    
    // Convenience aliases
    registerType(createBuiltinType_("int",
        {{"cpp", "int32_t"}, {"python", "int"}},
        {{"cpp", "0"}, {"python", "0"}}));
    
    registerType(createBuiltinType_("float",
        {{"cpp", "float"}, {"python", "float"}},
        {{"cpp", "0.0f"}, {"python", "0.0"}}));
    
    registerType(createBuiltinType_("double",
        {{"cpp", "double"}, {"python", "routio.double"}},
        {{"cpp", "0.0"}, {"python", "0.0"}}));
    
    // Special types matching Python MessagesRegistry
    registerType(createBuiltinType_("char",
        {{"cpp", "char"}, {"python", "routio.char"}},
        {{"cpp", "'\\0'"}, {"python", "'\\0'"}}));
    
    TypeMetadata timestamp;
    timestamp.name = "timestamp";
    timestamp.is_builtin = true;
    timestamp.hash = computeHash("timestamp");
    timestamp.containers.containers = {
        {"cpp", "std::chrono::system_clock::time_point"},
        {"python", "datetime.datetime"}
    };
    timestamp.sources = {"chrono", "datetime"};
    registerType(timestamp);
    
    TypeMetadata header;
    header.name = "header";
    header.is_builtin = true;
    header.hash = computeHash("header");
    header.containers.containers = {
        {"cpp", "routio::Header"},
        {"python", "routio.Header"}
    };
    header.containers.defaults = {
        {"cpp", "routio::Header()"},
        {"python", "routio.Header()"}
    };
    header.sources = {"routio/datatypes.h"};
    registerType(header);
    
    TypeMetadata array;
    array.name = "array";
    array.is_builtin = true;
    array.hash = computeHash("array");
    array.containers.containers = {
        {"cpp", "routio::Array"},
        {"python", "numpy.ndarray"}
    };
    array.containers.defaults = {
        {"cpp", "routio::Array()"},
        {"python", "numpy.zeros((0,))"}
    };
    array.sources = {"routio/array.h", "numpy"};
    registerType(array);
    
    TypeMetadata tensor;
    tensor.name = "tensor";
    tensor.is_builtin = true;
    tensor.hash = computeHash("tensor");
    tensor.containers.containers = {
        {"cpp", "routio::Tensor"},
        {"python", "numpy.ndarray"}
    };
    tensor.containers.defaults = {
        {"cpp", "routio::Tensor()"},
        {"python", "numpy.zeros((0,))"}
    };
    tensor.sources = {"routio/array.h", "numpy"};
    registerType(tensor);
}

void TypeRegistry::registerType(const TypeMetadata& metadata) {
    types_[metadata.name] = metadata;
}

void TypeRegistry::registerEnum(const std::string& name, const std::map<std::string, int>& values) {
    enums_[name] = values;
    
    // Register as a type too
    TypeMetadata meta;
    meta.name = name;
    meta.hash = computeHash(name);
    meta.is_builtin = false;
    for (const auto& [val, idx] : values) {
        meta.hash = computeHash(meta.hash + val);
    }
    registerType(meta);
}

void TypeRegistry::registerStruct(const std::string& name, const std::map<std::string, Field>& fields) {
    structs_[name] = fields;
    
    // Register as a type
    TypeMetadata meta;
    meta.name = name;
    meta.is_builtin = false;
    std::string content = name;
    for (const auto& [fname, field] : fields) {
        content += field.type + fname;
    }
    meta.hash = computeHash(content);
    registerType(meta);
}

void TypeRegistry::registerMessage(const std::string& name, const std::map<std::string, Field>& fields) {
    messages_.push_back(name);
    registerStruct(name, fields);
}

void TypeRegistry::registerExternal(const External& ext) {
    TypeMetadata meta;
    meta.name = ext.name;
    meta.is_external = true;
    meta.hash = computeHash(ext.name);
    
    for (const auto& lang : ext.languages) {
        meta.containers.containers[lang.language] = lang.container;
        if (lang.deflt) {
            meta.containers.defaults[lang.language] = *lang.deflt;
        }
        if (lang.read) {
            meta.containers.readers[lang.language] = *lang.read;
        }
        if (lang.write) {
            meta.containers.writers[lang.language] = *lang.write;
        }
        meta.sources.insert(meta.sources.end(), lang.sources.begin(), lang.sources.end());
    }
    
    registerType(meta);
}

const TypeMetadata* TypeRegistry::getType(const std::string& name) const {
    auto it = types_.find(name);
    return it != types_.end() ? &it->second : nullptr;
}

const std::map<std::string, int>* TypeRegistry::getEnum(const std::string& name) const {
    auto it = enums_.find(name);
    return it != enums_.end() ? &it->second : nullptr;
}

const std::map<std::string, Field>* TypeRegistry::getStruct(const std::string& name) const {
    auto it = structs_.find(name);
    return it != structs_.end() ? &it->second : nullptr;
}

bool TypeRegistry::isMessage(const std::string& name) const {
    return std::find(messages_.begin(), messages_.end(), name) != messages_.end();
}

std::string TypeRegistry::getHash(const std::string& name) const {
    const auto* meta = getType(name);
    return meta ? meta->hash : computeHash(name);
}

std::vector<std::string> TypeRegistry::getSources(const std::string& language) const {
    std::vector<std::string> sources;
    
    // Add language-specific default sources
    if (language == "cpp") {
        sources = {"vector", "chrono", "routio/datatypes.h", "routio/array.h"};
    } else if (language == "python") {
        sources = {"routio", "datetime", "numpy"};
    }
    
    // Add sources from all types
    for (const auto& [name, meta] : types_) {
        for (const auto& src : meta.sources) {
            if (std::find(sources.begin(), sources.end(), src) == sources.end()) {
                sources.push_back(src);
            }
        }
    }
    
    return sources;
}

std::string TypeRegistry::computeHash(const std::string& content) const {
    // Simple hash implementation - can be replaced with proper MD5/SHA256 if needed
    unsigned char hash[16] = {0};
    for (size_t i = 0; i < content.size(); ++i) {
        hash[i % 16] ^= content[i];
    }
    
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 16; ++i) {
        oss << std::setw(2) << (int)hash[i];
    }
    return oss.str();
}

// --------------------------- CppGenerator ---------------------------

CppGenerator::CppGenerator(const Description& desc) : desc_(desc) {
    if (desc_.ns) {
        namespace_ = desc_.ns->name;
    }
    buildRegistry_();
}

void CppGenerator::buildRegistry_() {
    registry_.registerBuiltinTypes_();
    
    for (const auto& decl : desc_.decls) {
        std::visit([this](auto&& d) {
            using T = std::decay_t<decltype(d)>;
            if constexpr (std::is_same_v<T, Enumerate>) {
                std::map<std::string, int> values;
                int idx = 0;
                for (const auto& val : d.values) {
                    values[val.name] = idx++;
                }
                registry_.registerEnum(d.name, values);
            }
            else if constexpr (std::is_same_v<T, Structure>) {
                std::map<std::string, Field> fields;
                for (const auto& f : d.fields.fields) {
                    fields[f.name] = f;
                }
                registry_.registerStruct(d.name, fields);
            }
            else if constexpr (std::is_same_v<T, Message>) {
                std::map<std::string, Field> fields;
                for (const auto& f : d.fields.fields) {
                    fields[f.name] = f;
                }
                registry_.registerMessage(d.name, fields);
            }
            else if constexpr (std::is_same_v<T, External>) {
                TypeMetadata meta;
                meta.name = d.name;
                for (const auto& lang : d.languages) {
                    if (lang.language == "cpp" || lang.language == "c++") {
                        meta.containers.containers["cpp"] = lang.container;
                        meta.sources.insert(meta.sources.end(), lang.sources.begin(), lang.sources.end());
                        if (lang.deflt) meta.containers.defaults["cpp"] = *lang.deflt;
                        if (lang.read) meta.containers.readers["cpp"] = *lang.read;
                        if (lang.write) meta.containers.writers["cpp"] = *lang.write;
                    }
                    else if (lang.language == "python" || lang.language == "py") {
                        meta.containers.containers["python"] = lang.container;
                        meta.sources.insert(meta.sources.end(), lang.sources.begin(), lang.sources.end());
                        if (lang.deflt) meta.containers.defaults["python"] = *lang.deflt;
                        if (lang.read) meta.containers.readers["python"] = *lang.read;
                        if (lang.write) meta.containers.writers["python"] = *lang.write;
                    }
                }
                registry_.registerType(meta);
            }
        }, decl);
    }
}

std::string CppGenerator::generate(const std::string& basename) {
    std::ostringstream out;
    
    out << "// This is an autogenerated file, do not modify!\n\n";
    out << generateHeader_(basename);
    out << generateIncludes_();
    out << "\nnamespace routio {\n\n";
    out << generateTypeSpecializations_();
    out << "}\n\n";
    out << generateNamespaceOpen_();
    out << generateEnums_();
    out << generateForwardDeclarations_();
    out << generateStructs_();
    out << generateNamespaceClose_();
    out << "\nnamespace routio {\n\n";
    out << generateEnumSerializers_();
    out << generateStructSerializers_();
    out << generateMessageSpecializations_();
    out << "}\n\n";
    out << "#endif\n";
    
    return out.str();
}

std::string CppGenerator::generateHeader_(const std::string& basename) {
    std::string guard = basename;
    std::transform(guard.begin(), guard.end(), guard.begin(), ::toupper);
    
    std::ostringstream out;
    out << "#ifndef __" << guard << "_MSGS_H\n";
    out << "#define __" << guard << "_MSGS_H\n\n";
    return out.str();
}

std::string CppGenerator::generateIncludes_() {
    std::ostringstream out;
    auto sources = registry_.getSources("cpp");
    for (const auto& src : sources) {
        out << "#include <" << src << ">\n";
    }
    return out.str();
}

std::string CppGenerator::generateTypeSpecializations_() {
    std::ostringstream out;
    
    for (const auto& [name, meta] : registry_.getTypes()) {
        std::string reader = meta.getReader("cpp");
        std::string writer = meta.getWriter("cpp");
        std::string container = meta.getContainer("cpp");
        
        if (!reader.empty() && !writer.empty()) {
            out << "template <> inline void read(MessageReader& reader, " 
                << container << "& dst) {\n";
            out << "\tdst = " << reader << "(reader);\n";
            out << "}\n\n";
            
            out << "template <> inline void write(MessageWriter& writer, const " 
                << container << "& src) {\n";
            out << "\t" << writer << "(writer, src);\n";
            out << "}\n\n";
        }
    }
    
    return out.str();
}

std::string CppGenerator::generateNamespaceOpen_() {
    if (namespace_.empty()) return "";
    
    std::ostringstream out;
    std::istringstream iss(namespace_);
    std::string part;
    while (std::getline(iss, part, '.')) {
        out << "namespace " << part << " {\n";
    }
    out << "\n";
    return out.str();
}

std::string CppGenerator::generateNamespaceClose_() {
    if (namespace_.empty()) return "";
    
    std::ostringstream out;
    std::istringstream iss(namespace_);
    std::string part;
    std::vector<std::string> parts;
    while (std::getline(iss, part, '.')) {
        parts.push_back(part);
    }
    
    for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
        out << "}\n";
    }
    return out.str();
}

std::string CppGenerator::generateEnums_() {
    std::ostringstream out;
    
    for (const auto& [name, values] : registry_.getEnums()) {
        out << "enum " << name << " { ";
        bool first = true;
        for (const auto& [val, idx] : values) {
            if (!first) out << ", ";
            std::string upper_name = name;
            std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(), ::toupper);
            out << upper_name << "_" << val;
            first = false;
        }
        out << " };\n\n";
    }
    
    return out.str();
}

std::string CppGenerator::generateForwardDeclarations_() {
    std::ostringstream out;
    
    for (const auto& [name, fields] : registry_.getStructs()) {
        out << "class " << name << ";\n";
    }
    out << "\n";
    
    return out.str();
}

std::string CppGenerator::formatValue_(const Value& val) const {
    return std::visit([](auto&& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, double>) {
            std::ostringstream oss;
            oss << v;
            return oss.str();
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            return "\"" + v + "\"";
        }
        else if constexpr (std::is_same_v<T, bool>) {
            return v ? "true" : "false";
        }
        return "";
    }, val);
}

std::string CppGenerator::getFieldType_(const Field& field) const {
    auto* meta = registry_.getType(field.type);
    std::string baseType = meta ? meta->getContainer("cpp") : field.type;
    
    if (field.array) {
        if (field.array->length) {
            // Fixed-size array
            return baseType + "[" + std::to_string(*field.array->length) + "]";
        } else {
            // Dynamic array
            return "std::vector<" + baseType + ">";
        }
    }
    
    return baseType;
}

std::string CppGenerator::getDefaultValue_(const Field& field) const {
    if (field.default_value) {
        return formatValue_(*field.default_value);
    }
    
    auto* meta = registry_.getType(field.type);
    if (meta) {
        std::string defval = meta->getDefault("cpp");
        if (!defval.empty()) {
            return defval;
        }
    }
    
    if (field.array) {
        if (field.array->length) {
            return "{}";
        } else {
            std::string baseType = meta ? meta->getContainer("cpp") : field.type;
            return "std::vector<" + baseType + ">()";
        }
    }
    
    return (meta ? meta->getContainer("cpp") : field.type) + "()";
}

std::string CppGenerator::generateStructs_() {
    std::ostringstream out;
    
    for (const auto& [name, fields] : registry_.getStructs()) {
        out << "class " << name << " {\n";
        out << "public:\n";
        
        // Constructor
        out << "\t" << name << "(\n";
        bool first = true;
        for (const auto& [fname, field] : fields) {
            if (!first) out << ",\n";
            out << "\t\t" << getFieldType_(field) << " " << fname 
                << " = " << getDefaultValue_(field);
            first = false;
        }
        out << "\n\t) {\n";
        
        // Constructor body
        for (const auto& [fname, field] : fields) {
            out << "\t\tthis->" << fname << " = " << fname << ";\n";
        }
        out << "\t};\n\n";
        
        out << "\tvirtual ~" << name << "() {};\n";
        
        // Fields
        for (const auto& [fname, field] : fields) {
            out << "\t" << getFieldType_(field) << " " << fname << ";\n";
        }
        
        out << "};\n\n";
    }
    
    return out.str();
}

std::string CppGenerator::getCppNamespace_() const {
    if (namespace_.empty()) return "";
    std::string ns = namespace_;
    std::replace(ns.begin(), ns.end(), '.', ':');
    return "::" + ns + "::";
}

std::string CppGenerator::generateEnumSerializers_() {
    std::ostringstream out;
    std::string cppns = getCppNamespace_();
    
    for (const auto& [name, values] : registry_.getEnums()) {
        std::string upper_name = name;
        std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(), ::toupper);
        
        // Read
        out << "template <> inline void read(MessageReader& reader, " 
            << cppns << name << "& dst) {\n";
        out << "\tswitch (reader.read<int>()) {\n";
        for (const auto& [val, idx] : values) {
            out << "\tcase " << idx << ": dst = " << cppns << upper_name 
                << "_" << val << "; break;\n";
        }
        out << "\t}\n";
        out << "}\n\n";
        
        // Write
        out << "template <> inline void write(MessageWriter& writer, const " 
            << cppns << name << "& src) {\n";
        out << "\tswitch (src) {\n";
        for (const auto& [val, idx] : values) {
            out << "\tcase " << cppns << upper_name << "_" << val 
                << ": writer.write<int>(" << idx << "); return;\n";
        }
        out << "\t}\n";
        out << "}\n\n";
    }
    
    return out.str();
}

std::string CppGenerator::generateStructSerializers_() {
    std::ostringstream out;
    std::string cppns = getCppNamespace_();
    
    for (const auto& [name, fields] : registry_.getStructs()) {
        // Read
        out << "template <> inline void read(MessageReader& reader, " 
            << cppns << name << "& dst) {\n";
        for (const auto& [fname, field] : fields) {
            out << "\tread(reader, dst." << fname << ");\n";
        }
        out << "}\n\n";
        
        // Write
        out << "template <> inline void write(MessageWriter& writer, const " 
            << cppns << name << "& src) {\n";
        for (const auto& [fname, field] : fields) {
            out << "\twrite(writer, src." << fname << ");\n";
        }
        out << "}\n\n";
    }
    
    return out.str();
}

std::string CppGenerator::generateMessageSpecializations_() {
    std::ostringstream out;
    std::string cppns = getCppNamespace_();
    
    for (const auto& msgName : registry_.getMessages()) {
        std::string hash = registry_.getHash(msgName);
        
        // get_type_identifier
        out << "template <> inline string get_type_identifier<" 
            << cppns << msgName << ">() { return string(\"" << hash << "\"); }\n\n";
        
        // pack
        out << "template<> inline shared_ptr<Message> routio::Message::pack<" 
            << cppns << msgName << ">(const " << cppns << msgName << " &data) {\n";
        out << "\tMessageWriter writer;\n";
        out << "\twrite(writer, data);\n";
        out << "\treturn make_shared<BufferedMessage>(writer);\n";
        out << "}\n\n";
        
        // unpack
        out << "template<> inline shared_ptr<" << cppns << msgName 
            << "> routio::Message::unpack<" << cppns << msgName << ">(SharedMessage message) {\n";
        out << "\tMessageReader reader(message);\n";
        out << "\tshared_ptr<" << cppns << msgName << "> result(new " 
            << cppns << msgName << "());\n";
        out << "\tread(reader, *result);\n";
        out << "\treturn result;\n";
        out << "}\n\n";
    }
    
    return out.str();
}

// --------------------------- Public API ---------------------------

std::string generate_cpp(const Description& desc, const std::string& basename) {
    CppGenerator gen(desc);
    return gen.generate(basename);
}

// --------------------------- PythonGenerator ---------------------------

PythonGenerator::PythonGenerator(const Description& desc) : desc_(desc) {
    buildRegistry_();
}

void PythonGenerator::buildRegistry_() {
    registry_.registerBuiltinTypes_();
    
    for (const auto& decl : desc_.decls) {
        std::visit([this](auto&& d) {
            using T = std::decay_t<decltype(d)>;
            if constexpr (std::is_same_v<T, Enumerate>) {
                std::map<std::string, int> values;
                int idx = 0;
                for (const auto& val : d.values) {
                    values[val.name] = idx++;
                }
                registry_.registerEnum(d.name, values);
            }
            else if constexpr (std::is_same_v<T, Structure>) {
                std::map<std::string, Field> fields;
                for (const auto& f : d.fields.fields) {
                    fields[f.name] = f;
                }
                registry_.registerStruct(d.name, fields);
            }
            else if constexpr (std::is_same_v<T, Message>) {
                std::map<std::string, Field> fields;
                for (const auto& f : d.fields.fields) {
                    fields[f.name] = f;
                }
                registry_.registerMessage(d.name, fields);
            }
            else if constexpr (std::is_same_v<T, External>) {
                TypeMetadata meta;
                meta.name = d.name;
                for (const auto& lang : d.languages) {
                    if (lang.language == "cpp" || lang.language == "c++") {
                        meta.containers.containers["cpp"] = lang.container;
                        meta.sources.insert(meta.sources.end(), lang.sources.begin(), lang.sources.end());
                        if (lang.deflt) meta.containers.defaults["cpp"] = *lang.deflt;
                        if (lang.read) meta.containers.readers["cpp"] = *lang.read;
                        if (lang.write) meta.containers.writers["cpp"] = *lang.write;
                    }
                    else if (lang.language == "python" || lang.language == "py") {
                        meta.containers.containers["python"] = lang.container;
                        meta.sources.insert(meta.sources.end(), lang.sources.begin(), lang.sources.end());
                        if (lang.deflt) meta.containers.defaults["python"] = *lang.deflt;
                        if (lang.read) meta.containers.readers["python"] = *lang.read;
                        if (lang.write) meta.containers.writers["python"] = *lang.write;
                    }
                }
                registry_.registerType(meta);
            }
        }, decl);
    }
}

std::string PythonGenerator::generate() {
    std::ostringstream out;
    
    out << generateHeader_();
    out << generateImports_();
    out << generateEnumHelper_();
    out << generateEnums_();
    out << generateExternalTypes_();
    out << generateStructs_();
    out << generateMessages_();
    
    return out.str();
}

std::string PythonGenerator::generateHeader_() {
    return "# This is an autogenerated file, do not modify!\n"
           "from __future__ import absolute_import\n"
           "from __future__ import division\n"
           "from __future__ import print_function\n"
           "from __future__ import unicode_literals\n\n"
           "from builtins import super\n\n";
}

std::string PythonGenerator::generateImports_() {
    std::ostringstream out;
    auto sources = registry_.getSources();
    for (const auto& src : sources) {
        out << "import " << src << "\n";
    }
    if (!sources.empty()) out << "\n";
    return out.str();
}

std::string PythonGenerator::generateEnumHelper_() {
    return "def enum(name, enums):\n"
           "    reverse = dict((value, key) for key, value in enums.items())\n"
           "    enums[\"str\"] = staticmethod(lambda x: reverse[x])\n"
           "    return type(name, (), enums)\n\n"
           "def enum_conversion(enum, obj):\n"
           "    if isinstance(obj, int):\n"
           "        return obj\n"
           "    if isinstance(obj, str):\n"
           "        return getattr(enum, obj)\n"
           "    return 0\n\n";
}

std::string PythonGenerator::generateEnums_() {
    std::ostringstream out;
    
    for (const auto& [name, values] : registry_.getEnums()) {
        out << name << " = enum(\"" << name << "\", { ";
        bool first = true;
        for (const auto& [val, idx] : values) {
            if (!first) out << ", ";
            out << "'" << val << "' : " << idx;
            first = false;
        }
        out << " })\n\n";
        
        out << "routio.registerType(" << name 
            << ", lambda x: x.readInt(), lambda x, o: x.writeInt(o), "
            << "lambda x: enum_conversion(" << name << ", x))\n\n";
    }
    
    return out.str();
}

std::string PythonGenerator::generateExternalTypes_() {
    std::ostringstream out;
    
    for (const auto& [name, meta] : registry_.getTypes()) {
        std::string reader = meta.getReader("python");
        std::string writer = meta.getWriter("python");
        if (!reader.empty() && !writer.empty()) {
            out << "routio.registerType(" << meta.getContainer("python") 
                << ", " << reader 
                << ", " << writer << " )\n";
        }
    }
    if (!registry_.getTypes().empty()) out << "\n";
    
    return out.str();
}

std::string PythonGenerator::formatValue_(const Value& val) const {
    return std::visit([](auto&& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, double>) {
            std::ostringstream oss;
            oss << v;
            return oss.str();
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            return "\"" + v + "\"";
        }
        else if constexpr (std::is_same_v<T, bool>) {
            return v ? "True" : "False";
        }
        return "None";
    }, val);
}

std::string PythonGenerator::getPythonType_(const Field& field) const {
    auto* meta = registry_.getType(field.type);
    return meta ? meta->getContainer("python") : field.type;
}

std::string PythonGenerator::getDefaultValue_(const Field& field) const {
    if (field.default_value) {
        return formatValue_(*field.default_value);
    }
    
    if (field.array) {
        return "None";
    }
    
    auto* meta = registry_.getType(field.type);
    return meta ? meta->getDefault("python") : "None";
}

bool PythonGenerator::isArray_(const Field& field) const {
    return field.array.has_value();
}

bool PythonGenerator::isFixedArray_(const Field& field) const {
    return field.array && field.array->length.has_value();
}

std::string PythonGenerator::generateStructs_() {
    std::ostringstream out;
    
    for (const auto& [name, fields] : registry_.getStructs()) {
        out << "class " << name << "(object):\n";
        out << "    def __init__(self";
        
        // Constructor parameters
        for (const auto& [fname, field] : fields) {
            out << ",\n        " << fname << " = " << getDefaultValue_(field);
        }
        out << "):\n";
        
        // Constructor body
        for (const auto& [fname, field] : fields) {
            if (isArray_(field)) {
                out << "        if " << fname << " is None:\n";
                out << "            self." << fname << " = []\n";
                out << "        else:\n";
                out << "            self." << fname << " = " << fname << "\n";
            } else if (!field.default_value) {
                auto* meta = registry_.getType(field.type);
                auto defval = meta ? meta->getDefault("python") : "None";
                if (defval == "None") {
                    out << "        if " << fname << " is None:\n";
                    out << "            self." << fname << " = " 
                        << getPythonType_(field) << "()\n";
                    out << "        else:\n";
                    out << "            self." << fname << " = " << fname << "\n";
                } else {
                    out << "        self." << fname << " = " << fname << "\n";
                }
            } else {
                out << "        self." << fname << " = " << fname << "\n";
            }
        }
        out << "        pass\n\n";
        
        // Read method
        out << "    @staticmethod\n";
        out << "    def read(reader):\n";
        out << "        dst = " << name << "()\n";
        for (const auto& [fname, field] : fields) {
            if (isArray_(field)) {
                out << "        dst." << fname << " = routio.readList(" 
                    << getPythonType_(field) << ", reader)\n";
            } else {
                out << "        dst." << fname << " = routio.readType(" 
                    << getPythonType_(field) << ", reader)\n";
            }
        }
        out << "        return dst\n\n";
        
        // Write method
        out << "    @staticmethod\n";
        out << "    def write(writer, obj):\n";
        for (const auto& [fname, field] : fields) {
            if (isArray_(field)) {
                out << "        routio.writeList(" << getPythonType_(field) 
                    << ", writer, obj." << fname << ")\n";
            } else {
                out << "        routio.writeType(" << getPythonType_(field) 
                    << ", writer, obj." << fname << ")\n";
            }
        }
        out << "        pass\n\n";
        
        // Register type
        out << "routio.registerType(" << name << ", " << name 
            << ".read, " << name << ".write)\n\n";
    }
    
    return out.str();
}

std::string PythonGenerator::generateMessages_() {
    std::ostringstream out;
    
    for (const auto& msgName : registry_.getMessages()) {
        std::string hash = registry_.getHash(msgName);
        
        // Subscriber class
        out << "class " << msgName << "Subscriber(routio.Subscriber):\n\n";
        out << "    def __init__(self, client, alias, callback):\n";
        out << "        def _read(message):\n";
        out << "            reader = routio.MessageReader(message)\n";
        out << "            return " << msgName << ".read(reader)\n\n";
        out << "        super(" << msgName << "Subscriber, self).__init__(client, alias, \"" 
            << hash << "\", lambda x: callback(_read(x)))\n\n\n";
        
        // Publisher class
        out << "class " << msgName << "Publisher(routio.Publisher):\n\n";
        out << "    def __init__(self, client, alias):\n";
        out << "        super(" << msgName << "Publisher, self).__init__(client, alias, \"" 
            << hash << "\")\n\n";
        out << "    def send(self, obj):\n";
        out << "        writer = routio.MessageWriter()\n";
        out << "        " << msgName << ".write(writer, obj)\n";
        out << "        super(" << msgName << "Publisher, self).send(writer)\n\n";
    }
    
    return out.str();
}

std::string generate_python(const Description& desc) {
    PythonGenerator gen(desc);
    return gen.generate();
}

} // namespace routio::generator
