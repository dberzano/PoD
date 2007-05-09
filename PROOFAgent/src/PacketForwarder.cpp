/************************************************************************/
/**
 * @file PacketForwarder.cpp
 * @brief $$File comment$$
 * @author Anar Manafov A.Manafov@gsi.de
 */ /*
 
        version number:   $LastChangedRevision$
        created by:          Anar Manafov
                                  2007-03-01
        last changed by:   $LastChangedBy$ $LastChangedDate$
 
        Copyright (c) 2007 GSI GridTeam. All rights reserved.
*************************************************************************/ 
// PROOFAgent
#include "ErrorCode.h"
#include "PacketForwarder.h"

// BOOST
#include <boost/bind.hpp>

// API
#include <signal.h>

using namespace MiscCommon;
using namespace MiscCommon::INet;
using namespace PROOFAgent;
using namespace std;

const unsigned int g_BUF_SIZE = 1024;

extern sig_atomic_t graceful_quit;

void CPacketForwarder::ThreadWorker( smart_socket *_SrvSocket, smart_socket *_CltSocket )
{
    BYTEVector_t buf( g_BUF_SIZE );
    while ( true )
    {
        // Checking whether signal has arrived
        if ( graceful_quit )
        {
            InfoLog( erOK, "STOP signal received." );
            return ;
        }

        try
        {
            if ( _SrvSocket->is_read_ready( 10 ) )
            {

                *_SrvSocket >> &buf;
                if ( !_SrvSocket->is_valid() )
                {
                    InfoLog( erOK, "DISCONNECT has been detected." );
                    return ;
                }

                WriteBuffer( buf, *_CltSocket );
                ReportPackage( *_SrvSocket, buf );
                buf.clear();
                buf.resize( g_BUF_SIZE );
            }
        }
        catch ( exception & e )
        {
            FaultLog( erError, e.what() );
            return ;
        }
    }
}

ERRORCODE CPacketForwarder::Start( bool _Join )
{
    // executing PF threads
    m_thrd_serversocket = Thread_PTR_t( new boost::thread(
                                            boost::bind( &CPacketForwarder::_Start, this, _Join ) ) );
    if ( _Join )  //  Join Threads (for Client) and non-join Threads (for Server mode - server shouldn't sleep while PF is working)
        m_thrd_serversocket->join();

    return erOK;
}

void CPacketForwarder::SpawnClientMode()
{
    m_ClientSocket.set_nonblock();
    while (true)
    {
        // Checking whether signal has arrived
        if ( graceful_quit )
        {
            InfoLog( erOK, "STOP signal received." );
            return ;
        }

        try
        {
            if ( m_ClientSocket.is_read_ready( 10 ) )
                break;
        }
        catch ( exception & e )
        {
            FaultLog( erError, e.what() );
            return ;
        }
    }
    CSocketClient proof_client;
    proof_client.Connect( m_nPort, "127.0.0.1" );

    // Checking whether signal has arrived
    if ( graceful_quit )
    {
        InfoLog( erOK, "STOP signal received." );
        return ;
    }
    m_ServerSocket = proof_client.GetSocket().detach();


    m_ServerSocket.set_nonblock();

    // executing PF threads
    m_thrd_clnt = Thread_PTR_t( new boost::thread(
                                    boost::bind( &CPacketForwarder::ThreadWorker, this, &m_ServerSocket, &m_ClientSocket ) ) );
    m_thrd_srv = Thread_PTR_t( new boost::thread(
                                   boost::bind( &CPacketForwarder::ThreadWorker, this, &m_ClientSocket, &m_ServerSocket ) ) );

    m_thrd_clnt->join();
    m_thrd_srv->join();
}

void CPacketForwarder::SpawnServerMode()
{
    while ( true )
    {
        CSocketServer server;
        server.Bind( m_nPort );
        server.Listen( 1 );
        server.GetSocket().set_nonblock();

        // Checking whether signal has arrived
        if ( graceful_quit )
        {
            InfoLog( erOK, "STOP signal received." );
            return ;
        }

        if ( server.GetSocket().is_read_ready( 10 ) )
        {
            m_ServerSocket = server.Accept();

            // executing PF threads
            m_thrd_clnt = Thread_PTR_t( new boost::thread(
                                            boost::bind( &CPacketForwarder::ThreadWorker, this, &m_ServerSocket, &m_ClientSocket ) ) );
            m_thrd_srv = Thread_PTR_t( new boost::thread(
                                           boost::bind( &CPacketForwarder::ThreadWorker, this, &m_ClientSocket, &m_ServerSocket ) ) );
            break;
        }
    }
}


ERRORCODE CPacketForwarder::_Start( bool _ClientMode )
{
    try
    {
        if ( _ClientMode )
            SpawnClientMode();
        else
            SpawnServerMode();
    }
    catch ( exception & e )
    {
        FaultLog( erError, e.what() );
        return erError;
    }
    return erOK;
}

void CPacketForwarder::WriteBuffer( BYTEVector_t &_Buf, smart_socket &_socket ) throw ( exception )
{
    //  boost::mutex::scoped_lock lock(m_Buf_mutex);
    _socket << _Buf;
    ReportPackage( _socket, _Buf, false );
}
