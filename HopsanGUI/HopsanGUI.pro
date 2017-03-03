# -------------------------------------------------
# Project created by QtCreator 2009-12-28T14:27:59
# -------------------------------------------------
# -------------------------------------------------
# Global project options
# -------------------------------------------------
include( HopsanGuiBuild.prf )
include( $${PWD}/../HopsanRemote/HopsanRemoteBuild.pri )

TARGET = HopsanGUI
TEMPLATE = app
DESTDIR = $${PWD}/../bin

QT += svg xml
QT += core gui network

isEqual(QT_MAJOR_VERSION, 5){
    QT += widgets webkitwidgets printsupport
} else {
    QT += webkit
}

TARGET = $${TARGET}$${DEBUG_EXT}

#--------------------------------------------------------
# Set the QWT paths and dll/so/dylib/framework post linking copy command
d = $$setQWTPathInfo($$(QWT_PATH), $$DESTDIR)
!isEmpty(d){
    LIBS *= $$magic_hopsan_libpath
    INCLUDEPATH *= $$magic_hopsan_includepath
    QMAKE_POST_LINK *= $$magic_hopsan_qmake_post_link

    macx:QMAKE_LFLAGS *= -lqwt
    macx:message(LIBS=$$LIBS)
    macx:message(INCLUDEPATH=$$INCLUDEPATH)
    macx:message(QMAKE_LFLAGS=$$QMAKE_LFLAGS)
} else {
    !build_pass:error("Failed to locate QWT libs, have you compiled them and put them in the expected location")
}
#--------------------------------------------------------

#--------------------------------------------------------
# Set the PythonQt paths and dll/so post linking copy command
d = $$setPythonQtPathInfo($$(PYTHONQT_PATH), $$DESTDIR)
!isEmpty(d){
    DEFINES *= USEPYTHONQT       #If PythonQt was found then lets build GUI with PythonQt and Python support
    !build_pass:message(Compiling HopsanGUI with PythonQt support)
    LIBS *= $$magic_hopsan_libpath
    INCLUDEPATH *= $$magic_hopsan_includepath
    QMAKE_POST_LINK *= $$magic_hopsan_qmake_post_link
} else {
    !build_pass:message(Compiling HopsanGUI WITHOUT PythonQt and Python support)
}
#--------------------------------------------------------

#--------------------------------------------------------
# Set the ZMQ paths and dll/so post linking copy command
d = $$setZMQPathInfo($$(ZMQ_PATH), $$DESTDIR)
!isEmpty(d){
    DEFINES *= USEZMQ       #If ZMQ was found then lets build GUI with ZMQ / msgpack support
    !build_pass:message(Compiling HopsanGUI with ZeroMQ and msgpack support)
    LIBS *= $$magic_hopsan_libpath
    INCLUDEPATH *= $$magic_hopsan_includepath
    QMAKE_POST_LINK *= $$magic_hopsan_qmake_post_link

    INCLUDEPATH *= $${PWD}/../HopsanRemote/HopsanServer
    INCLUDEPATH *= $${PWD}/../HopsanRemote/HopsanServerClient
    SOURCES += $${PWD}/../HopsanRemote/HopsanServerClient/RemoteHopsanClient.cpp
    HEADERS += $${PWD}/../HopsanRemote/HopsanServerClient/RemoteHopsanClient.h
    SOURCES += $${PWD}/../HopsanRemote/include/FileAccess.cpp

} else {
    !build_pass:message(Compiling HopsanGUI WITHOUT ZeroMQ and msgpack support)
}
#--------------------------------------------------------

#--------------------------------------------------------
# Set HopsanCore Paths
INCLUDEPATH *= $${PWD}/../HopsanCore/include/
LIBS *= -L$${PWD}/../bin -lHopsanCore$${DEBUG_EXT}
#--------------------------------------------------------

#--------------------------------------------------------
# Set SymHop Paths
INCLUDEPATH *= $${PWD}/../SymHop/include/
LIBS *= -L$${PWD}/../bin -lSymHop$${DEBUG_EXT}
#--------------------------------------------------------

#--------------------------------------------------------
# Set Ops Paths
INCLUDEPATH *= $${PWD}/../Ops/include/
LIBS *= -L$${PWD}/../bin -lOps$${DEBUG_EXT}
#--------------------------------------------------------

