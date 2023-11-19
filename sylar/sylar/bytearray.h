#pragma once
#ifndef __SYLAR_BYTEARRAY_H__
#define __SYLAR_BYTEARRAY_H__

#include <memory>
#include <string>
#include <vector>
#include "util.h"

namespace sylar{

class ByteArray {
public:
    using ptr = std::shared_ptr<ByteArray>;

    //ByteArray�洢�Ľڵ�
    struct Node {

        //����ָ����С���ڴ��
        Node(size_t s);

        //�޲ι���
        Node();

        ~Node();

        //�ͷ��ڴ�
        void free();

        /// �ڴ���ַָ��
        char* ptr;
        /// ��һ���ڴ���ַ
        Node* next;
        /// �ڴ���С
        size_t size;

    };


    /**
    * @brief ʹ��ָ�����ȵ��ڴ�鹹��ByteArray
    * @param[in] base_size �ڴ���С
    */
    ByteArray(size_t base_size = 4096);

    /**
    * @brief �����ⲿ�����ڴ�,���ownerΪfalse,��֧��д����
    * @param[in] data �ڴ�ָ��
    * @param[in] size ���ݴ�С
    * @param[in] owner �Ƿ������ڴ�
    */
    ByteArray(void* data, size_t size, bool owner = false);

    ~ByteArray();

    /**
    * @brief д��̶�����int8_t���͵�����
    * @post m_position += sizeof(value)
    *       ���m_position > m_size �� m_size = m_position
    */
    void writeFint8(int8_t value);
    /**
     * @brief д��̶�����uint8_t���͵�����
     * @post m_position += sizeof(value)
     *       ���m_position > m_size �� m_size = m_position
     */
    void writeFuint8(uint8_t value);
    /**
     * @brief д��̶�����int16_t���͵�����(���/С��)
     * @post m_position += sizeof(value)
     *       ���m_position > m_size �� m_size = m_position
     */
    void writeFint16(int16_t value);
    /**
     * @brief д��̶�����uint16_t���͵�����(���/С��)
     * @post m_position += sizeof(value)
     *       ���m_position > m_size �� m_size = m_position
     */
    void writeFuint16(uint16_t value);

    /**
     * @brief д��̶�����int32_t���͵�����(���/С��)
     * @post m_position += sizeof(value)
     *       ���m_position > m_size �� m_size = m_position
     */
    void writeFint32(int32_t value);

    /**
     * @brief д��̶�����uint32_t���͵�����(���/С��)
     * @post m_position += sizeof(value)
     *       ���m_position > m_size �� m_size = m_position
     */
    void writeFuint32(uint32_t value);

    /**
     * @brief д��̶�����int64_t���͵�����(���/С��)
     * @post m_position += sizeof(value)
     *       ���m_position > m_size �� m_size = m_position
     */
    void writeFint64(int64_t value);

    /**
     * @brief д��̶�����uint64_t���͵�����(���/С��)
     * @post m_position += sizeof(value)
     *       ���m_position > m_size �� m_size = m_position
     */
    void writeFuint64(uint64_t value);

    /**
     * @brief д���з���Varint32���͵�����
     * @post m_position += ʵ��ռ���ڴ�(1 ~ 5)
     *       ���m_position > m_size �� m_size = m_position
     */
    void writeInt32(int32_t value);
    /**
     * @brief д���޷���Varint32���͵�����
     * @post m_position += ʵ��ռ���ڴ�(1 ~ 5)
     *       ���m_position > m_size �� m_size = m_position
     */
    void writeUint32(uint32_t value);

    /**
     * @brief д���з���Varint64���͵�����
     * @post m_position += ʵ��ռ���ڴ�(1 ~ 10)
     *       ���m_position > m_size �� m_size = m_position
     */
    void writeInt64(int64_t value);

    /**
     * @brief д���޷���Varint64���͵�����
     * @post m_position += ʵ��ռ���ڴ�(1 ~ 10)
     *       ���m_position > m_size �� m_size = m_position
     */
    void writeUint64(uint64_t value);

    /**
     * @brief д��float���͵�����
     * @post m_position += sizeof(value)
     *       ���m_position > m_size �� m_size = m_position
     */
    void writeFloat(float value);

