/************************************************************************/
/**
 * @file LSFDlg.h
 * @brief $$File comment$$
 * @author Anar Manafov A.Manafov@gsi.de
 *//*

        version number:    $LastChangedRevision$
        created by:        Anar Manafov
                           2008-12-09
        last changed by:   $LastChangedBy$ $LastChangedDate$

        Copyright (c) 2008 GSI GridTeam. All rights reserved.
*************************************************************************/
// BOOST
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
// PAConsole LSF plug-in
#include "LSFDlg.h"
// Qt
#include <QtGui>
// STD
#include <fstream>
// MiscCommon
#include "def.h"
#include "SysHelper.h"
// PAConsole
#include "ServerInfo.h"

using namespace std;
using namespace MiscCommon;
// default Job Script file
const LPCTSTR g_szDefaultJobScript = "$GLITE_PROOF_LOCATION/etc/Job.lsf";
// configuration file of the plug-in
const LPCTSTR g_szLSFPluginCfgFileName = "$GLITE_PROOF_LOCATION/etc/PAConsole_LSF.xml.cfg";


// TODO: avoid of code duplications (this two function must be put in MiscCommon)
// Serialization helpers
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

CLSFDlg::CLSFDlg( QWidget *parent ) :
        QWidget( parent ),
        m_AllJobsCount( 0 ),
        m_JobSubmitter( this )
{
    m_ui.setupUi( this );

    m_Timer = new QTimer( this );
    connect( m_Timer, SIGNAL( timeout() ), this, SLOT( updateJobsTree() ) );

    connect( &m_JobSubmitter, SIGNAL( changeProgress( int ) ), this, SLOT( setProgress( int ) ) );
    connect( &m_JobSubmitter, SIGNAL( sendThreadMsg( const QString& ) ), this, SLOT( recieveThreadMsg( const QString& ) ) );
    connect( &m_JobSubmitter, SIGNAL( changeNumberOfJobs( int ) ), this, SLOT( setNumberOfJobs( int ) ) );

    createActions();

    clipboard = QApplication::clipboard();

    // Set completion for the edit box of JDL file name
    QCompleter *completer = new QCompleter( this );
    completer->setModel( new QDirModel( completer ) );
    m_ui.edtJobScriptFileName->setCompleter( completer );

    try
    {
        // Loading class from the config file
        _loadcfg( *this, g_szLSFPluginCfgFileName );
    }
    catch ( ... )
    {
        setAllDefault();
    }

    // TODO: move this to GUI setting
    m_JobSubmitter.setQueue( "proof" );
}

CLSFDlg::~CLSFDlg()
{
    try
    {
        // Saving class to the config file
        _savecfg( *this, g_szLSFPluginCfgFileName );
    }
    catch ( ... )
    {
    }
}

void CLSFDlg::setAllDefault()
{
    m_JobScript = g_szDefaultJobScript;
    m_WorkersCount = 1;
    m_JobSubmitter.setAllDefault();
    UpdateAfterLoad();
}

void CLSFDlg::UpdateAfterLoad()
{
    smart_path( &m_JobScript );
    m_ui.edtJobScriptFileName->setText( m_JobScript.c_str() );
    m_ui.spinNumWorkers->setValue( m_WorkersCount );

    updateJobsTree();
}

void CLSFDlg::recieveThreadMsg( const QString &_Msg )
{
    QMessageBox::critical( this, tr( "PROOFAgent Console" ), _Msg );
    m_ui.btnSubmitClient->setEnabled( true );
}

void CLSFDlg::on_btnSubmitClient_clicked()
{
    // Checking first that gLitePROOF server is running
    CServerInfo si;
    if ( !si.IsRunning( true ) )
    {
        const string msg( "gLitePROOF server is not running.\n"
                          "Do you want to submit this job anyway?" );
        const QMessageBox::StandardButton reply =
            QMessageBox::question( this, tr( "PROOFAgent Console" ), tr( msg.c_str() ),
                                   QMessageBox::Yes | QMessageBox::No );
        if ( QMessageBox::Yes != reply )
            return;
    }

    if ( !m_JobSubmitter.isRunning() )
    {
        if ( !QFileInfo( m_ui.edtJobScriptFileName->text() ).exists() )
        {
            QMessageBox::critical( this,
                                   tr( "PROOFAgent Console" ),
                                   tr( "File\n\"%1\"\ndoesn't exist!" ).arg(
                                       m_ui.edtJobScriptFileName->text() ) );
            return;
        }

        m_JobScript = m_ui.edtJobScriptFileName->text().toAscii().data();

        m_WorkersCount = m_ui.spinNumWorkers->value();
        m_JobSubmitter.setNumberOfWorkers( m_WorkersCount );

        m_JobSubmitter.setJobScriptFilename( m_JobScript );
        // submit gLite jobs
        m_JobSubmitter.start();
        m_ui.btnSubmitClient->setEnabled( false );
    }
    else
    {
        // Job submitter's thread
        m_JobSubmitter.terminate();
        setProgress( 0 );
        m_ui.btnSubmitClient->setEnabled( true );
    }
}

