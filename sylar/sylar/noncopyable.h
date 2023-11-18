#pragma once
#ifndef __SYLAR_NONCOPYABLE_H__
#define __SYLAR_NONCOPYABLE_H__

namespace sylar {

    /**
     * @brief �����޷�����,��ֵ
     */
    class Noncopyable {
    public:
        /**
         * @brief Ĭ�Ϲ��캯��
         */
        Noncopyable() = default;

        /**
         * @brief Ĭ����������
         */
        ~Noncopyable() = default;

        /**
         * @brief �������캯��(����)
         */
        Noncopyable(const Noncopyable&) = delete;

        /**
         * @brief ��ֵ����(����)
         */
        Noncopyable& operator=(const Noncopyable&) = delete;
    };

}

#endif