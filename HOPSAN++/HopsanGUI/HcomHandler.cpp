﻿/*-----------------------------------------------------------------------------
 This source file is part of Hopsan NG

 Copyright (c) 2011
    Mikael Axin, Robert Braun, Alessandro Dell'Amico, Björn Eriksson,
    Peter Nordin, Karl Pettersson, Petter Krus, Ingo Staack

 This file is provided "as is", with no guarantee or warranty for the
 functionality or reliability of the contents. All contents in this file is
 the original work of the copyright holders at the Division of Fluid and
 Mechatronic Systems (Flumes) at Linköping University. Modifying, using or
 redistributing any part of this file is prohibited without explicit
 permission from the copyright holders.
-----------------------------------------------------------------------------*/

//!
//! @file   HcomHandler.cpp
//! @author Robert Braun <robert.braun@liu.se>
//! @date   2013
//! @version $Id$
//!
//! @brief Contains a handler for the HCOM scripting language
//!
//HopsanGUI includes
#include "common.h"
#include "Configuration.h"
#include "DesktopHandler.h"
#include "GUIConnector.h"
#include "GUIObjects/GUIComponent.h"
#include "GUIObjects/GUISystem.h"
#include "GUIPort.h"
#include "HcomHandler.h"
#include "MainWindow.h"
#include "PlotCurve.h"
#include "PlotHandler.h"
#include "PlotTab.h"
#include "PlotWindow.h"
#include "SimulationThreadHandler.h"
#include "Utilities/GUIUtilities.h"
#include "Widgets/HcomWidget.h"
#include "Widgets/PlotWidget.h"
#include "Widgets/ProjectTabWidget.h"

//HopsanGenerator includes
#include "symhop/SymHop.h"

//Dependency includes
#include "qwt_plot.h"


HcomHandler::HcomHandler(TerminalConsole *pConsole) : QObject(pConsole)
{
    mAborted = false;
    mLocalVars.insert("ans", 0);
    mRetvalType = Scalar;

    mpConsole = pConsole;

    mCurrentPlotWindowName = "PlotWindow0";

    mPwd = gDesktopHandler.getDocumentsPath();
    mPwd.chop(1);

    mOptAlgorithm = Uninitialized;

    //Setup local function pointers (used to evaluate expressions in SymHop)
    registerFunction("aver", "Calculate average value of vector", &(_funcAver));
    registerFunction("min", "Calculate minimum value of vector", &(_funcMin));
    registerFunction("max", "Calculate maximum value of vector", &(_funcMax));
    registerFunction("imin", "Calculate index of minimum value of vector", &(_funcIMin));
    registerFunction("imax", "Calculate index of maximum value of vector", &(_funcIMax));
    registerFunction("size", "Calculate the size of a vector", &(_funcSize));
    registerFunction("rand", "Generates a random value between 0 and 1", &(_funcRand));
    registerFunction("peek", "Returns vector value at specified index", &(_funcPeek));
    registerFunction("obj", "Returns optimization objective function value with specified index", &(_funcObj));
    registerFunction("time", "Returns last simulation time", &(_funcTime));

    createCommands();
}


//! @brief Creates all command objects that can be used in terminal
void HcomHandler::createCommands()
{
    HcomCommand helpCmd;
    helpCmd.cmd = "help";
    helpCmd.description.append("Shows help information");
    helpCmd.help.append("Usage: help [command]");
    helpCmd.fnc = &HcomHandler::executeHelpCommand;
    mCmdList << helpCmd;

    HcomCommand simCmd;
    simCmd.cmd = "sim";
    simCmd.description.append("Simulates current model");
    simCmd.help.append("Usage: sim [no arguments]");
    simCmd.fnc = &HcomHandler::executeSimulateCommand;
    simCmd.group = "Simulation Commands";
    mCmdList << simCmd;

    HcomCommand chpvCmd;
    chpvCmd.cmd = "chpv";
    chpvCmd.description.append("Change plot variables in current plot");
    chpvCmd.help.append("Usage: chpv [leftvar1 [leftvar2] ... [-r rightvar1 rightvar2 ... ]]");
    chpvCmd.fnc = &HcomHandler::executePlotCommand;
    simCmd.group = "Plot Commands";
    mCmdList << chpvCmd;

    HcomCommand exitCmd;
    exitCmd.cmd = "exit";
    exitCmd.description.append("Exits the program");
    exitCmd.help.append("Usage: exit [no arguments]");
    exitCmd.fnc = &HcomHandler::executeExitCommand;
    mCmdList << exitCmd;

    HcomCommand dipaCmd;
    dipaCmd.cmd = "dipa";
    dipaCmd.description.append("Display parameter value");
    dipaCmd.help.append("Usage: dipa [parameter]");
    dipaCmd.fnc = &HcomHandler::executeDisplayParameterCommand;
    dipaCmd.group = "Parameter Commands";
    mCmdList << dipaCmd;

    HcomCommand chpaCmd;
    chpaCmd.cmd = "chpa";
    chpaCmd.description.append("Change parameter value");
    chpaCmd.help.append("Usage: chpa [parameter value]");
    chpaCmd.fnc = &HcomHandler::executeChangeParameterCommand;
    chpaCmd.group = "Parameter Commands";
    mCmdList << chpaCmd;

    HcomCommand chssCmd;
    chssCmd.cmd = "chss";
    chssCmd.description.append("Change simulation settings");
    chssCmd.help.append("Usage: chss [starttime timestep stoptime [samples]]");
    chssCmd.fnc = &HcomHandler::executeChangeSimulationSettingsCommand;
    chssCmd.group = "Simulation Commands";
    mCmdList << chssCmd;

    HcomCommand execCmd;
    execCmd.cmd = "exec";
    execCmd.description.append("Executes a script file");
    execCmd.help.append("Usage: exec [filepath]");
    execCmd.fnc = &HcomHandler::executeRunScriptCommand;
    execCmd.group = "File Commands";
    mCmdList << execCmd;

    HcomCommand wrhiCmd;
    wrhiCmd.cmd = "wrhi";
    wrhiCmd.description.append("Writes history to file");
    wrhiCmd.help.append("Usage: wrhi [filepath]");
    wrhiCmd.fnc = &HcomHandler::executeWriteHistoryToFileCommand;
    wrhiCmd.group = "File Commands";
    mCmdList << wrhiCmd;

    HcomCommand printCmd;
    printCmd.cmd = "print";
    printCmd.description.append("Prints arguments on the screen");
    printCmd.help.append("Usage: print [\"Text\" (variable)]\n");
    printCmd.help.append("Note: Not implemented yet.");
    printCmd.fnc = &HcomHandler::executePrintCommand;
    mCmdList << printCmd;

    HcomCommand chpwCmd;
    chpwCmd.cmd = "chpw";
    chpwCmd.description.append("Changes current plot window");
    chpwCmd.help.append("Usage: chpw [number]");
    chpwCmd.fnc = &HcomHandler::executeChangePlotWindowCommand;
    chpwCmd.group = "Plot Commands";
    mCmdList << chpwCmd;

    HcomCommand dipwCmd;
    dipwCmd.cmd = "dipw";
    dipwCmd.description.append("Displays current plot window");
    dipwCmd.help.append(" Usage: dipw [no arguments]");
    dipwCmd.fnc = &HcomHandler::executeDisplayPlotWindowCommand;
    dipwCmd.group = "Plot Commands";
    mCmdList << dipwCmd;

    HcomCommand chpvlCmd;
    chpvlCmd.cmd = "chpvl";
    chpvlCmd.description.append("Changes plot variables on left axis in current plot");
    chpvlCmd.help.append(" Usage: chpvl [var1 var2 ... ]");
    chpvlCmd.fnc = &HcomHandler::executePlotLeftAxisCommand;
    chpvlCmd.group = "Plot Commands";
    mCmdList << chpvlCmd;

    HcomCommand chpvrCmd;
    chpvrCmd.cmd = "chpvr";
    chpvrCmd.description.append("Changes plot variables on right axis in current plot");
    chpvrCmd.help.append(" Usage: chpvr [var1 var2 ... ]");
    chpvrCmd.fnc = &HcomHandler::executePlotRightAxisCommand;
    chpvrCmd.group = "Plot Commands";
    mCmdList << chpvrCmd;

    HcomCommand dispCmd;
    dispCmd.cmd = "disp";
    dispCmd.description.append("Shows a list of all variables matching specified name filter (using asterisks)");
    dispCmd.help.append("Usage: disp [filter]");
    dispCmd.fnc = &HcomHandler::executeDisplayVariablesCommand;
    dispCmd.group = "Variable Commands";
    mCmdList << dispCmd;

    HcomCommand peekCmd;
    peekCmd.cmd = "peek";
    peekCmd.description.append("Shows the value at a specified index in a specified data variable");
    peekCmd.help.append("Usage: peek [variable index]");
    peekCmd.group = "Variable Commands";
    peekCmd.fnc = &HcomHandler::executePeekCommand;

    mCmdList << peekCmd;

    HcomCommand pokeCmd;
    pokeCmd.cmd = "poke";
    pokeCmd.description.append("Changes the value at a specified index in a specified data variable");
    pokeCmd.help.append("Usage: poke [variable index newvalue]");
    pokeCmd.fnc = &HcomHandler::executePokeCommand;
    pokeCmd.group = "Variable Commands";
    mCmdList << pokeCmd;

    HcomCommand aliasCmd;
    aliasCmd.cmd = "alias";
    aliasCmd.description.append("Defines an alias for a variable");
    aliasCmd.help.append("Usage: alias [variable alias]");
    aliasCmd.fnc = &HcomHandler::executeDefineAliasCommand;
    aliasCmd.group = "Variable Commands";
    mCmdList << aliasCmd;

    HcomCommand setCmd;
    setCmd.cmd = "set";
    setCmd.description.append("Sets Hopsan preferences");
    setCmd.help.append(" Usage: set [preference value]\n");
    setCmd.help.append(" Available commands:\n");
    setCmd.help.append("  multicore [on/off]\n");
    setCmd.help.append("  threads [number]\n");
    setCmd.help.append("  cachetodisk [on/off]\n");
    setCmd.help.append("  generationlimit [number]\n");
    setCmd.help.append("  samples [number]");
    setCmd.fnc = &HcomHandler::executeSetCommand;
    mCmdList << setCmd;

    HcomCommand saplCmd;
    saplCmd.cmd = "sapl";
    saplCmd.description.append("Saves plot file to .PLO");
    saplCmd.help.append("Usage: sapl [filepath variables]");
    saplCmd.fnc = &HcomHandler::executeSaveToPloCommand;
    saplCmd.group = "Plot Commands";
    mCmdList << saplCmd;

    HcomCommand loadCmd;
    loadCmd.cmd = "load";
    loadCmd.description.append("Loads a model file");
    loadCmd.help.append("Usage: load [filepath variables]");
    loadCmd.fnc = &HcomHandler::executeLoadModelCommand;
    loadCmd.group = "Model Commands";
    mCmdList << loadCmd;

    HcomCommand loadrCmd;
    loadrCmd.cmd = "loadr";
    loadrCmd.description.append("Loads most recent model file");
    loadrCmd.help.append("Usage: loadr [no arguments]");
    loadrCmd.fnc = &HcomHandler::executeLoadRecentCommand;
    loadrCmd.group = "Model Commands";
    mCmdList << loadrCmd;

    HcomCommand pwdCmd;
    pwdCmd.cmd = "pwd";
    pwdCmd.description.append("Displays present working directory");
    pwdCmd.help.append("Usage: pwd [no arguments]");
    pwdCmd.fnc = &HcomHandler::executePwdCommand;
    pwdCmd.group = "Model Commands";
    mCmdList << pwdCmd;

    HcomCommand cdCmd;
    cdCmd.cmd = "cd";
    cdCmd.description.append("Changes present working directory");
    cdCmd.help.append(" Usage: cd [directory]");
    cdCmd.fnc = &HcomHandler::executeChangeDirectoryCommand;
    cdCmd.group = "File Commands";
    mCmdList << cdCmd;

    HcomCommand lsCmd;
    lsCmd.cmd = "ls";
    lsCmd.description.append("List files in current directory");
    lsCmd.help.append("Usage: ls [no arguments]");
    lsCmd.fnc = &HcomHandler::executeListFilesCommand;
    lsCmd.group = "File Commands";
    mCmdList << lsCmd;

    HcomCommand closeCmd;
    closeCmd.cmd = "close";
    closeCmd.description.append("Closes current model");
    closeCmd.help.append("Usage: close [no arguments]");
    closeCmd.fnc = &HcomHandler::executeCloseModelCommand;
    mCmdList << closeCmd;

    HcomCommand chtabCmd;
    chtabCmd.cmd = "chtab";
    chtabCmd.description.append("Changes current model tab");
    chtabCmd.help.append("Usage: chtab [index]");
    chtabCmd.fnc = &HcomHandler::executeChangeTabCommand;
    chtabCmd.group = "Model Commands";
    mCmdList << chtabCmd;

    HcomCommand adcoCmd;
    adcoCmd.cmd = "adco";
    adcoCmd.description.append("Adds a new component to current model");
    adcoCmd.help.append(" Usage: adco [typename name -flag value]");
    adcoCmd.fnc = &HcomHandler::executeAddComponentCommand;
    adcoCmd.group = "Model Commands";
    mCmdList << adcoCmd;

    HcomCommand cocoCmd;
    cocoCmd.cmd = "coco";
    cocoCmd.description.append("Connect components in current model");
    cocoCmd.help.append("Usage: coco [comp1 port1 comp2 port2]");
    cocoCmd.fnc = &HcomHandler::executeConnectCommand;
    cocoCmd.group = "Model Commands";
    mCmdList << cocoCmd;

    HcomCommand crmoCmd;
    crmoCmd.cmd = "crmo";
    crmoCmd.description.append("Creates a new model");
    crmoCmd.help.append("Usage: crmo [no arguments]");
    crmoCmd.fnc = &HcomHandler::executeCreateModelCommand;
    crmoCmd.group = "Model Commands";
    mCmdList << crmoCmd;

    HcomCommand fmuCmd;
    fmuCmd.cmd = "fmu";
    fmuCmd.description.append("Exports current model to Functional Mockup Unit (FMU)");
    fmuCmd.help.append("Usage: fmu [path]");
    fmuCmd.fnc = &HcomHandler::executeExportToFMUCommand;
    mCmdList << fmuCmd;

    HcomCommand chtsCmd;
    chtsCmd.cmd = "chts";
    chtsCmd.description.append("Change time step of sub-component");
    chtsCmd.help.append("Usage: chts [comp timestep]");
    chtsCmd.fnc = &HcomHandler::executeChangeTimestepCommand;
    chtsCmd.group = "Simulation Commands";
    mCmdList << chtsCmd;

    HcomCommand intsCmd;
    intsCmd.cmd = "ints";
    intsCmd.description.append("Inherit time step of sub-component from system time step");
    intsCmd.help.append("Usage: ints [comp]");
    intsCmd.fnc = &HcomHandler::executeInheritTimestepCommand;
    intsCmd.group = "Simulation Commands";
    mCmdList << intsCmd;

    HcomCommand bodeCmd;
    bodeCmd.cmd = "bode";
    bodeCmd.description.append("Creates a bode plot from specified curves");
    bodeCmd.help.append("Usage: bode [invar outvar maxfreq]");
    bodeCmd.fnc = &HcomHandler::executeBodeCommand;
    bodeCmd.group = "Plot Commands";
    mCmdList << bodeCmd;

    HcomCommand absCmd;
    absCmd.cmd = "abs";
    absCmd.description.append("Calculates absolute value of scalar of variable");
    absCmd.help.append("Usage: abs [var]");
    absCmd.fnc = &HcomHandler::executeAbsCommand;
    absCmd.group = "Variable Commands";
    mCmdList << absCmd;

    HcomCommand optCmd;
    optCmd.cmd = "opt";
    optCmd.description.append("Initialize an optimization");
    optCmd.help.append("Usage: opt [algorithm partype parnum parmin parmax -flags]");
    optCmd.help.append("\nAlgorithms:   Flags:");
    optCmd.help.append("\ncomplex       alpha");
    optCmd.fnc = &HcomHandler::executeOptimizationCommand;
    mCmdList << optCmd;

    HcomCommand callCmd;
    callCmd.cmd = "call";
    callCmd.description.append("Calls a pre-defined function");
    callCmd.help.append("Usage: call [funcname]");
    callCmd.fnc = &HcomHandler::executeCallFunctionCommand;
    mCmdList << callCmd;

    HcomCommand echoCmd;
    echoCmd.cmd = "echo";
    echoCmd.description.append("Sets terminal output on or off");
    echoCmd.help.append("Usage: echo [on/off]");
    echoCmd.fnc = &HcomHandler::executeEchoCommand;
    mCmdList << echoCmd;

    HcomCommand editCmd;
    editCmd.cmd = "edit";
    editCmd.description.append("Open file in external editor");
    editCmd.help.append("Usage: edit [filepath]");
    editCmd.fnc = &HcomHandler::executeEditCommand;
    editCmd.group = "File Commands";
    mCmdList << editCmd;

    HcomCommand lp1Cmd;
    lp1Cmd.cmd = "lp1";
    lp1Cmd.description.append("Applies low-pass filter of first degree to vector");
    lp1Cmd.help.append("Usage: lp1 [var]");
    lp1Cmd.fnc = &HcomHandler::executeLp1Command;
    lp1Cmd.group = "Variable Commands";
    mCmdList << lp1Cmd;
}