#--------------------------------------------------------
# Set Discount Paths
d = $$setDiscountPathInfo($$(DISCOUNT_PATH), $$DESTDIR)
!isEmpty(d){
    DEFINES *= USEDISCOUNT
    LIBS *= $$magic_hopsan_libpath
    INCLUDEPATH *= $$magic_hopsan_includepath
    QMAKE_POST_LINK *= $$magic_hopsan_qmake_post_link
    !build_pass:message(Compiling with Discount (libmarkdown) support)
}
#--------------------------------------------------------

#--------------------------------------------------------
# Set our own HopsanGUI Include Path
INCLUDEPATH *= $${PWD}/
#--------------------------------------------------------

# -------------------------------------------------
# Platform independent additional project options
# -------------------------------------------------
# Development flag, will Gui be development version
DEFINES *= DEVELOPMENT

# Make c++11 mandatory but allow non-strict ANSI
QMAKE_CXXFLAGS *= -std=c++11 -U__STRICT_ANSI__ -Wno-c++0x-compat

# -------------------------------------------------
# Platform specific additional project options
# -------------------------------------------------
unix {
    # Set Python paths
    contains(DEFINES, USEPYTHONQT) {
        !build_pass:message("Looking for Python include and lib paths since USEPYTHONQT is defined")
        QMAKE_CXXFLAGS *= $$system(python$${PYTHON_VERSION}-config --includes) #TODO: Why does not include path work here
        LIBS *= $$system(python$${PYTHON_VERSION}-config --libs)
        INCLUDEPATH *= $$system(python$${PYTHON_VERSION}-config --includes)
    } else {
        !build_pass:message("Not looking for Python since we are not using PythonQT")
    }

    system(ldconfig -p | grep libhdf5_cpp) {
        build_pass:message("Found libHDF5_cpp in system")
        build_pass:message("Compiling with HDF5 support")
        DEFINES += USEHDF5
        LIBS += -lhdf5_cpp

        # This is kind of a hack, on newer versions of Ubuntu (where we choose to use Qt5, the HDF libraries and headers har stored elswhere and have slightly different names
        isEqual(QT_MAJOR_VERSION, 5){
            INCLUDEPATH += /usr/include/hdf5/serial
            LIBS += -lhdf5_serial
        } else {
            # Includepath not needed here, H5Cpp.h file should reside directly in /usr/include (searched by default)
            LIBS += -lhdf5
        }

        !build_pass:message("Compiling with HDF5 support")
    } else {
        !build_pass:message("Compiling without HDF5 support")
    }

    # This will add runtime .so search paths to the executable, by using $ORIGIN these paths will be relative the executable (regardless of working dir, VERY useful)
    # The QMAKE_LFLAGS_RPATH and QMAKE_RPATHDIR does not seem to be able to handle the $$ORIGIN stuff, adding manually to LFLAGS
    !macx:QMAKE_LFLAGS *= -Wl,-rpath,\'\$$ORIGIN/./\'
     macx:QMAKE_RPATHDIR *= $${PWD}/../bin

    # Get the git commit timestamp
     timestamp=$$system($${PWD}/../getGitInfo.sh date.time $${PWD})
     DEFINES *= "HOPSANGUI_COMMIT_TIMESTAMP=$${timestamp}"

}
win32 {
    #DEFINES += STATICCORE

    # Set Python paths
    contains(DEFINES, USEPYTHONQT) {
        !build_pass:message("Looking for Python include and lib paths since USEPYTHONQT is defined")
        PYTHON_DEFAULT_PATHS *= c:/Python27
        PYTHON_PATH = $$selectPath($$(PYTHON_PATH), $$PYTHON_DEFAULT_PATHS, "python")
        INCLUDEPATH += $${PYTHON_PATH}/include
        LIBS += -L$${PYTHON_PATH}/libs
    } else {
       !build_pass: message("Not looking for Python since we are not using PythonQT")
    }

    # Set hdf5 paths
    d = $$setHDF5PathInfo($$(HDF5_PATH), $$DESTDIR)
    !isEmpty(d){
        DEFINES *= USEHDF5
        LIBS *= $$magic_hopsan_libpath
        INCLUDEPATH *= $$magic_hopsan_includepath
        QMAKE_POST_LINK *= $$magic_hopsan_qmake_post_link
        !build_pass:message("Compiling with HDF5 support")
    } else {
        !build_pass:message("Compiling without HDF5 support")
    }


    # Enable auto-import
    QMAKE_LFLAGS += -Wl,--enable-auto-import

    # Activate large address aware, to access more the 2GB virtual RAM (for 32-bit version)
    !contains(QMAKE_HOST.arch, x86_64){
        QMAKE_LFLAGS += -Wl,--large-address-aware
    }

    # Activate console output of cout for debug builds (you also need to run in console but hopsan seems slow)
#    CONFIG(debug, debug|release) {
        CONFIG += console
#    }

    # Get the svn revision in here if script succeed, Note! Checking return code does not work, so we compare version instead
#    rev = $$system($${PWD}/../getSvnRevision.bat)
#    message(GUI revision: $${rev})
#    !equals(rev, "RevisionInformationNotFound") {
#        DEFINES *= "HOPSANGUISVNREVISION=$${rev}"
#    }
    # Get the git commit timestamp
     timestamp=$$system($${PWD}/../getGitInfo.bat date.time $${PWD})
     DEFINES *= "HOPSANGUI_COMMIT_TIMESTAMP=$${timestamp}"
}
macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9
}

