/************************************************************************/
/**
 * @file AgentImpl.h
 * @brief $$File comment$$
 * @author Anar Manafov A.Manafov@gsi.de
 */ /*
 
        version number:    $LastChangedRevision$
        created by:        Anar Manafov
                                    2007-03-01
        last changed by:   $LastChangedBy$ $LastChangedDate$
 
        Copyright (c) 2006,2007 GSI GridTeam. All rights reserved.
*************************************************************************/
#ifndef AGENTIMPL_H
#define AGENTIMPL_H

// API
#include <signal.h>

// BOOST
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

// STD
#include <list>

// PROOFAgent
#include "ErrorCode.h"
#include "LogImp.h"
#include "IXMLPersist.h"
#include "PacketForwarder.h"

namespace PROOFAgent
{
    typedef enum{ Unknown, Server, Client }EAgentMode_t;

    // declaration of signal handler
    void signal_handler( int _SignalNumber );

    /** @class CAgentBase
      *  @brief
     */
    class CAgentBase: MiscCommon::IXMLPersist
    {
        public:
            CAgentBase() : Mode( Unknown )
            {
                struct sigaction sa;
                ::sigemptyset (&sa.sa_mask);
                sa.sa_flags = 0;

                // Register the handler for SIGINT.
                sa.sa_handler = signal_handler;
                ::sigaction (SIGINT, &sa, 0);
                // Register the handler for SIGTERM
                sa.sa_handler = signal_handler;
                ::sigaction (SIGTERM, &sa, 0);
            }
            virtual ~CAgentBase()
            {}
            virtual MiscCommon::ERRORCODE Init( xercesc::DOMNode* _element )
            {
                return this->Read( _element );
            }

            MiscCommon::ERRORCODE Start()
            {
                boost::thread thrd( boost::bind( &CAgentBase::ThreadWorker, this ) );
                thrd.join();
                return MiscCommon::erOK;
            }

        protected:
            virtual void ThreadWorker() = 0;

        public:
            const EAgentMode_t Mode;
    };

    typedef struct SAgentServerData
    {
        SAgentServerData() :
                m_nPort( 0 ),
                m_nLocalClientPortMin( 0 ),
                m_nLocalClientPortMax( 0 )
        {}
        unsigned short m_nPort;
        unsigned short m_nLocalClientPortMin;
        unsigned short m_nLocalClientPortMax;
    }
    AgentServerData_t;
    inline std::ostream &operator <<( std::ostream &_stream, const AgentServerData_t &_data )
    {
        _stream
        << "Listen on Port: " << _data.m_nPort << "\n"
        << "a Local Clients Ports: " << _data.m_nLocalClientPortMin << "-" << _data.m_nLocalClientPortMax << std::endl;
        return _stream;
    }

    typedef struct SAgentClientData
    {
        SAgentClientData() :
                m_nServerPort( 0 ),
                m_nLocalClientPort( 0 )
        {}
        unsigned short m_nServerPort;
        std::string m_strServerHost;
        unsigned short m_nLocalClientPort;
    }
    AgentClientData_t;
    inline std::ostream &operator <<( std::ostream &_stream, const AgentClientData_t &_data )
    {
        _stream
        << "server info: [" << _data.m_strServerHost << ":" << _data.m_nServerPort << "];"
        << "\n"
        << "local listen port: " << _data.m_nLocalClientPort
        << std::endl;
        return _stream;
    }

    template <class _T>
    struct SDelete
    {
        bool operator() ( _T *_val )
        {
            if ( _val )
                delete _val;
            return true;
        }
    };

    class PF_Container
    {
            typedef CPacketForwarder pf_container_value;
            typedef std::list<pf_container_value *> pf_container_type;

        public:
            PF_Container()
            {}
            ~PF_Container()
            {
                std::for_each( m_container.begin(), m_container.end(), SDelete<pf_container_value>() );
            }
            void add( MiscCommon::INet::Socket_t _ClientSocket, unsigned short _nNewLocalPort )
            {
                CPacketForwarder * pf = new CPacketForwarder( _ClientSocket, _nNewLocalPort );
                pf->Start();
                m_container.push_back( pf );
            }

        private:
            pf_container_type m_container;

    };

    /** @class CAgentServer
     *  @brief
     */
    class CAgentServer :
                public CAgentBase,
                MiscCommon::CLogImp<CAgentServer>
    {
        public:
            CAgentServer() :
                    Mode( Server )
            {}
            virtual ~CAgentServer()
            {}
            REGISTER_LOG_MODULE( AgentServer );

        public:
            MiscCommon::ERRORCODE Read( xercesc::DOMNode* _element );
            MiscCommon::ERRORCODE Write( xercesc::DOMNode* _element );

            void AddPF( MiscCommon::INet::Socket_t _ClientSocket, unsigned short _nNewLocalPort )
            {
                boost::mutex::scoped_lock lock ( m_PFList_mutex );
                m_PFList.add( _ClientSocket, _nNewLocalPort );
            }

        protected:
            void ThreadWorker();

        private:
            const EAgentMode_t Mode;
            AgentServerData_t m_Data;
            PF_Container m_PFList;
            boost::mutex m_PFList_mutex;
    };

    /** @class CAgentClient
      *  @brief
      */
    class CAgentClient:
                public CAgentBase,
                MiscCommon::CLogImp<CAgentClient>
    {
        public:
            CAgentClient() : Mode( Client )
            {}
            virtual ~CAgentClient()
            {}
            REGISTER_LOG_MODULE( AgentClient );

        public:
            MiscCommon::ERRORCODE Read( xercesc::DOMNode* _element );
            MiscCommon::ERRORCODE Write( xercesc::DOMNode* _element );

        protected:
            void ThreadWorker();

        private:
            const EAgentMode_t Mode;
            AgentClientData_t m_Data;
    };

}

#endif
