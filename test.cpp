#include <fstream>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>


class BinaryReader {
public:
    BinaryReader(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::in);
        std::vector<char> buffer(std::istreambuf_iterator<char>{file}, {});
        this->buffer = std::move(buffer);
        _position = 0;
        _limit = this->buffer.size();
    }

    ~BinaryReader() {}

    int32_t readInt32(bool* error = nullptr) {
        if (_position + 4 > _limit) {
            if (error != nullptr) {
                *error = true;
            }
            return 0;
        }
        int32_t result = ((buffer[_position] & 0xff)) |
                         ((buffer[_position + 1] & 0xff) << 8) |
                         ((buffer[_position + 2] & 0xff) << 16) |
                         ((buffer[_position + 3] & 0xff) << 24);
        _position += 4;
        return result;
    }

    uint32_t readUint32(bool* error = nullptr) {
        return static_cast<uint32_t>(readInt32(error));
    }

    int64_t readInt64(bool* error = nullptr) {
        if (_position + 8 > _limit) {
            if (error != nullptr) {
                *error = true;
            }
            return 0;
        }
        int64_t result = static_cast<int64_t>(buffer[_position] & 0xff) |
                         static_cast<int64_t>(buffer[_position + 1] & 0xff) << 8 |
                         static_cast<int64_t>(buffer[_position + 2] & 0xff) << 16 |
                         static_cast<int64_t>(buffer[_position + 3] & 0xff) << 24 |
                         static_cast<int64_t>(buffer[_position + 4] & 0xff) << 32 |
                         static_cast<int64_t>(buffer[_position + 5] & 0xff) << 40 |
                         static_cast<int64_t>(buffer[_position + 6] & 0xff) << 48 |
                         static_cast<int64_t>(buffer[_position + 7] & 0xff) << 56;
        _position += 8;
        return result;
    }

    uint64_t readUint64(bool* error = nullptr) {
        return static_cast<uint64_t>(readInt64(error));
    }

    bool readBool(bool* error = nullptr) {
        uint32_t constructor = readUint32(error);
      std::cout << "bool_int: " << constructor << std::endl;
        if (constructor == 0x997275b5) {
            return true;
        } else if (constructor == 0xbc799737) {
            return false;
        }
        if (error != nullptr) {
            *error = true;
        }
        return false;
    }

    std::string readString(bool* error = nullptr) {
        uint32_t sl = 1;
        if (_position + 1 > _limit) {
            if (error != nullptr) {
                *error = true;
            }
            return std::string("");
        }
        uint32_t l = buffer[_position++];
        if (l >= 254) {
            if (_position + 3 > _limit) {
                if (error != nullptr) {
                    *error = true;
                }
                return std::string("");
            }
            l = buffer[_position] | (buffer[_position + 1] << 8) | (buffer[_position + 2] << 16);
            _position += 3;
            sl = 4;
        }
        uint32_t addition = (l + sl) % 4;
        if (addition != 0) {
            addition = 4 - addition;
        }
        if (_position + l + addition > _limit) {
            if (error != nullptr) {
                *error = true;
            }
            return std::string("");
        }
        std::string result(buffer.begin() + _position, buffer.begin() + _position + l);
        _position += l + addition;
        return result;
    }

private:
    std::vector<char> buffer;
    size_t _position;
    size_t _limit;
};


using namespace std;
int main() {
  BinaryReader reader("tgnet.dat");
  cout << reader.readUint32(nullptr) << endl;
  cout << reader.readUint32(nullptr) << endl;
  cout << reader.readBool(nullptr) << endl;
  cout << reader.readBool(nullptr) << endl;
  cout << reader.readString(nullptr) << endl;
  cout << reader.readBool(nullptr) << endl;
  cout << reader.readUint32(nullptr) << endl;
  return 0;
}