#Debug output
#message(GUI Includepath is $$INCLUDEPATH)
#message(GUI Libs is $${LIBS})
#message(GUI QMAKE_POST_LINK $$QMAKE_POST_LINK)

RESOURCES += \  
    Resources.qrc

# Release compile only, will add the application icon
RC_FILE = HOPSANGUI.rc

# -------------------------------------------------
# Project files
# -------------------------------------------------
SOURCES += main.cpp \
    MainWindow.cpp \
    Widgets/ProjectTabWidget.cpp \
    GUIConnector.cpp \
    GUIPort.cpp \
    Widgets/MessageWidget.cpp \
    InitializationThread.cpp \
    Dialogs/OptionsDialog.cpp \
    UndoStack.cpp \
    GraphicsView.cpp \
    ProgressBarThread.cpp \
    GUIPortAppearance.cpp \
    GUIConnectorAppearance.cpp \
    Widgets/SystemParametersWidget.cpp \
    PlotWindow.cpp \
    PyWrapperClasses.cpp \
    GUIObjects/GUIWidgets.cpp \
    GUIObjects/GUISystem.cpp \
    GUIObjects/GUIObject.cpp \
    GUIObjects/GUIModelObjectAppearance.cpp \
    GUIObjects/GUIModelObject.cpp \
    GUIObjects/GUIContainerObject.cpp \
    GUIObjects/GUIComponent.cpp \
    Utilities/XMLUtilities.cpp \
    Utilities/GUIUtilities.cpp \
    Configuration.cpp \
    CopyStack.cpp \
    Dialogs/AboutDialog.cpp \
    CoreAccess.cpp \
    Widgets/UndoWidget.cpp \
    Widgets/QuickNavigationWidget.cpp \
    GUIObjects/GUIContainerPort.cpp \
    Dialogs/ContainerPortPropertiesDialog.cpp \
    Dialogs/HelpDialog.cpp \
    Dependencies/BarChartPlotter/plotterbase.cpp \
    Dependencies/BarChartPlotter/barchartplotter.cpp \
    Dependencies/BarChartPlotter/axisbase.cpp \
    Dialogs/OptimizationDialog.cpp \
    Dialogs/SensitivityAnalysisDialog.cpp \
    Dialogs/MovePortsDialog.cpp \
    loadFunctions.cpp \
    Widgets/WelcomeWidget.cpp \
    Widgets/AnimationWidget.cpp \
    GUIObjects/AnimatedComponent.cpp \
    AnimatedConnector.cpp \
    Dialogs/AnimatedIconPropertiesDialog.cpp \
    SimulationThreadHandler.cpp \
    Widgets/HcomWidget.cpp \
    LogDataHandler2.cpp \
    PlotTab.cpp \
    PlotCurve.cpp \
    PlotHandler.cpp \
    LogVariable.cpp \
    CachableDataVector.cpp \
    DesktopHandler.cpp \
    Dialogs/ComponentPropertiesDialog3.cpp \
    Dialogs/EditComponentDialog.cpp \
    Widgets/DebuggerWidget.cpp \
    HcomHandler.cpp \
    Widgets/HVCWidget.cpp \
    ModelHandler.cpp \
    Widgets/ModelWidget.cpp \
    OptimizationHandler.cpp \
    Utilities/HighlightingUtilities.cpp \
    Widgets/DataExplorer.cpp \
    Widgets/LibraryWidget.cpp \
    LibraryHandler.cpp \
    UnitScale.cpp \
    PlotArea.cpp \
    Utilities/HelpPopUpWidget.cpp \
    PlotCurveControlBox.cpp \
    MessageHandler.cpp \
    Widgets/FindWidget.cpp \
    ModelicaLibrary.cpp \
    Widgets/ModelicaEditor.cpp \
    Widgets/PlotWidget2.cpp \
    Utilities/IndexIntervalCollection.cpp \
    LogDataGeneration.cpp \
    RemoteCoreAccess.cpp \
    RemoteSimulationUtils.cpp \
    Dialogs/LicenseDialog.cpp \
    Widgets/TimeOffsetWidget.cpp \
    Dialogs/NumHopScriptDialog.cpp

