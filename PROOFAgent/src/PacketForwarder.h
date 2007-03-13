/************************************************************************/
/**
 * @file PacketForwarder.h
 * @brief $$File comment$$
 * @author Anar Manafov A.Manafov@gsi.de
 */ /*
 
        version number:    $LastChangedRevision$
        created by:        Anar Manafov
                                     2007-03-01
        last changed by:   $LastChangedBy$ $LastChangedDate$
 
        Copyright (c) 2006,2007 GSI GridTeam. All rights reserved.
*************************************************************************/
#ifndef PROOFAGENTPACKETFORWARDER_H
#define PROOFAGENTPACKETFORWARDER_H

// BOOST
#include <boost/thread/mutex.hpp>

// Our
#include "INet.h"
#include "LogImp.h"
#include "def.h"

namespace PROOFAgent
{

    class CPacketForwarder: MiscCommon::CLogImp<CPacketForwarder>
    {
            CPacketForwarder( const CPacketForwarder &_Obj ); //HACK: Since smart_socket doesn't support copy-constructor and ref. count
            const CPacketForwarder& operator=( const CPacketForwarder& );
        public:
            CPacketForwarder( MiscCommon::INet::Socket_t _ClientSocket, unsigned short _nNewLocalPort ) :
                    m_ClientSocket( _ClientSocket ),
                    m_nPort( _nNewLocalPort )
            {}

            ~CPacketForwarder()
            {}

            REGISTER_LOG_MODULE( PacketForwarder )

            void WriteBuffer( MiscCommon::BYTEVector_t &_Buf, MiscCommon::INet::smart_socket &_socket ) throw ( std::exception );

        public:
            MiscCommon::ERRORCODE Start();

        protected:
            void ThreadWorker( MiscCommon::INet::smart_socket *_SrvSocket, MiscCommon::INet::smart_socket *_CltSocket );

        private:
            MiscCommon::INet::smart_socket m_ClientSocket;
            MiscCommon::INet::smart_socket m_ServerCocket;
            unsigned short m_nPort;
            boost::mutex m_Buf_mutex;
    };

}

#endif