void CLSFDlg::updateJobsTree()
{
    m_TreeItems.update( m_JobSubmitter.getActiveJobList(), m_ui.treeJobs );
}

void CLSFDlg::on_btnBrowseJobScript_clicked()
{
    const QString dir = QFileInfo( m_ui.edtJobScriptFileName->text() ).absolutePath();
    const QString filename = QFileDialog::getOpenFileName( this, tr( "Select a job script file" ), dir,
                             tr( "LSF script (*.lsf)" ) );
    if ( QFileInfo( filename ).exists() )
    {
        m_JobScript = filename.toAscii().data();
        m_ui.edtJobScriptFileName->setText( filename );
    }
}

void CLSFDlg::createActions()
{
//    // COPY Job ID
//    copyJobIDAct = new QAction( tr( "&Copy JobID" ), this );
//    copyJobIDAct->setShortcut( tr( "Ctrl+C" ) );
//    copyJobIDAct->setStatusTip( tr( "Copy selected jod id to the clipboard" ) );
//    connect( copyJobIDAct, SIGNAL( triggered() ), this, SLOT( copyJobID() ) );
//    // CANCEL Job
//    cancelJobAct = new QAction( tr( "Canc&el Job" ), this );
//    cancelJobAct->setShortcut( tr( "Ctrl+E" ) );
//    cancelJobAct->setStatusTip( tr( "Cancel the selected jod" ) );
//    connect( cancelJobAct, SIGNAL( triggered() ), this, SLOT( cancelJob() ) );
//    // GET OUTPUT of the Job
//    getJobOutputAct = new QAction( tr( "Get &output" ), this );
//    getJobOutputAct->setShortcut( tr( "Ctrl+O" ) );
//    getJobOutputAct->setStatusTip( tr( "Get output sandbox of the selected jod" ) );
//    connect( getJobOutputAct, SIGNAL( triggered() ), this, SLOT( getJobOutput() ) );
//    // Get Logging Info
//    getJobLoggingInfoAct = new QAction( tr( "Get &logging info" ), this );
//    getJobLoggingInfoAct->setShortcut( tr( "Ctrl+L" ) );
//    getJobLoggingInfoAct->setStatusTip( tr( "Get logging info of the selected jod" ) );
//    connect( getJobLoggingInfoAct, SIGNAL( triggered() ), this, SLOT( getJobLoggingInfo() ) );
    // Remove Job from monitoring
    removeJobAct = new QAction( tr( "&Remove Job" ), this );
    removeJobAct->setShortcut( tr( "Ctrl+R" ) );
    removeJobAct->setStatusTip( tr( "Remove the selected job from monitoring" ) );
    connect( removeJobAct, SIGNAL( triggered() ), this, SLOT( removeJob() ) );
}

void CLSFDlg::contextMenuEvent( QContextMenuEvent *event )
{
    // Checking that *treeJobs* has been selected
    QPoint pos = event->globalPos();
    if ( !m_ui.treeJobs->childrenRect().contains( m_ui.treeJobs->mapFromGlobal( pos ) ) )
        return;

    // We need to disable menu items when no jobID is selected
    const QTreeWidgetItem * item( m_ui.treeJobs->currentItem() );

    QMenu menu( this );
//   menu.addAction( copyJobIDAct );
//   copyJobIDAct->setEnabled( item );

//    menu.addAction( getJobOutputAct );
//    getJobOutputAct->setEnabled( item );

//    menu.addAction( getJobLoggingInfoAct );
//    getJobLoggingInfoAct->setEnabled( item );

    menu.addSeparator();
    menu.addAction( removeJobAct );
    removeJobAct->setEnabled( item );

//    menu.addSeparator();
//    menu.addAction( cancelJobAct );
//    cancelJobAct->setEnabled( item );

    menu.exec( event->globalPos() );
}