HEADERS += MainWindow.h \
    Widgets/ProjectTabWidget.h \
    GUIConnector.h \
    GUIPort.h \
    Widgets/MessageWidget.h \
    InitializationThread.h \
    version_gui.h \
    Dialogs/OptionsDialog.h \
    UndoStack.h \
    CoreAccess.h \
    GraphicsView.h \
    ProgressBarThread.h \
    common.h \
    CoreAccess.h \
    GUIPortAppearance.h \
    GUIConnectorAppearance.h \
    Widgets/SystemParametersWidget.h \
    PlotWindow.h \
    PyWrapperClasses.h \
    GUIObjects/GUIWidgets.h \
    GUIObjects/GUISystem.h \
    GUIObjects/GUIObject.h \
    GUIObjects/GUIModelObjectAppearance.h \
    GUIObjects/GUIModelObject.h \
    GUIObjects/GUIContainerObject.h \
    GUIObjects/GUIComponent.h \
    Utilities/XMLUtilities.h \
    Utilities/GUIUtilities.h \
    Configuration.h \
    CopyStack.h \
    Dialogs/AboutDialog.h \
    Widgets/UndoWidget.h \
    Widgets/QuickNavigationWidget.h \
    GUIObjects/GUIContainerPort.h \
    Dialogs/ContainerPortPropertiesDialog.h \
    Dialogs/HelpDialog.h \
    Dependencies/BarChartPlotter/plotterbase.h \
    Dependencies/BarChartPlotter/barchartplotter.h \
    Dependencies/BarChartPlotter/axisbase.h \
    Dialogs/OptimizationDialog.h \
    Dialogs/SensitivityAnalysisDialog.h \
    Dialogs/MovePortsDialog.h \
    loadFunctions.h \
    Widgets/WelcomeWidget.h \
    Widgets/AnimationWidget.h \
    GUIObjects/AnimatedComponent.h \
    AnimatedConnector.h \
    Dialogs/AnimatedIconPropertiesDialog.h \
    SimulationThreadHandler.h \
    Widgets/HcomWidget.h \
    LogDataHandler2.h \
    PlotTab.h \
    PlotCurve.h \
    PlotHandler.h \
    LogVariable.h \
    CachableDataVector.h \
    DesktopHandler.h \
    Dialogs/ComponentPropertiesDialog3.h \
    Dialogs/EditComponentDialog.h \
    Widgets/DebuggerWidget.h \
    HcomHandler.h \
    Widgets/HVCWidget.h \
    ModelHandler.h \
    Widgets/ModelWidget.h \
    OptimizationHandler.h \
    Utilities/HighlightingUtilities.h \
    Widgets/DataExplorer.h \
    Widgets/LibraryWidget.h \
    LibraryHandler.h \
    global.h \
    UnitScale.h \
    PlotArea.h \
    Utilities/HelpPopUpWidget.h \
    PlotCurveControlBox.h \
    MessageHandler.h \
    Widgets/FindWidget.h \
    ModelicaLibrary.h \
    Widgets/ModelicaEditor.h \
    GraphicsViewPort.h \
    Widgets/PlotWidget2.h \
    Utilities/IndexIntervalCollection.h \
    LogDataGeneration.h \
    RemoteCoreAccess.h \
    RemoteSimulationUtils.h \
    Dialogs/LicenseDialog.h \
    Utilities/EventFilters.h \
    Widgets/TimeOffsetWidget.h \
    Dialogs/NumHopScriptDialog.h \
    PlotCurveStyle.h

    contains(DEFINES, USEPYTHONQT) {
        SOURCES += Widgets/PyDockWidget.cpp
        HEADERS += Widgets/PyDockWidget.h
    }

OTHER_FILES += \
    ../hopsandefaults \
    HopsanGuiBuild.prf