    /**
     * @brief д��double���͵�����
     * @post m_position += sizeof(value)
     *       ���m_position > m_size �� m_size = m_position
     */
    void writeDouble(double value);

    /**
     * @brief д��std::string���͵�����,��uint16_t��Ϊ��������
     * @post m_position += 2 + value.size()
     *       ���m_position > m_size �� m_size = m_position
     */
    void writeStringF16(const std::string& value);

    /**
     * @brief д��std::string���͵�����,��uint32_t��Ϊ��������
     * @post m_position += 4 + value.size()
     *       ���m_position > m_size �� m_size = m_position
     */
    void writeStringF32(const std::string& value);

    /**
     * @brief д��std::string���͵�����,��uint64_t��Ϊ��������
     * @post m_position += 8 + value.size()
     *       ���m_position > m_size �� m_size = m_position
     */
    void writeStringF64(const std::string& value);

    /**
     * @brief д��std::string���͵�����,���޷���Varint64��Ϊ��������
     * @post m_position += Varint64���� + value.size()
     *       ���m_position > m_size �� m_size = m_position
     */
    void writeStringVint(const std::string& value);

    /**
     * @brief д��std::string���͵�����,�޳���
     * @post m_position += value.size()
     *       ���m_position > m_size �� m_size = m_position
     */
    void writeStringWithoutLength(const std::string& value);

    /**
     * @brief ��ȡint8_t���͵�����
     * @pre getReadSize() >= sizeof(int8_t)
     * @post m_position += sizeof(int8_t);
     * @exception ���getReadSize() < sizeof(int8_t) �׳� std::out_of_range
     */
    int8_t   readFint8();

    /**
     * @brief ��ȡuint8_t���͵�����
     * @pre getReadSize() >= sizeof(uint8_t)
     * @post m_position += sizeof(uint8_t);
     * @exception ���getReadSize() < sizeof(uint8_t) �׳� std::out_of_range
     */
    uint8_t  readFuint8();

    /**
     * @brief ��ȡint16_t���͵�����
     * @pre getReadSize() >= sizeof(int16_t)
     * @post m_position += sizeof(int16_t);
     * @exception ���getReadSize() < sizeof(int16_t) �׳� std::out_of_range
     */
    int16_t  readFint16();

    /**
     * @brief ��ȡuint16_t���͵�����
     * @pre getReadSize() >= sizeof(uint16_t)
     * @post m_position += sizeof(uint16_t);
     * @exception ���getReadSize() < sizeof(uint16_t) �׳� std::out_of_range
     */
    uint16_t readFuint16();

    /**
     * @brief ��ȡint32_t���͵�����
     * @pre getReadSize() >= sizeof(int32_t)
     * @post m_position += sizeof(int32_t);
     * @exception ���getReadSize() < sizeof(int32_t) �׳� std::out_of_range
     */
    int32_t  readFint32();

    /**
     * @brief ��ȡuint32_t���͵�����
     * @pre getReadSize() >= sizeof(uint32_t)
     * @post m_position += sizeof(uint32_t);
     * @exception ���getReadSize() < sizeof(uint32_t) �׳� std::out_of_range
     */
    uint32_t readFuint32();

    /**
     * @brief ��ȡint64_t���͵�����
     * @pre getReadSize() >= sizeof(int64_t)
     * @post m_position += sizeof(int64_t);
     * @exception ���getReadSize() < sizeof(int64_t) �׳� std::out_of_range
     */
    int64_t  readFint64();

    /**
     * @brief ��ȡuint64_t���͵�����
     * @pre getReadSize() >= sizeof(uint64_t)
     * @post m_position += sizeof(uint64_t);
     * @exception ���getReadSize() < sizeof(uint64_t) �׳� std::out_of_range
     */
    uint64_t readFuint64();

    /**
     * @brief ��ȡ�з���Varint32���͵�����
     * @pre getReadSize() >= �з���Varint32ʵ��ռ���ڴ�
     * @post m_position += �з���Varint32ʵ��ռ���ڴ�
     * @exception ���getReadSize() < �з���Varint32ʵ��ռ���ڴ� �׳� std::out_of_range
     */
    int32_t  readInt32();