void HcomHandler::generateCommandsHelpText()
{
    QFile htmlFile(gDesktopHandler.getHelpPath()+"userHcomScripting.html");
    htmlFile.open(QFile::ReadOnly | QFile::Text);
    QString htmlString = htmlFile.readAll();
    htmlFile.close();

    QString commands="</p>";
    QStringList groups;
    for(int c=0; c<mCmdList.size(); ++c)
    {
        if(!groups.contains(mCmdList[c].group))
        {
            groups << mCmdList[c].group;
        }
    }
    groups.removeAll("");
    groups << "";
    for(int g=0; g<groups.size(); ++g)
    {
        if(groups[g].isEmpty())
        {

            commands.append("\n<h2><a class=\"anchor\" id=\"othercommands\"></a>Other Commands</h2>");
        }
        else
        {
            commands.append("\n<h2><a class=\"anchor\" id=\""+groups[g].toLower().replace(" ","")+"\"></a>"+groups[g]+"</h2>");
        }
        for(int c=0; c<mCmdList.size(); ++c)
        {
            if(mCmdList[c].group == groups[g])
            {
                commands.append("\n<h3><a class=\"anchor\" id=\""+mCmdList[c].cmd+"\"></a>"+mCmdList[c].cmd+"</h3>");
                commands.append("\n<p>"+mCmdList[c].description.replace("\n", "<br>")+"<br>"+mCmdList[c].help.replace("\n", "<br>")+"</p>");
            }
        }
    }

    htmlFile.open(QFile::WriteOnly | QFile::Text | QFile::Truncate);
    htmlString.replace("<<<commands>>>", commands);
    htmlFile.write(htmlString.toUtf8());
    htmlFile.close();
}


//! @brief Returns a list of available commands
QStringList HcomHandler::getCommands() const
{
    QStringList ret;
    for(int i=0; i<mCmdList.size(); ++i)
    {
        ret.append(mCmdList.at(i).cmd);
    }
    return ret;
}


//! @brief Returns a map with all local variables and their values
QMap<QString, double> HcomHandler::getLocalVariables() const
{
    return mLocalVars;
}


//! @brief Returns a map with all local functions and pointers to them
QMap<QString, SymHop::Function> HcomHandler::getLocalFunctionPointers() const
{
    return mLocalFunctionPtrs;
}

double HcomHandler::getOptimizationObjectiveValue(int idx)
{
    if(idx<0 || idx > mOptObjectives.size()-1)
    {
        return 0;
    }
    return mOptObjectives[idx];
}


//! @brief Executes a HCOM command
//! @param cmd The command entered by user
void HcomHandler::executeCommand(QString cmd)
{
    cmd = cmd.simplified();

    QString majorCmd = cmd.split(" ").first();
    QString subCmd;
    if(cmd.split(" ").size() == 1)
    {
        subCmd = QString();
    }
    else
    {
        subCmd = cmd.right(cmd.size()-majorCmd.size()-1);
    }

    int idx = -1;
    for(int i=0; i<mCmdList.size(); ++i)
    {
        if(mCmdList[i].cmd == majorCmd) { idx = i; }
    }

//    ProjectTab *pCurrentTab = gpMainWindow->mpProjectTabs->getCurrentTab();
//    SystemContainer *pCurrentSystem;
//    if(pCurrentTab)
//    {
//        pCurrentSystem = pCurrentTab->getTopLevelSystem();
//    }
//    else
//    {
//        pCurrentSystem = 0;
//    }

    if(idx<0)
    {
        if(evaluateArithmeticExpression(cmd)) { return; }
        mpConsole->printErrorMessage("Unrecognized command: " + majorCmd, "", false);
    }
    else
    {
        mCmdList[idx].runCommand(subCmd, this);
    }
}


//! @brief Execute function for "exit" command
void HcomHandler::executeExitCommand(const QString /*cmd*/)
{
    gpMainWindow->close();
}


//! @brief Execute function for "sim" command
void HcomHandler::executeSimulateCommand(const QString /*cmd*/)
{
    ProjectTab *pCurrentTab = gpMainWindow->mpProjectTabs->getCurrentTab();
    if(pCurrentTab)
    {
        pCurrentTab->simulate_blocking();
    }
}


//! @brief Execute function for "chpv" command
void HcomHandler::executePlotCommand(const QString cmd)
{
    changePlotVariables(cmd, -1);
}


//! @brief Execute function for "chpvl" command
void HcomHandler::executePlotLeftAxisCommand(const QString cmd)
{
    changePlotVariables(cmd, 0);
}


//! @brief Execute function for "chpvr" command
void HcomHandler::executePlotRightAxisCommand(const QString cmd)
{
    changePlotVariables(cmd, 1);
}


//! @brief Execute function for "disp" command
void HcomHandler::executeDisplayParameterCommand(const QString cmd)
{
    QStringList parameters;
    getParameters(cmd, parameters);

    int longestParameterName=0;
    for(int p=0; p<parameters.size(); ++p)
    {
        if(parameters.at(p).size() > longestParameterName)
        {
            longestParameterName = parameters.at(p).size();
        }
    }


    for(int p=0; p<parameters.size(); ++p)
    {
        QString output = parameters[p];
        int space = longestParameterName-parameters[p].size()+3;
        for(int s=0; s<space; ++s)
        {
            output.append(" ");
        }
        output.append(getParameterValue(parameters[p]));
        mpConsole->print(output);
    }
}


//! @brief Execute function for "chpa" command
void HcomHandler::executeChangeParameterCommand(const QString cmd)
{
    QStringList splitCmd;
    bool withinQuotations = false;
    int start=0;
    for(int i=0; i<cmd.size(); ++i)
    {
        if(cmd[i] == '\"')
        {
            withinQuotations = !withinQuotations;
        }
        if(cmd[i] == ' ' && !withinQuotations)
        {
            splitCmd.append(cmd.mid(start, i-start));
            start = i+1;
        }
    }
    splitCmd.append(cmd.right(cmd.size()-start));
//    for(int i=0; i<splitCmd.size(); ++i)
//    {
//        splitCmd[i].remove("\"");
//    }

    if(splitCmd.size() == 2)
    {
        ProjectTab *pCurrentTab = gpMainWindow->mpProjectTabs->getCurrentTab();
        if(!pCurrentTab) { return; }
        SystemContainer *pSystem = pCurrentTab->getTopLevelSystem();
        if(!pSystem) { return; }

        QStringList parameterNames;
        getParameters(splitCmd[0], parameterNames);
        QString newValue = splitCmd[1];

        for(int p=0; p<parameterNames.size(); ++p)
        {
            if(pSystem->getParameterNames().contains(parameterNames[p]))
            {
                pSystem->setParameterValue(parameterNames[p], newValue);
            }
            else
            {
                parameterNames[p].remove("\"");
                QStringList splitFirstCmd = parameterNames[p].split(".");
                if(splitFirstCmd.size() == 2)
                {
                    QList<ModelObject*> components;
                    getComponents(splitFirstCmd[0], components);
                    for(int c=0; c<components.size(); ++c)
                    {
                        QStringList parameters;
                        getParameters(splitFirstCmd[1], components[c], parameters);
                        for(int p=0; p<parameters.size(); ++p)
                        {
                            bool ok;
                            VariableType varType;
                            components[c]->setParameterValue(parameters[p], evaluateExpression(newValue, &varType, &ok));
                        }
                    }
                }
            }
        }
    }
    else
    {
        mpConsole->printErrorMessage("Wrong number of arguments.","",false);
    }
}


//! @brief Execute function for "chss" command
void HcomHandler::executeChangeSimulationSettingsCommand(const QString cmd)
{
    QString temp = cmd;
    temp.remove("\"");
    QStringList splitCmd = temp.split(" ");
    if(splitCmd.size() == 3 || splitCmd.size() == 4)
    {
        bool allOk=true;
        bool ok;
        double startT = getNumber(splitCmd[0], &ok);
        if(!ok) { allOk=false; }
        double timeStep = getNumber(splitCmd[1], &ok);
        if(!ok) { allOk=false; }
        double stopT = getNumber(splitCmd[2], &ok);
        if(!ok) { allOk=false; }

        int samples;
        if(splitCmd.size() == 4)
        {
            samples = splitCmd[3].toInt(&ok);
            if(!ok) { allOk=false; }
        }

        if(allOk)
        {
            gpMainWindow->setStartTimeInToolBar(startT);
            gpMainWindow->setTimeStepInToolBar(timeStep);
            gpMainWindow->setStopTimeInToolBar(stopT);
            if(splitCmd.size() == 4)
            {
                ProjectTab *pCurrentTab = gpMainWindow->mpProjectTabs->getCurrentTab();
                if(!pCurrentTab) { return; }
                SystemContainer *pCurrentSystem = pCurrentTab->getTopLevelSystem();
                if(!pCurrentSystem) { return; }
                pCurrentSystem->setNumberOfLogSamples(samples);
            }
        }
        else
        {
            mpConsole->printErrorMessage("Failed to apply simulation settings.","",false);
        }
    }
    else
    {
        mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
    }
}


//! @brief Execute function for "help" command
void HcomHandler::executeHelpCommand(const QString cmd)
{
    QString temp=cmd;
    temp.remove(" ");
    if(temp.isEmpty())
    {
        mpConsole->print("-------------------------------------------------------------------------");
        mpConsole->print(" Hopsan HCOM Terminal v0.1");
        QString commands;
        int n=0;
        QStringList groups;
        for(int c=0; c<mCmdList.size(); ++c)
        {
            n=max(mCmdList[c].cmd.size(), n);
            if(!groups.contains(mCmdList[c].group))
            {
                groups << mCmdList[c].group;
            }
        }
        n=n+4;
        groups.removeAll("");
        groups << "";
        for(int g=0; g<groups.size(); ++g)
        {
            if(groups[g].isEmpty())
            {
                commands.append("\n Other commands:\n\n");
            }
            else
            {
                commands.append("\n "+groups[g]+":\n\n");
            }
            for(int c=0; c<mCmdList.size(); ++c)
            {
                if(mCmdList[c].group == groups[g])
                {
                    commands.append("   ");
                    commands.append(mCmdList[c].cmd);
                    for(int i=0; i<n-mCmdList[c].cmd.size(); ++i)
                    {
                        commands.append(" ");
                    }
                    commands.append(mCmdList[c].description);
                    commands.append("\n");
                }
            }
        }

        //Print help descriptions for local functions
        commands.append("\n Custom Functions:\n\n");
        QMapIterator<QString, QString> funcIt(mLocalFunctionDescriptions);
        while(funcIt.hasNext())
        {
            funcIt.next();
            commands.append("   "+funcIt.key()+"()");
            for(int i=0; i<n-funcIt.key().size()-2; ++i)
            {
                commands.append(" ");
            }
            commands.append(funcIt.value()+"\n");
        }

        mpConsole->print(commands);
        mpConsole->print(" Type: \"help [command]\" for more information about a specific command.");
        mpConsole->print("-------------------------------------------------------------------------");
    }
    else
    {
        int idx = -1;
        for(int i=0; i<mCmdList.size(); ++i)
        {
            if(mCmdList[i].cmd == temp) { idx = i; }
        }

        if(idx < 0)
        {
            mpConsole->printErrorMessage("No help available for this command.","",false);
        }
        else
        {
            int length=max(mCmdList[idx].description.size(), mCmdList[idx].help.size())+2;
            QString delimiterLine;
            for(int i=0; i<length; ++i)
            {
                delimiterLine.append("-");
            }
            QString descLine = mCmdList[idx].description;
            descLine.prepend(" ");
            QString helpLine = mCmdList[idx].help;
            helpLine.prepend(" ");
            helpLine.replace("\n", "\n ");
            mpConsole->print(delimiterLine+"\n"+descLine+"\n"+helpLine+"\n"+delimiterLine);
        }
    }
}


