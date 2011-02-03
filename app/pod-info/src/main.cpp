/************************************************************************/
/**
 * @file main.cpp
 * @brief main file
 * @author Anar Manafov A.Manafov@gsi.de
 */ /*

        version number:     $LastChangedRevision$
        created by:         Anar Manafov
                            2011-01-17
        last changed by:    $LastChangedBy$ $LastChangedDate$

        Copyright (c) 2011 GSI, Scientific Computing devision. All rights reserved.
*************************************************************************/
// STD
#include <stdexcept>
// BOOST
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
// MiscCommon
#include "BOOSTHelper.h"
#include "Process.h"
#include "SysHelper.h"
#include "PoDUserDefaultsOptions.h"
// pod-agent
#include "ProofStatusFile.h"
// pod-info
#include "version.h"
#include "Environment.h"
#include "Server.h"
#include "Options.h"
//=============================================================================
using namespace MiscCommon;
using namespace std;
namespace pod_agent = PROOFAgent;
namespace bpo = boost::program_options;
namespace boost_hlp = MiscCommon::BOOSTHelper;
//=============================================================================
// 0 - success, 1 - an error, 2 - server is partially running
int g_exitCode = 0;
enum EPoDServerType
{
    // PoD Server can't be found
    SrvType_Unknown,
    // a local PoD server.
    SrvType_Local,
    // a remote PoD server
    SrvType_Remote
};
//=============================================================================
string version( const CEnvironment &_env, const pod_info::CServer &_srv )
{
    bool noServer( false );
    PROOFAgent::SHostInfoCmd srvHostInfo;
    try
    {
        _srv.getSrvHostInfo( &srvHostInfo );
    }
    catch( ... )
    {
        noServer = true;
    }
    ostringstream ss;
    ss
            << "PoD location: " << _env.PoDPath() << "\n"
            << "Local Version: " << _env.version() << "\n";

    if( noServer )
    {
        ss << "PoD server is NOT found.";
    }
    else
    {
        ss
                << "Server PoD location: "
                << srvHostInfo.m_username << "@" << srvHostInfo.m_host << ":"
                << srvHostInfo.m_PoDPath << "\n"
                << "Server Version: " << srvHostInfo.m_version;
    }
    return ( ss.str() );
}
//=============================================================================
size_t listWNs( string *_output, const pod_info::CServer &_srv,
                const SOptions &_opt )
{
    PROOFAgent::SWnListCmd lst;
    try
    {
        _srv.getListOfWNs( &lst );
    }
    catch( exception &_e )
    {
        string msg;
        msg += "PoD server is NOT found.\n";
        if( _opt.m_debug )
            msg += _e.what();
        throw runtime_error( msg );
    }
    if( _output )
    {
        stringstream ss;
        std::ostream_iterator< std::string > output( ss, "\n" );
        std::copy( lst.m_container.begin(), lst.m_container.end(), output );
        *_output = ss.str();
    }
    return ( lst.m_container.size() );
}
//=============================================================================
// check the local xpd
pid_t getLocalXPDPid( const CEnvironment &_env )
{
    try
    {
        pod_agent::CProofStatusFile proofStatus;
        if( proofStatus.readAdminPath( _env.getXpdCfgFile(), adminp_server ) )
            return proofStatus.xpdPid();
    }
    catch( ... )
    {
    }
    return 0;
}
//=============================================================================
void srvPoDStatus( string *_status, string *_con_string,
                   const pod_info::CServer &_srv, const SOptions &_opt,
                   const CEnvironment &_env )
{
    PROOFAgent::SHostInfoCmd srvHostInfo;
    try
    {
        _srv.getSrvHostInfo( &srvHostInfo );
    }
    catch( exception &_e )
    {
        string msg;
        if( _status && _opt.m_sshConnectionStr.empty() )
        {
            // if we are here, that means there is no neither
            // a remote or a local pod-agent found.
            // Let us check now, whether there is no local xpd processes left
            pid_t xpd_pid( getLocalXPDPid( _env ) );
            if( IsProcessExist( xpd_pid ) )
            {
                stringstream ss;
                ss << "PoD server is NOT running.\n"
                   << "WARNING: There is a local XPROOFD [" << xpd_pid << "] process detected as a part of PoD server.\n"
                   << "Please, either restart PoD server or stop it explicitly: \"pod-server stop\".";
                msg += ss.str();
                g_exitCode = 2;
                throw runtime_error( msg );
            }
        }

        msg += "PoD server is NOT found.\n";
        if( _opt.m_debug )
            msg += _e.what();

        throw runtime_error( msg );
    }

    if( _status )
    {
        ostringstream ss;
        ss
                << "XPROOFD [" << srvHostInfo.m_xpdPid << "] port: " << srvHostInfo.m_xpdPort
                << "\nPoD agent [" << srvHostInfo.m_agentPid << "] port: " << srvHostInfo.m_agentPort;
        *_status = ss.str();
    }

    if( _con_string )
    {
        ostringstream ss;
        ss
                << srvHostInfo.m_username << "@" << srvHostInfo.m_host << ":"
                << srvHostInfo.m_xpdPort;
        *_con_string = ss.str();
    }

}
//=============================================================================
pid_t tunnelPid( const CEnvironment &_env )
{
    ifstream f( _env.getTunnelPidFile().c_str() );
    if( !f.is_open() )
        return 0;
    pid_t tunnel_pid;
    f >> tunnel_pid;
    return tunnel_pid;
}
//=============================================================================
void killTunnel( const CEnvironment &_env )
{
    pid_t pid = tunnelPid( _env );
    if( 0 != pid )
        kill( pid, SIGKILL );

    unlink( _env.getTunnelPidFile().c_str() );
}
//=============================================================================
void createSSHTunnel( const CEnvironment &_env, const SOptions &_opt )
{
    // delete tunnel's file
    killTunnel( _env );
    // create an ssh tunnel on PoD Server port
    switch( fork() )
    {
        case - 1:
            // Unable to fork
            throw std::runtime_error( "Unable to create an SSH tunnel." );
        case 0:
            {
                // create SSH Tunnel
                string cmd( "$POD_LOCATION/bin/private/pod-ssh-tunnel" );
                smart_path( &cmd );

                string pid_arg( "-f" );
                pid_arg += _env.getTunnelPidFile();

                string l_arg( "-l" );
                l_arg += _opt.m_sshConnectionStr;
                stringstream p_arg;
                p_arg << "-p" << _env.serverPort();
                string o_arg( "-o" );
                o_arg += _opt.m_openDomain;

                string sBatch;
                if( _opt.m_batchMode )
                    sBatch = "-b";

                execl( cmd.c_str(), "pod-ssh-tunnel",
                       pid_arg.c_str(),
                       l_arg.c_str(), p_arg.str().c_str(),
                       o_arg.c_str(), sBatch.c_str(), NULL );
                exit( 1 );
            }
    }
    // wait for tunnel to start
    short count( 0 );
    const short max_try( 50 );
    pid_t pid( tunnelPid( _env ) );
    while( 0 == pid || !IsProcessExist( pid ) ||
           0 != MiscCommon::INet::get_free_port( _env.serverPort() ) )
    {
        ++count;
        pid = tunnelPid( _env );
        if( count >= max_try )
            throw runtime_error( "Can't setup an SSH tunnel." );
        usleep( 50000 ); // delays for 0.05 seconds
    }
}
//=============================================================================
void retrieveRemoteServerInfo( const SOptions &_opt,
                               const string &_destinationFile )
{
    // delete first the remote srv info file
    unlink( _destinationFile.c_str() );

    StringVector_t arg;
    string sourceFile( _opt.m_remotePath );
    sourceFile += "etc/server_info.cfg";
    arg.push_back( "-s" + sourceFile );
    arg.push_back( "-l " + _opt.m_sshConnectionStr );
    arg.push_back( "-f " + _destinationFile );
    if( _opt.m_debug )
        arg.push_back( "-d" );
    if( _opt.m_batchMode )
        arg.push_back( "-b" );
    string cmd( "$POD_LOCATION/bin/private/pod-remote-srv-info" );
    smart_path( &cmd );
    string stdout;
    do_execv( cmd, arg, 60, NULL );
    if( !does_file_exists( _destinationFile ) )
    {
        stringstream ss;
        ss << "Remote PoD server is NOT running.";
        throw runtime_error( ss.str() );
    }
}
//=============================================================================
int main( int argc, char *argv[] )
{
    CEnvironment env;
    EPoDServerType srvType( SrvType_Unknown );

    try
    {
        env.init();

        SOptions options;
        if( !parseCmdLine( argc, argv, &options ) )
            return 0;

        // Check PoD server's Type
        srvType = ( options.m_sshConnectionStr.empty() ) ? SrvType_Local : SrvType_Remote;

        // if the type of the server is remote, than we need to get a remote
        // server info file
        string srvHost( env.serverHost() );
        // use SSH to retrieve server_info.cfg
        if( SrvType_Remote == srvType )
        {
            if( options.m_debug )
            {
                cout << "Trying to connect to a remote PoD server" << endl;
            }
            string outfile( env.remoteSrvInfoFile() );
            retrieveRemoteServerInfo( options, outfile );
            env.processServerInfoCfg( &outfile );
            // now we can delete the remote server file
            // we can't reuse it in next sessions, since the PoD port could change
            unlink( outfile.c_str() );

            // we tunnel the connection to PoD server
            createSSHTunnel( env, options );
            // if we tunnel pod-agent's port, than we need to connect to a localhost
            srvHost = "localhost";
        }
        else
        {
            if( options.m_debug )
            {
                cout
                        << "Trying to connect to a local PoD server"
                        << "Server Info: " << env.localSrvInfoFile() << endl;
            }
            // process a local server-info
            if( !env.processServerInfoCfg() )
            {
                throw runtime_error( "Can't process server info: " + env.localSrvInfoFile() );
            }
            srvHost = env.serverHost();
        }

        if( options.m_debug )
        {
            cout
                    << "connecting to PoD server: "
                    << srvHost << ":" << env.serverPort() << endl;
        }
        pod_info::CServer srv( srvHost, env.serverPort() );

        // Show version information
        if( options.m_version )
        {
            cout << version( env, srv ) << endl;
            killTunnel( env );
            return 0;
        }

        // PoD Server status
        if( options.m_status || options.m_connectionString )
        {
            string status;
            string con_string;
            srvPoDStatus( &status, &con_string, srv, options, env );
            if( options.m_status )
            {
                cout << status << endl;
            }
            if( options.m_connectionString )
            {
                cout << con_string << endl;
            }
        }

        // list of and a number of available WNs
        if( options.m_countWNs || options.m_listWNs )
        {
            string lst;
            size_t n = listWNs(( options.m_listWNs ? &lst : NULL ), srv, options );

            if( options.m_countWNs )
            {
                cout << n << endl;
            }
            if( options.m_listWNs )
            {
                cout << lst;
                cout.flush();
            }
        }
    }
    catch( exception& e )
    {
        killTunnel( env );
        cerr << PROJECT_NAME << ": " << e.what() << endl;
        return g_exitCode;
    }
    killTunnel( env );
    return g_exitCode;
}
