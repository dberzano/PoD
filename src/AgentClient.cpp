/************************************************************************/
/**
 * @file AgentClient.cpp
 * @brief Implementation file of AgentServer and AgentClient
 * @author Anar Manafov A.Manafov@gsi.de
 */ /*

        version number:     $LastChangedRevision$
        created by:         Anar Manafov
                            2007-03-01
        last changed by:    $LastChangedBy$ $LastChangedDate$

        Copyright (c) 2007-2009 GSI GridTeam. All rights reserved.
*************************************************************************/
#include "AgentClient.h"
// STD
#include <csignal>
// pod-agent
#include "Node.h"
#include "PARes.h"
#include "Protocol.h"
#include "ProtocolCommands.h"
//=============================================================================
using namespace std;
using namespace MiscCommon;
using namespace PROOFAgent;
namespace inet = MiscCommon::INet;
//=============================================================================
const size_t g_monitorTimeout = 10; // in seconds
extern sig_atomic_t graceful_quit;
//=============================================================================
CAgentClient::CAgentClient( const SOptions_t &_data ):
        CAgentBase( _data.m_podOptions.m_worker.m_common )
{
    m_Data = _data.m_podOptions.m_worker;
    m_serverInfoFile = _data.m_serverInfoFile;
    m_proofPort = _data.m_proofPort;
}

//=============================================================================
CAgentClient::~CAgentClient()
{
}

//=============================================================================
void CAgentClient::run()
{
    try
    {
        createPROOFCfg();

        while ( true )
        {
            if ( graceful_quit )
            {
                InfoLog( "shutting Agent's instance down..." );
                return;
            }

            readServerInfoFile( m_serverInfoFile );

            InfoLog( "looking for PROOFAgent server to connect..." );
            // a connection to a Agent's server
            // TODO: implement this using nonblock sockets
            inet::CSocketClient client;
            client.Connect( m_agentServerListenPort, m_agentServerHost );
            InfoLog( "connected!" );

            CProtocol protocol;
            SVersionCmd v;
            v.m_version = protocol.getVersion();
            BYTEVector_t data;
            v.convertToData( &data );
            protocol.write( client.GetSocket(), static_cast<uint16_t>( cmdVERSION ), data );

            CProtocol::EStatus_t ret = protocol.read( client.GetSocket() );
            switch ( ret )
            {
                case CProtocol::stDISCONNECT:
                    break;
                case CProtocol::stAGAIN:
                    break;
                case CProtocol::stUNKNOWN:
                    break;
                case CProtocol::stOK:
                    {
                        BYTEVector_t data;
                        SMessageHeader header = protocol.getMsg( &data );
                        switch ( static_cast<ECmdType>( header.m_cmd ) )
                        {
                            case cmdVERSION_BAD:
                                break;
                            case cmdVERSION_OK:
                                break;
                            case cmdGET_HOST_INFO:
                                {
                                    SHostInfoCmd h;
                                    h.m_username = "me";
                                    h.m_host = "my.host.de";
                                    h.m_proofPort = 256;
                                    h.convertToData( data );
                                    protocol.write( client.GetSocket(), static_cast<uint16_t>( cmdHOST_INFO ), data );
                                    break;
                                }
                        }
                        break;
                    }
                case CProtocol::stERR:
                    break;
            }


//            // sending protocol version to the server
//            string sProtocol( g_szPROTOCOL_VERSION );
//            DebugLog( erOK, "Sending protocol version: " + sProtocol );
//            send_string( client.GetSocket(), sProtocol );
//
//            //TODO: Checking response
//            string sResponse;
//            receive_string( client.GetSocket(), &sResponse, g_nBUF_SIZE );
//            DebugLog( erOK, "Client received: " + sResponse );
//
//            // Sending User name
//            string sUser;
//            get_cuser_name( &sUser );
//            DebugLog( erOK, "Sending user name: " + sUser );
//            send_string( client.GetSocket(), sUser );
//
//            //TODO: Checking response
//            receive_string( client.GetSocket(), &sResponse, g_nBUF_SIZE );
//            DebugLog( erOK, "Client received: " + sResponse );

            InfoLog( "Entering into the main \"select\" loop..." );

            try
            {
                client.GetSocket().set_nonblock();

                // waiting until Agent server sends something
                // that would mean that a user initializes a PROOF session
                waitForServerToConnect( client.GetSocket().get() );

                // now we need to connect to a local PROOF worker
                MiscCommon::INet::Socket_t proof_socket = connectToLocalPROOF( m_proofPort );
                CNode node( client.GetSocket().detach(), proof_socket,
                            "", m_Data.m_common.m_agentNodeReadBuffer );

                // now we are ready to proxy all packages
                mainSelect( &node );
            }
            catch ( exception & e )
            {
                WarningLog( erError, e.what() );
                continue;
            }
        }
    }
    catch ( exception & e )
    {
        FaultLog( erError, e.what() );
    }
}

