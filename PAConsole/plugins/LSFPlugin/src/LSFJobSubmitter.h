/************************************************************************/
/**
 * @file LSFJobSubmitter.h
 * @brief A thread-class which submits Grid jobs and reports a progress status
 * @author Anar Manafov A.Manafov@gsi.de
 */ /*

        version number:     $LastChangedRevision$
        created by:         Anar Manafov
                            2007-01-01
        last changed by:    $LastChangedBy$ $LastChangedDate$

        Copyright (c) 2009 GSI GridTeam. All rights reserved.
*************************************************************************/
#ifndef LSFJOBSUBMITTER_H_
#define LSFJOBSUBMITTER_H_

// gLite plug-in
#include "LsfMng.h"
// STD
#include <sstream>
// Qt
#include <QThread>
#include <QMutex>
// BOOST
#include <boost/serialization/version.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/set.hpp>


class CLSFJobSubmitter: public QThread
{
        Q_OBJECT

        friend class boost::serialization::access;

    public:
        typedef std::set<LS_LONG_INT_t> jobslist_t;

    public:
        CLSFJobSubmitter( QObject *parent ): QThread( parent )
        {
            init();
        }
        ~CLSFJobSubmitter()
        {
            if ( isRunning() )
                terminate();
        }

    public:
        bool init()
        {
            try
            {
                m_lsf.init();
            }
            catch ( const std::exception &e )
            {
                return false;
            }
            return true;
        }
        const jobslist_t &getActiveJobList()
        {
            // Retrieving a number of children of the parametric job
            emit changeNumberOfJobs( getNumberOfJobs() );

            return m_JobsList;
        }
        void setAllDefault()
        {
            emit changeNumberOfJobs( 0 );
            m_JobsList.clear();
        }
        void setJobScriptFilename( const std::string &_JobScriptFilename )
        {
            m_JobScriptFilename = _JobScriptFilename;
        }
        void setNumberOfWorkers( int _WrkN )
        {
            // setting job array
            std::ostringstream jobname;
            jobname << getpid() << "[1-" << _WrkN << "]";

            m_lsf.addProperty( CLsfMng::JP_SUB_JOB_NAME, jobname.str() );
        }
        void setQueue( const std::string &_queue )
        {
            m_lsf.addProperty( CLsfMng::JP_SUB_QUEUE, _queue );
        }
        void RemoveJob( const LS_LONG_INT_t &_JobID )
        {
            m_mutex.lock();
            m_JobsList.erase( _JobID );
            m_mutex.unlock();

            emit changeNumberOfJobs( getNumberOfJobs() );
        }

    signals:
        void changeProgress( int _Val );
        void changeNumberOfJobs( int _Val );
        void sendThreadMsg( const QString &_Msg );

    protected:
        void run()
        {
            emit changeProgress( 0 );

            // Submit a Grid Job
            try
            {
                emit changeProgress( 30 );

                LS_LONG_INT_t nLastJobID = m_lsf.jobSubmit( m_JobScriptFilename );

                emit changeProgress( 90 );

                m_mutex.lock();
                m_JobsList.insert( nLastJobID );
                m_mutex.unlock();

                // Retrieving a number of children of the parametric job
                emit changeNumberOfJobs( getNumberOfJobs() );
            }
            catch ( const std::exception &_e )
            {
                emit sendThreadMsg( tr( _e.what() ) );
                emit changeProgress( 0 );
                return;
            }

            emit changeProgress( 100 );
        }

        // this function is very "expensive",
        // we therefore use it only via signal only when number of jobs may be changed.
        // Users can't call it any time they want.
        int getNumberOfJobs() const
        {
            if ( m_JobsList.empty() )
                return 0;

            try
            {
                jobslist_t::const_iterator iter = m_JobsList.begin();
                jobslist_t::const_iterator iter_end = m_JobsList.end();
                // Retrieving a number of children of the parametric job
                size_t num( 0 );
                //      MiscCommon::StringVector_t jobs;
                for ( ; iter != iter_end; ++iter )
                {
                    //            MiscCommon::gLite::CJobStatusObj( *iter ).GetChildren( &jobs );
                    //        num += jobs.size();
                    //  jobs.clear();
                }
                return ( num );
            }
            catch ( ... )
                {}
            return 0;
        }

        // serialization
        template<class Archive>
        void save( Archive & _ar, const unsigned int /*_version*/ ) const
        {
            _ar & BOOST_SERIALIZATION_NVP( m_JobsList );
        }
        template<class Archive>
        void load( Archive & _ar, const unsigned int _version )
        {
            m_mutex.lock();
            _ar & BOOST_SERIALIZATION_NVP( m_JobsList );
            m_mutex.unlock();
        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()

    private:
        jobslist_t m_JobsList;
        QMutex m_mutex;
        std::string m_JobScriptFilename;
        CLsfMng m_lsf;
};

BOOST_CLASS_VERSION( CLSFJobSubmitter, 1 )

#endif /*LSFJOBSUBMITTER_H_*/
