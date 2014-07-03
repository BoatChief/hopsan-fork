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

//$Id$

#ifndef SOCKETIOTESTCOMPONENT_HPP_INCLUDED
#define SOCKETIOTESTCOMPONENT_HPP_INCLUDED

#include "ComponentEssentials.h"
#include "socketutility.h"
#include <iostream>

#define ACK 0x01
#define NACK 0x02

namespace hopsan {

    class SocketIOTestComponent : public ComponentSignal
    {
    private:
        HString mOtherIp, mOtherPort, mThisPort;
        SocketUtility mSocketUtility;
        int mMasterSlave, mRequireACK, mInitTimout, mSimTimout;
        double *mpInput, *mpOutput;

    public:
        static Component *Creator()
        {
            return new SocketIOTestComponent();
        }

        void configure()
        {
            addConstant("other_ip", "The ip adress", "", "127.0.0.1", mOtherIp);
            addConstant("other_port", "Write port", "", "30000", mOtherPort);
            addConstant("this_port", "Read port", "", "30001", mThisPort);
            std::vector<HString> conds;
            conds.push_back("Master");
            conds.push_back("Slave");
            addConditionalConstant("masl", "Master or Slave", conds, mMasterSlave);
            conds.clear();
            conds.push_back("True");
            conds.push_back("False");
            addConditionalConstant("requireAck", "Require or Send ACK", conds, mRequireACK);
            addConstant("initTimeout", "The timeout during initialization", "ms", 5000, mInitTimout);
            addConstant("simTimeout", "The timeout during simulation", "ms", 1000, mSimTimout);

            addInputVariable("in", "Value input", "", 0, &mpInput);
            addOutputVariable("out", "Value input", "", &mpOutput);

            addReadPort("sortIn", "NodeSignal", "Sorting port, value has no effect", Port::NotRequired);
            addWritePort("sortOut", "NodeSignal", "Sorting port, value has no effect", Port::NotRequired);

            mSocketUtility.setSleepUS(2);
        }

        bool preInitialize()
        {
            if (!mSocketUtility.openSocket(mOtherIp.c_str(), mOtherPort.c_str(), mThisPort.c_str()))
            {
                addErrorMessage(mSocketUtility.getErrorString().c_str());
                return false;
            }
            return true;
        }

        void initialize()
        {
            // Master
            if (mMasterSlave == 0)
            {
                // Write init message
                if (mSocketUtility.writeSocket("Initialize") == 0)
                {
                    addErrorMessage(mSocketUtility.getErrorString().c_str());
                }

                // Wait for ACK or NACK
                char ack;
                if (mSocketUtility.readSocket(ack, mInitTimout) == 0)
                {
                    addWarningMessage(HString("Failed to read ACK! ")+mSocketUtility.getErrorString().c_str());
                }
                if ( (mRequireACK == 0) && (ack != ACK) )
                {
                    stopSimulation();
                    return;
                }

            }
            // Slave
            else if (mMasterSlave == 1)
            {
                // Wait for startup message
                std::string input;
                if (mSocketUtility.readSocket(input, 10, mInitTimout) == 0)
                {
                    addWarningMessage(HString("Failed to read startup message! "));
                }

                // Send ACK or NACK
                if (input == "Initialize" && mRequireACK == 0 )
                {
                    if (mSocketUtility.writeSocket(char(ACK)) == 0)
                    {
                        addErrorMessage(mSocketUtility.getErrorString().c_str());
                    }
                }
            }
        }


        void simulateOneTimestep()
        {
            // Master
            if (mMasterSlave == 0)
            {
                // Write input to socket
                if (mSocketUtility.writeSocket(*mpInput) == 0)
                {
                    addErrorMessage(mSocketUtility.getErrorString().c_str());
                }

                // Wait for ack
                if (mRequireACK == 0)
                {
                    char ack;
                    if (mSocketUtility.readSocket(ack, mSimTimout) == 0 || ack != ACK )
                    {
                        addErrorMessage(HString("Failed to read ACK message! ")+mSocketUtility.getErrorString().c_str());
                        stopSimulation();
                        return;
                    }
                }
            }
            // Slave
            else if (mMasterSlave == 1)
            {
                // Read input from socket
                if (mSocketUtility.readSocket(*mpOutput, mSimTimout) == 0)
                {
                    addWarningMessage(HString("Failed to read double from socket before timeout!"));
                    stopSimulation();
                    return;
                }

                // Send ACK or NACK
                if (mRequireACK == 0)
                {
                    if (mSocketUtility.writeSocket(char(ACK)) == 0)
                    {
                        addErrorMessage(mSocketUtility.getErrorString().c_str());
                    }
                }
            }
        }

        void finalize()
        {
            mSocketUtility.closeSocket();
        }

        bool isExperimental() const
        {
            return true;
        }
    };
}

#endif