//! @brief Execute function for "exec" command
void HcomHandler::executeRunScriptCommand(const QString cmd)
{
    QStringList splitCmd = splitWithRespectToQuotations(cmd, ' ');
    if(splitCmd.size() != 1)
    {
        mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
        return;
    }

    QString path = splitCmd[0];
    if(!path.contains("/"))
    {
        path.prepend("./");
    }
    QString dir = path.left(path.lastIndexOf("/"));
    dir = getDirectory(dir);
    path = dir+path.right(path.size()-path.lastIndexOf("/"));

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        mpConsole->printErrorMessage("Unable to read file.","",false);
        return;
    }

    QString code;
    QTextStream t(&file);

    code = t.readAll();

    for(int i=0; i<splitCmd.size()-1; ++i)  //Replace arguments with their values
    {
        QString str = "$"+QString::number(i+1);
        code.replace(str, splitCmd[i+1]);
    }

    QStringList lines = code.split("\n");
    lines.removeAll("");
    bool *abort = new bool;
    *abort = false;
    QString gotoLabel = runScriptCommands(lines, abort);
    if(*abort)
    {
        delete abort;
        return;
    }
    delete abort;
    while(!gotoLabel.isEmpty())
    {
        if(gotoLabel == "%%%%%EOF")
        {
            break;
        }
        for(int l=0; l<lines.size(); ++l)
        {
            if(lines[l].startsWith("&"+gotoLabel))
            {
                QStringList commands = lines.mid(l, lines.size()-l);
                bool *abort = new bool;
                gotoLabel = runScriptCommands(commands, abort);
            }
        }
    }

    file.close();
}


//! @brief Execute function for "wrhi" command
void HcomHandler::executeWriteHistoryToFileCommand(const QString cmd)
{
    QStringList split = splitWithRespectToQuotations(cmd, ' ');
    if(split.size() != 1)
    {
        mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
        return;
    }

    QString path = cmd;
    if(!path.contains("/"))
    {
        path.prepend("./");
    }
    QString dir = path.left(path.lastIndexOf("/"));
    dir = getDirectory(dir);
    path = dir+path.right(path.size()-path.lastIndexOf("/"));


    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        mpConsole->printErrorMessage("Unable to write to file.","",false);
        return;
    }

    QTextStream t(&file);
    for(int h=mpConsole->mHistory.size()-1; h>-1; --h)
    {
        t << mpConsole->mHistory[h] << "\n";
    }
    file.close();
}


//! @brief Execute function for "print" command
void HcomHandler::executePrintCommand(const QString /*cmd*/)
{
    mpConsole->printErrorMessage("Function not yet implemented.","",false);
}


//! @brief Execute function for "chpw" command
void HcomHandler::executeChangePlotWindowCommand(const QString cmd)
{
    QStringList split = splitWithRespectToQuotations(cmd, ' ');
    if(split.size() != 1)
    {
        mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
        return;
    }

    mCurrentPlotWindowName = cmd;
}


//! @brief Execute function for "dipw" command
void HcomHandler::executeDisplayPlotWindowCommand(const QString /*cmd*/)
{
    mpConsole->print(mCurrentPlotWindowName);
}


//! @brief Execute function for "disp" command
void HcomHandler::executeDisplayVariablesCommand(const QString cmd)
{
    QStringList split = splitWithRespectToQuotations(cmd, ' ');
    if(split.size() != 1)
    {
        mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
        return;
    }

    QStringList output;
    getVariables(cmd, output);

    for(int o=0; o<output.size(); ++o)
    {
        mpConsole->print(output[o]);
    }
}


//! @brief Execute function for "peek" command
void HcomHandler::executePeekCommand(const QString cmd)
{
    QStringList split = cmd.split(" ");
    if(split.size() != 2)
    {
        mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
        return;
    }

    QString variable = split.first();
    bool ok;
    int id = getNumber(split.last(), &ok);
    if(!ok)
    {
        mpConsole->printErrorMessage("Illegal value.","",false);
        return;
    }

    SharedLogVariableDataPtrT pData = getVariablePtr(variable);
    if(pData)
    {
        QString err;
        double r = pData->peekData(id, err);
        if (err.isEmpty())
        {
            returnScalar(r);
        }
        else
        {
            mpConsole->printErrorMessage(err,"",false);
        }
    }
    else
    {
        mpConsole->printErrorMessage("Data variable not found","",false);
    }
}


//! @brief Execute function for "poke" command
void HcomHandler::executePokeCommand(const QString cmd)
{
    QStringList split = cmd.split(" ");
    if(split.size() != 3)
    {
        mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
        return;
    }

    QString variable = split.first();
    bool ok1, ok2;
    int id = getNumber(split[1], &ok1);
    double value = getNumber(split.last(), &ok2);
    if(!ok1 || !ok2)
    {
        mpConsole->printErrorMessage("Illegal value.","",false);
        return;
    }

    SharedLogVariableDataPtrT pData = getVariablePtr(variable);
    if(pData)
    {
        QString err;
        double r = pData->pokeData(id, value, err);
        if (err.isEmpty())
        {
            mpConsole->print(QString::number(r));
        }
        else
        {
            mpConsole->printErrorMessage(err,"",false);
        }
    }
    else
    {
        mpConsole->printErrorMessage("Data variable not found.","",false);
    }
    return;
}


//! @brief Execute function for "alias" command
void HcomHandler::executeDefineAliasCommand(const QString cmd)
{
    if(splitWithRespectToQuotations(cmd, ' ').size() != 2)
    {
        mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
        return;
    }

    QString variable = splitWithRespectToQuotations(cmd, ' ')[0];
    toShortDataNames(variable);
    variable.remove("\"");
    QString alias = splitWithRespectToQuotations(cmd, ' ')[1];

    SharedLogVariableDataPtrT pVariable = getVariablePtr(variable);

    if(!pVariable || !gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem()->getLogDataHandler()->definePlotAlias(alias, pVariable->getFullVariableName()))
    {
        mpConsole->printErrorMessage("Failed to assign variable alias.","",false);
    }

    gpMainWindow->mpPlotWidget->mpPlotVariableTree->updateList();

    return;
}


//! @brief Execute function for "set" command
void HcomHandler::executeSetCommand(const QString cmd)
{
    QStringList splitCmd = cmd.split(" ");
    if(splitCmd.size() != 2)
    {
        mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
        return;
    }
    QString pref = splitCmd[0];
    QString value = splitCmd[1];

    if(pref == "multicore")
    {
        if(value != "on" && value != "off")
        {
            mpConsole->printErrorMessage("Unknown value.","",false);
            return;
        }
        gConfig.setUseMultiCore(value=="on");
    }
    else if(pref == "threads")
    {
        bool ok;
        int nThreads = value.toInt(&ok);
        if(!ok)
        {
            mpConsole->printErrorMessage("Unknown value.","",false);
            return;
        }
        gConfig.setNumberOfThreads(nThreads);
    }
    else if(pref == "cachetodisk")
    {
        if(value != "on" && value != "off")
        {
            mpConsole->printErrorMessage("Unknown value.","",false);
        }
        gConfig.setCacheLogData(value=="on");
    }
    else if(pref == "generationlimit")
    {
        bool ok;
        int limit = value.toInt(&ok);
        if(!ok)
        {
            mpConsole->printErrorMessage("Unknown value.","",false);
            return;
        }
        gConfig.setGenerationLimit(limit);
    }
    else if(pref == "samples")
    {
        bool ok;
        int samples = value.toInt(&ok);
        if(!ok)
        {
            mpConsole->printErrorMessage("Unknown value.","",false);
            return;
        }
        gpMainWindow->mpProjectTabs->getCurrentContainer()->setNumberOfLogSamples(samples);
    }
    else
    {
        mpConsole->printErrorMessage("Unknown command.","",false);
    }
}


//! @brief Execute function for "sapl" command
void HcomHandler::executeSaveToPloCommand(const QString cmd)
{
    QStringList split = cmd.split(" ");
    if(split.size() < 2)
    {
        mpConsole->printErrorMessage("Too few arguments.", "", false);
        return;
    }
    QString path = split.first();

    if(!path.contains("/"))
    {
        path.prepend("./");
    }
    QString dir = path.left(path.lastIndexOf("/"));
    dir = getDirectory(dir);
    path = dir+path.right(path.size()-path.lastIndexOf("/"));


    QString temp = cmd.right(cmd.size()-path.size()-1);

    QStringList splitCmdMajor;
    bool withinQuotations = false;
    int start=0;
    for(int i=0; i<temp.size(); ++i)
    {
        if(temp[i] == '\"')
        {
            withinQuotations = !withinQuotations;
        }
        if(temp[i] == ' ' && !withinQuotations)
        {
            splitCmdMajor.append(temp.mid(start, i-start));
            start = i+1;
        }
    }
    splitCmdMajor.append(temp.right(temp.size()-start));
    QStringList allVariables;
    for(int i=0; i<splitCmdMajor.size(); ++i)
    {
        splitCmdMajor[i].remove("\"");
        QStringList variables;
        getVariables(splitCmdMajor[i], variables);
        for(int v=0; v<variables.size(); ++v)
        {
            variables[v].replace(".", "#");
            variables[v].remove("\"");

            if(variables[v].endsWith("#x"))
            {
                variables[v].chop(2);
                variables[v].append("#Position");
            }
            else if(variables[v].endsWith("#v"))
            {
                variables[v].chop(2);
                variables[v].append("#Velocity");
            }
            else if(variables[v].endsWith("#f"))
            {
                variables[v].chop(2);
                variables[v].append("#Force");
            }
            else if(variables[v].endsWith("#p"))
            {
                variables[v].chop(2);
                variables[v].append("#Pressure");
            }
            else if(variables[v].endsWith("#q"))
            {
                variables[v].chop(2);
                variables[v].append("#Flow");
            }
            else if(variables[v].endsWith("#val"))
            {
                variables[v].chop(4);
                variables[v].append("#Value");
            }
            else if(variables[v].endsWith("#Zc"))
            {
                variables[v].chop(3);
                variables[v].append("#CharImpedance");
            }
            else if(variables[v].endsWith("#c"))
            {
                variables[v].chop(2);
                variables[v].append("#WaveVariable");
            }
            else if(variables[v].endsWith("#me"))
            {
                variables[v].chop(3);
                variables[v].append("#EquivalentMass");
            }
            else if(variables[v].endsWith("#Q"))
            {
                variables[v].chop(2);
                variables[v].append("#HeatFlow");
            }
            else if(variables[v].endsWith("#t"))
            {
                variables[v].chop(2);
                variables[v].append("#Temperature");
            }

        }
        allVariables.append(variables);
        //splitCmdMajor[i] = getVariablePtr(splitCmdMajor[i])->getFullVariableName();
    }

    gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem()->getLogDataHandler()->exportToPlo(path, allVariables);
}


//! @brief Execute function for "load" command
void HcomHandler::executeLoadModelCommand(const QString cmd)
{
    QString path = cmd;
    if(!path.contains("/"))
    {
        path.prepend("./");
    }
    QString dir = path.left(path.lastIndexOf("/"));
    dir = getDirectory(dir);
    path = dir+path.right(path.size()-path.lastIndexOf("/"));

    gpMainWindow->mpProjectTabs->loadModel(path);
}


//! @brief Execute function for "loadr" command
void HcomHandler::executeLoadRecentCommand(const QString /*cmd*/)
{
    gpMainWindow->mpProjectTabs->loadModel(gConfig.getRecentModels().first());
}


//! @brief Execute function for "pwd" command
void HcomHandler::executePwdCommand(const QString /*cmd*/)
{
    mpConsole->print(mPwd);
}


//! @brief Execute function for "cd" command
void HcomHandler::executeChangeDirectoryCommand(const QString cmd)
{
    if(getNumberOfArguments(cmd) != 1)
    {
        mpConsole->printErrorMessage("Wrong number of arguments", "", false);
        return;
    }
    mPwd = QDir().cleanPath(mPwd+"/"+cmd);
    mpConsole->print(mPwd);
}


//! @brief Execute function for "ls" command
void HcomHandler::executeListFilesCommand(const QString cmd)
{
    if(getNumberOfArguments(cmd) != 0)
    {
        mpConsole->printErrorMessage("Wrong number of arguments", "", false);
        return;
    }

    QStringList contents;
    if(!cmd.isEmpty())
    {
        contents = QDir(mPwd).entryList(QStringList() << "*");
    }
    else
    {
        contents = QDir(mPwd).entryList(QStringList() << cmd);
    }
    for(int c=0; c<contents.size(); ++c)
    {
        mpConsole->print(contents[c]);
    }
}


//! @brief Execute function for "close" command
void HcomHandler::executeCloseModelCommand(const QString cmd)
{
    if(getNumberOfArguments(cmd) != 0)
    {
        mpConsole->printErrorMessage("Wrong number of arguments", "", false);
        return;
    }

    if(gpMainWindow->mpProjectTabs->count() > 0)
    {
        gpMainWindow->mpProjectTabs->closeProjectTab(gpMainWindow->mpProjectTabs->currentIndex());
    }
}


