/************************************************************************/
/**
 * @file Options.h
 * @brief
 * @author Anar Manafov A.Manafov@gsi.de
 */ /*

        version number:     $LastChangedRevision$
        created by:         Anar Manafov
                            2009-05-27
        last changed by:    $LastChangedBy$ $LastChangedDate$

        Copyright (c) 2009 GSI GridTeam. All rights reserved.
*************************************************************************/
#ifndef OPTIONS_H
#define OPTIONS_H
// MiscCommon
#include "PARes.h"
#include "PoDUserDefaultsOptions.h"

namespace PROOFAgent
{
    /**
     *
     * @brief PROOFAgent's container of options
     *
     */
    typedef struct SOptions
    {
        typedef enum ECommand { Start, Stop, Status } ECommand_t;

        SOptions():                        // Default options' values
            m_Command( Start ),
            m_bDaemonize( false ),
            m_bValidate( false ),
            m_agentMode( Server ),
            m_numberOfPROOFWorkers( 1 )
        {}

        std::string m_sConfigFile;
        ECommand_t m_Command;
        bool m_bDaemonize;
        bool m_bValidate;
        EAgentMode_t m_agentMode;       //!< A mode of PROOFAgent, defined by ::EAgentMode_t.
        std::string m_serverInfoFile;
        unsigned int m_numberOfPROOFWorkers;

        PoD::SPoDUserDefaultsOptions_t m_podOptions;
    } SOptions_t;
}

#endif
