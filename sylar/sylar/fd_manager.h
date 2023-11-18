/**
 * @file fd_manager.h
 * @brief �ļ����������
 * @author sylar.yin
 * @email 564628276@qq.com
 * @date 2019-05-30
 * @copyright Copyright (c) 2019�� sylar.yin All rights reserved (www.sylar.top)
 */
#ifndef __FD_MANAGER_H__
#define __FD_MANAGER_H__

#include <memory>
#include <vector>
#include "thread.h"
#include "singleton.h"
#include "mutex.h"
#include "util.h"
#include "hook.h"

namespace sylar {

    /**
     * @brief �ļ������������
     * @details �����ļ��������(�Ƿ�socket)
     *          �Ƿ�����,�Ƿ�ر�,��/д��ʱʱ��
     */
    class FdCtx : public std::enable_shared_from_this<FdCtx> {
    public:
        typedef std::shared_ptr<FdCtx> ptr;
        /**
         * @brief ͨ���ļ��������FdCtx
         */
        FdCtx(uintptr_t fd);
        /**
         * @brief ��������
         */
        ~FdCtx();

        /**
         * @brief �Ƿ��ʼ�����
         */
        bool isInit() const { return m_isInit; }

        /**
         * @brief �Ƿ�socket
         */
        bool isSocket() const { return m_isSocket; }

        /**
         * @brief �Ƿ��ѹر�
         */
        bool isClose() const { return m_isClosed; }

        /**
         * @brief �����û��������÷�����
         * @param[in] v �Ƿ�����
         */
        void setUserNonblock(bool v) { m_userNonblock = v; }

        /**
         * @brief ��ȡ�Ƿ��û��������õķ�����
         */
        bool getUserNonblock() const { return m_userNonblock; }

        /**
         * @brief ����ϵͳ������
         * @param[in] v �Ƿ�����
         */
        void setSysNonblock(bool v) { m_sysNonblock = v; }

        /**
         * @brief ��ȡϵͳ������
         */
        bool getSysNonblock() const { return m_sysNonblock; }

        /**
         * @brief ���ó�ʱʱ��
         * @param[in] type ����SO_RCVTIMEO(����ʱ), SO_SNDTIMEO(д��ʱ)
         * @param[in] v ʱ�����
         */
        void setTimeout(int type, uint64_t v);

        /**
         * @brief ��ȡ��ʱʱ��
         * @param[in] type ����SO_RCVTIMEO(����ʱ), SO_SNDTIMEO(д��ʱ)
         * @return ��ʱʱ�����
         */
        uint64_t getTimeout(int type);
    private:
        /**
         * @brief ��ʼ��
         */
        bool init();
    private:
        /// �Ƿ��ʼ��
        bool m_isInit : 1;
        /// �Ƿ�socket
        bool m_isSocket : 1;
        /// �Ƿ�hook������
        bool m_sysNonblock : 1;
        /// �Ƿ��û��������÷�����
        bool m_userNonblock : 1;
        /// �Ƿ�ر�
        bool m_isClosed : 1;
        /// �ļ����
        uintptr_t m_fd;
        /// ����ʱʱ�����
        uint64_t m_recvTimeout;
        /// д��ʱʱ�����
        uint64_t m_sendTimeout;
    };

    /**
     * @brief �ļ����������
     */
    class FdManager {
    public:
        using RWMutexType =RWMutex;
        /**
         * @brief �޲ι��캯��
         */
        FdManager();

        /**
         * @brief ��ȡ/�����ļ������FdCtx
         * @param[in] fd �ļ����
         * @param[in] auto_create �Ƿ��Զ�����
         * @return ���ض�Ӧ�ļ������FdCtx::ptr
         */
        FdCtx::ptr get(uintptr_t fd, bool auto_create = false);

        /**
         * @brief ɾ���ļ������
         * @param[in] fd �ļ����
         */
        void del(uintptr_t fd);
    private:
        /// ��д��
        RWMutexType m_mutex;
        /// �ļ��������
        std::unordered_map<uintptr_t, FdCtx::ptr> m_datas;
    };

    /// �ļ��������
    using FdMgr= Singleton<FdManager> ;

}

#endif