//! @brief Execute function for "chtab" command
void HcomHandler::executeChangeTabCommand(const QString cmd)
{
    if(getNumberOfArguments(cmd) != 1)
    {
        mpConsole->printErrorMessage("Wrong number of arguments", "", false);
        return;
    }

    gpMainWindow->mpProjectTabs->setCurrentIndex(cmd.toInt());
}


//! @brief Execute function for "adco" command
void HcomHandler::executeAddComponentCommand(const QString cmd)
{
    if(getNumberOfArguments(cmd) < 5)
    {
        mpConsole->printErrorMessage("Wrong number of arguments", "", false);
        return;
    }
    QStringList args = getArguments(cmd);

    QString typeName = args[0];
    QString name = args[1];
    args.removeFirst();
    args.removeFirst();

    double xPos;
    double yPos;
    double rot;

    if(!args.isEmpty())
    {
        if(args.first() == "-a")
        {
            //Absolute
            if(args.size() != 4)
            {
                mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
                return;
            }
            xPos = args[1].toDouble();
            yPos = args[2].toDouble();
            rot = args[3].toDouble();
        }
        else if(args.first() == "-e")
        {
            //East of
            if(args.size() != 4)
            {
                mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
                return;
            }
            QString otherName = args[1];
            Component *pOther = qobject_cast<Component*>(gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem()->getModelObject(otherName));
            if(!pOther)
            {
                mpConsole->printErrorMessage("Master component not found.");
                return;
            }
            double offset = args[2].toDouble();
            xPos = pOther->getCenterPos().x()+offset;
            yPos = pOther->getCenterPos().y();
            rot = args[3].toDouble();
        }
        else if(args.first() == "-w")
        {
            //West of
            if(args.size() != 4)
            {
                mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
                return;
            }
            QString otherName = args[1];
            Component *pOther = qobject_cast<Component*>(gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem()->getModelObject(otherName));
            if(!pOther)
            {
                mpConsole->printErrorMessage("Master component not found.");
                return;
            }
            double offset = args[2].toDouble();
            xPos = pOther->getCenterPos().x()-offset;
            yPos = pOther->getCenterPos().y();
            rot = args[3].toDouble();
        }
        else if(args.first() == "-n")
        {
            //North of
            if(args.size() != 4)
            {
                mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
                return;
            }
            QString otherName = args[1];
            Component *pOther = qobject_cast<Component*>(gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem()->getModelObject(otherName));
            if(!pOther)
            {
                mpConsole->printErrorMessage("Master component not found.");
                return;
            }
            double offset = args[2].toDouble();
            xPos = pOther->getCenterPos().x();
            yPos = pOther->getCenterPos().y()-offset;
            rot = args[3].toDouble();
        }
        else if(args.first() == "-s")
        {
            //South of
            if(args.size() != 4)
            {
                mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
                return;
            }
            QString otherName = args[1];
            Component *pOther = qobject_cast<Component*>(gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem()->getModelObject(otherName));
            if(!pOther)
            {
                mpConsole->printErrorMessage("Master component not found.");
                return;
            }
            double offset = args[2].toDouble();
            xPos = pOther->getCenterPos().x();
            yPos = pOther->getCenterPos().y()+offset;
            rot = args[3].toDouble();
        }
    }

    QPointF pos = QPointF(xPos, yPos);
    Component *pObj = qobject_cast<Component*>(gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem()->addModelObject(typeName, pos, rot));
    if(!pObj)
    {
        mpConsole->printErrorMessage("Failed to add new component. Incorrect typename?", "", false);
    }
    else
    {
        mpConsole->print("Added "+typeName+" to current model.");
        gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem()->renameModelObject(pObj->getName(), name);
    }
}


//! @brief Execute function for "coco" command
void HcomHandler::executeConnectCommand(const QString cmd)
{
    QStringList args = getArguments(cmd);
    if(args.size() != 4)
    {
        mpConsole->printErrorMessage("Wrong number of arguments", "", false);
        return;
    }

    Port *pPort1 = gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem()->getModelObject(args[0])->getPort(args[1]);
    Port *pPort2 = gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem()->getModelObject(args[2])->getPort(args[3]);

    Connector *pConn = gpMainWindow->mpProjectTabs->getCurrentContainer()->createConnector(pPort1, pPort2);

    if (pConn != 0)
    {
        QVector<QPointF> pointVector;
        pointVector.append(pPort1->pos());
        pointVector.append(pPort2->pos());

        QStringList geometryList;
        geometryList.append("diagonal");

        pConn->setPointsAndGeometries(pointVector, geometryList);
        pConn->refreshConnectorAppearance();
    }
}


//! @brief Execute function for "crmo" command
void HcomHandler::executeCreateModelCommand(const QString cmd)
{
    if(getNumberOfArguments(cmd) != 0)
    {
        mpConsole->printErrorMessage("Wrong number of arguments", "", false);
        return;
    }
    gpMainWindow->mpProjectTabs->addNewProjectTab();
}


//! @brief Execute function for "fmu" command
void HcomHandler::executeExportToFMUCommand(const QString cmd)
{
    if(getNumberOfArguments(cmd) != 1)
    {
        mpConsole->printErrorMessage("Wrong number of arguments.");
    }

    gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem()->exportToFMU(getArgument(cmd, 0));
}


//! @brief Execute function for "chts" command
void HcomHandler::executeChangeTimestepCommand(const QString cmd)
{
    QStringList split = splitWithRespectToQuotations(cmd, ' ');
    if(split.size() != 2)
    {
        mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
        return;
    }
    QString component = split[0];
    QString timeStep = split[1];

    VariableType retType;
    bool isNumber;
    double value = evaluateExpression(split[1], &retType, &isNumber).toDouble();
    if(!isNumber)
    {
        mpConsole->printErrorMessage("Second argument is not a number.", "", false);
    }
    else if(!gpMainWindow->mpProjectTabs->getCurrentContainer()->hasModelObject(component))
    {
        mpConsole->printErrorMessage("Component not found.", "", false);
    }
    else
    {
        gpMainWindow->mpProjectTabs->getCurrentContainer()->getCoreSystemAccessPtr()->setDesiredTimeStep(component, value);
        //gpMainWindow->mpProjectTabs->getCurrentContainer()->getCoreSystemAccessPtr()->setInheritTimeStep(false);
        mpConsole->print("Setting time step of "+component+" to "+QString::number(value));
    }
}


//! @brief Execute function for "ihts" command
void HcomHandler::executeInheritTimestepCommand(const QString cmd)
{
    QStringList split = splitWithRespectToQuotations(cmd, ' ');
    if(split.size() != 1)
    {
        mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
        return;
    }
    QString component = split[0];

    if(!gpMainWindow->mpProjectTabs->getCurrentContainer()->hasModelObject(component))
    {
        mpConsole->printErrorMessage("Component not found.", "", false);
    }
    else
    {
        gpMainWindow->mpProjectTabs->getCurrentContainer()->getCoreSystemAccessPtr()->setInheritTimeStep(component, true);
        mpConsole->print("Setting time step of "+component+" to inherited.");
    }
}


//! @brief Execute function for "bode" command
void HcomHandler::executeBodeCommand(const QString cmd)
{
    int nArgs = getNumberOfArguments(cmd);
    if(nArgs < 2 || nArgs > 4)
    {
        mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
        return;
    }

    QString var1 = getArgument(cmd,0);
    QString var2 = getArgument(cmd,1);
    SharedLogVariableDataPtrT pData1 = getVariablePtr(var1);
    SharedLogVariableDataPtrT pData2 = getVariablePtr(var2);
    if(!pData1 || !pData2)
    {
        mpConsole->printErrorMessage("Data variable not found.", "", false);
        return;
    }
    int fMax = 500;
    if(nArgs > 2)
    {
        fMax = getArgument(cmd,2).toInt();
    }

    gpPlotHandler->createNewPlotWindowOrGetCurrentOne("Bode plot")->closeAllTabs();
    gpPlotHandler->createNewPlotWindowOrGetCurrentOne("Bode plot")->addPlotTab();
    gpPlotHandler->createNewPlotWindowOrGetCurrentOne("Bode plot")->createBodePlot(pData1, pData2, fMax);
}


//! @brief Execute function for "abs" command
void HcomHandler::executeAbsCommand(const QString cmd)
{
    if(getNumberOfArguments(cmd) != 1)
    {
        mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
        return;
    }
    QString varName = getArgument(cmd,0);

    SharedLogVariableDataPtrT var = getVariablePtr(varName);
    if(var)
    {
        var.data()->absData();
    }
    else
    {
        bool ok;
        double retval = fabs(getNumber(varName, &ok));
        if(ok) returnScalar(retval);
        else
        {
            mpConsole->printErrorMessage("Variable not found.", "", false);
        }
    }
}


//! @brief Execute function for "opt" command
void HcomHandler::executeOptimizationCommand(const QString cmd)
{
    QStringList split = getArguments(cmd);
//    if(split.size() == 1 && split[0] == "undo")
//    {
//        if(mOptAlgorithm == Uninitialized)
//        {
//            mpConsole->printErrorMessage("Optimization not initialized.", "", false);
//            return;
//        }
//        if(mOptAlgorithm != Complex)
//        {
//            mpConsole->printErrorMessage("Only available for complex algorithm.", "", false);
//            return;
//        }

//        mOptParameters = mOptOldParameters;
//        return;
//    }
    if(split.size() == 1 && split[0] == "worst")
    {
        if(mOptAlgorithm == Uninitialized)
        {
            mpConsole->printErrorMessage("Optimization not initialized.", "", false);
            return;
        }
        if(mOptAlgorithm != Complex)
        {
            mpConsole->printErrorMessage("Only available for complex algorithm.", "", false);
            return;
        }

        returnScalar(mOptLastWorstId);
        return;
    }
    if(split.size() == 1 && split[0] == "best")
    {
        if(mOptAlgorithm == Uninitialized)
        {
            mpConsole->printErrorMessage("Optimization not initialized.", "", false);
            return;
        }
        if(mOptAlgorithm != Complex)
        {
            mpConsole->printErrorMessage("Only available for complex algorithm.", "", false);
            return;
        }

        returnScalar(mOptBestId);
        return;
    }
    if(split.size() == 3 && split[0] == "setobj")
    {
        bool ok;
        int nPoint = getNumber(split[1], &ok);
        if(!ok)
        {
            mpConsole->printErrorMessage("Argument number 2 must be a number.");
            return;
        }
        if(nPoint < 0 || nPoint > mOptObjectives.size()-1)
        {
            mpConsole->printErrorMessage("Index out of range.");
            return;
        }

        double val = getNumber(split[2], &ok);
        if(!ok)
        {
            mpConsole->printErrorMessage("Argument number 3 must be a number.");
            return;
        }

        mOptObjectives[nPoint] = val;
        return;
    }
    if(split.size() == 4 && split[0] == "setparminmax")
    {
        bool ok;
        int nPoint = getNumber(split[1], &ok);
        if(!ok)
        {
            mpConsole->printErrorMessage("Argument number 2 must be a number.");
            return;
        }
        if(nPoint < 0 || nPoint > mOptParameters.size()-1)
        {
            mpConsole->printErrorMessage("Index out of range.");
            return;
        }


        double min = getNumber(split[2], &ok);
        if(!ok)
        {
            mpConsole->printErrorMessage("Argument number 3 must be a number.");
            return;
        }

        double max = getNumber(split[3], &ok);
        if(!ok)
        {
            mpConsole->printErrorMessage("Argument number 4 must be a number.");
            return;
        }

        mOptParMin[nPoint] = min;
        mOptParMax[nPoint] = max;
        return;
    }

    if(split.size() == 3 && split[0] == "init")
    {
        if(split[1] == "complex")
        {
            mOptAlgorithm = Complex;
        }
        else if(split[1] == "particleswarm")
        {
            mOptAlgorithm = ParticleSwarm;
        }
        else
        {
            mpConsole->printErrorMessage("Unknown algorithm. Only complex is supported.", "", false);
            return;
        }

        if(split[2] == "int")
        {
            mOptParameterType = Int;
        }
        else if(split[2] == "double")
        {
            mOptParameterType = Double;
        }
        else
        {
            mpConsole->printErrorMessage("Unknown data type. Only int and double are supported.");
            return;
        }



        //Everything is fine, initialize and run optimization

        bool ok;
        if(mOptAlgorithm == Complex)
        {
            mOptNumPoints = getNumber("npoints", &ok);
            mOptNumParameters = getNumber("nparams", &ok);
            mOptLastWorstId = -1;
            mOptWorstCounter = 0;
            mOptParameters.resize(mOptNumPoints);
            mOptParMin.resize(mOptNumPoints);
            mOptParMax.resize(mOptNumPoints);
            mOptMaxEvals = getNumber("maxevals", &ok);
            mOptAlpha = getNumber("alpha", &ok);
            mOptRfak = getNumber("rfak", &ok);
            mOptGamma = getNumber("gamma", &ok);
            mOptFuncTol = getNumber("functol", &ok);
            mOptParTol = getNumber("partol", &ok);
        }
        else if(mOptAlgorithm == ParticleSwarm)
        {
            mOptNumPoints = getNumber("npoints", &ok);
            mOptNumParameters = getNumber("nparams", &ok);
            mOptParameters.resize(mOptNumPoints);
            mOptVelocities.resize(mOptNumPoints);
            mOptBestKnowns.resize(mOptNumPoints);
            mOptBestPoint.resize(mOptNumParameters);
            mOptBestObjectives.resize(mOptNumPoints);
            mOptParMin.resize(mOptNumPoints);
            mOptParMax.resize(mOptNumPoints);
            mOptMaxEvals = getNumber("maxevals", &ok);
            mOptOmega = getNumber("omega", &ok);
            mOptC1 = getNumber("c1", &ok);
            mOptC2 = getNumber("c2", &ok);
            mOptFuncTol = getNumber("functol", &ok);
            mOptParTol = getNumber("partol", &ok);
        }

        return;
    }

    if(split.size() == 1 && split[0] == "run")
    {
        if(mOptAlgorithm == Complex)
        {
            optComplexInit();
            optComplexRun();
        }
        else if(mOptAlgorithm == ParticleSwarm)
        {
            optParticleInit();
            optParticleRun();
        }
    }
}