//=============================================================================
void CAgentClient::monitor()
{
    while ( true )
    {
        // TODO: we need to check real PROOF port here (from cfg)
        if ( !IsPROOFReady( m_proofPort ) )
        {
            FaultLog( erError, "Can't connect to PROOF/XRD service." );
            graceful_quit = 1;
        }


        if ( m_idleWatch.isTimedout( m_Data.m_common.m_shutdownIfIdleForSec ) )
        {
            InfoLog( "Agent's idle time has just reached a defined maximum. Exiting..." );
            graceful_quit = 1;
        }

        if ( graceful_quit )
        {
            // wake up (from "select") the main thread, so that it can update it self
            if ( write( m_fdSignalPipe, "1", 1 ) < 0 )
                FaultLog( erError, "Can't signal to the main thread via a named pipe: " + errno2str() );

            return;
        }

        sleep( g_monitorTimeout );
    }
}

//=============================================================================
void CAgentClient::waitForServerToConnect( MiscCommon::INet::Socket_t _sockToWait )
{
    InfoLog( "waiting for Agent's server to initialize a redirection procedure..." );

    fd_set readset;
    FD_ZERO( &readset );
    FD_SET( _sockToWait, &readset );
    // setting a signal pipe as well
    // we want to be interrupted
    FD_SET( m_fdSignalPipe, &readset );

    int retval = ::select(( _sockToWait > m_fdSignalPipe ? _sockToWait : m_fdSignalPipe ) + 1,
                          &readset, NULL, NULL, NULL );
    if ( retval < 0 )
        throw system_error( "Error occurred while waiting for Agent server:" );

    // must actually never happen
    if ( 0 == retval )
        throw system_error( "Select has timeout while waiting for an Agent server." );

    // we got a signal for update
    // reading everything from the pipe and exiting from the function
    if ( FD_ISSET( m_fdSignalPipe, &readset ) )
    {
        const int read_size = 20;
        char buf[read_size];
        int numread( 0 );
        do
        {
            numread = read( m_fdSignalPipe, buf, read_size );
        }
        while ( numread > 0 );

        throw runtime_error( "Got a wake up signal from the signal pipe. Stopping the main select..." );
    }

    m_idleWatch.touch();

    // if there were no exception raised we can process further
    InfoLog( "done waiting for Agent's server. Initializing a redirection procedure..." );
}

//=============================================================================
MiscCommon::INet::Socket_t CAgentClient::connectToLocalPROOF( unsigned int _proofPort )
{
    InfoLog( "Connecting to a local PROOF worker..." );

    // Connecting to the local client (a proof slave)
    inet::CSocketClient proof_client;
    proof_client.Connect( _proofPort, "127.0.0.1" );
    InfoLog( "connected to the local proof service" );

    proof_client.GetSocket().set_nonblock();

    return proof_client.GetSocket().detach();
}

//=============================================================================
void CAgentClient::mainSelect( CNode *_node )
{
    while ( true )
    {
        m_idleWatch.touch();

        // Checking whether signal has arrived
        if ( graceful_quit )
        {
            InfoLog( erOK, "STOP signal received." );
            return;
        }


        fd_set readset;
        FD_ZERO( &readset );

        if ( !_node || !_node->isValid() )
            return;

        int fd_first = _node->getSocket( CNode::nodeSocketFirst );
        int fd_second = _node->getSocket( CNode::nodeSocketSecond );
        FD_SET( fd_first, &readset );
        FD_SET( fd_second, &readset );
        int fd_max( fd_first > fd_second ? fd_first : fd_second );
        // setting a signal pipe as well
        // we want to be interrupted
        FD_SET( m_fdSignalPipe, &readset );
        fd_max = fd_max > m_fdSignalPipe ? fd_max : m_fdSignalPipe;

        int retval = ::select( fd_max + 1, &readset, NULL, NULL, NULL );
        if ( retval < 0 )
            throw system_error( "Error occurred while in the main select:" );

        // must actually never happen
        if ( 0 == retval )
            throw system_error( "The main select has timeout." );

        if ( FD_ISSET( fd_first, &readset ) )
        {
            _node->dealWithData( CNode::nodeSocketFirst );
        }
        if ( FD_ISSET( fd_second, &readset ) )
        {
            _node->dealWithData( CNode::nodeSocketSecond );
        }


        // we got a signal for update
        // reading everything from the pipe and exiting from the function
        if ( FD_ISSET( m_fdSignalPipe, &readset ) )
        {
            const int read_size = 20;
            char buf[read_size];
            int numread( 0 );
            //  do
            //  {
            numread = read( m_fdSignalPipe, buf, read_size );
            //  }
            //   while ( numread > 0 );

            throw system_error( "Got a wake up signal from the signal pipe." );
        }

    }
}

//=============================================================================
void CAgentClient::createPROOFCfg()
{
    DebugLog( erOK, "Creating a PROOF configuration file..." );

    ofstream f( m_commonOptions.m_proofCFG.c_str() );
    if ( !f.is_open() )
        throw runtime_error( "can't open " + m_commonOptions.m_proofCFG + " for writing." );

    // getting local host name
    string host;
    MiscCommon::get_hostname( &host );
    f << "worker " << host << " perf=100" << std::endl;
}