    /**
     * @brief ��ȡ�޷���Varint32���͵�����
     * @pre getReadSize() >= �޷���Varint32ʵ��ռ���ڴ�
     * @post m_position += �޷���Varint32ʵ��ռ���ڴ�
     * @exception ���getReadSize() < �޷���Varint32ʵ��ռ���ڴ� �׳� std::out_of_range
     */
    uint32_t readUint32();

    /**
     * @brief ��ȡ�з���Varint64���͵�����
     * @pre getReadSize() >= �з���Varint64ʵ��ռ���ڴ�
     * @post m_position += �з���Varint64ʵ��ռ���ڴ�
     * @exception ���getReadSize() < �з���Varint64ʵ��ռ���ڴ� �׳� std::out_of_range
     */
    int64_t  readInt64();

    /**
     * @brief ��ȡ�޷���Varint64���͵�����
     * @pre getReadSize() >= �޷���Varint64ʵ��ռ���ڴ�
     * @post m_position += �޷���Varint64ʵ��ռ���ڴ�
     * @exception ���getReadSize() < �޷���Varint64ʵ��ռ���ڴ� �׳� std::out_of_range
     */
    uint64_t readUint64();

    /**
     * @brief ��ȡfloat���͵�����
     * @pre getReadSize() >= sizeof(float)
     * @post m_position += sizeof(float);
     * @exception ���getReadSize() < sizeof(float) �׳� std::out_of_range
     */
    float    readFloat();

    /**
     * @brief ��ȡdouble���͵�����
     * @pre getReadSize() >= sizeof(double)
     * @post m_position += sizeof(double);
     * @exception ���getReadSize() < sizeof(double) �׳� std::out_of_range
     */
    double   readDouble();

    /**
     * @brief ��ȡstd::string���͵�����,��uint16_t��Ϊ����
     * @pre getReadSize() >= sizeof(uint16_t) + size
     * @post m_position += sizeof(uint16_t) + size;
     * @exception ���getReadSize() < sizeof(uint16_t) + size �׳� std::out_of_range
     */
    std::string readStringF16();

    /**
     * @brief ��ȡstd::string���͵�����,��uint32_t��Ϊ����
     * @pre getReadSize() >= sizeof(uint32_t) + size
     * @post m_position += sizeof(uint32_t) + size;
     * @exception ���getReadSize() < sizeof(uint32_t) + size �׳� std::out_of_range
     */
    std::string readStringF32();

    /**
     * @brief ��ȡstd::string���͵�����,��uint64_t��Ϊ����
     * @pre getReadSize() >= sizeof(uint64_t) + size
     * @post m_position += sizeof(uint64_t) + size;
     * @exception ���getReadSize() < sizeof(uint64_t) + size �׳� std::out_of_range
     */
    std::string readStringF64();

    /**
     * @brief ��ȡstd::string���͵�����,���޷���Varint64��Ϊ����
     * @pre getReadSize() >= �޷���Varint64ʵ�ʴ�С + size
     * @post m_position += �޷���Varint64ʵ�ʴ�С + size;
     * @exception ���getReadSize() < �޷���Varint64ʵ�ʴ�С + size �׳� std::out_of_range
     */
    std::string readStringVint();

    /**
     * @brief ���ByteArray
     * @post m_position = 0, m_size = 0
     */
    void clear();

    /**
     * @brief д��size���ȵ�����
     * @param[in] buf �ڴ滺��ָ��
     * @param[in] size ���ݴ�С
     * @post m_position += size, ���m_position > m_size �� m_size = m_position
     */
    void write(const void* buf, size_t size);

    /**
     * @brief ��ȡsize���ȵ�����
     * @param[out] buf �ڴ滺��ָ��
     * @param[in] size ���ݴ�С
     * @post m_position += size, ���m_position > m_size �� m_size = m_position
     * @exception ���getReadSize() < size ���׳� std::out_of_range
     */
    void read(void* buf, size_t size);

