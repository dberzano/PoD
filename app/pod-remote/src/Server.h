//
//  Server.h
//  PoD
//
//  Created by Anar Manafov on 17.01.11.
//  Copyright 2011 GSI. All rights reserved.
//
#ifndef SERVER_POD_REMOTE_H
#define SERVER_POD_REMOTE_H
//=============================================================================
//STD
#include <string>
// MiscCommon
#include "INet.h"
// PoD Protocol
#include "ProtocolCommands.h"
//=============================================================================
namespace inet = MiscCommon::INet;
//=============================================================================
namespace pod_remote
{
    class CServer
    {
            enum Requests { Req_Host_Info };
        public:
            CServer( int _fdIn, int _fdOut );
            void getSrvHostInfo() const;

        private:
            void processAdminConnection( MiscCommon::BYTEVector_t *_data,
                                         CServer::Requests _req ) const;
            int processProtocolMsgs( MiscCommon::BYTEVector_t *_data,
                                     PROOFAgent::CProtocol * _protocol,
                                     CServer::Requests _req ) const;

        private:
            int m_fdIn;
            int m_fdOut;
    };
}
#endif
