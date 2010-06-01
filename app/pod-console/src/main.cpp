/************************************************************************/
/**
 * @file main.cpp
 * @brief main file
 * @author Anar Manafov A.Manafov@gsi.de
 */ /*

        version number:     $LastChangedRevision$
        created by:         Anar Manafov
                            2007-05-23
        last changed by:    $LastChangedBy$ $LastChangedDate$

        Copyright (c) 2007-2010 GSI GridTeam. All rights reserved.
*************************************************************************/
// Qt
#include <QApplication>
// Our
#include "MainDlg.h"
//=============================================================================
int main( int argc, char **argv, char** envp )
{
    Q_INIT_RESOURCE( paconsole );

    QApplication app( argc, argv );
    CMainDlg dlg;
    dlg.setEnvironment( envp );
    dlg.show();
    return app.exec();
}