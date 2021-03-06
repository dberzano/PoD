/************************************************************************/
/**
 * @file ThreadPool.h
 * @brief
 * @author Anar Manafov A.Manafov@gsi.de
 */ /*

        version number:     $LastChangedRevision$
        created by:         Anar Manafov
                            2009-09-28
        last changed by:    $LastChangedBy$ $LastChangedDate$

        Copyright (c) 2009 GSI GridTeam. All rights reserved.
*************************************************************************/
#ifndef THREADPOOL_H_
#define THREADPOOL_H_
// STD
#include <queue>
// BOOST
#include <boost/shared_ptr.hpp>

// FIX: silence a warning until BOOST fix it
// boost/thread/pthread/condition_variable.hpp:53:19: warning: unused variable 'res' [-Wunused-variable]
// clang
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#endif
// gcc
#if defined(__GNUG__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
// push doesn't work for gcc 4.2
//#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif
#include <boost/thread/thread.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#if defined(__GNUG__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
//#pragma GCC diagnostic pop
#pragma GCC diagnostic warning "-Wunused-variable"
#endif


#include <boost/thread/condition.hpp>
// MiscCommon
#include "LogImp.h"
#include "Node.h"

namespace PROOFAgent
{
//=============================================================================
    class CThreadPool: public MiscCommon::CLogImp<CThreadPool>
    {
            typedef std::pair<CNode::ENodeSocket_t, CNode*> task_t;
            typedef std::queue<task_t*> taskqueue_t;
        public:
            REGISTER_LOG_MODULE( "ThreadPool" )

            CThreadPool( size_t _threadsCount, const std::string &_signalPipePath );
            ~CThreadPool();

            void pushTask( CNode::ENodeSocket_t _which, CNode* _node );
            void execute();
            void stop( bool processRemainingJobs = false );

        private:
            boost::thread_group m_threads;
            taskqueue_t m_tasks;
            boost::mutex m_mutex;
            boost::condition m_threadNeeded;
            boost::condition m_threadAvailable;
            bool m_stopped;
            bool m_stopping;
            int m_fdSignalPipe;
    };

}

#endif
