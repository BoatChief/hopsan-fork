/*-----------------------------------------------------------------------------
 This source file is a part of Hopsan

 Copyright (c) 2009 to present year, Hopsan Group

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 For license details and information about the Hopsan Group see the files
 GPLv3 and HOPSANGROUP in the Hopsan source code root directory

 For author and contributor information see the AUTHORS file
-----------------------------------------------------------------------------*/

//!
//! @file   RemoteCoreAccess.h
//! @author Peter Nordin <peter.nordin@liu.se>
//! @date   2015-05-11
//!
//! @brief Contains the HopsanCore Qt API classes for communication with the Remote HopsanCore
//!
//$Id$

#ifndef REMOTECOREACCESS_H
#define REMOTECOREACCESS_H

#include <QString>
#include <QMultiMap>
#include <QSharedPointer>
#include <QStringList>

#ifdef USEZMQ
class RemoteHopsanClient;

class RemoteCoreAddressHandler
{
private:
    typedef struct
    {
        QString addr;
        double speed=-1;
        int nSlots=0;
        int nOpenSlots=0;
        bool mResponding=false;
    }ServerInfoT;

    QString mHopsanAddressServerIP, mHopsanAddressServerPort;
    RemoteHopsanClient *mpRemoteHopsanClient=0;

    //! @todo shared pointer to info maybe
    QMap<QString, ServerInfoT> mAvailableServers;
    QMultiMap<double, QString> mServerSpeedMap;
    QList<QString> mNotRespondingServers;

    void requestServerInfo(QString address);
    void removeNotRespondingServers();

public:
    RemoteCoreAddressHandler();
    ~RemoteCoreAddressHandler();
    void setHopsanAddressServer( QString ip, QString port );
    void setHopsanAddressServer( QString ip_port );
    QString getAddressAndPort() const;

    bool isConnected();

    bool connect();
    void disconnect();

    QList<QString> requestAvailableServers();
    //QList<QString> requestAvailableServers(int nOpenSlots);

    QString getBestAvailableServer(int nRequiredSlots, const QStringList &rExcludeList=QStringList());
    QList<QString> getMatchingAvailableServers(double requiredSpeed, int nRequiredSlots, const QStringList &rExcludeList=QStringList());
    void getMaxNumSlots(int &rMaxNumSlots, int &rNumServers);
};

typedef QSharedPointer<RemoteCoreAddressHandler> SharedRemoteCoreAddressHandlerT;
SharedRemoteCoreAddressHandlerT getSharedRemoteCoreAddressHandler();

class RemoteCoreSimulationHandler
{
private:
    QString mRemoteServerAddress,   mRemoteServerPort;
    RemoteHopsanClient *mpRemoteHopsanClient=0;
    int mNumThreads=1;

public:
    RemoteCoreSimulationHandler();
    ~RemoteCoreSimulationHandler();

    void setHopsanServer(QString ip, QString port );
    void setHopsanServer(QString ip_port );
    QString getHopsanServerAddress() const;
    void setNumThreads(int nThreads);
    int numThreads() const;

    bool connect();
    void disconnect();

    bool connectServer();
    bool connectWorker();
    void disconnectServer();
    void disconnectWorker();

    bool loadModel(QString hmfModelFile);
    bool loadModelStr(QString hmfStr);
    bool simulateModel_blocking(double *pProgress);
    bool simulateModel_nonblocking();
    bool abortSimulation();

    bool benchmarkModel_blocking(const QString &rModel, const int nThreads, double &rSimTime);


    bool requestSimulationProgress(double *pProgress);

    bool getCoreMessages(QVector<QString> &rTypes, QVector<QString> &rTags, QVector<QString> &rMessages, bool includeDebug=true);

    bool getLogData(std::vector<std::string> *pNames, std::vector<double> *pData);

    QString getLastError() const;
};

typedef QSharedPointer<RemoteCoreSimulationHandler> SharedRemoteCoreSimulationHandlerT;
#endif

#endif // REMOTECOREACCESS_H