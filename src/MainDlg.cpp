/************************************************************************/
/**
 * @file MainDlg.cpp
 * @brief Main dialog implementation
 * @author Anar Manafov A.Manafov@gsi.de
 */ /*

        version number:     $LastChangedRevision$
        created by:         Anar Manafov
                            2007-05-23
        last changed by:    $LastChangedBy$ $LastChangedDate$

        Copyright (c) 2007-2009 GSI GridTeam. All rights reserved.
*************************************************************************/
// Qt
#include <QtGui>
// STD
#include <fstream>
#include <exception>
// BOOST
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
// MiscCommon
#include "def.h"
// PAConsole
#include "IJobManager.h"
#include "ServerInfo.h"
#include "MainDlg.h"
#include "version.h"
//=============================================================================
using namespace std;
using namespace MiscCommon;
//=============================================================================
// this is very expensive call, we therefore using 10 sec. timeout
const size_t g_UpdateInterval = 100;  // in seconds

const LPCTSTR g_szCfgFileName = "$POD_LOCATION/etc/pod-console.xml.cfg";
const LPCTSTR g_szPluginDir = "$POD_LOCATION/plugins";

// idle timeout. In ms.
// default 15 min.
const int g_idleTimeout = 120000;//900000;
//=============================================================================
template<class T>
void _loadcfg( T &_s, string _FileName )
{
    smart_path( &_FileName );
    if ( _FileName.empty() || !is_file_exists( _FileName ) )
        throw exception();

    ifstream f( _FileName.c_str() );
    //assert(f.good());
    boost::archive::xml_iarchive ia( f );
    ia >> BOOST_SERIALIZATION_NVP( _s );
}
//=============================================================================
template<class T>
void _savecfg( const T &_s, string _FileName )
{
    smart_path( &_FileName );
    if ( _FileName.empty() )
        throw exception();

    // make an archive
    ofstream f( _FileName.c_str() );
    //assert(f.good());
    boost::archive::xml_oarchive oa( f );
    oa << BOOST_SERIALIZATION_NVP( _s );
}
//=============================================================================
CMainDlg::CMainDlg( QDialog *_Parent ):
        QDialog( _Parent ),
        m_CurrentPage( 0 )
{
    m_ui.setupUi( this );

    Qt::WindowFlags flags( 0 );
    // Adding "Maximize" and "Minimize" buttons
    // Removing "What's this" button
    flags |= Qt::WindowTitleHint;
    flags |= Qt::WindowSystemMenuHint;
    flags |= Qt::WindowMaximizeButtonHint;
    flags |= Qt::WindowMinimizeButtonHint;
    setWindowFlags( flags );

    QString title( PROJECT_NAME" - " PROJECT_VERSION_STRING );
    setWindowTitle( title );

    // creating the idle timer
    m_idleTimer = new QTimer( this );
    connect( m_idleTimer, SIGNAL( timeout() ), this, SLOT( idleTimeout() ) );

    try
    {
        // Loading class from the config file
        _loadcfg( *this, g_szCfgFileName );
    }
    catch ( ... )
    {
        cerr << PROJECT_NAME << " Warning: "
             << "Can't load configuration from "
             << g_szCfgFileName
             << ". PAConsole will use its default settings." << endl;
    }

    // loading PAConsole plug-ins
    loadPlugins();

    size_t index( 0 );
    m_ui.pagesWidget->insertWidget( index, &m_server );

    PluginVec_t::const_iterator iter = m_plugins.begin();
    PluginVec_t::const_iterator iter_end = m_plugins.end();
    for ( ; iter != iter_end; ++iter )
    {
        QWidget *w = ( *iter )->getWidget();
        if ( !w )
            continue;
        m_ui.pagesWidget->insertWidget( ++index, w );

        // Immediately update interval when a user changes settings
        connect( &m_preferences, SIGNAL( changedJobStatusUpdInterval( int ) ), this, SLOT( updatePluginTimer( int ) ) );

        // jobs counters
        // We collecting signals from all plug-ins and the result is a sum of all jobs from all plug-ins
        connect( w, SIGNAL( changeNumberOfJobs( int ) ), this, SLOT( changeNumberOfJobs( int ) ) );
        connect( this, SIGNAL( numberOfJobs( int ) ), &m_workers, SLOT( setNumberOfJobs( int ) ) );
    }

    // Immediately update interval when a user changes settings
    connect( &m_preferences, SIGNAL( changedWorkersUpdInterval( int ) ), &m_workers, SLOT( restartUpdTimer( int ) ) );

    m_ui.pagesWidget->insertWidget( ++index, &m_workers );
    m_ui.pagesWidget->insertWidget( ++index, &m_preferences );

    createIcons();
    m_ui.contentsWidget->setCurrentRow( m_CurrentPage );

    // catching mouse events on the parent and all its children
    this->installEventFilter( this );
    enumAllChildren( qApp );
    foreach( QWidget* w, qApp->topLevelWidgets() )
    {
        enumAllChildren( w );
    }

    setMouseTracking( true );

    switchAllSensors( true );
}
//=============================================================================
void CMainDlg::enumAllChildren( QObject* o )
{
    o->installEventFilter( this );
    foreach( QObject* child, o->children() )
    enumAllChildren( child );
}
//=============================================================================
CMainDlg::~CMainDlg()
{
    try
    {
        // Saving class to the config file
        _savecfg( *this, g_szCfgFileName );
    }
    catch ( const exception &_e )
    {
        QMessageBox::warning( this, PROJECT_NAME,
                              "Can't save configuration to\n" + QString( g_szCfgFileName ) +
                              "\n Error: " + QString( _e.what() ) );
    }
    catch ( ... )
    {
        QMessageBox::warning( this, PROJECT_NAME,
                              "Can't save configuration to\n" + QString( g_szCfgFileName ) );
    }
}
//=============================================================================
void CMainDlg::createIcons()
{
    QListWidgetItem *serverButton = new QListWidgetItem( m_ui.contentsWidget );
    serverButton->setIcon( QIcon( ":/images/server.png" ) );
    serverButton->setText( tr( "Server" ) );
    serverButton->setTextAlignment( Qt::AlignHCenter );
    serverButton->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );

    PluginVec_t::const_iterator iter = m_plugins.begin();
    PluginVec_t::const_iterator iter_end = m_plugins.end();
    for ( ; iter != iter_end; ++iter )
    {
        // Starting the update timer
        ( *iter )->startUpdTimer( m_preferences.getJobStatusUpdInterval() );

        QWidget *w = ( *iter )->getWidget();
        if ( !w )
            continue;

        QListWidgetItem *btn = new QListWidgetItem( m_ui.contentsWidget );
        btn->setIcon(( *iter )->getIcon() );
        btn->setText(( *iter )->getName() );
        btn->setTextAlignment( Qt::AlignHCenter );
        btn->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );

    }

    QListWidgetItem *workersButton = new QListWidgetItem( m_ui.contentsWidget );
    workersButton->setIcon( QIcon( ":/images/workers.png" ) );
    workersButton->setText( tr( "PROOF Workers" ) );
    workersButton->setTextAlignment( Qt::AlignHCenter );
    workersButton->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );

    QListWidgetItem *preferencesButton = new QListWidgetItem( m_ui.contentsWidget );
    preferencesButton->setIcon( QIcon( ":/images/preferences.png" ) );
    preferencesButton->setText( tr( "Preferences" ) );
    preferencesButton->setTextAlignment( Qt::AlignHCenter );
    preferencesButton->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );

    connect( m_ui.contentsWidget,
             SIGNAL( currentItemChanged( QListWidgetItem *, QListWidgetItem * ) ),
             this, SLOT( changePage( QListWidgetItem *, QListWidgetItem* ) ) );
}
//=============================================================================
void CMainDlg::changePage( QListWidgetItem *_Current, QListWidgetItem *_Previous )
{
    if ( !_Current )
        _Current = _Previous;

    m_ui.pagesWidget->setCurrentIndex( m_ui.contentsWidget->row( _Current ) );
    m_CurrentPage = m_ui.pagesWidget->currentIndex();
}
//=============================================================================
void CMainDlg::updatePluginTimer( int _interval )
{
    // TODO: fix the code using for_each algorithm
    PluginVec_t::const_iterator iter = m_plugins.begin();
    PluginVec_t::const_iterator iter_end = m_plugins.end();
    for ( ; iter != iter_end; ++iter )
    {
        ( *iter )->startUpdTimer( _interval );
    }
}
//=============================================================================
void CMainDlg::loadPlugins()
{
    string pluginDir( g_szPluginDir );
    smart_path( &pluginDir );
    QDir pluginDirectory( pluginDir.c_str() );

    foreach( QString fileName, pluginDirectory.entryList( QDir::Files ) )
    {
        QPluginLoader loader( pluginDirectory.absoluteFilePath( fileName ) );
        QObject *plugin = loader.instance();
        if ( plugin )
        {
            IJobManager *obj( qobject_cast<IJobManager *>( plugin ) );
            if ( obj )
                m_plugins.push_back( obj );
        }
    }
}
//=============================================================================
/// This function collects a number of jobs from all of the plug-ins
/// It emits the numberOfJobs signal
void CMainDlg::changeNumberOfJobs( int /*_count*/ )
{
    int count( 0 );
    // TODO: fix the code using accumulate algorithm
    PluginVec_t::const_iterator iter = m_plugins.begin();
    PluginVec_t::const_iterator iter_end = m_plugins.end();
    for ( ; iter != iter_end; ++iter )
    {
        count += ( *iter )->getJobsCount();
    }

    emit numberOfJobs( count );
}
//=============================================================================
void CMainDlg::on_closeButton_clicked()
{
    CServerInfo si;
    if ( si.IsRunning( true ) )
    {
        const string msg( "PoD server is running.\n"
                          "Do you want to stop the server and shut all workers down?\n"
                          "If you answer NO, then the server and workers will continue to run after PAConsole is closed." );
        const QMessageBox::StandardButton reply =
            QMessageBox::question( this, tr( PROJECT_NAME ), tr( msg.c_str() ),
                                   QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel );
        if ( QMessageBox::Cancel == reply )
            return;
        if ( QMessageBox::Yes == reply )
            m_server.CommandServer( CServerDlg::srvSTOP );
    }

    close();
}
//=============================================================================
void CMainDlg::idleTimeout()
{
    // Now, stopping all sensors
    switchAllSensors( false );
    stringstream ss;
    ss << "There were no user interactions for the last "
    << g_idleTimeout / 60000 << " minutes.\n"
    << "To avoid the system overloading the console has switched monitoring and all sensors off.\n"
    << "To continue to monitor, please close this dialog.";
    QMessageBox::information( this, PROJECT_NAME,
                              tr( ss.str().c_str() ) );
    // restarting all sensors here
    switchAllSensors( true );
}
//=============================================================================
void CMainDlg::childEvent( QChildEvent *_event )
{
    if ( !_event->child()->isWidgetType() )
        return;

    if ( _event->type() == QEvent::ChildAdded )
    {
        // setting up the event filter for the new child
        _event->child()->installEventFilter( this );
        QWidget *w( dynamic_cast<QWidget*>( _event->child() ) );
        w->setMouseTracking( true );
    }
    QWidget::childEvent( _event );
}
//=============================================================================
bool CMainDlg::eventFilter( QObject *obj, QEvent *event )
{
    if ( event->type() == QEvent::MouseMove )
    {
        m_idleTimer->start( g_idleTimeout );
    }

    // standard event processing
    return QObject::eventFilter( obj, event );
}
//=============================================================================
void CMainDlg::switchAllSensors( bool _on )
{
    if ( _on )
    {
        m_idleTimer->start( g_idleTimeout );
        m_workers.restartUpdTimer( m_preferences.getWorkersUpdInterval() );
        m_server.setUpdTimer( g_UpdateInterval );
        PluginVec_t::const_iterator iter = m_plugins.begin();
        PluginVec_t::const_iterator iter_end = m_plugins.end();
        for ( ; iter != iter_end; ++iter )
        {
            ( *iter )->startUpdTimer( m_preferences.getJobStatusUpdInterval() );
        }
    }
    else
    {
        m_idleTimer->stop();
        m_workers.restartUpdTimer( 0 );
        m_server.setUpdTimer( 0 );
        PluginVec_t::const_iterator iter = m_plugins.begin();
        PluginVec_t::const_iterator iter_end = m_plugins.end();
        for ( ; iter != iter_end; ++iter )
        {
            ( *iter )->startUpdTimer( 0 );
        }
    }
}