//void CLSFDlg::copyJobID() const
//{
////    // Copy selected JobID to clipboard
////    const QTreeWidgetItem *item( m_ui.treeJobs->currentItem() );
////    if ( !item )
////        return;
////    clipboard->setText( item->text( 0 ), QClipboard::Clipboard );
////    clipboard->setText( item->text( 0 ), QClipboard::Selection );
//}
//
//void CLSFDlg::cancelJob()
//{
////    const QTreeWidgetItem *item( m_ui.treeJobs->currentItem() );
////    if ( !item )
////        return;
////
////    const string jobid( item->text( 0 ).toAscii().data() );
////    const string msg( "Do you really want to cancel the job:\n" + jobid );
////    const QMessageBox::StandardButton
////    reply( QMessageBox::question( this, tr( "PROOFAgent Console" ), tr( msg.c_str() ),
////                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel ) );
////    if ( QMessageBox::Yes != reply )
////        return;
////
////    try
////    {
////        CGLiteAPIWrapper::Instance().GetJobManager().JobCancel( jobid );
////    }
////    catch ( const exception &_e )
////    {
////        QMessageBox::critical( this, tr( "PROOFAgent Console" ), tr( _e.what() ) );
////        return;
////    }
//}
//
//void CLSFDlg::getJobOutput()
//{
////    const QTreeWidgetItem *item( m_ui.treeJobs->currentItem() );
////    if ( !item )
////        return;
////
////    bool ok;
////    const QString
////    path =
////        QInputDialog::getText(
////            this,
////            tr( "PROOFAgent Console" ),
////            tr(
////                "Enter the path, where output files should be delivered to:" ),
////            QLineEdit::Normal, QDir::home().absolutePath(), &ok );
////    if ( !ok || path.isEmpty() )
////        return;
////
////    const string jobid( item->text( 0 ).toAscii().data() );
////
////    gaw_path_type joboutput_path;
////    try
////    {
////        CGLiteAPIWrapper::Instance().GetJobManager().JobOutput( jobid, path.toAscii().data(),
////                                                                &joboutput_path, true );
////    }
////    catch ( const exception &_e )
////    {
////        QMessageBox::critical( this, tr( "PROOFAgent Console" ), tr( _e.what() ) );
////        return;
////    }
////
////    ostringstream ss;
////    transform( joboutput_path.begin(), joboutput_path.end(),
////               ostream_iterator<string> ( ss, "\n____\n" ), gaw_path_type_to_string );
////    QMessageBox::information( this, tr( "PROOFAgent Console" ), tr( ss.str().c_str() ) );
//}
//
//void CLSFDlg::getJobLoggingInfo()
//{
////    // Job ID
////    const QTreeWidgetItem *item( m_ui.treeJobs->currentItem() );
////    if ( !item )
////        return;
////
////    const string jobid( item->text( 0 ).toAscii().data() );
////
////    CLogInfoDlg dlg( this, jobid );
////    dlg.exec();
//}

void CLSFDlg::removeJob()
{
    // Job ID
    const QTreeWidgetItem *item( m_ui.treeJobs->currentItem() );
    if ( !item )
        return;

    const string jobid( item->text( 0 ).toAscii().data() );

    m_JobSubmitter.removeJob( jobid );
    updateJobsTree();
}

void CLSFDlg::setProgress( int _Val )
{
    if ( 100 == _Val )
    {
        m_ui.btnSubmitClient->setEnabled( true );
        // The job is submitted, we need to update the tree
        updateJobsTree();
    }
    m_ui.progressSubmittedJobs->setValue( _Val );
}

void CLSFDlg::on_edtJobScriptFileName_textChanged( const QString &/*_text*/ )
{
    m_JobScript = m_ui.edtJobScriptFileName->text().toAscii().data();
}

void CLSFDlg::setNumberOfJobs( int _count )
{
    m_AllJobsCount = _count;
    emit changeNumberOfJobs( _count );
}

QString CLSFDlg::getName() const
{
    return QString( "LSF\nJob Manager" );
}
QWidget* CLSFDlg::getWidget()
{
    return this;
}
QIcon CLSFDlg::getIcon()
{
    return QIcon( ":/images/lsf.png" );
}
void CLSFDlg::startUpdTimer( int _JobStatusUpdInterval )
{
    // start or restart the timer
    if ( _JobStatusUpdInterval > 0 )
        m_Timer->start( _JobStatusUpdInterval * 1000 );
}
int CLSFDlg::getJobsCount() const
{
    return m_AllJobsCount;
}

Q_EXPORT_PLUGIN2( LSFJobManager, CLSFDlg );