//! @brief Execute function for "call" command
void HcomHandler::executeCallFunctionCommand(const QString cmd)
{
    if(getNumberOfArguments(cmd) != 1)
    {
        mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
        return;
    }
    QString funcName = getArgument(cmd,0);

    if(!mFunctions.contains(funcName))
    {
        mpConsole->printErrorMessage("Undefined function.", "", false);
        return;
    }

    bool *abort = new bool;
    *abort = false;
    runScriptCommands(mFunctions.find(funcName).value(), abort);
    if(*abort)
    {
        mpConsole->print("Function aborted");
        returnScalar(-1);
        return;
    }
    returnScalar(0);
}


//! @brief Execute function for "echo" command
void HcomHandler::executeEchoCommand(const QString cmd)
{
    if(getNumberOfArguments(cmd) != 1)
    {
        mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
        return;
    }
    QString arg = getArgument(cmd,0);


    if(arg == "on")
    {
        mpConsole->setDontPrint(false);
    }
    else if(arg == "off")
    {
        mpConsole->setDontPrint(true);
    }
    else
    {
        mpConsole->printErrorMessage("Unknown argument, use \"on\" or \"off\"");
    }
}


//! @brief Execute function for "edit" command
void HcomHandler::executeEditCommand(const QString cmd)
{
    if(getNumberOfArguments(cmd) != 1)
    {
        mpConsole->printErrorMessage("Wrong number of arguments.", "", false);
        return;
    }

    QString path = getArgument(cmd,0);
    QDesktopServices::openUrl(QUrl(path));
}


void HcomHandler::executeLp1Command(const QString /*cmd*/)
{

}



//! @brief Changes plot variables on specified axes
//! @param cmd Command containing the plot variables
//! @param axis Axis specification (0=left, 1=right, -1=both, separeted by "-r")
void HcomHandler::changePlotVariables(const QString cmd, const int axis) const
{
    QStringList varNames = getArguments(cmd);

    if(axis == -1 || axis == 0)
    {
        removePlotCurves(QwtPlot::yLeft);
    }
    if(axis == -1 || axis == 1)
    {
        removePlotCurves(QwtPlot::yRight);
    }

    int axisId;
    if(axis == -1 || axis == 0)
    {
        axisId = QwtPlot::yLeft;
    }
    else
    {
        axisId = QwtPlot::yRight;
    }
    for(int s=0; s<varNames.size(); ++s)
    {
        if(axis == -1 && varNames[s] == "-r")
        {
            axisId = QwtPlot::yRight;
        }
        else
        {
            QStringList variables;
            getVariables(varNames[s], variables);
            for(int v=0; v<variables.size(); ++v)
            {
                addPlotCurve(variables[v], axisId);
            }
        }
    }
}


//! @brief Adds a plot curve to specified axis in current plot
//! @param cmd Name of variable
//! @param axis Axis to add curve to
void HcomHandler::addPlotCurve(QString cmd, const int axis) const
{
    cmd.remove("\"");

    SystemContainer *pCurrentSystem = gpMainWindow->mpProjectTabs->getCurrentTab()->getTopLevelSystem();
    if(!pCurrentSystem) { return; }

    SharedLogVariableDataPtrT pData = getVariablePtr(cmd);
    if(!pData)
    {
        mpConsole->printErrorMessage("Variable not found.","",false);
        return;
    }

    gpPlotHandler->plotDataToWindow(mCurrentPlotWindowName, pData, axis);
}


//! @brief Removes all curves at specified axis in current plot
//! @param axis Axis to remove from
void HcomHandler::removePlotCurves(const int axis) const
{
    PlotWindow *pPlotWindow = gpPlotHandler->getPlotWindow(mCurrentPlotWindowName);
    if(pPlotWindow)
    {
        pPlotWindow->getCurrentPlotTab()->removeAllCurvesOnAxis(axis);
    }
}


QString HcomHandler::evaluateExpression(QString expr, VariableType *returnType, bool *evalOk) const
{
    *evalOk = true;
    *returnType = Scalar;

    //expr.replace("==", "§§§§§");
    expr.replace("**", "%%%%%");

    //Remove parentheses around expression
    QString tempStr = expr.mid(1, expr.size()-2);
    if(expr.count("(") == 1 && expr.count(")") == 1 && expr.startsWith("(") && expr.endsWith(")"))
    {
        expr = tempStr;
    }
    else if(expr.count("(") > 1 && expr.count(")") > 1 && expr.startsWith("(") && expr.endsWith(")"))
    {

        if(tempStr.indexOf("(") < tempStr.indexOf(")"))
        {
            expr = tempStr;
        }
    }

    //Numerical value, return it
    bool ok;
    expr.toDouble(&ok);
    if(ok)
    {
        return expr;
    }

    //Pre-defined variable, return its value
    if(mLocalVars.contains(expr))
    {
        return QString::number(mLocalVars.find(expr).value());
    }

    //Optimization parameter
    if(expr.startsWith("par(") && expr.endsWith(")"))
    {
        QString nPointsStr = expr.section("(",1,1).section(",",0,0);
        QString nParStr = expr.section(",",1,1).section(")",0,0);
        bool ok1, ok2;
        int nPoint = getNumber(nPointsStr,&ok1);
        int nPar = getNumber(nParStr, &ok2);
        if(ok1 && ok2 && nPoint>=0 && nPoint < mOptParameters.size() && nPar>= 0 && nPar < mOptParameters[nPoint].size())
        {
            return QString::number(mOptParameters[nPoint][nPar]);
        }
    }

//    //Optimization objective
//    if(expr.startsWith("obj(") && expr.endsWith(")"))
//    {
//        QString nPointsStr = expr.section("(",1,1).section(")",0,0);

//        bool ok;
//        int nPoint = nPointsStr.toInt(&ok);
//        if(ok && nPoint>=0 && nPoint < mOptObjectives.size())
//        {
//            return QString::number(mOptObjectives[nPoint]);
//        }
//    }

    //Parameter name, return its value
    if(getParameterValue(expr) != "NaN")
    {
        return getParameterValue(expr);
    }

    //Data variable, return it
    QStringList variables;
    getVariables(expr,variables);
    if(!variables.isEmpty())
    {
        *returnType = DataVector;
        return variables.first();
    }

    //Vector functions
    LogDataHandler *pLogData=0;
    if(gpMainWindow->mpProjectTabs->count() > 0)
    {
        pLogData = gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem()->getLogDataHandler();
    }
    if(expr.startsWith("ddt(") && expr.endsWith(")"))
    {
        QString args = expr.mid(4, expr.size()-5);
        if(!args.contains(","))
        {
            *returnType = DataVector;
            SharedLogVariableDataPtrT var = getVariablePtr(args);
            return pLogData->diffVariables(var.data()->getFullVariableName(), "time");
        }
        else
        {
            *returnType = DataVector;
            SharedLogVariableDataPtrT var1 = getVariablePtr(args.section(",",0,0));
            SharedLogVariableDataPtrT var2 = getVariablePtr(args.section(",",1,1));
            return pLogData->diffVariables(var1.data()->getFullVariableName(), var2.data()->getFullVariableName());
        }
    }
    else if(expr.startsWith("lp1(") && expr.endsWith(")"))
    {
        QString args = expr.mid(4, expr.size()-5);
        if(args.count(",")==1)
        {
            double freq = args.section(",",1,1).toDouble();
            *returnType = DataVector;
            SharedLogVariableDataPtrT var = getVariablePtr(args.section(",",0,0));
            return pLogData->lowPassFilterVariable(var.data()->getFullVariableName(), "time", freq);
        }
        else if(args.count(",") == 2)
        {
            double freq = args.section(",",2,2).toDouble();
            *returnType = DataVector;
            SharedLogVariableDataPtrT var = getVariablePtr(args.section(",",0,0));
            SharedLogVariableDataPtrT timeVar = getVariablePtr(args.section(",",1,1));
            return pLogData->lowPassFilterVariable(var.data()->getFullVariableName(), timeVar.data()->getFullVariableName(), freq);
        }
    }
    else if(expr.startsWith("fft(") && expr.endsWith(")"))
    {
        QString args = expr.mid(4, expr.size()-5);
        if(args.count(",")==0)
        {
            *returnType = DataVector;
            SharedLogVariableDataPtrT var = getVariablePtr(args.section(",",0,0));
            return pLogData->fftVariable(var.data()->getFullVariableName(), "time", false);
        }
        if(args.count(",")==1)
        {
            QString arg2=args.section(",",1,1);
            bool power=false;
            QString timeVec = "time";
            if(arg2 == "true" || arg2 == "false")
            {
                power = (args.section(",",1,1) == "true");
            }
            else
            {
                timeVec = arg2;
            }
            *returnType = DataVector;
            SharedLogVariableDataPtrT var = getVariablePtr(args.section(",",0,0));
            return pLogData->fftVariable(var.data()->getFullVariableName(), timeVec, power);
        }
        else if(args.count(",") == 2)
        {
            bool power = (args.section(",",2,2) == "true");
            *returnType = DataVector;
            SharedLogVariableDataPtrT var = getVariablePtr(args.section(",",0,0));
            SharedLogVariableDataPtrT timeVar = getVariablePtr(args.section(",",1,1));
            return pLogData->fftVariable(var.data()->getFullVariableName(), timeVar.data()->getFullVariableName(), power);
        }
    }

    //Evaluate expression using SymHop
    SymHop::Expression symHopExpr = SymHop::Expression(expr);

    //Multiplication between data vector and scalar
    *returnType = DataVector;
    if(symHopExpr.getFactors().size() == 2 && pLogData)
    {
        SymHop::Expression f0 = symHopExpr.getFactors()[0];
        SymHop::Expression f1 = symHopExpr.getFactors()[1];
        if(!getVariablePtr(f0.toString()).isNull() && f1.isNumericalSymbol())
            return pLogData->mulVariableWithScalar(getVariablePtr(f0.toString()), f1.toDouble()).data()->getFullVariableName();
        else if(!getVariablePtr(f1.toString()).isNull() && f0.isNumericalSymbol())
            return pLogData->mulVariableWithScalar(getVariablePtr(f1.toString()), f0.toDouble()).data()->getFullVariableName();
        else if(!getVariablePtr(f0.toString()).isNull() && !getVariablePtr(f1.toString()).isNull())
            return pLogData->multVariables(getVariablePtr(f0.toString()), getVariablePtr(f1.toString())).data()->getFullVariableName();
    }
    if(symHopExpr.isPower() && pLogData)
    {
        SymHop::Expression b = *symHopExpr.getBase();
        SymHop::Expression p = *symHopExpr.getPower();
        if(!getVariablePtr(b.toString()).isNull() && p.toDouble() == 2.0)
            return pLogData->multVariables(getVariablePtr(b.toString()), getVariablePtr(b.toString())).data()->getFullVariableName();
    }
    if(symHopExpr.getFactors().size() == 1 && symHopExpr.getDivisors().size() == 1 && pLogData)
    {
        SymHop::Expression f = symHopExpr.getFactors()[0];
        SymHop::Expression d = symHopExpr.getDivisors()[0];
        if(!getVariablePtr(f.toString()).isNull() && d.isNumericalSymbol())
            return pLogData->divVariableWithScalar(getVariablePtr(f.toString()), d.toDouble()).data()->getFullVariableName();
        else if(!getVariablePtr(f.toString()).isNull() && !getVariablePtr(d.toString()).isNull())
            return pLogData->divVariables(getVariablePtr(f.toString()), getVariablePtr(d.toString())).data()->getFullVariableName();
    }
    if(symHopExpr.getTerms().size() == 2 && pLogData)
    {
        SymHop::Expression t0 = symHopExpr.getTerms()[0];
        SymHop::Expression t1 = symHopExpr.getTerms()[1];
        if(!getVariablePtr(t0.toString()).isNull() && t1.isNumericalSymbol())
            return pLogData->addVariableWithScalar(getVariablePtr(t0.toString()), t1.toDouble()).data()->getFullVariableName();
        else if(!getVariablePtr(t1.toString()).isNull() && t0.isNumericalSymbol())
            return pLogData->addVariableWithScalar(getVariablePtr(t1.toString()), t0.toDouble()).data()->getFullVariableName();
        else if(!getVariablePtr(t0.toString()).isNull() && !getVariablePtr(t1.toString()).isNull())
            return pLogData->addVariables(getVariablePtr(t0.toString()), getVariablePtr(t1.toString())).data()->getFullVariableName();

        if(t1.isNegative())
        {
            t1.changeSign();
            if(!getVariablePtr(t0.toString()).isNull() && t1.isNumericalSymbol())
                return pLogData->subVariableWithScalar(getVariablePtr(t0.toString()), t1.toDouble()).data()->getFullVariableName();
            else if(!getVariablePtr(t1.toString()).isNull() && t0.isNumericalSymbol())
                return pLogData->subVariableWithScalar(getVariablePtr(t1.toString()), t0.toDouble()).data()->getFullVariableName();
            else if(!getVariablePtr(t0.toString()).isNull() && !getVariablePtr(t1.toString()).isNull())
                return pLogData->subVariables(getVariablePtr(t0.toString()), getVariablePtr(t1.toString())).data()->getFullVariableName();
        }
    }

    *returnType = Scalar;

    return QString::number(symHopExpr.evaluate(mLocalVars, mLocalFunctionPtrs));
}


void HcomHandler::splitAtFirst(QString str, QString c, QString &left, QString &right)
{
    int idx=0;
    int parBal=0;
    for(; idx<str.size(); ++idx)
    {
        if(str[idx] == '(') { ++parBal; }
        if(str[idx] == ')') { --parBal; }
        if(str.mid(idx, c.size()) == c && parBal == 0)
        {
            break;
        }
    }

    left = str.left(idx);
    right = str.right(str.size()-idx-c.size());
    return;
}


