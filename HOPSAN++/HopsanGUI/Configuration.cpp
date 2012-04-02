/*-----------------------------------------------------------------------------
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
//! @file   Configuration.cpp
//! @author Robert Braun <robert.braun@liu.se>
//! @date   2010-11-15
//!
//! @brief Contains a class for the configuration object
//!
//$Id$

#include "common.h"
#include "version_gui.h"
#include "Configuration.h"
#include "Utilities/XMLUtilities.h"
#include "Utilities/GUIUtilities.h"
#include "MainWindow.h"
#include "Widgets/MessageWidget.h"
#include "Widgets/PyDockWidget.h"

#include <QDomElement>
#include <QMessageBox>
#include <QMap>


//! @class Configuration
//! @brief The Configuration class is used as a global XML-based storage for program configuration variables
//!
//! Use loadFromXml or saveToXml functions to read or write to hopsanconfig.xml. Use get, set, add and clear functions to access the data.
//!


//! @brief Saves the current settings to hopsanconfig.xml
void Configuration::saveToXml()
{
        //Write to hopsanconfig.xml
    QDomDocument domDocument;
    QDomElement configRoot = domDocument.createElement("hopsanconfig");
    configRoot.setAttribute(HMF_HOPSANGUIVERSIONTAG, HOPSANGUIVERSION);
    domDocument.appendChild(configRoot);

    QDomElement settings = appendDomElement(configRoot,"settings");
    appendDomIntegerNode(settings, "librarystyle", mLibraryStyle);
    appendDomBooleanNode(settings, "showwelcomedialog", mShowWelcomeDialog);
    appendDomBooleanNode(settings, "showpopuphelp", mShowPopupHelp);
    appendDomBooleanNode(settings, "nativestylesheet", mUseNativeStyleSheet);
    appendDomTextNode(settings, "backgroundcolor", mBackgroundColor.name());
    appendDomBooleanNode(settings, "antialiasing", mAntiAliasing);
    appendDomBooleanNode(settings, "invertwheel", mInvertWheel);
    appendDomBooleanNode(settings, "snapping", mSnapping);
    appendDomBooleanNode(settings, "progressbar", mEnableProgressBar);
    appendDomValueNode(settings, "progressbar_step", mProgressBarStep);
    appendDomBooleanNode(settings, "multicore", mUseMulticore);
    appendDomValueNode(settings, "numberofthreads", mNumberOfThreads);
    appendDomBooleanNode(settings, "togglenamesbuttonchecked", gpMainWindow->mpToggleNamesAction->isChecked());
    appendDomBooleanNode(settings, "toggleportsbuttonchecked", gpMainWindow->mpTogglePortsAction->isChecked());
    appendDomBooleanNode(settings, "groupmessagesbytag", mGroupMessagesByTag);
    appendDomIntegerNode(settings, "generationlimit", mGenerationLimit);
    appendDomTextNode(settings, "loadmodeldir", mLoadModelDir);
    appendDomTextNode(settings, "modelgfxdir", mModelGfxDir);
    appendDomTextNode(settings, "plotdatadir", mPlotDataDir);
    appendDomTextNode(settings, "plotgfxdir", mPlotGfxDir);
    appendDomTextNode(settings, "simulinkexportdir", mSimulinkExportDir);
    appendDomTextNode(settings, "subsystemdir", mSubsystemDir);
    appendDomTextNode(settings, "modelicamodelsdir", mModelicaModelsDir);
    appendDomTextNode(settings, "externallibdir", mExternalLibDir);
    appendDomTextNode(settings, "scriptdir", mScriptDir);


    QDomElement style = appendDomElement(configRoot, HMF_STYLETAG);

    QMap<connectorStyle, QMap<QString, QMap<QString, QPen> > >::iterator it1;
    QMap<QString, QMap<QString, QPen> >::iterator it2;
    QMap<QString, QPen>::iterator it3;

    for(it1 = mPenStyles.begin(); it1 != mPenStyles.end(); ++it1)
    {
        for(it2 = it1.value().begin(); it2 != it1.value().end(); ++it2)
        {
            for(it3 = it2.value().begin(); it3 != it2.value().end(); ++it3)
            {
                QString type;
                if(it1.key() == POWERCONNECTOR) type = "Power";
                if(it1.key() == SIGNALCONNECTOR) type = "Signal";
                if(it1.key() == UNDEFINEDCONNECTOR) type = "Undefined";

                QDomElement tempElement = appendDomElement(style, "penstyle");
                tempElement.setAttribute("type", type);
                tempElement.setAttribute("gfxtype", it2.key());
                tempElement.setAttribute("situation", it3.key());
                tempElement.setAttribute("color", it3.value().color().name());
                setQrealAttribute(tempElement, "width", it3.value().widthF());
                tempElement.setAttribute(HMF_STYLETAG, it3.value().style());
                tempElement.setAttribute("capstyle", it3.value().capStyle());
            }
        }
    }

    QDomElement libs = appendDomElement(configRoot, "libs");
    for(int i=0; i<mUserLibs.size(); ++i)
    {
        appendDomTextNode(libs, "userlib", mUserLibs.at(i));
    }

    QDomElement models = appendDomElement(configRoot, "models");
    for(int i=0; i<mLastSessionModels.size(); ++i)
    {
        if(mLastSessionModels.at(i) != "")
        {
            appendDomTextNode(models, "lastsessionmodel", mLastSessionModels.at(i));
        }
    }
    for(int i = mRecentModels.size()-1; i>-1; --i)
    {
        if(mRecentModels.at(i) != "")
            appendDomTextNode(models, "recentmodel", mRecentModels.at(i));
    }

    QDomElement units = appendDomElement(configRoot, "units");
    QMap<QString, QString>::iterator itdu;
    for(itdu = mDefaultUnits.begin(); itdu != mDefaultUnits.end(); ++itdu)
    {
        QDomElement xmlTemp = appendDomElement(units, "defaultunit");
        xmlTemp.setAttribute("name", itdu.key());
        xmlTemp.setAttribute("unit", itdu.value());
    }
    QMap<QString, QMap<QString, double> >::iterator itpcu;
    QMap<QString, double>::iterator itcu;
    for(itpcu = mCustomUnits.begin(); itpcu != mCustomUnits.end(); ++itpcu)
    {
        for(itcu = itpcu.value().begin(); itcu != itpcu.value().end(); ++itcu)
        {
            QDomElement xmlTemp = appendDomElement(units, "customunit");
            xmlTemp.setAttribute("name", itpcu.key());
            xmlTemp.setAttribute("unit", itcu.key());
            setQrealAttribute(xmlTemp, "scale", itcu.value());
        }
    }

    //Save python session
    QDomElement python = appendDomElement(configRoot, "python");
    gpMainWindow->getPythonDock()->saveSettingsToDomElement(python);

    appendRootXMLProcessingInstruction(domDocument);

    //Save to file
    const int IndentSize = 4;
    if(!QDir(DATAPATH).exists())
    {
        QDir().mkpath(DATAPATH);
    }
    QFile xmlsettings(QString(DATAPATH) + QString("hopsanconfig.xml"));
    if (!xmlsettings.open(QIODevice::WriteOnly | QIODevice::Text))  //open file
    {
        qDebug() << "Failed to open file for writing: " << QString(DATAPATH) + QString("hopsanconfig.xml");
        return;
    }
    QTextStream out(&xmlsettings);
    domDocument.save(out, IndentSize);
}


//! @brief Updates all settings from hopsanconfig.xml
void Configuration::loadFromXml()
{
    //Read from hopsandefaults.xml
    loadDefaultsFromXml();

    //Read from hopsanconfig.xml
    QFile file(QString(DATAPATH) + QString("hopsanconfig.xml"));
    qDebug() << "Reading config from " << file.fileName();
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        gpMainWindow->mpMessageWidget->printGUIWarningMessage("Unable to find configuration file. Configuration file was recreated with default settings.");
        return;
    }

    QDomDocument domDocument;
    QString errorStr;
    int errorLine, errorColumn;
    if (!domDocument.setContent(&file, false, &errorStr, &errorLine, &errorColumn))
    {
        QMessageBox::information(gpMainWindow->window(), gpMainWindow->tr("Hopsan"),
                                 gpMainWindow->tr("hopsanconfig.xml: Parse error at line %1, column %2:\n%3")
                                 .arg(errorLine)
                                 .arg(errorColumn)
                                 .arg(errorStr));
    }
    else
    {
        QDomElement configRoot = domDocument.documentElement();
        if (configRoot.tagName() != "hopsanconfig")
        {
            QMessageBox::information(gpMainWindow->window(), gpMainWindow->tr("Hopsan"),
                                     "The file is not an Hopsan Configuration File. Incorrect hmf root tag name: "
                                     + configRoot.tagName() + " != hopsanconfig");
        }
        else
        {
            verifyConfigurationCompatibility(configRoot);     //Check version compatibility
            QDomElement settingsElement = configRoot.firstChildElement("settings");

            if(!settingsElement.firstChildElement("librarystyle").isNull())
                mLibraryStyle = parseDomIntegerNode(settingsElement.firstChildElement("librarystyle"));
            if(!settingsElement.firstChildElement("showwelcomedialog").isNull())
                mShowWelcomeDialog = parseDomBooleanNode(settingsElement.firstChildElement("showwelcomedialog"));
            if(!settingsElement.firstChildElement("showpopuphelp").isNull())
                mShowPopupHelp = parseDomBooleanNode(settingsElement.firstChildElement("showpopuphelp"));
            if(!settingsElement.firstChildElement("nativestylesheet").isNull())
                mUseNativeStyleSheet = parseDomBooleanNode(settingsElement.firstChildElement("nativestylesheet"));
            if(!settingsElement.firstChildElement("backgroundcolor").isNull())
                mBackgroundColor.setNamedColor(settingsElement.firstChildElement("backgroundcolor").text());
            if(!settingsElement.firstChildElement("antialiasing").isNull())
                mAntiAliasing = parseDomBooleanNode(settingsElement.firstChildElement("antialiasing"));
            if(!settingsElement.firstChildElement("invertwheel").isNull())
                mInvertWheel = parseDomBooleanNode(settingsElement.firstChildElement("invertwheel"));
            if(!settingsElement.firstChildElement("snapping").isNull())
                mSnapping = parseDomBooleanNode(settingsElement.firstChildElement("snapping"));
            if(!settingsElement.firstChildElement("progressbar").isNull())
                mEnableProgressBar = parseDomBooleanNode(settingsElement.firstChildElement("progressbar"));
            if(!settingsElement.firstChildElement("progressbar_step").isNull())
                mProgressBarStep = parseDomValueNode(settingsElement.firstChildElement("progressbar_step"));
            if(!settingsElement.firstChildElement("multicore").isNull())
                mUseMulticore = parseDomBooleanNode(settingsElement.firstChildElement("multicore"));
            if(!settingsElement.firstChildElement("numberofthreads").isNull())
                mNumberOfThreads = parseDomValueNode(settingsElement.firstChildElement("numberofthreads"));
            if(!settingsElement.firstChildElement("togglenamesbuttonchecked").isNull())
                mToggleNamesButtonCheckedLastSession = parseDomBooleanNode(settingsElement.firstChildElement("togglenamesbuttonchecked"));
            if(!settingsElement.firstChildElement("toggleportsbuttonchecked").isNull())
                mTogglePortsButtonCheckedLastSession = parseDomBooleanNode(settingsElement.firstChildElement("toggleportsbuttonchecked"));
            if(!settingsElement.firstChildElement("groupmessagesbytag").isNull())
                mGroupMessagesByTag = parseDomBooleanNode(settingsElement.firstChildElement("groupmessagesbytag"));
            if(!settingsElement.firstChildElement("generationlimit").isNull())
                mGenerationLimit = parseDomIntegerNode(settingsElement.firstChildElement("generationlimit"));
            if(!settingsElement.firstChildElement("loadmodeldir").isNull())
                mLoadModelDir = settingsElement.firstChildElement("loadmodeldir").text();
            if(!settingsElement.firstChildElement("modelgfxdir").isNull())
                mModelGfxDir = settingsElement.firstChildElement("modelgfxdir").text();
            if(!settingsElement.firstChildElement("plotdatadir").isNull())
                mPlotDataDir = settingsElement.firstChildElement("plotdatadir").text();
            if(!settingsElement.firstChildElement("plotgfxdir").isNull())
                mPlotGfxDir = settingsElement.firstChildElement("plotgfxdir").text();
            if(!settingsElement.firstChildElement("simulinkexportdir").isNull())
                mSimulinkExportDir = settingsElement.firstChildElement("simulinkexportdir").text();
            if(!settingsElement.firstChildElement("subsystemdir").isNull())
                mSubsystemDir = settingsElement.firstChildElement("subsystemdir").text();
            if(!settingsElement.firstChildElement("modelicamodelsdir").isNull())
                mModelicaModelsDir = settingsElement.firstChildElement("modelicamodelsdir").text();
            if(!settingsElement.firstChildElement("externallibdir").isNull())
                mExternalLibDir = settingsElement.firstChildElement("externallibdir").text();
            if(!settingsElement.firstChildElement("scriptdir").isNull())
                mScriptDir = settingsElement.firstChildElement("scriptdir").text();

            QDomElement styleElement = configRoot.firstChildElement(HMF_STYLETAG);
            QDomElement penElement = styleElement.firstChildElement("penstyle");
            while(!penElement.isNull())
            {
                QString type = penElement.attribute("type");
                QString gfxType = penElement.attribute("gfxtype");
                QString situation = penElement.attribute("situation");
                QString color = penElement.attribute("color");
                double width = penElement.attribute("width").toDouble();
                Qt::PenStyle penstyle = Qt::PenStyle(penElement.attribute(HMF_STYLETAG).toInt());
                Qt::PenCapStyle capStyle = Qt::PenCapStyle(penElement.attribute("capstyle").toInt());
                QPen pen = QPen(QColor(color), width, penstyle, capStyle, Qt::RoundJoin);

                connectorStyle style;
                if(type=="Power") style = POWERCONNECTOR;
                if(type=="Signal") style = SIGNALCONNECTOR;
                if(type=="Undefined") style = UNDEFINEDCONNECTOR;

                if(!mPenStyles.contains(style))
                {
                    QMap<QString, QMap<QString, QPen> > tempMap;
                    mPenStyles.insert(style, tempMap);
                }
                if(!mPenStyles.find(style).value().contains(gfxType))
                {
                    QMap<QString, QPen> tempMap;
                    mPenStyles.find(style).value().insert(gfxType, tempMap);
                }
                mPenStyles.find(style).value().find(gfxType).value().insert(situation, pen);

                penElement = penElement.nextSiblingElement("penstyle");
            }

            QDomElement libsElement = configRoot.firstChildElement("libs");
            QDomElement userLibElement = libsElement.firstChildElement("userlib");
            while (!userLibElement.isNull())
            {
                mUserLibs.prepend(userLibElement.text());
                userLibElement = userLibElement.nextSiblingElement(("userlib"));
            }

            QDomElement modelsElement = configRoot.firstChildElement("models");
            QDomElement lastSessionElement = modelsElement.firstChildElement("lastsessionmodel");
            while (!lastSessionElement.isNull())
            {
                mLastSessionModels.prepend(lastSessionElement.text());
                lastSessionElement = lastSessionElement.nextSiblingElement("lastsessionmodel");
            }
            QDomElement recentModelElement = modelsElement.firstChildElement("recentmodel");
            while (!recentModelElement.isNull())
            {
                mRecentModels.prepend(recentModelElement.text());
                recentModelElement = recentModelElement.nextSiblingElement("recentmodel");
            }

            QDomElement unitsElement = configRoot.firstChildElement("units");
            QDomElement defaultUnitElement = unitsElement.firstChildElement("defaultunit");
            while (!defaultUnitElement.isNull())
            {
                mDefaultUnits.insert(defaultUnitElement.attribute(HMF_NAMETAG),
                                     defaultUnitElement.attribute("unit"));
                defaultUnitElement = defaultUnitElement.nextSiblingElement("defaultunit");
            }

            QDomElement customUnitElement = unitsElement.firstChildElement("customunit");
            while (!customUnitElement.isNull())
            {
                QString physicalQuantity = customUnitElement.attribute(HMF_NAMETAG);
                QString unitName = customUnitElement.attribute("unit");
                double unitScale = customUnitElement.attribute("scale").toDouble();

                if(!mCustomUnits.find(physicalQuantity).value().contains(unitName))
                {
                    mCustomUnits.find(physicalQuantity).value().insert(unitName, unitScale);
                }
                customUnitElement = customUnitElement.nextSiblingElement("customunit");
            }

            //Load settings to PyDockWidget in MainWindow
            QDomElement pythonElement = configRoot.firstChildElement("python");
            if(!pythonElement.isNull())
            {
                QDomElement lastScriptElement = pythonElement.firstChildElement("lastscript");
                mLastScriptFile = lastScriptElement.attribute("file");
            }


        }
    }
    file.close();
}



void Configuration::loadDefaultsFromXml()
{
    //Read from hopsandefaults.xml
    QFile file(QString(MAINPATH) + "hopsandefaults");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::information(gpMainWindow->window(), gpMainWindow->tr("Hopsan"),
                                 "Unable to read default configuration file. Please reinstall program.\n" + QString(MAINPATH));

        qApp->quit();
    }
    QDomDocument domDocument;
    QString errorStr;
    int errorLine, errorColumn;
    if (!domDocument.setContent(&file, false, &errorStr, &errorLine, &errorColumn))
    {
        QMessageBox::information(gpMainWindow->window(), gpMainWindow->tr("Hopsan"),
                                 gpMainWindow->tr("hopsandefaults: Parse error at line %1, column %2:\n%3")
                                 .arg(errorLine)
                                 .arg(errorColumn)
                                 .arg(errorStr));
    }
    else
    {
        QDomElement configRoot = domDocument.documentElement();
        if (configRoot.tagName() != "hopsandefaults")
        {
            QMessageBox::information(gpMainWindow->window(), gpMainWindow->tr("Hopsan"),
                                     "The file is not an Hopsan Configuration File. Incorrect hmf root tag name: "
                                     + configRoot.tagName() + " != hopsandefaults");
        }
        else
        {
                //Load default user settings
            QDomElement settingsElement = configRoot.firstChildElement("settings");
            if(!settingsElement.firstChildElement("librarystyle").isNull())
                mLibraryStyle = parseDomIntegerNode(settingsElement.firstChildElement("librarystyle"));
            if(!settingsElement.firstChildElement("showwelcomedialog").isNull())
                mShowWelcomeDialog = parseDomBooleanNode(settingsElement.firstChildElement("showwelcomedialog"));
            if(!settingsElement.firstChildElement("showpopuphelp").isNull())
                mShowPopupHelp = parseDomBooleanNode(settingsElement.firstChildElement("showpopuphelp"));
            if(!settingsElement.firstChildElement("nativestylesheet").isNull())
                mUseNativeStyleSheet = parseDomBooleanNode(settingsElement.firstChildElement("nativestylesheet"));
            if(!settingsElement.firstChildElement("backgroundcolor").isNull())
                mBackgroundColor.setNamedColor(settingsElement.firstChildElement("backgroundcolor").text());
            if(!settingsElement.firstChildElement("antialiasing").isNull())
                mAntiAliasing = parseDomBooleanNode(settingsElement.firstChildElement("antialiasing"));
            if(!settingsElement.firstChildElement("invertwheel").isNull())
                mInvertWheel = parseDomBooleanNode(settingsElement.firstChildElement("invertwheel"));
            if(!settingsElement.firstChildElement("snapping").isNull())
                mSnapping = parseDomBooleanNode(settingsElement.firstChildElement("snapping"));
            if(!settingsElement.firstChildElement("progressbar").isNull())
                mEnableProgressBar = parseDomBooleanNode(settingsElement.firstChildElement("progressbar"));
            if(!settingsElement.firstChildElement("progressbar_step").isNull())
                mProgressBarStep = parseDomValueNode(settingsElement.firstChildElement("progressbar_step"));
            if(!settingsElement.firstChildElement("multicore").isNull())
                mUseMulticore = parseDomBooleanNode(settingsElement.firstChildElement("multicore"));
            if(!settingsElement.firstChildElement("numberofthreads").isNull())
                this->mNumberOfThreads = parseDomValueNode(settingsElement.firstChildElement("numberofthreads"));
            if(!settingsElement.firstChildElement("togglenamesbuttonchecked").isNull())
                mToggleNamesButtonCheckedLastSession = parseDomBooleanNode(settingsElement.firstChildElement("togglenamesbuttonchecked"));
            if(!settingsElement.firstChildElement("toggleportsbuttonchecked").isNull())
                mTogglePortsButtonCheckedLastSession = parseDomBooleanNode(settingsElement.firstChildElement("toggleportsbuttonchecked"));
            if(!settingsElement.firstChildElement("groupmessagesbytag").isNull())
                mGroupMessagesByTag = parseDomBooleanNode(settingsElement.firstChildElement("groupmessagesbytag"));
            if(!settingsElement.firstChildElement("generationlimit").isNull())
                mGenerationLimit = parseDomIntegerNode(settingsElement.firstChildElement("generationlimit"));
            if(!settingsElement.firstChildElement("loadmodeldir").isNull())
                mLoadModelDir = settingsElement.firstChildElement("loadmodeldir").text();
            if(!settingsElement.firstChildElement("modelgfxdir").isNull())
                mModelGfxDir = settingsElement.firstChildElement("modelgfxdir").text();
            if(!settingsElement.firstChildElement("plotdatadir").isNull())
                mPlotDataDir = settingsElement.firstChildElement("plotdatadir").text();
            if(!settingsElement.firstChildElement("plotgfxdir").isNull())
                mPlotGfxDir = settingsElement.firstChildElement("plotgfxdir").text();
            if(!settingsElement.firstChildElement("simulinkexportdir").isNull())
                mSimulinkExportDir = settingsElement.firstChildElement("simulinkexportdir").text();
            if(!settingsElement.firstChildElement("subsystemdir").isNull())
                mSubsystemDir = settingsElement.firstChildElement("subsystemdir").text();
            if(!settingsElement.firstChildElement("modelicamodelsdir").isNull())
                mModelicaModelsDir = settingsElement.firstChildElement("modelicamodelsdir").text();
            if(!settingsElement.firstChildElement("externallibdir").isNull())
                mExternalLibDir = settingsElement.firstChildElement("externallibdir").text();
            if(!settingsElement.firstChildElement("scriptdir").isNull())
                mScriptDir = settingsElement.firstChildElement("scriptdir").text();

                //Load default GUI style
            QDomElement styleElement = configRoot.firstChildElement(HMF_STYLETAG);
            QDomElement penElement = styleElement.firstChildElement("penstyle");
            while(!penElement.isNull())
            {
                QString type = penElement.attribute("type");
                QString gfxType = penElement.attribute("gfxtype");
                QString situation = penElement.attribute("situation");
                QString color = penElement.attribute("color");
                double width = penElement.attribute("width").toDouble();
                Qt::PenStyle penstyle = Qt::PenStyle(penElement.attribute(HMF_STYLETAG).toInt());
                Qt::PenCapStyle capStyle = Qt::PenCapStyle(penElement.attribute("capstyle").toInt());
                QPen pen = QPen(QColor(color), width, penstyle, capStyle);

                connectorStyle style;
                if(type=="Power")
                {
                    style = POWERCONNECTOR;
                }
                else if(type=="Signal")
                {
                    style = SIGNALCONNECTOR;
                }
                else
                {
                    style = UNDEFINEDCONNECTOR;
                }

                if(!mPenStyles.contains(style))
                {
                    QMap<QString, QMap<QString, QPen> > tempMap;
                    mPenStyles.insert(style, tempMap);
                }
                if(!mPenStyles.find(style).value().contains(gfxType))
                {
                    QMap<QString, QPen> tempMap;
                    mPenStyles.find(style).value().insert(gfxType, tempMap);
                }
                mPenStyles.find(style).value().find(gfxType).value().insert(situation, pen);

                penElement = penElement.nextSiblingElement("penstyle");
            }
            QDomElement paletteElement = styleElement.firstChildElement("palette");
            double red, green, blue;
            QColor windowText, button, light, dark, mid, text, bright_text, base, window;
            parseRgbString(paletteElement.attribute("windowText"), red, green, blue);
            windowText.setRgb(red, green, blue);
            parseRgbString(paletteElement.attribute("button"), red, green, blue);
            button.setRgb(red, green, blue);
            parseRgbString(paletteElement.attribute("light"), red, green, blue);
            light.setRgb(red, green, blue);
            parseRgbString(paletteElement.attribute("dark"), red, green, blue);
            dark.setRgb(red, green, blue);
            parseRgbString(paletteElement.attribute("mid"), red, green, blue);
            mid.setRgb(red, green, blue);
            parseRgbString(paletteElement.attribute("text"), red, green, blue);
            text.setRgb(red, green, blue);
            parseRgbString(paletteElement.attribute("bright_text"), red, green, blue);
            bright_text.setRgb(red, green, blue);
            parseRgbString(paletteElement.attribute("base"), red, green, blue);
            base.setRgb(red, green, blue);
            parseRgbString(paletteElement.attribute("window"), red, green, blue);
            window.setRgb(red, green, blue);
            mPalette = QPalette(windowText, button, light, dark, mid, text, bright_text, base, window);
            QDomElement fontElement = styleElement.firstChildElement("font");
            mFont = QFont(fontElement.attribute("family"), fontElement.attribute("size").toInt());
            mFont.setStyleStrategy(QFont::PreferAntialias);
            mFont.setStyleHint(QFont::SansSerif);
            qDebug() << "Setting font to: " << mFont.family();
            QDomElement styleSheetElement = styleElement.firstChildElement("stylesheet");
            mStyleSheet.append(styleSheetElement.text());

                //Load default units
            QDomElement unitsElement = configRoot.firstChildElement("units");
            QDomElement defaultUnitElement = unitsElement.firstChildElement("defaultunit");
            while (!defaultUnitElement.isNull())
            {
                mDefaultUnits.insert(defaultUnitElement.attribute(HMF_NAMETAG),
                                     defaultUnitElement.attribute("unit"));
                defaultUnitElement = defaultUnitElement.nextSiblingElement("defaultunit");
            }
            mCustomUnits.insert("Pressure", QMap<QString, double>());
            mCustomUnits.insert("Flow", QMap<QString, double>());
            mCustomUnits.insert("Force", QMap<QString, double>());
            mCustomUnits.insert("Position", QMap<QString, double>());
            mCustomUnits.insert("Velocity", QMap<QString, double>());
            mCustomUnits.insert("Torque", QMap<QString, double>());
            mCustomUnits.insert("Angle", QMap<QString, double>());
            mCustomUnits.insert("Angular Velocity", QMap<QString, double>());
            mCustomUnits.insert("Value", QMap<QString, double>());
            mCustomUnits.insert("Voltage", QMap<QString, double>());
            mCustomUnits.insert("Current", QMap<QString, double>());
            QDomElement customUnitElement = unitsElement.firstChildElement("customunit");
            while (!customUnitElement.isNull())
            {
                QString physicalQuantity = customUnitElement.attribute(HMF_NAMETAG);
                QString unitName = customUnitElement.attribute("unit");
                double unitScale = customUnitElement.attribute("scale").toDouble();
                if(!mCustomUnits.find(physicalQuantity).value().contains(unitName))
                {
                    mCustomUnits.find(physicalQuantity).value().insert(unitName, unitScale);
                }
                customUnitElement = customUnitElement.nextSiblingElement("customunit");
            }
        }
    }
    file.close();

    return;
}


//! @brief Returns which library style to use
int Configuration::getLibraryStyle()
{
    return this->mLibraryStyle;
}


//! @brief Returns whether or not the welcome dialog shall be shown
bool Configuration::getShowWelcomeDialog()
{
    return this->mShowWelcomeDialog;
}


//! @brief Returns whether or not the popup help shall be shown
bool Configuration::getShowPopupHelp()
{
    return this->mShowPopupHelp;
}


//! @brief Returns whether or not the welcome dialog shall be shown
bool Configuration::getUseNativeStyleSheet()
{
    return this->mUseNativeStyleSheet;
}


//! @brief Returns whether or not invert wheel shall be used
bool Configuration::getInvertWheel()
{
    return this->mInvertWheel;
}


//! @brief Returns whether or not multi-threading shall be used
bool Configuration::getUseMulticore()
{
    return this->mUseMulticore;
}


//! @brief Returns number of simulation threads that shall be used
size_t Configuration::getNumberOfThreads()
{
    return this->mNumberOfThreads;
}


//! @brief Returns whether or not the toggle names button was checked at the end of last session
bool Configuration::getToggleNamesButtonCheckedLastSession()
{
    return this->mToggleNamesButtonCheckedLastSession;
}


//! @brief Returns whether or not the toggle ports button was checked at the end of last session
bool Configuration::getTogglePortsButtonCheckedLastSession()
{
    return this->mTogglePortsButtonCheckedLastSession;
}


//! @brief Returns the step size that shall be used in progress bar
int Configuration::getProgressBarStep()
{
    return this->mProgressBarStep;
}


//! @brief Returns whether or not the progress bar shall be displayed during simulation
bool Configuration::getEnableProgressBar()
{
    return this->mEnableProgressBar;
}


//! @brief Returns the background color
QColor Configuration::getBackgroundColor()
{
    return this->mBackgroundColor;
}


//! @brief Returns whether or not anti-aliasing shall be used
bool Configuration::getAntiAliasing()
{
    return this->mAntiAliasing;
}


//! @brief Returns a list of paths to the user libraries that shall be loaded
QStringList Configuration::getUserLibs()
{
    return this->mUserLibs;
}


//! @brief Returns whether or not connector snapping shall be used
bool Configuration::getSnapping()
{
    return this->mSnapping;
}


//! @brief Returns a list of paths to recently opened models
QStringList Configuration::getRecentModels()
{
    return this->mRecentModels;
}


//! @brief Returns a list of paths to models that were open last time program was closed
QStringList Configuration::getLastSessionModels()
{
    return this->mLastSessionModels;
}


//! @brief Returns the selected default unit for the specified physical quantity
//! @param key Name of the physical quantity (e.g. "Pressure" or "Velocity")
QString Configuration::getDefaultUnit(QString key)
{
    if(mDefaultUnits.contains(key))
        return this->mDefaultUnits.find(key).value();
    else
        return "";
}


//! @brief Returns a map with custom units (names and scale factor) for specified physical quantity
//! @param key Name of the physical quantity (e.g. "Pressure" or "Velocity")
QMap<QString, double> Configuration::getCustomUnits(QString key)
{
    return this->mCustomUnits.find(key).value();
}


//! @brief Returns connector pen for specified connector type
//! @param style Style of connector (POWERCONNECTOR, SIGNALCONNECTOR or UNDEFINEDCONNECTOR)
//! @param gfxType Graphics type (User or Iso)
//! @param situation Defines when connector is used (Primary, Hovered, Active)
QPen Configuration::getPen(connectorStyle style, graphicsType gfxType, QString situation)
{
    QString gfxString;
    if(gfxType == ISOGRAPHICS) { gfxString = "Iso"; }
    else { gfxString = "User"; }

    if(mPenStyles.contains(style))
    {
        if(mPenStyles.find(style).value().contains(gfxString))
        {
            if(mPenStyles.find(style).value().find(gfxString).value().contains(situation))
            {
                return mPenStyles.find(style).value().find(gfxString).value().find(situation).value();
            }
        }
    }
    return QPen(QColor("black"), 1, Qt::SolidLine, Qt::SquareCap);
}


QPalette Configuration::getPalette()
{
    if(this->mUseNativeStyleSheet)
    {
        QMainWindow *dummy = new QMainWindow();
        QPalette dummyPalette = dummy->palette();
        delete(dummy);
        return dummyPalette;
    }
    else
        return mPalette;
}


QFont Configuration::getFont()
{
    //! @note Embedded truetype fonts does not seem to work in Linux, so ignore them
#ifdef WIN32
    return mFont;
#else
    return qApp->font();
#endif
}


//! @brief Returns the current style sheet
QString Configuration::getStyleSheet()
{
    if(mUseNativeStyleSheet)
        return QString();
    else
        return mStyleSheet;
}


//! @brief Returns the last used script file
QString Configuration::getLastScriptFile()
{
    return mLastScriptFile;
}


//! @brief Returns the group message by tag setting
bool Configuration::getGroupMessagesByTag()
{
    return mGroupMessagesByTag;
}


//! @brief Returns the limit setting for plot generations
int Configuration::getGenerationLimit()
{
    return mGenerationLimit;
}


//! @brief Returns the last used model directory
QString Configuration::getLoadModelDir()
{
    if(mLoadModelDir.isEmpty())
    {
        return gModelsPath;
    }
    return mLoadModelDir;
}


//! @brief Returns the last used model graphics export directory
QString Configuration::getModelGfxDir()
{
    if(mModelGfxDir.isEmpty())
    {
        return QString(DATAPATH);
    }
    return mModelGfxDir;
}


//! @brief Returns the last used plot data export directory
QString Configuration::getPlotDataDir()
{
    if(mPlotDataDir.isEmpty())
    {
        return QString(DATAPATH);
    }
    return mPlotDataDir;
}


//! @brief Returns the last used plot graphics export directory
QString Configuration::getPlotGfxDir()
{
    if(mPlotGfxDir.isEmpty())
    {
        return QString(DATAPATH);
    }
    return mPlotGfxDir;
}


//! @brief Returns the last used simulink export directory
QString Configuration::getSimulinkExportDir()
{
    if(mSimulinkExportDir.isEmpty())
    {
        return QString(DATAPATH);
    }
    return mSimulinkExportDir;
}


//! @brief Returns the last used external subsystem directory
QString Configuration::getSubsystemDir()
{
    if(mSubsystemDir.isEmpty())
    {
        return gModelsPath;
    }
    return mSubsystemDir;
}


//! @brief Returns the last used modelica model directory
QString Configuration::getModelicaModelsDir()
{
    if(mModelicaModelsDir.isEmpty())
    {
        return gModelsPath;
    }
    return mModelicaModelsDir;
}


//! @brief Returns the last used external library directory
QString Configuration::getExternalLibDir()
{
    if(mExternalLibDir.isEmpty())
    {
        return QString(DATAPATH);
    }
    return mExternalLibDir;
}


//! @brief Returns the last used scripts directory
QString Configuration::getScriptDir()
{
    if(mScriptDir.isEmpty())
    {
        return gScriptsPath;
    }
    return mScriptDir;
}


//! @brief Set function for library style option
//! @param value Desired setting
void Configuration::setLibraryStyle(int value)
{
    this->mLibraryStyle = value;
    saveToXml();
}


//! @brief Set function for invert wheel option
//! @param value Desired setting
void Configuration::setShowWelcomeDialog(bool value)
{
    this->mShowWelcomeDialog = value;
    saveToXml();
}


void Configuration::setShowPopupHelp(bool value)
{
    this->mShowPopupHelp = value;
    saveToXml();
}


void Configuration::setUseNativeStyleSheet(bool value)
{
    this->mUseNativeStyleSheet = value;
    saveToXml();
}

//! @brief Set function for invert wheel option
//! @param value Desired setting
void Configuration::setInvertWheel(bool value)
{
    this->mInvertWheel = value;
    saveToXml();
}


//! @brief Set function for multi-threading option
//! @param value Desired setting
void Configuration::setUseMultiCore(bool value)
{
    this->mUseMulticore = value;
    saveToXml();
}


//! @brief Set function for number of simulation threads
//! @param value Desired number of threads
void Configuration::setNumberOfThreads(size_t value)
{
    this->mNumberOfThreads = value;
    saveToXml();
}


//! @brief Set function for progress bar time step
//! @param value Desired step
void Configuration::setProgressBarStep(int value)
{
    this->mProgressBarStep = value;
    saveToXml();
}


//! @brief Set function for use progress bar setting
//! @param value Desired setting
void Configuration::setEnableProgressBar(bool value)
{
    this->mEnableProgressBar = value;
    saveToXml();
}


//! @brief Set function for background color setting
//! @param value Desired color
void Configuration::setBackgroundColor(QColor value)
{
    this->mBackgroundColor = value;
    saveToXml();
}


//! @brief Set function for anti-aliasing setting
//! @param value Desired setting
void Configuration::setAntiAliasing(bool value)
{
    this->mAntiAliasing = value;
    saveToXml();
}


//! @brief Adds a user library to the library list
//! @param value Path to the new library
void Configuration::addUserLib(QString value)
{
    value.replace("\\","/");
    if(!mUserLibs.contains(value))
    {
        this->mUserLibs.append(value);
    }
    saveToXml();
}


//! @brief Removes a user library from the library list
//! @param value Path to the library that is to be removed
void Configuration::removeUserLib(QString value)
{
    value.replace("\\","/");
    mUserLibs.removeAll(value);
    saveToXml();
}


//! @brief Tells whether or not a specified user library exist in the library list
//! @param value Path to the library
bool Configuration::hasUserLib(QString value)
{
    value.replace("\\","/");
    return mUserLibs.contains(value);
}


//! @brief Set function for connector snapping setting
//! @param value Desired setting
void Configuration::setSnapping(bool value)
{
    this->mSnapping = value;
    saveToXml();
}


//! @brief Adds a model to the list of recently opened models
//! @brief value Path to the model
void Configuration::addRecentModel(QString value)
{
    mRecentModels.removeAll(value);
    mRecentModels.prepend(value);
    while(mRecentModels.size() > 10)
    {
        mRecentModels.pop_back();
    }
    saveToXml();
}


//! @brief Adds a model to the list of models that was open last time program closed
//! @param value Path to the model
void Configuration::addLastSessionModel(QString value)
{
    mLastSessionModels.append(value);
    saveToXml();
}


//! @brief Removes all last session models from the list
void Configuration::clearLastSessionModels()
{
    mLastSessionModels.clear();
    saveToXml();
}


//! @brief Sets the default unit for specified physical quantity
//! @param key Name of the physical quantity (e.g. "Pressure" or "Velocity")
//! @param value Name of the desired default unit
void Configuration::setDefaultUnit(QString key, QString value)
{
    this->mDefaultUnits.remove(key);
    this->mDefaultUnits.insert(key, value);
    saveToXml();
}


//! @brief Adds a new custom unit to the specified physical quantity
//! @param dataname Name of the physical quantity (e.g. "Pressure" or "Velocity")
//! @param unitname Name of the new unit
//! @param scale Scale factor from SI unit to the new unit
void Configuration::addCustomUnit(QString dataname, QString unitname, double scale)
{
    this->mCustomUnits.find(dataname).value().insert(unitname, scale);
    saveToXml();
}



void Configuration::setLastScriptFile(QString file)
{
    mLastScriptFile = file;
    saveToXml();
}


void Configuration::setGroupMessagesByTag(bool value)
{
    mGroupMessagesByTag = value;
    saveToXml();
}


void Configuration::setGenerationLimit(int value)
{
    mGenerationLimit = value;
    saveToXml();
}


void Configuration::setLoadModelDir(QString value)
{
    mLoadModelDir = value;
}


void Configuration::setModelGfxDir(QString value)
{
    mModelGfxDir = value;
}


void Configuration::setPlotDataDir(QString value)
{
    mPlotDataDir = value;
}


void Configuration::setPlotGfxDir(QString value)
{
    mPlotGfxDir = value;
}


void Configuration::setSimulinkExportDir(QString value)
{
    mSimulinkExportDir = value;
}


void Configuration::setSubsystemDir(QString value)
{
    mSubsystemDir = value;
}

void Configuration::setModelicaModelsDir(QString value)
{
    mModelicaModelsDir = value;
}

void Configuration::setExternalLibDir(QString value)
{
    mExternalLibDir = value;
}


void Configuration::setScriptDir(QString value)
{
    mScriptDir = value;
}

