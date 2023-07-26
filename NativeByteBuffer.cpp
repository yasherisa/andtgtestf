/*
 * This is the source code of tgnet library v. 1.1
 * It is licensed under GNU GPL v. 2 or later.
 * You should have received a copy of the license in this archive (see LICENSE).
 *
 * Copyright Nikolai Kudashov, 2015-2018.
 */

#include <memory.h>
#include <stdlib.h>
#include "NativeByteBuffer.h"
#include "FileLog.h"
#include "ByteArray.h"
#include "ConnectionsManager.h"
#include "BuffersStorage.h"

static int buffersCount = 0;

NativeByteBuffer::NativeByteBuffer(uint32_t size) {
#ifdef ANDROID
    if (jclass_ByteBuffer != nullptr) {
        JNIEnv *env = 0;
        if (javaVm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
            if (LOGS_ENABLED) DEBUG_E("can't get jnienv");
            exit(1);
        }
        javaByteBuffer = env->CallStaticObjectMethod(jclass_ByteBuffer, jclass_ByteBuffer_allocateDirect, size);
        if (javaByteBuffer == nullptr) {
            if (LOGS_ENABLED) DEBUG_E("can't create javaByteBuffer");
            exit(1);
        }
        DEBUG_REF("nativebytebuffer");
        jobject globalRef = env->NewGlobalRef(javaByteBuffer);
        env->DeleteLocalRef(javaByteBuffer);
        javaByteBuffer = globalRef;
        buffer = (uint8_t *) env->GetDirectBufferAddress(javaByteBuffer);
        bufferOwner = false;
    } else {
#endif
        buffer = new uint8_t[size];
        bufferOwner = true;
#ifdef ANDROID
    }
#endif
    if (buffer == nullptr) {
        if (LOGS_ENABLED) DEBUG_E("can't allocate NativeByteBuffer buffer");
        exit(1);
    }

    _limit = _capacity = size;
}

NativeByteBuffer::NativeByteBuffer(bool calculate) {
    calculateSizeOnly = calculate;
}

NativeByteBuffer::NativeByteBuffer(uint8_t *buff, uint32_t length) {
    buffer = buff;
    sliced = true;
    _limit = _capacity = length;
}

NativeByteBuffer::~NativeByteBuffer() {
#ifdef ANDROID
    if (javaByteBuffer != nullptr) {
        JNIEnv *env = 0;
        if (javaVm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
		    if (LOGS_ENABLED) DEBUG_E("can't get jnienv");
            exit(1);
	    }
        DEBUG_DELREF("nativebytebuffer");
        env->DeleteGlobalRef(javaByteBuffer);
        javaByteBuffer = nullptr;
    }
#endif
    if (bufferOwner && !sliced && buffer != nullptr) {
        delete[] buffer;
        buffer = nullptr;
    }
    _limit = _capacity = 0;
}

uint32_t NativeByteBuffer::position() {
    return _position;
}

void NativeByteBuffer::position(uint32_t position) {
    if (position > _limit) {
        return;
    }
    _position = position;
}

uint32_t NativeByteBuffer::capacity() {
    return _capacity;
}

uint32_t NativeByteBuffer::limit() {
    return _limit;
}

uint32_t NativeByteBuffer::remaining() {
    return _limit - _position;
}

void NativeByteBuffer::clearCapacity() {
    if (!calculateSizeOnly) {
        return;
    }
    _capacity = 0;
}

void NativeByteBuffer::limit(uint32_t limit) {
    if (limit > _capacity) {
        return;
    }
    if (_position > limit) {
        _position = limit;
    }
    _limit = limit;
}

void NativeByteBuffer::flip() {
    _limit = _position;
    _position = 0;
}

void NativeByteBuffer::clear() {
    _position = 0;
    _limit = _capacity;
}

uint8_t *NativeByteBuffer::bytes() {
    return buffer;
}

void NativeByteBuffer::rewind() {
    _position = 0;
}

void NativeByteBuffer::compact() {
    if (_position == _limit) {
        return;
    }
    memmove(buffer, buffer + _position, sizeof(uint8_t) * (_limit - _position));
    _position = (_limit - _position);
    _limit = _capacity;
}