bool HcomHandler::containsOutsideParentheses(QString str, QString c)
{
    int idx=0;
    int parBal=0;
    for(; idx<str.size(); ++idx)
    {
        if(str[idx] == '(') { ++parBal; }
        if(str[idx] == ')') { --parBal; }
        if(str.mid(idx, c.size()) == c && parBal == 0)
        {
            return true;
        }
    }
    return false;
}


QString HcomHandler::runScriptCommands(QStringList &lines, bool *abort)
{
    mAborted = false; //Reset if pushed when script didn't run

    QString funcName="";
    QStringList funcCommands;

    qDebug() << "Number of commands to run: " << lines.size();

    for(int l=0; l<lines.size(); ++l)
    {
        qApp->processEvents();
        if(mAborted)
        {
            mpConsole->print("Script aborted.");
            mAborted = false;
            *abort=true;
        }

        while(lines[l].startsWith(" "))
        {
            lines[l] = lines[l].right(lines[l].size()-1);
        }
        if(lines[l].startsWith("#") || lines[l].startsWith("&")) { continue; }  //Ignore comments and labels

        if(lines[l].startsWith("stop"))
        {
            return "%%%%%EOF";
        }
        else if(lines[l].startsWith("define "))
        {
            funcName = lines[l].section(" ",1);
            ++l;
            while(!lines[l].startsWith("enddefine"))
            {
                funcCommands << lines[l];
                ++l;
            }
            mFunctions.insert(funcName, funcCommands);
            mpConsole->print("Defined function: "+funcName);
            funcName.clear();
            funcCommands.clear();
        }
        else if(lines[l].startsWith("goto"))
        {
            QString argument = lines[l].section(" ",1);
            return argument;
        }
        else if(lines[l].startsWith("while"))        //Handle while loops
        {
            QString argument = lines[l].section("(",1).remove(")");
            QStringList loop;
            int nLoops=1;
            while(nLoops > 0)
            {
                ++l;
                lines[l] = lines[l].trimmed();
                if(l>lines.size()-1)
                {
                    mpConsole->printErrorMessage("Missing REPEAT in while loop.","",false);
                    return QString();
                }

                if(lines[l].startsWith("while")) { ++nLoops; }
                if(lines[l].startsWith("repeat")) { --nLoops; }

                loop.append(lines[l]);
            }
            loop.removeLast();

            //Evaluate expression using SymHop
            SymHop::Expression symHopExpr = SymHop::Expression(argument);
            while(symHopExpr.evaluate(mLocalVars) > 0)
            {
                qApp->processEvents();
                if(mAborted)
                {
                    mpConsole->print("Script aborted.");
                    mAborted = false;
                    *abort=true;
                    return "";
                }
                QString gotoLabel = runScriptCommands(loop, abort);
                if(*abort)
                {
                    return "";
                }
                if(!gotoLabel.isEmpty())
                {
                    return gotoLabel;
                }
            }
        }
        else if(lines[l].startsWith("if"))        //Handle if statements
        {
            QString argument = lines[l].section("(",1).remove(")");
            qDebug() << "Argument: " << argument;
            QStringList ifCode;
            QStringList elseCode;
            bool inElse=false;
            while(true)
            {
                ++l;
                lines[l] = lines[l].trimmed();
                if(l>lines.size()-1)
                {
                    mpConsole->printErrorMessage("Missing ENDIF in if-statement.","",false);
                    return QString();
                }
                if(lines[l].startsWith("endif"))
                {
                    break;
                }
                if(lines[l].startsWith("else"))
                {
                    inElse=true;
                }
                else if(!inElse)
                {
                    ifCode.append(lines[l]);
                }
                else
                {
                    elseCode.append(lines[l]);
                }
            }

            bool evalOk;
            VariableType type;
            if(evaluateExpression(argument, &type, &evalOk).toInt() > 0)
            {
                if(!evalOk)
                {
                    mpConsole->printErrorMessage("Evaluation of if-statement argument failed.","",false);
                    return QString();
                }
                QString gotoLabel = runScriptCommands(ifCode, abort);
                if(*abort)
                {
                    return "";
                }
                if(!gotoLabel.isEmpty())
                {
                    return gotoLabel;
                }
            }
            else
            {
                if(!evalOk)
                {
                    mpConsole->printErrorMessage("Evaluation of if-statement argument failed.","",false);
                    return QString();
                }
                QString gotoLabel = runScriptCommands(elseCode, abort);
                if(*abort)
                {
                    return "";
                }
                if(!gotoLabel.isEmpty())
                {
                    return gotoLabel;
                }
            }
        }
        else if(lines[l].startsWith("foreach"))        //Handle foreach loops
        {
            QString var = lines[l].section(" ",1,1);
            QString filter = lines[l].section(" ",2,2);
            QStringList vars;
            getVariables(filter, vars);
            QStringList loop;
            while(!lines[l].startsWith("endforeach"))
            {
                ++l;
                loop.append(lines[l]);
            }
            loop.removeLast();

            for(int v=0; v<vars.size(); ++v)
            {
                //Append quotations around spaces
                QStringList splitVar = vars[v].split(".");
                vars[v].clear();
                for(int s=0; s<splitVar.size(); ++s)
                {
                    if(splitVar[s].contains(" "))
                    {
                        splitVar[s].append("\"");
                        splitVar[s].prepend("\"");
                    }
                    vars[v].append(splitVar[s]);
                    vars[v].append(".");
                }
                vars[v].chop(1);

                //Execute command
                QStringList tempCmds;
                for(int l=0; l<loop.size(); ++l)
                {
                    QString tempCmd = loop[l];
                    tempCmd.replace("$"+var, vars[v]);
                    tempCmds.append(tempCmd);
                }
                QString gotoLabel = runScriptCommands(tempCmds, abort);
                if(*abort)
                {
                    return "";
                }
                if(!gotoLabel.isEmpty())
                {
                    return gotoLabel;
                }
            }
        }
        else
        {
            this->executeCommand(lines[l]);
        }
    }
    return QString();
}


//! @brief Help function that returns a list of components depending on input (with support for asterisks)
//! @param str String to look for
//! @param components Reference to list of found components
void HcomHandler::getComponents(QString str, QList<ModelObject*> &components)
{
    QString left = str.split("*").first();
    QString right = str.split("*").last();

    ProjectTab *pCurrentTab = gpMainWindow->mpProjectTabs->getCurrentTab();
    if(!pCurrentTab) { return; }
    SystemContainer *pCurrentSystem = pCurrentTab->getTopLevelSystem();
    if(!pCurrentSystem) { return; }
    for(int n=0; n<pCurrentSystem->getModelObjectNames().size(); ++n)
    {
        if(pCurrentSystem->getModelObjectNames().at(n).startsWith(left) && pCurrentSystem->getModelObjectNames().at(n).endsWith(right))
        {
            components.append(pCurrentSystem->getModelObject(pCurrentSystem->getModelObjectNames().at(n)));
        }
    }
}


//! @brief Help function that returns a list of parameters according to input (with support for asterisks)
//! @param str String to look for
//! @param pComponent Pointer to component to look in
//! @param parameterse Reference to list of found parameters
void HcomHandler::getParameters(QString str, ModelObject* pComponent, QStringList &parameters)
{
    QString left = str.split("*").first();
    QString right = str.split("*").last();


    for(int n=0; n<pComponent->getParameterNames().size(); ++n)
    {
        if(pComponent->getParameterNames().at(n).startsWith(left) && pComponent->getParameterNames().at(n).endsWith(right))
        {
            parameters.append(pComponent->getParameterNames().at(n));
        }
    }
}


//! @brief Generates a list of parameters based on wildcards
//! @param str String with (or without) wildcards
//! @param parameters Reference to list of parameters
void HcomHandler::getParameters(const QString str, QStringList &parameters)
{
    if(gpMainWindow->mpProjectTabs->count() == 0) { return; }

    SystemContainer *pSystem = gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem();

    QStringList componentNames = pSystem->getModelObjectNames();

    QStringList allParameters;

    //Add quotation marks around component name if it contains spaces
    for(int n=0; n<componentNames.size(); ++n)
    {
        QStringList parameterNames = pSystem->getModelObject(componentNames[n])->getParameterNames();

        if(componentNames[n].contains(" "))
        {
            componentNames[n].prepend("\"");
            componentNames[n].append("\"");
        }

        for(int p=0; p<parameterNames.size(); ++p)
        {
            allParameters.append(componentNames[n]+"."+parameterNames[p]);
        }
    }

    QStringList systemParameters = pSystem->getParameterNames();
    for(int s=0; s<systemParameters.size(); ++s)
    {
        if(systemParameters[s].contains(" "))
        {
            systemParameters[s].prepend("\"");
            systemParameters[s].append("\"");
        }
        allParameters.append(systemParameters[s]);
    }

    QString temp = str;
    QStringList splitStr = temp.split("*");
    for(int p=0; p<allParameters.size(); ++p)
    {
        bool ok=true;
        QString name = allParameters[p];
        for(int s=0; s<splitStr.size(); ++s)
        {
            if(s==0)
            {
                if(!name.startsWith(splitStr[s]))
                {
                    ok=false;
                    break;
                }
                name.remove(0, splitStr[s].size());
            }
            else if(s==splitStr.size()-1)
            {
                if(!name.endsWith(splitStr[s]))
                {
                    ok=false;
                    break;
                }
            }
            else
            {
                if(!name.contains(splitStr[s]))
                {
                    ok=false;
                    break;
                }
                name.remove(0, name.indexOf(splitStr[s])+splitStr[s].size());
            }
        }
        if(ok)
        {
            parameters.append(allParameters[p]);
        }
    }
}


//! @brief Returns the value of specified parameter
QString HcomHandler::getParameterValue(QString parameter) const
{
    parameter.remove("\"");
    QString compName = parameter.split(".").first();
    QString parName = parameter.split(".").last();

    if(gpMainWindow->mpProjectTabs->count() == 0)
    {
        return "NaN";
    }

    SystemContainer *pSystem = gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem();
    ModelObject *pComp = pSystem->getModelObject(compName);
    if(pComp && pComp->getParameterNames().contains(parName))
    {
        return pComp->getParameterValue(parName);
    }
    else if(pSystem->getParameterNames().contains(parameter))
    {
        return pSystem->getParameterValue(parameter);
    }
    return "NaN";
}

//! @brief Help function that returns a list of variables according to input (with support for asterisks)
//! @param str String to look for
//! @param variables Reference to list of found variables
void HcomHandler::getVariables(const QString str, QStringList &variables) const
{
    if(gpMainWindow->mpProjectTabs->count() == 0) { return; }

    SystemContainer *pSystem = gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem();
    QStringList names = pSystem->getLogDataHandler()->getPlotDataNames();
    names.append(pSystem->getAliasNames());

    //Add quotation marks around component name if it contains spaces
    for(int n=0; n<names.size(); ++n)
    {
        if(names[n].split(".").first().contains(" "))
        {
            names[n].prepend("\"");
            names[n].insert(names[n].indexOf("."),"\"");
        }
    }

    //Translate long data names to short equivalents
    for(int n=0; n<names.size(); ++n)
    {
        toShortDataNames(names[n]);
    }

    QStringList splitStr = str.split("*");
    for(int n=0; n<names.size(); ++n)
    {
        bool ok=true;
        QString name = names[n];
        if(splitStr.size() == 1)   //Special case, no asterixes
        {
            ok = (name == splitStr[0]);
        }
        else        //Other cases, loop substrings and check them
        {
            for(int s=0; s<splitStr.size(); ++s)
            {
                if(s==0)
                {
                    if(!name.startsWith(splitStr[s]))
                    {
                        ok=false;
                        break;
                    }
                    name.remove(0, splitStr[s].size());
                }
                else if(s==splitStr.size()-1)
                {
                    if(!name.endsWith(splitStr[s]))
                    {
                        ok=false;
                        break;
                    }
                }
                else
                {
                    if(!name.contains(splitStr[s]))
                    {
                        ok=false;
                        break;
                    }
                    name.remove(0, name.indexOf(splitStr[s])+splitStr[s].size());
                }
            }
        }
        if(ok)
        {
            variables.append(names[n]);
        }
    }
}

//! @brief Help function that returns a list of variables according to input (with support for asterisks)
//! @param str String to look for
//! @param variables Reference to list of found variables
void HcomHandler::getVariablesThatStartsWithString(const QString str, QStringList &variables) const
{
    if(gpMainWindow->mpProjectTabs->count() == 0) { return; }

    SystemContainer *pSystem = gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem();
    QStringList names = pSystem->getLogDataHandler()->getPlotDataNames();
    names.append(pSystem->getAliasNames());

    //Add quotation marks around component name if it contains spaces
    for(int n=0; n<names.size(); ++n)
    {
        if(names[n].split(".").first().contains(" "))
        {
            names[n].prepend("\"");
            names[n].insert(names[n].indexOf("."),"\"");
        }
    }

    //Translate long data names to short equivalents
    for(int n=0; n<names.size(); ++n)
    {
        toShortDataNames(names[n]);
    }

    for(int n=0; n<names.size(); ++n)
    {
        QString name = names[n];
        if(name.startsWith(str))
        {
            variables.append(names[n]);
        }
    }
}


QString HcomHandler::getWorkingDirectory() const
{
    return mPwd;
}


