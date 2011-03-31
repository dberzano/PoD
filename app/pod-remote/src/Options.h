//
//  Options.h
//  PoD
//
//  Created by Anar Manafov on 02.02.11.
//  Copyright 2011 GSI. All rights reserved.
//
#ifndef OPTIONS_H
#define OPTIONS_H
//=============================================================================
// STD
#include <stdexcept>
// BOOST
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
// MiscCommon
#include "BOOSTHelper.h"
//=============================================================================
namespace bpo = boost::program_options;
namespace boost_hlp = MiscCommon::BOOSTHelper;
//=============================================================================
struct SOptions
{
    SOptions():
        m_version( false ),
        m_debug( false ),
        m_start( false ),
        m_stop( false ),
        m_restart( false ),
        m_executor( false )
    {
    }
    /// this function extracts a PoD Location from the stored connection string
    std::string remotePoDLocation()
    {
        std::string::size_type pos = m_sshConnectionStr.find( ":" );
        if( std::string::npos == pos )
            return "";

        return m_sshConnectionStr.substr( pos + 1 );
    }
    /// this function extracts an ssh connection string without the PoD location part
    std::string cleanConnectionString()
    {
        std::string::size_type pos = m_sshConnectionStr.find( ":" );
        if( std::string::npos == pos )
            return m_sshConnectionStr;

        return m_sshConnectionStr.substr( pos );
    }

    bool m_version;
    bool m_debug;
    bool m_start;
    bool m_stop;
    bool m_restart;
    bool m_executor;
    std::string m_sshConnectionStr;
    std::string m_sshArgs;
    std::string m_openDomain;
    std::string m_remotePath;
};
//=============================================================================
// Command line parser
inline bool parseCmdLine( int _Argc, char *_Argv[], SOptions *_options ) throw( std::exception )
{
    if( !_options )
        throw std::runtime_error( "Internal error: options' container is empty." );

    // Generic options
    bpo::options_description general_options( "General options" );
    general_options.add_options()
    ( "help,h", "Produce help message" )
    ( "version,v", bpo::bool_switch( &( _options->m_version ) ), "Version information" )
    ( "debug,d", bpo::bool_switch( &( _options->m_debug ) ), "Show debug messages" )
    ;
    // Connection options
    bpo::options_description connection_options( "Connection options" );
    connection_options.add_options()
    ( "remote", bpo::value<std::string>(), "A connection string including a remote PoD location" )
    ( "remote_path", bpo::value<std::string>(), "A working directory of the remote PoD server, if not a default one"
      " It is very important either to write an explicit path or use quotes, so that shell will not substitute local variable in the remote path. (default: ~/.PoD/)" )
    ( "ssh_opt", bpo::value<std::string>(), "Additional options, which will be used in SSH commands" )
    ( "ssh_open_domain", bpo::value<std::string>(), "The name of a third party machine open to the outside world"
      " and from which direct connections to the server are possible" )
    ;
    // Commands
    bpo::options_description commands_options( "Commands options" );
    commands_options.add_options()
    ( "start", bpo::bool_switch( &( _options->m_start ) ), "Start remote PoD server" )
    ( "stop", bpo::bool_switch( &( _options->m_stop ) ), "Stop remote PoD server" )
    ( "restart", bpo::bool_switch( &( _options->m_start ) ), "Restart remote PoD server" )
    ;
    // Options for internal use only
    bpo::options_description backend_options( "Backend options" );
    backend_options.add_options()
    ( "executor", bpo::bool_switch( &( _options->m_executor ) ), "Switch pod-remote to the executor mode" )
    ;

    // Declare an options description instance which will include
    // all the options
    bpo::options_description all( "Allowed options" );
    all.add( general_options ).add( connection_options ).add( commands_options ).add( backend_options );

    // Declare an options description instance which will be shown
    // to the user
    bpo::options_description visible( "Allowed options" );
    visible.add( general_options ).add( connection_options ).add( commands_options );

    // Parsing command-line
    bpo::variables_map vm;
    bpo::store( bpo::command_line_parser( _Argc, _Argv ).options( all ).run(), vm );
    bpo::notify( vm );

    boost_hlp::option_dependency( vm, "start", "remote" );
    boost_hlp::option_dependency( vm, "stop", "remote" );
    boost_hlp::option_dependency( vm, "restart", "remote" );
    boost_hlp::option_dependency( vm, "ssh_opt", "remote" );
    boost_hlp::option_dependency( vm, "remote_path", "remote" );
    boost_hlp::option_dependency( vm, "ssh_open_domain", "remote" );
    boost_hlp::conflicting_options( vm, "start", "stop" );
    boost_hlp::conflicting_options( vm, "start", "restart" );
    boost_hlp::conflicting_options( vm, "restart", "stop" );

    if( vm.count( "remote" ) )
    {
        _options->m_sshConnectionStr = vm["remote"].as<std::string>();
        // check that the given url contains a PoD location
        //if( _options->remotePoDLocation().empty() )
        //    throw std::runtime_error( "Bad PoD Location path in the remote url." );

        _options->m_remotePath = ( vm.count( "remote_path" ) ) ?
                                 vm["remote_path"].as<std::string>() : "~/.PoD/";
        MiscCommon::smart_append( &_options->m_remotePath, '/' );

        if( vm.count( "ssh_opt" ) )
        {
            _options->m_sshArgs = vm["ssh_opt"].as<std::string>();
        }
        if( vm.count( "ssh_open_domain" ) )
        {
            _options->m_openDomain = vm["ssh_open_domain"].as<std::string>();
        }
    }

    // if there are no arguments is given, produce a help message
    bpo::variables_map::const_iterator iter = vm.begin();
    bpo::variables_map::const_iterator iter_end = vm.end();
    bool show_help( true );
    for( ; iter != iter_end; ++iter )
    {
        if( !iter->second.defaulted() )
            show_help = false;
    }
    if( vm.count( "help" ) || show_help )
    {
        std::cout << visible << std::endl;
        return false;
    }

    return true;
}

#endif