bool NativeByteBuffer::hasRemaining() {
    return _position < _limit;
}

void NativeByteBuffer::skip(uint32_t length) {
    if (!calculateSizeOnly) {
        if (_position + length > _limit) {
            return;
        }
        _position += length;
    } else {
        _capacity += length;
    }
}

int32_t NativeByteBuffer::readInt32(bool *error) {
    if (_position + 4 > _limit || calculateSizeOnly) {
        if (error != nullptr) {
            *error = true;
        }
        if (LOGS_ENABLED) DEBUG_E("read int32 error");
        return 0;
    }
    int32_t result = ((buffer[_position] & 0xff)) |
                     ((buffer[_position + 1] & 0xff) << 8) |
                     ((buffer[_position + 2] & 0xff) << 16) |
                     ((buffer[_position + 3] & 0xff) << 24);
    _position += 4;
    return result;
}

uint32_t NativeByteBuffer::readUint32(bool *error) {
    return (uint32_t) readInt32(error);
}

uint64_t NativeByteBuffer::readUint64(bool *error) {
    return (uint64_t) readInt64(error);
}

int32_t NativeByteBuffer::readBigInt32(bool *error) {
    if (_position + 4 > _limit) {
        if (error != nullptr) {
            *error = true;
        }
        if (LOGS_ENABLED) DEBUG_E("read big int32 error");
        return 0;
    }
    int32_t result = ((buffer[_position] & 0xff) << 24) |
                     ((buffer[_position + 1] & 0xff) << 16) |
                     ((buffer[_position + 2] & 0xff) << 8) |
                     ((buffer[_position + 3] & 0xff));
    _position += 4;
    return result;
}

int64_t NativeByteBuffer::readInt64(bool *error) {
    if (_position + 8 > _limit) {
        if (error != nullptr) {
            *error = true;
        }
        if (LOGS_ENABLED) DEBUG_E("read int64 error");
        return 0;
    }
    int64_t result = ((int64_t) (buffer[_position] & 0xff)) |
                     ((int64_t) (buffer[_position + 1] & 0xff) << 8) |
                     ((int64_t) (buffer[_position + 2] & 0xff) << 16) |
                     ((int64_t) (buffer[_position + 3] & 0xff) << 24) |
                     ((int64_t) (buffer[_position + 4] & 0xff) << 32) |
                     ((int64_t) (buffer[_position + 5] & 0xff) << 40) |
                     ((int64_t) (buffer[_position + 6] & 0xff) << 48) |
                     ((int64_t) (buffer[_position + 7] & 0xff) << 56);
    _position += 8;
    return result;
}

uint8_t NativeByteBuffer::readByte(bool *error) {
    if (_position + 1 > _limit || calculateSizeOnly) {
        if (error != nullptr) {
            *error = true;
        }
        if (LOGS_ENABLED) DEBUG_E("read byte error");
        return 0;
    }
    return buffer[_position++];
}

bool NativeByteBuffer::readBool(bool *error) {
    uint32_t consructor = readUint32(error);
    if (consructor == 0x997275b5) {
        return true;
    } else if (consructor == 0xbc799737) {
        return false;
    }
    if (error != nullptr) {
        *error = true;
        if (LOGS_ENABLED) DEBUG_E("read bool error");
    }
    return false;
}

void NativeByteBuffer::readBytes(uint8_t *b, uint32_t length, bool *error) {
    if (length > _limit - _position || calculateSizeOnly) {
        if (error != nullptr) {
            *error = true;
        }
        if (LOGS_ENABLED) DEBUG_E("read bytes error");
        return;
    }
    memcpy(b, buffer + _position, length);
    _position += length;
}

ByteArray *NativeByteBuffer::readBytes(uint32_t length, bool *error) {
    if (length > _limit - _position || calculateSizeOnly) {
        if (error != nullptr) {
            *error = true;
        }
        if (LOGS_ENABLED) DEBUG_E("read bytes error");
        return nullptr;
    }
    ByteArray *byteArray = new ByteArray(length);
    memcpy(byteArray->bytes, buffer + _position, sizeof(uint8_t) * length);
    _position += length;
    return byteArray;
}