//! @brief Checks if a command is an arithmetic expression and evaluates it if possible
//! @param cmd Command to evaluate
//! @returns True if it is a correct exrpession, otherwise false
bool HcomHandler::evaluateArithmeticExpression(QString cmd)
{
    cmd.replace("**", "%%%%%");

    if(cmd.endsWith("*")) { return false; }

    SymHop::Expression expr = SymHop::Expression(cmd);

    //Assignment  (handle separately to update local variables not known to SymHop)
    if(expr.isAssignment())
    {
        QString left = expr.getLeft()->toString();

        //Make sure left side is an acceptable variable name
        bool leftIsOk = left[0].isLetter();
        for(int i=1; i<left.size(); ++i)
        {
            if(!left.at(i).isLetterOrNumber())
            {
                leftIsOk = false;
            }
        }

//        QStringList plotDataNames;
//        if(gpMainWindow->mpProjectTabs->count() > 0)
//        {
//            LogDataHandler *pLogData = gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem()->getLogDataHandler();
//            plotDataNames = pLogData->getPlotDataNames();
//        }
//        if(!leftIsOk && !plotDataNames.contains(left))
        if(!leftIsOk && (gpMainWindow->mpProjectTabs->count() == 0 || !getVariablePtr(left)))
        {
            mpConsole->printErrorMessage("Illegal variable name.","",false);
            return false;
        }

        VariableType type;
        bool evalOk;
        QString value = evaluateExpression(expr.getRight()->toString(), &type, &evalOk);

        if(evalOk && type==Scalar)
        {
            mLocalVars.insert(left, value.toDouble());
            mpConsole->print("Assigning "+left+" with "+value);
            mLocalVars.insert("ans", value.toDouble());
            return true;
        }
        else if(evalOk && type==DataVector)
        {
            SharedLogVariableDataPtrT pLeftData = getVariablePtr(left);
            SharedLogVariableDataPtrT pValueData = getVariablePtr(value);
            if(pLeftData != 0) { left = pLeftData->getFullVariableName(); }
            if(pValueData != 0) { value = pValueData->getFullVariableName(); }

            gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem()->getLogDataHandler()->assignVariable(left, value);
            gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem()->getLogDataHandler()->getPlotData(left,-1).data()->preventAutoRemoval();
            return true;
        }
        else
        {
            return false;
        }
    }
    else  //Not an assignment, evaluate with SymHop
    {
        //! @todo Should we allow pure expessions without assignment?
        bool evalOk;
        VariableType type;
        QString value=evaluateExpression(cmd, &type, &evalOk);
        if(evalOk && type==Scalar)
        {
            returnScalar(value.toDouble());
            return true;
        }
        else if(evalOk && type==DataVector)
        {
            mRetvalType = DataVector;
            mpConsole->print(value);
            return true;
        }
        else
        {
            return false;
        }
    }
    return false;
}


//! @brief Returns a pointer to a data variable for given full data name
//! @param fullName Full concatinated name of the variable
//! @returns Pointer to the data variable
SharedLogVariableDataPtrT HcomHandler::getVariablePtr(QString fullName) const
{
    if(gpMainWindow->mpProjectTabs->count() == 0)
    {
        return SharedLogVariableDataPtrT(0);
    }

    fullName.replace(".","#");
    int generation = -1;

    if(fullName.count("#") == 1 || fullName.count("#") == 3)
    {
        generation = fullName.split("#").last().toInt();
        fullName.chop(fullName.split("#").last().size()+1);
    }

    if(fullName.endsWith("#x"))
    {
        fullName.chop(2);
        fullName.append("#Position");
    }
    else if(fullName.endsWith("#v"))
    {
        fullName.chop(2);
        fullName.append("#Velocity");
    }
    else if(fullName.endsWith("#f"))
    {
        fullName.chop(2);
        fullName.append("#Force");
    }
    else if(fullName.endsWith("#p"))
    {
        fullName.chop(2);
        fullName.append("#Pressure");
    }
    else if(fullName.endsWith("#q"))
    {
        fullName.chop(2);
        fullName.append("#Flow");
    }
    else if(fullName.endsWith("#val"))
    {
        fullName.chop(4);
        fullName.append("#Value");
    }
    else if(fullName.endsWith("#Zc"))
    {
        fullName.chop(3);
        fullName.append("#CharImpedance");
    }
    else if(fullName.endsWith("#c"))
    {
        fullName.chop(2);
        fullName.append("#WaveVariable");
    }
    else if(fullName.endsWith("#me"))
    {
        fullName.chop(3);
        fullName.append("#EquivalentMass");
    }
    else if(fullName.endsWith("#Q"))
    {
        fullName.chop(2);
        fullName.append("#HeatFlow");
    }
    else if(fullName.endsWith("#t"))
    {
        fullName.chop(2);
        fullName.append("#Temperature");
    }

//    SharedLogVariableDataPtrT pRetVal = gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem()->getLogDataHandler()->getPlotDataByAlias(fullName,generation);
//    if(!pRetVal)
//    {
        SharedLogVariableDataPtrT pRetVal = gpMainWindow->mpProjectTabs->getCurrentTopLevelSystem()->getLogDataHandler()->getPlotData(fullName,generation);
//    }
    return pRetVal;
}


//! @brief Parses a string into a number
//! @param str String to parse, should be a number of a variable name
//! @param ok Pointer to boolean that tells if parsing was successful
double HcomHandler::getNumber(const QString str, bool *ok) const
{
    *ok = true;
    if(str.toDouble())
    {
        return str.toDouble();
    }
    else if(mLocalVars.contains(str))
    {
        return mLocalVars.find(str).value();
    }
    else
    {
        VariableType retType;
        bool isNumber;
        double retval = evaluateExpression(str, &retType, &isNumber).toDouble();
        if(isNumber && retType==Scalar)
        {
            return retval;
        }
    }
    *ok = false;
    return 0;
}


//! @brief Converts long data names to short data names (e.g. "Pressure" -> "p")
//! @param variable Reference to variable string
void HcomHandler::toShortDataNames(QString &variable) const
{
    if(variable.endsWith(".Position"))
    {
        variable.chop(9);
        variable.append(".x");
    }
    else if(variable.endsWith(".Velocity"))
    {
        variable.chop(9);
        variable.append(".v");
    }
    else if(variable.endsWith(".Force"))
    {
        variable.chop(6);
        variable.append(".f");
    }
    else if(variable.endsWith(".Pressure"))
    {
        variable.chop(9);
        variable.append(".p");
    }
    else if(variable.endsWith(".Flow"))
    {
        variable.chop(5);
        variable.append(".q");
    }
    else if(variable.endsWith(".Value"))
    {
        variable.chop(6);
        variable.append(".val");
    }
    else if(variable.endsWith(".CharImpedance"))
    {
        variable.chop(14);
        variable.append(".Zc");
    }
    else if(variable.endsWith(".WaveVariable"))
    {
        variable.chop(13);
        variable.append(".c");
    }
    else if(variable.endsWith(".EquivalentMass"))
    {
        variable.chop(15);
        variable.append(".me");
    }
    else if(variable.endsWith(".HeatFlow"))
    {
        variable.chop(9);
        variable.append(".Q");
    }
    else if(variable.endsWith(".Temperature"))
    {
        variable.chop(12);
        variable.append(".t");
    }
}


//! @brief Converts a command to a directory path, or returns an empty string if command is invalid
QString HcomHandler::getDirectory(const QString cmd) const
{
    if(QDir().exists(QDir().cleanPath(mPwd+"/"+cmd)))
    {
        return QDir().cleanPath(mPwd+"/"+cmd);
    }
    else if(QDir().exists(cmd))
    {
        return cmd;
    }
    else
    {
        return "";
    }
}


//! @brief Returns a list of arguments in a command with respect to quotation marks
QStringList HcomHandler::getArguments(const QString cmd) const
{
    QStringList splitCmd;
    bool withinQuotations = false;
    int start=0;
    for(int i=0; i<cmd.size(); ++i)
    {
        if(cmd[i] == '\"')
        {
            withinQuotations = !withinQuotations;
        }
        if(cmd[i] == ' ' && !withinQuotations)
        {
            splitCmd.append(cmd.mid(start, i-start));
            start = i+1;
        }
    }
    splitCmd.append(cmd.right(cmd.size()-start));
    //splitCmd.removeFirst();
    splitCmd.removeAll("");

    return splitCmd;
}


//! @brief Returns number of arguments in command with respect to quotation marks
int HcomHandler::getNumberOfArguments(const QString cmd) const
{
    return getArguments(cmd).size();
}


//! @brief Returns argument in command at specified index with respect to quotation marks
//! @param cmd Command
//! @param idx Index
QString HcomHandler::getArgument(const QString cmd, const int idx) const
{
    return getArguments(cmd).at(idx);
}


//! @brief Slot that aborts any HCOM script currently running
void HcomHandler::abortHCOM()
{
    mAborted = true;
}


//! @brief Auxiliary function used by command functions to "return" a scalar
void HcomHandler::returnScalar(const double retval)
{
    mLocalVars.insert("ans", retval);
    mRetvalType = Scalar;
    mpConsole->print(QString::number(retval));
}

void HcomHandler::registerFunction(const QString func, const QString description, const SymHop::Function fptr)
{
    mLocalFunctionPtrs.insert(func, fptr);
    mLocalFunctionDescriptions.insert(func, description);
}

double _funcAver(QString str)
{
    SharedLogVariableDataPtrT pData = HcomHandler(gpMainWindow->mpTerminalWidget->mpConsole).getVariablePtr(str);

    if(pData)
    {
        return(pData->averageOfData());
    }
    return 0;
}

double _funcSize(QString str)
{
    SharedLogVariableDataPtrT pData = HcomHandler(gpMainWindow->mpTerminalWidget->mpConsole).getVariablePtr(str);

    if(pData)
    {
         return(pData->getDataSize());
    }
    return 0;
}

double _funcTime(QString /*str*/)
{
    if(gpMainWindow->mpProjectTabs->count() > 0)
    {
        return gpMainWindow->mpProjectTabs->getCurrentTab()->getLastSimulationTime();
    }
    return 0;
}


double _funcObj(QString str)
{
    int idx = str.toDouble();
    return gpMainWindow->mpTerminalWidget->mpHandler->getOptimizationObjectiveValue(idx);
}

double _funcMin(QString str)
{
    SharedLogVariableDataPtrT pData = HcomHandler(gpMainWindow->mpTerminalWidget->mpConsole).getVariablePtr(str);

    if(pData)
    {
        return(pData->minOfData());
    }
    return 0;
}

double _funcMax(QString str)
{
    SharedLogVariableDataPtrT pData = HcomHandler(gpMainWindow->mpTerminalWidget->mpConsole).getVariablePtr(str);

    if(pData)
    {
        return(pData->maxOfData());
    }
    return 0;
}

double _funcIMin(QString str)
{
    SharedLogVariableDataPtrT pData = HcomHandler(gpMainWindow->mpTerminalWidget->mpConsole).getVariablePtr(str);

    if(pData)
    {
        return(pData->indexOfMinOfData());
    }
    return 0;
}

double _funcIMax(QString str)
{
    SharedLogVariableDataPtrT pData = HcomHandler(gpMainWindow->mpTerminalWidget->mpConsole).getVariablePtr(str);

    if(pData)
    {
        return(pData->indexOfMaxOfData());
    }
    return 0;
}

double _funcPeek(QString str)
{
    QString var = str.section(",",0,0);
    SharedLogVariableDataPtrT pData = HcomHandler(gpMainWindow->mpTerminalWidget->mpConsole).getVariablePtr(var);
    QString idxStr = str.section(",",1,1);

    SymHop::Expression idxExpr = SymHop::Expression(idxStr);
    QMap<QString, double> localVars = gpMainWindow->mpTerminalWidget->mpHandler->getLocalVariables();
    QMap<QString, SymHop::Function> localFuncs = gpMainWindow->mpTerminalWidget->mpHandler->getLocalFunctionPointers();
    int idx = idxExpr.evaluate(localVars, localFuncs);

    if(pData)
    {
        QString err;
        return(pData->peekData(idx,err));
    }
    return 0;
}

double _funcRand(QString str)
{
    Q_UNUSED(str);
    return rand() / (double)RAND_MAX;          //Random value between  0 and 1
}


void HcomHandler::optComplexInit()
{
    //Load default optimization functions
    executeCommand("exec ../ScriptsHCOM/optDefaultFunctions.hcom");

    for(int p=0; p<mOptNumPoints; ++p)
    {
        mOptParameters[p].resize(mOptNumParameters);
        for(int i=0; i<mOptNumParameters; ++i)
        {
            double r = (double)rand() / (double)RAND_MAX;
            mOptParameters[p][i] = mOptParMin[i] + r*(mOptParMax[i]-mOptParMin[i]);
            if(mOptParameterType == Int)
            {
                mOptParameters[p][i] = round(mOptParameters[p][i]);
            }
        }
    }
    mOptObjectives.resize(mOptNumPoints);

    mOptKf = 1.0-pow(mOptAlpha/2.0, mOptGamma/mOptNumPoints);
}


