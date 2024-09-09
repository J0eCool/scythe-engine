#pragma once

#include "common.h"
#include "test.h"

#include <functional>
#include <fstream>
#include <vector>

template <typename T>
class Serialize {
    struct Field {
        const char* name;

        std::function<void (std::istream&)> read;
        std::function<void (std::ostream&)> write;
    };
    std::vector<Field> fields;
public:
    Serialize(T &item) {}

    template <typename V>
    void addField(const char* name, V &field) {
        auto read = [&](std::istream &is) {
            is >> field;
        };
        auto write = [&](std::ostream &os) {
            os << field << ' ';
        };
        fields.push_back({name, read, write});
    }
    // SFINAE
    // template <typename V>
    // void addField(const char* name, V &field) {
    //     auto read = [&](std::istream &is) {
    //         serialize(field).read(is);
    //     };
    //     auto write = [&](std::ostream &os) {
    //         serialize(field).write(os);
    //     };
    //     fields.emplace_back(name, read, write);
    // }

    void read(std::istream &in) {
        for (auto &field : fields) {
            field.read(in);
        }
    }
    void write(std::ostream &out) {
        for (auto &field : fields) {
            field.write(out);
        }
    }
};

template <typename T>
std::istream& operator>>(std::istream &in, Serialize<T> &serial) {
    serial.read(in);
    return in;
}
template <typename T>
std::ostream& operator<<(std::ostream &out, Serialize<T> &serial) {
    serial.write(out);
    return out;
}

template <typename T>
static void saveToFile(const char* filename, T &item) {
    std::ofstream file;
    file.open(filename);
    auto serial = serialize(item);
    file << serial;
    file.close();
}

template <typename T>
static bool loadFromFile(const char* filename, T &item) {
    std::ifstream file;
    file.open(filename);
    if (check(file.is_open(), "could not open file: %s", filename)) {
        auto serial = serialize(item);
        file >> serial;
        file.close();
        return true;
    }
    return false;
}

TEST(foo, {
    TEST_EQ_MSG(2+2, 4, "addition works");
})