std::string NativeByteBuffer::readString(bool *error) {
    uint32_t sl = 1;
    if (_position + 1 > _limit || calculateSizeOnly) {
        if (error != nullptr) {
            *error = true;
        }
        if (LOGS_ENABLED) DEBUG_E("read string error");
        return std::string("");
    }
    uint32_t l = buffer[_position++];
    if (l >= 254) {
        if (_position + 3 > _limit) {
            if (error != nullptr) {
                *error = true;
            }
            if (LOGS_ENABLED) DEBUG_E("read string error");
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
        if (LOGS_ENABLED) DEBUG_E("read string error");
        return std::string("");
    }
    std::string result = std::string((const char *) (buffer + _position), l);
    _position += l + addition;
    return result;
}

ByteArray *NativeByteBuffer::readByteArray(bool *error) {
    uint32_t sl = 1;
    if (_position + 1 > _limit || calculateSizeOnly) {
        if (error != nullptr) {
            *error = true;
        }
        if (LOGS_ENABLED) DEBUG_E("read byte array error");
        return nullptr;
    }
    uint32_t l = buffer[_position++];
    if (l >= 254) {
        if (_position + 3 > _limit) {
            if (error != nullptr) {
                *error = true;
            }
            if (LOGS_ENABLED) DEBUG_E("read byte array error");
            return nullptr;
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
        if (LOGS_ENABLED) DEBUG_E("read byte array error");
        return nullptr;
    }
    ByteArray *result = new ByteArray(l);
    memcpy(result->bytes, buffer + _position, sizeof(uint8_t) * l);
    _position += l + addition;
    return result;
}

NativeByteBuffer *NativeByteBuffer::readByteBuffer(bool copy, bool *error) {
    uint32_t sl = 1;
    if (_position + 1 > _limit || calculateSizeOnly) {
        if (error != nullptr) {
            *error = true;
        }
        if (LOGS_ENABLED) DEBUG_E("read byte buffer error");
        return nullptr;
    }
    uint32_t l = buffer[_position++];
    if (l >= 254) {
        if (_position + 3 > _limit) {
            if (error != nullptr) {
                *error = true;
            }
            if (LOGS_ENABLED) DEBUG_E("read byte buffer error");
            return nullptr;
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
        if (LOGS_ENABLED) DEBUG_E("read byte buffer error");
        return nullptr;
    }
    NativeByteBuffer *result = nullptr;
    if (copy) {
        result = BuffersStorage::getInstance().getFreeBuffer(l);
        memcpy(result->buffer, buffer + _position, sizeof(uint8_t) * l);
    } else {
        result = new NativeByteBuffer(buffer + _position, l);
    }
    _position += l + addition;
    return result;
}

double NativeByteBuffer::readDouble(bool *error) {
    double value;
    int64_t value2 = readInt64(error);
    memcpy(&value, &value2, sizeof(double));
    return value;
}

void NativeByteBuffer::reuse() {
    if (sliced) {
        return;
    }
    BuffersStorage::getInstance().reuseFreeBuffer(this);
}

#ifdef ANDROID
jobject NativeByteBuffer::getJavaByteBuffer() {
    if (javaByteBuffer == nullptr && javaVm != nullptr) {
        JNIEnv *env = 0;
        if (javaVm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
		    if (LOGS_ENABLED) DEBUG_E("can't get jnienv");
            exit(1);
	    }
        javaByteBuffer = env->NewDirectByteBuffer(buffer, _capacity);
        if (javaByteBuffer == nullptr) {
            if (LOGS_ENABLED) DEBUG_E("can't allocate NativeByteBuffer buffer");
            exit(1);
        }
        DEBUG_REF("nativebytebuffer");
        jobject globalRef = env->NewGlobalRef(javaByteBuffer);
        env->DeleteLocalRef(javaByteBuffer);
        javaByteBuffer = globalRef;
    }
    return javaByteBuffer;
}
#endif