void HcomHandler::optComplexRun()
{
    mOptConvergenceReason=0;

    if(mOptAlgorithm == Uninitialized)
    {
        mpConsole->printErrorMessage("Optimization not initialized.", "", false);
        return;
    }
    if(!mFunctions.contains("evalall"))
    {
        mpConsole->printErrorMessage("Function \"evalall\" not defined.","",false);
        return;
    }
    if(!mFunctions.contains("evalworst"))
    {
        mpConsole->printErrorMessage("Function \"evalworst\" not defined.","",false);
        return;
    }

    mpConsole->print("Running optimization...");
    executeCommand("echo off");

    //Evaluate initial objevtive values
    executeCommand("call evalall");

    //Store parameters for undo
    mOptOldParameters = mOptParameters;

    int i=0;
    int percent=-1;
    for(; i<mOptMaxEvals && !mAborted; ++i)
    {
        qApp->processEvents();
        if(mAborted)
        {
            mpConsole->print("Optimization aborted.");
            return;
        }

        //Print progress %
        int dummy=int(100.0*double(i)/mOptMaxEvals);
        if(dummy != percent)
        {
            mpConsole->setDontPrint(false);
            mpConsole->print(QString::number(dummy)+" %");
            mpConsole->setDontPrint(true);
            percent = dummy;
        }

        //Check convergence
        if(optComlexCheckconvergence()) break;

        //Increase all objective values (forgetting principle)
        optComplexForget();

        //Calculate best and worst point
        optComplexCalculatebestandworstid();
        mpConsole->print("WORST: "+QString::number(mOptWorstId));
        int wid = mOptWorstId;

        //Find geometrical center
        optComplexFindcenter();

        //Reflect worst point
        QVector<double> newPoint;
        newPoint.resize(mOptNumParameters);
        for(int j=0; j<mOptNumParameters; ++j)
        {


            //Reflect
            double worst = mOptParameters[wid][j];
            mOptParameters[wid][j] = mOptCenter[j] + (mOptCenter[j]-worst)*mOptAlpha;

            //Add some random noise
            double maxDiff = optComplexMaxpardiff();
            double r = (double)rand() / (double)RAND_MAX;
            mOptParameters[wid][j] = mOptParameters[wid][j] + mOptRfak*(mOptParMax[wid]-mOptParMin[wid])*maxDiff*(r-0.5);
            mOptParameters[wid][j] = min(mOptParameters[wid][j], mOptParMax[j]);
            mOptParameters[wid][j] = max(mOptParameters[wid][j], mOptParMin[j]);
        }
        newPoint = mOptParameters[wid];

        //Evaluate new point
        executeCommand("call evalworst");
        if(mLocalVars.find("ans").value() == -1)    //This check is needed if abort key is pressed while evaluating
        {
            executeCommand("echo on");
            mpConsole->print("Optimization aborted.");
            return;
        }

        //Calculate best and worst points
        mOptLastWorstId=wid;
        optComplexCalculatebestandworstid();
        wid = mOptWorstId;

        //Iterate until worst point is no longer the same
        mOptWorstCounter=0;
        while(mOptLastWorstId == wid)
        {
            qApp->processEvents();
            if(mAborted)
            {
                executeCommand("echo on");
                mpConsole->print("Optimization aborted.");
                mAborted = false;
                return;
            }

            if(i>mOptMaxEvals) break;

            double a1 = 1.0-exp(-double(mOptWorstCounter)/5.0);

            //Reflect worst point
            for(int j=0; j<mOptNumParameters; ++j)
            {
                int best = mOptParameters[mOptBestId][j];
                double maxDiff = optComplexMaxpardiff();
                double r = (double)rand() / (double)RAND_MAX;
                mOptParameters[wid][j] = (mOptCenter[j]*(1.0-a1) + best*a1 + newPoint[j])/2.0 + mOptRfak*(mOptParMax[wid]-mOptParMin[wid])*maxDiff*(r-0.5);
                mOptParameters[wid][j] = min(mOptParameters[wid][j], mOptParMax[j]);
                mOptParameters[wid][j] = max(mOptParameters[wid][j], mOptParMin[j]);
            }
            newPoint = mOptParameters[wid];

            //Evaluate new point
            executeCommand("call evalworst");
            if(mLocalVars.find("ans").value() == -1)    //This check is needed if abort key is pressed while evaluating
            {
                executeCommand("echo on");
                mpConsole->print("Optimization aborted.");
                return;
            }

            //Calculate best and worst points
            mOptLastWorstId=wid;
            optComplexCalculatebestandworstid();
            wid = mOptWorstId;

            ++mOptWorstCounter;
            ++i;
        }
    }

    executeCommand("echo on");

    switch(mOptConvergenceReason)
    {
    case 0:
        mpConsole->print("Optimization failed to converge after "+QString::number(i)+" iterations.");
        break;
    case 1:
        mpConsole->print("Optimization converged in function values after "+QString::number(i)+" iterations.");
        break;
    case 2:
        mpConsole->print("Optimization converged in parameter values after "+QString::number(i)+" iterations.");
        break;
    }

    return;
}


void HcomHandler::optComplexForget()
{
    double maxObj = mOptObjectives[0];
    double minObj = mOptObjectives[0];
    for(int i=0; i<mOptNumPoints; ++i)
    {
        double obj = mOptObjectives[i];
        if(obj > maxObj) maxObj = obj;
        if(obj < minObj) minObj = obj;
    }
    for(int i=0; i<mOptObjectives[0]; ++i)
    {
        mOptObjectives[0] = mOptObjectives[0]+(maxObj-minObj)*mOptKf;
    }
}

void HcomHandler::optComplexCalculatebestandworstid()
{
    double maxObj = mOptObjectives[0];
    double minObj = mOptObjectives[0];
    mOptWorstId=0;
    mOptBestId=0;
    for(int i=1; i<mOptNumPoints; ++i)
    {
        double obj = mOptObjectives[i];
        if(obj > maxObj)
        {
            maxObj = obj;
            mOptWorstId = i;
        }
        if(obj < minObj)
        {
            minObj = obj;
            mOptBestId = i;
        }
    }
}

void HcomHandler::optComplexFindcenter()
{
    mOptCenter.resize(mOptNumParameters);
    for(int p=0; p<mOptNumPoints; ++p)
    {
        for(int i=0; i<mOptNumParameters; ++i)
        {
            mOptCenter[i] = mOptCenter[i]+mOptParameters[p][i];
        }
    }
    for(int i=0; i<mOptCenter.size(); ++i)
    {
        mOptCenter[i] = mOptCenter[i]/double(mOptNumPoints);
    }
}

bool HcomHandler::optComlexCheckconvergence()
{
    //Check objective function convergence
    double maxObj = mOptObjectives[0];
    double minObj = mOptObjectives[0];
    for(int i=0; i<mOptNumPoints; ++i)
    {
        double obj = mOptObjectives[i];
        if(obj > maxObj) maxObj = obj;
        if(obj < minObj) minObj = obj;
    }
    if(minObj == 0.0)
    {
        if(fabs(maxObj-minObj) <= mOptFuncTol)
        {
            mOptConvergenceReason=1;
            return true;
        }
        else if(fabs(maxObj-minObj)/fabs(minObj) <= mOptFuncTol)
        {
            mOptConvergenceReason=1;
            return true;
        }
    }

    //Check parameter value convergence
    double maxDiff=optComplexMaxpardiff();
    if(abs(maxDiff) < mOptParTol)
    {
        mOptConvergenceReason=2;
        return true;
    }
    return false;
}


double HcomHandler::optComplexMaxpardiff()
{
    double maxDiff = -1e100;
    for(int i=0; i<mOptNumParameters; ++i)
    {
        double maxPar = -1e100;
        double minPar = 1e100;
        for(int p=0; p<mOptNumPoints; ++p)
        {
            if(mOptParameters[p][i] > maxPar) maxPar = mOptParameters[p][i];
            if(mOptParameters[p][i] < minPar) minPar = mOptParameters[p][i];
        }
        if((maxPar-minPar)/(mOptParMax[i]-mOptParMin[i]) > maxDiff)
        {
            maxDiff = (maxPar-minPar)/(mOptParMax[i]-mOptParMin[i]);
        }
    }
    return maxDiff;
}


void HcomHandler::optParticleInit()
{
    //Load default optimization functions
    executeCommand("exec ../ScriptsHCOM/optDefaultFunctions.hcom");

    for(int p=0; p<mOptNumPoints; ++p)
    {
        mOptParameters[p].resize(mOptNumParameters);
        mOptVelocities[p].resize(mOptNumParameters);
        for(int i=0; i<mOptNumParameters; ++i)
        {
            //Initialize points
            double r = double(rand()) / double(RAND_MAX);
            mOptParameters[p][i] = mOptParMin[i] + r*(mOptParMax[i]-mOptParMin[i]);
            if(mOptParameterType == Int)
            {
                mOptParameters[p][i] = round(mOptParameters[p][i]);
            }

            //Initialize velocities
            double minVel = -abs(mOptParMax[i]-mOptParMin[i]);
            double maxVel = abs(mOptParMax[i]-mOptParMin[i]);
            r = double(rand()) / double(RAND_MAX);
            mOptVelocities[p][i] = minVel + r*(maxVel-minVel);
        }
    }
    mOptObjectives.resize(mOptNumPoints);
}


void HcomHandler::optParticleRun()
{
    optPlotPoints();

    //connect(gpMainWindow->mpProjectTabs->getCurrentTab()->mpSimulationThreadHandler, SIGNAL(done(bool)), this, SLOT(optPlotPoints(bool)));

    mOptConvergenceReason=0;

    if(mOptAlgorithm == Uninitialized)
    {
        mpConsole->printErrorMessage("Optimization not initialized.", "", false);
        return;
    }
    if(!mFunctions.contains("evalall"))
    {
        mpConsole->printErrorMessage("Function \"evalall\" not defined.","",false);
        return;
    }

    mpConsole->print("Running optimization...");
    executeCommand("echo off");

    //Evaluate initial objevtive values
    executeCommand("call evalall");
    if(mLocalVars.find("ans").value() == -1)    //This check is needed if abort key is pressed while evaluating
    {
        executeCommand("echo on");
        mpConsole->print("Optimization aborted.");
        return;
    }

    //Initialize best known point for each point
    for(int i=0; i<mOptNumPoints; ++i)
    {
        mOptBestKnowns[i] = mOptParameters[i];
        mOptBestObjectives[i] = mOptObjectives[i];
    }

    //Calculate best known global position
    optComplexCalculatebestandworstid();
    mOptBestObj = mOptObjectives[mOptBestId];
    mOptBestPoint = mOptParameters[mOptBestId];

    int i=0;
    int percent=-1;
    for(; i<mOptMaxEvals && !mAborted; ++i)
    {
        qApp->processEvents();
        if(mAborted)
        {
            mpConsole->print("Optimization aborted.");
            return;
        }

        //Print progress %
        int dummy=int(100.0*double(i)/mOptMaxEvals);
        if(dummy != percent)
        {
            mpConsole->setDontPrint(false);
            mpConsole->print(QString::number(dummy)+" %");
            mpConsole->setDontPrint(true);
            percent = dummy;
        }

        //Move particles
        for (int p=0; p<mOptNumPoints; ++p)
        {
          double r1 = double(rand())/double(RAND_MAX);
          double r2 = double(rand())/double(RAND_MAX);
          for(int j=0; j<mOptNumParameters; ++j)
          {
              mOptVelocities[p][j] = mOptOmega*mOptVelocities[p][j] + mOptC1*r1*(mOptBestKnowns[p][j]-mOptParameters[p][j]) + mOptC2*r2*(mOptBestPoint[j]-mOptParameters[p][j]);
              mOptParameters[p][j] = mOptParameters[p][j]+mOptVelocities[p][j];
              if(mOptParameters[p][j] <= mOptParMin[j])
              {
                  mOptParameters[p][j] = mOptParMin[j];
                  mOptVelocities[p][j] = 0.0;
              }
              if(mOptParameters[p][j] >= mOptParMax[j])
              {
                  mOptParameters[p][j] = mOptParMax[j];
                  mOptVelocities[p][j] = 0.0;
              }
          }
        }

        //Evaluate objevtive values
        executeCommand("call evalall");
        if(mLocalVars.find("ans").value() == -1)    //This check is needed if abort key is pressed while evaluating
        {
            executeCommand("echo on");
            mpConsole->print("Optimization aborted.");
            return;
        }

        //Calculate best known positions
        for(int p=0; p<mOptNumPoints; ++p)
        {
            if(mOptObjectives[p] < mOptBestObjectives[p])
            {
                mOptBestKnowns[p] = mOptParameters[p];
                mOptBestObjectives[p] = mOptObjectives[p];
            }
        }

        //Calculate best known global position
        optComplexCalculatebestandworstid();
        if(mOptObjectives[mOptBestId] < mOptBestObj)
        {
            mOptBestObj = mOptObjectives[mOptBestId];
            mOptBestPoint = mOptParameters[mOptBestId];
        }

        optPlotPoints();

        //Check convergence
        if(optComlexCheckconvergence()) break;      //Use complex method, it's the same principle

        //optPlotPoints();
    }

    executeCommand("echo on");

    switch(mOptConvergenceReason)
    {
    case 0:
        mpConsole->print("Optimization failed to converge after "+QString::number(i)+" iterations.");
        break;
    case 1:
        mpConsole->print("Optimization converged in function values after "+QString::number(i)+" iterations.");
        break;
    case 2:
        mpConsole->print("Optimization converged in parameter values after "+QString::number(i)+" iterations.");
        break;
    }

    return;
}

void HcomHandler::optPlotPoints(bool /*dummy*/)
{
    if(mOptNumParameters != 2)
    {
        mpConsole->printErrorMessage("Points can only be plotted with two parameters.");
        return;
    }

    LogDataHandler *pHandler = gpMainWindow->mpProjectTabs->getCurrentContainer()->getLogDataHandler();
    for(int p=0; p<mOptNumPoints; ++p)
    {
        QString name = "par"+QString::number(p);
        double x = mOptParameters[p][0];
        double y = mOptParameters[p][1];
        SharedLogVariableDataPtrT parVar1 = pHandler->getPlotData(name, -1);
        if(!parVar1)
        {
            parVar1 = pHandler->defineNewVariable(name, QVector<double>() << x, QVector<double>() << y);
            parVar1.data()->preventAutoRemoval();
            gpPlotHandler->plotDataToWindow("parplot", parVar1, 0);
        }
        else
        {
            SharedLogVariableDataPtrT dummy = pHandler->defineNewVariable("dummy", QVector<double>() << x, QVector<double>() << y);

            parVar1.data()->assignFrom(dummy);
        }
    }

    PlotWindow *pPlotWindow = gpPlotHandler->getPlotWindow("parplot");
    if(!pPlotWindow)
    {
        gpPlotHandler->createPlotWindow("parplot");
        pPlotWindow = gpPlotHandler->getPlotWindow("parplot");
    }
    PlotTab *pTab = pPlotWindow->getCurrentPlotTab();
    pTab->getPlot(FirstPlot)->setAxisScale(QwtPlot::xBottom, mOptParMin[0], mOptParMax[0]);
    pTab->getPlot(FirstPlot)->setAxisScale(QwtPlot::yLeft, mOptParMin[1], mOptParMax[1]);
    pTab->update();

    for(int c=0; c<pTab->getCurves(FirstPlot).size(); ++c)
    {
        if(c==mOptBestId)
        {
            pTab->getCurves(FirstPlot).at(c)->setLineSymbol("Star 1");
        }
        else
        {
            pTab->getCurves(FirstPlot).at(c)->setLineSymbol("XCross");
        }
    }
}