    /**
     * @brief ��ȡsize���ȵ�����
     * @param[out] buf �ڴ滺��ָ��
     * @param[in] size ���ݴ�С
     * @param[in] position ��ȡ��ʼλ��
     * @exception ��� (m_size - position) < size ���׳� std::out_of_range
     */
    void read(void* buf, size_t size, size_t position) const;

    /**
     * @brief ����ByteArray��ǰλ��
     */
    size_t getPosition() const { return m_position; }

    /**
     * @brief ����ByteArray��ǰλ��
     * @post ���m_position > m_size �� m_size = m_position
     * @exception ���m_position > m_capacity ���׳� std::out_of_range
     */
    void setPosition(size_t v);

    /**
     * @brief ��ByteArray������д�뵽�ļ���
     * @param[in] name �ļ���
     */
    bool writeToFile(const std::string& name, bool with_md5 = false) const;

    /**
     * @brief ���ļ��ж�ȡ����
     * @param[in] name �ļ���
     */
    bool readFromFile(const std::string& name);

    /**
     * @brief �����ڴ��Ĵ�С
     */
    size_t getBaseSize() const { return m_baseSize; }

    /**
     * @brief ���ؿɶ�ȡ���ݴ�С
     */
    size_t getReadSize() const { return m_size - m_position; }

    /**
     * @brief �Ƿ���С��
     */
    bool isLittleEndian() const;

    /**
     * @brief �����Ƿ�ΪС��
     */
    void setIsLittleEndian(bool val);

    /**
     * @brief ��ByteArray���������[m_position, m_size)ת��std::string
     */
    std::string toString() const;

    /**
     * @brief ��ByteArray���������[m_position, m_size)ת��16���Ƶ�std::string(��ʽ:FF FF FF)
     */
    std::string toHexString() const;

    /**
     * @brief ��ȡ�ɶ�ȡ�Ļ���,�����iovec����
     * @param[out] buffers ����ɶ�ȡ���ݵ�iovec����
     * @param[in] len ��ȡ���ݵĳ���,���len > getReadSize() �� len = getReadSize()
     * @return ����ʵ�����ݵĳ���
     */
    uint64_t getReadBuffers(std::vector<WSABUF>& buffers, uint64_t len = ~0ull) const;

    /**
     * @brief ��ȡ�ɶ�ȡ�Ļ���,�����iovec����,��positionλ�ÿ�ʼ
     * @param[out] buffers ����ɶ�ȡ���ݵ�iovec����
     * @param[in] len ��ȡ���ݵĳ���,���len > getReadSize() �� len = getReadSize()
     * @param[in] position ��ȡ���ݵ�λ��
     * @return ����ʵ�����ݵĳ���
     */
    uint64_t getReadBuffers(std::vector<WSABUF>& buffers, uint64_t len, uint64_t position) const;

    /**
     * @brief ��ȡ��д��Ļ���,�����iovec����
     * @param[out] buffers �����д����ڴ��iovec����
     * @param[in] len д��ĳ���
     * @return ����ʵ�ʵĳ���
     * @post ���(m_position + len) > m_capacity �� m_capacity����N���ڵ�������len����
     */
    uint64_t getWriteBuffers(std::vector<WSABUF>& buffers, uint64_t len);

    /**
     * @brief �������ݵĳ���
     */
    size_t getSize() const { return m_size; }
private:

    /**
     * @brief ����ByteArray,ʹ���������size������(���ԭ�����Կ�������,������)
     */
    void addCapacity(size_t size);

    /**
     * @brief ��ȡ��ǰ�Ŀ�д������
     */
    size_t getCapacity() const { return m_capacity - m_position; }

    std::string getMd5() const;
private:
    /// �ڴ��Ĵ�С
    size_t m_baseSize;
    /// ��ǰ����λ��
    size_t m_position;
    /// ��ǰ��������
    size_t m_capacity;
    /// ��ǰ���ݵĴ�С
    size_t m_size;
    /// �ֽ���,Ĭ�ϴ��
    int8_t m_endian;
    /// �Ƿ�ӵ�����ݵĹ���Ȩ��
    bool m_owner;
    /// ��һ���ڴ��ָ��
    Node* m_root;
    /// ��ǰ�������ڴ��ָ��
    Node* m_cur;
};

}
#endif