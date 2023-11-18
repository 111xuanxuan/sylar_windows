#pragma once
#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <thread>
#include "noncopyable.h"
#include <string>
#include <functional>
#include <semaphore>

#ifdef _WIN32
using pid_t = uint32_t;
using pthread_t = std::thread;
using Semaphore = std::binary_semaphore;
#else
using pid_t = pid_t;
using pthread_t = pthread_t;
#endif // _WIN32


namespace sylar {


	class Thread :public Noncopyable {
	public:
		using ptr = std::shared_ptr<Thread>;

        /**
 * @brief ���캯��
 * @param[in] cb �߳�ִ�к���
 * @param[in] name �߳�����
 */
        Thread(std::function<void()> cb, const std::string& name);

        /**
         * @brief ��������
         */
        ~Thread();

        /**
         * @brief �߳�ID
         */
        pid_t getId() const { return m_id; }

        /**
         * @brief �߳�����
         */
        const std::string& getName() const { return m_name; }

        /**
         * @brief �ȴ��߳�ִ�����
         */
        void join();

        /**
         * @brief ��ȡ��ǰ���߳�ָ��
         */
        static Thread* GetThis();

        /**
         * @brief ��ȡ��ǰ���߳�����
         */
        static const std::string& GetName();

        /**
         * @brief ���õ�ǰ�߳�����
         * @param[in] name �߳�����
         */
        static void SetName(const std::string& name);
    private:

        /**
         * @brief �߳�ִ�к���
         */
        static void* run(void* arg);
    private:
        /// �߳�id
        pid_t m_id = -1;
        /// �߳̽ṹ
        pthread_t m_thread;
        /// �߳�ִ�к���
        std::function<void()> m_cb;
        /// �߳�����
        std::string m_name;
        /// �ź���
        Semaphore m_semaphore;
	};



}
#endif