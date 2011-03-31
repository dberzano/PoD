//
//  Server.cpp
//  PoD
//
//  Created by Anar Manafov on 17.01.11.
//  Copyright 2011 GSI. All rights reserved.
//
//=============================================================================
#include "Server.h"
// MiscCommon
#include "def.h"
//=============================================================================
using namespace pod_remote;
using namespace std;
using namespace PROOFAgent;
using namespace MiscCommon;
//=============================================================================
CServer::CServer( int _fdIn, int _fdOut ):
    m_fdIn( _fdIn ),
    m_fdOut( _fdOut )
{
}
//=============================================================================
void CServer::getSrvHostInfo() const
{
    BYTEVector_t data;
    processAdminConnection( &data, Req_Host_Info );
    SHostInfoCmd srvHostInfo;
    srvHostInfo.convertFromData( data );
}

//=============================================================================
void CServer::processAdminConnection( BYTEVector_t *_data, CServer::Requests _req ) const
{
    CProtocol protocol;
//    SVersionCmd v;
//    BYTEVector_t ver;
//    v.convertToData( &ver );
//    protocol.write( m_fd, static_cast<uint16_t>( cmdUI_CONNECT ), ver );
    bool bProcessNoMore( false );
    while( !bProcessNoMore )
    {
        CProtocol::EStatus_t ret = protocol.read( m_fdIn );
        switch( ret )
        {
            case CProtocol::stDISCONNECT:
                throw runtime_error( "a disconnect has been detected on the adminChannel" );
            case CProtocol::stAGAIN:
            case CProtocol::stOK:
                {
                    while( protocol.checkoutNextMsg() )
                    {
                        bProcessNoMore = ( processProtocolMsgs( _data, &protocol, _req ) > 0 );
                        if( bProcessNoMore )
                            break;
                    }
                }
                break;
        }
    }
}
//=============================================================================
int CServer::processProtocolMsgs( BYTEVector_t *_data,
                                  CProtocol * _protocol, CServer::Requests _req ) const
{
    BYTEVector_t data;
    SMessageHeader header = _protocol->getMsg( &data );
    switch( static_cast<ECmdType>( header.m_cmd ) )
    {
        case cmdUI_CONNECT_READY:
            {
                switch( _req )
                {
                    case Req_Host_Info:
                        _protocol->writeSimpleCmd( m_fdOut, static_cast<uint16_t>( cmdGET_HOST_INFO ) );
                        break;
                }
            }
            break;
        case cmdHOST_INFO:
        case cmdWNs_LIST:
            _data->swap( data );
            return 1;
        case cmdSHUTDOWN:
            throw runtime_error( "Server requests to shut down. Stop admin channel..." );
        default:
            throw runtime_error( "Unexpected message in the admin channel" );
    }
    return 0;
}
