#ifndef HYDRAULICPRESSURECONTROLVALVEG_HPP_INCLUDED
#define HYDRAULICPRESSURECONTROLVALVEG_HPP_INCLUDED

#include <iostream>
#include "ComponentEssentials.h"
#include "ComponentUtilities.h"
#include "math.h"

//!
//! @file HydraulicPressureControlValveG.hpp
//! @author Petter Krus <petter.krus@liu.se>
//! @date Fri 28 Jun 2013 13:04:36
//! @brief A hydraulic pressure relief valve based on geometry
//! @ingroup HydraulicComponents
//!
//==This code has been autogenerated using Compgen==
//from 
/*{, C:, HopsanTrunk, HOPSAN++, CompgenModels}/HydraulicComponents.nb*/

using namespace hopsan;

class HydraulicPressureControlValveG : public ComponentQ
{
private:
     double rho;
     double visc;
     double Dv;
     double frac;
     double Bv;
     double Xvmax;
     double Cq;
     double phi;
     double ks;
     double p0;
     Port *mpP1;
     Port *mpP2;
     Port *mpP3;
     Port *mpP4;
     double delayParts1[9];
     double delayParts2[9];
     double delayParts3[9];
     double delayParts4[9];
     Matrix jacobianMatrix;
     Vec systemEquations;
     Matrix delayedPart;
     int i;
     int iter;
     int mNoiter;
     double jsyseqnweight[4];
     int order[4];
     int mNstep;
     //Port P1 variable
     double p1;
     double q1;
     double T1;
     double dE1;
     double c1;
     double Zc1;
     //Port P2 variable
     double p2;
     double q2;
     double T2;
     double dE2;
     double c2;
     double Zc2;
     //Port P3 variable
     double p3;
     double q3;
     double T3;
     double dE3;
     double c3;
     double Zc3;
     //Port P4 variable
     double p4;
     double q4;
     double T4;
     double dE4;
     double c4;
     double Zc4;
//==This code has been autogenerated using Compgen==
     //inputVariables
     double pref;
     //outputVariables
     double xv;
     //LocalExpressions variables
     double Av;
     double w;
     //Expressions variables
     //Port P1 pointer
     double *mpND_p1;
     double *mpND_q1;
     double *mpND_T1;
     double *mpND_dE1;
     double *mpND_c1;
     double *mpND_Zc1;
     //Port P2 pointer
     double *mpND_p2;
     double *mpND_q2;
     double *mpND_T2;
     double *mpND_dE2;
     double *mpND_c2;
     double *mpND_Zc2;
     //Port P3 pointer
     double *mpND_p3;
     double *mpND_q3;
     double *mpND_T3;
     double *mpND_dE3;
     double *mpND_c3;
     double *mpND_Zc3;
     //Port P4 pointer
     double *mpND_p4;
     double *mpND_q4;
     double *mpND_T4;
     double *mpND_dE4;
     double *mpND_c4;
     double *mpND_Zc4;
     //Delay declarations
//==This code has been autogenerated using Compgen==
     //inputVariables pointers
     double *mppref;
     //inputParameters pointers
     double *mprho;
     double *mpvisc;
     double *mpDv;
     double *mpfrac;
     double *mpBv;
     double *mpXvmax;
     double *mpCq;
     double *mpphi;
     double *mpks;
     double *mpp0;
     //outputVariables pointers
     double *mpxv;
     Delay mDelayedPart10;
     Delay mDelayedPart11;
     Delay mDelayedPart20;
     EquationSystemSolver *mpSolver;

public:
     static Component *Creator()
     {
        return new HydraulicPressureControlValveG();
     }

     void configure()
     {
//==This code has been autogenerated using Compgen==

        mNstep=9;
        jacobianMatrix.create(4,4);
        systemEquations.create(4);
        delayedPart.create(5,6);
        mNoiter=2;
        jsyseqnweight[0]=1;
        jsyseqnweight[1]=0.67;
        jsyseqnweight[2]=0.5;
        jsyseqnweight[3]=0.5;


        //Add ports to the component
        mpP1=addPowerPort("P1","NodeHydraulic");
        mpP2=addPowerPort("P2","NodeHydraulic");
        mpP3=addPowerPort("P3","NodeHydraulic");
        mpP4=addPowerPort("P4","NodeHydraulic");
        //Add inputVariables to the component
            addInputVariable("pref","Reference pressure","Pa",1.e6,&mppref);

        //Add inputParammeters to the component
            addInputVariable("rho", "oil density", "kg/m3", 860.,&mprho);
            addInputVariable("visc", "viscosity ", "Ns/m2", 0.03,&mpvisc);
            addInputVariable("Dv", "Spool diameter", "m", 0.03,&mpDv);
            addInputVariable("frac", "Fraction of spool opening", "", \
0.1,&mpfrac);
            addInputVariable("Bv", "Damping", "N/(m s)", 100.,&mpBv);
            addInputVariable("Xvmax", "Max spool displacement", "m", \
0.03,&mpXvmax);
            addInputVariable("Cq", "Flow coefficient", " ", 0.67,&mpCq);
            addInputVariable("phi", "Stream angle", "rad", 0.03,&mpphi);
            addInputVariable("ks", "Spring constant", "N/m", 100.,&mpks);
            addInputVariable("p0", "Turbulent pressure trans.", "Pa", \
100000.,&mpp0);
        //Add outputVariables to the component
            addOutputVariable("xv","Spool position","m",0.,&mpxv);

//==This code has been autogenerated using Compgen==
        //Add constantParameters
        mpSolver = new EquationSystemSolver(this,4);
     }

    void initialize()
     {
        //Read port variable pointers from nodes
        //Port P1
        mpND_p1=getSafeNodeDataPtr(mpP1, NodeHydraulic::Pressure);
        mpND_q1=getSafeNodeDataPtr(mpP1, NodeHydraulic::Flow);
        mpND_T1=getSafeNodeDataPtr(mpP1, NodeHydraulic::Temperature);
        mpND_dE1=getSafeNodeDataPtr(mpP1, NodeHydraulic::HeatFlow);
        mpND_c1=getSafeNodeDataPtr(mpP1, NodeHydraulic::WaveVariable);
        mpND_Zc1=getSafeNodeDataPtr(mpP1, NodeHydraulic::CharImpedance);
        //Port P2
        mpND_p2=getSafeNodeDataPtr(mpP2, NodeHydraulic::Pressure);
        mpND_q2=getSafeNodeDataPtr(mpP2, NodeHydraulic::Flow);
        mpND_T2=getSafeNodeDataPtr(mpP2, NodeHydraulic::Temperature);
        mpND_dE2=getSafeNodeDataPtr(mpP2, NodeHydraulic::HeatFlow);
        mpND_c2=getSafeNodeDataPtr(mpP2, NodeHydraulic::WaveVariable);
        mpND_Zc2=getSafeNodeDataPtr(mpP2, NodeHydraulic::CharImpedance);
        //Port P3
        mpND_p3=getSafeNodeDataPtr(mpP3, NodeHydraulic::Pressure);
        mpND_q3=getSafeNodeDataPtr(mpP3, NodeHydraulic::Flow);
        mpND_T3=getSafeNodeDataPtr(mpP3, NodeHydraulic::Temperature);
        mpND_dE3=getSafeNodeDataPtr(mpP3, NodeHydraulic::HeatFlow);
        mpND_c3=getSafeNodeDataPtr(mpP3, NodeHydraulic::WaveVariable);
        mpND_Zc3=getSafeNodeDataPtr(mpP3, NodeHydraulic::CharImpedance);
        //Port P4
        mpND_p4=getSafeNodeDataPtr(mpP4, NodeHydraulic::Pressure);
        mpND_q4=getSafeNodeDataPtr(mpP4, NodeHydraulic::Flow);
        mpND_T4=getSafeNodeDataPtr(mpP4, NodeHydraulic::Temperature);
        mpND_dE4=getSafeNodeDataPtr(mpP4, NodeHydraulic::HeatFlow);
        mpND_c4=getSafeNodeDataPtr(mpP4, NodeHydraulic::WaveVariable);
        mpND_Zc4=getSafeNodeDataPtr(mpP4, NodeHydraulic::CharImpedance);

        //Read variables from nodes
        //Port P1
        p1 = (*mpND_p1);
        q1 = (*mpND_q1);
        T1 = (*mpND_T1);
        dE1 = (*mpND_dE1);
        c1 = (*mpND_c1);
        Zc1 = (*mpND_Zc1);
        //Port P2
        p2 = (*mpND_p2);
        q2 = (*mpND_q2);
        T2 = (*mpND_T2);
        dE2 = (*mpND_dE2);
        c2 = (*mpND_c2);
        Zc2 = (*mpND_Zc2);
        //Port P3
        p3 = (*mpND_p3);
        q3 = (*mpND_q3);
        T3 = (*mpND_T3);
        dE3 = (*mpND_dE3);
        c3 = (*mpND_c3);
        Zc3 = (*mpND_Zc3);
        //Port P4
        p4 = (*mpND_p4);
        q4 = (*mpND_q4);
        T4 = (*mpND_T4);
        dE4 = (*mpND_dE4);
        c4 = (*mpND_c4);
        Zc4 = (*mpND_Zc4);

        //Read inputVariables from nodes
        pref = (*mppref);

        //Read inputParameters from nodes
        rho = (*mprho);
        visc = (*mpvisc);
        Dv = (*mpDv);
        frac = (*mpfrac);
        Bv = (*mpBv);
        Xvmax = (*mpXvmax);
        Cq = (*mpCq);
        phi = (*mpphi);
        ks = (*mpks);
        p0 = (*mpp0);

        //Read outputVariables from nodes
        xv = (*mpxv);

//==This code has been autogenerated using Compgen==

        //LocalExpressions
        Av = 0.785398*Power(Dv,2);
        w = pi*Dv*frac*Sin(phi);
        p3 = c3;
        p4 = c4;

        //Initialize delays
        delayParts1[1] = (-(Av*mTimestep*p3) + Av*mTimestep*p4 + \
Av*mTimestep*pref - 2*Bv*xv + ks*mTimestep*xv + \
2*Cq*mTimestep*p1*w*xv*Cos(phi) - 2*Cq*mTimestep*p2*w*xv*Cos(phi))/(2*Bv + \
ks*mTimestep + 2*Cq*mTimestep*p1*w*Cos(phi) - 2*Cq*mTimestep*p2*w*Cos(phi));
        mDelayedPart11.initialize(mNstep,delayParts1[1]);

        delayedPart[1][1] = delayParts1[1];
        delayedPart[2][1] = delayParts2[1];
        delayedPart[3][1] = delayParts3[1];
        delayedPart[4][1] = delayParts4[1];
     }
    void simulateOneTimestep()
     {
        Vec stateVar(4);
        Vec stateVark(4);
        Vec deltaStateVar(4);

        //Read variables from nodes
        //Port P1
        T1 = (*mpND_T1);
        c1 = (*mpND_c1);
        Zc1 = (*mpND_Zc1);
        //Port P2
        T2 = (*mpND_T2);
        c2 = (*mpND_c2);
        Zc2 = (*mpND_Zc2);
        //Port P3
        T3 = (*mpND_T3);
        c3 = (*mpND_c3);
        Zc3 = (*mpND_Zc3);
        //Port P4
        T4 = (*mpND_T4);
        c4 = (*mpND_c4);
        Zc4 = (*mpND_Zc4);

        //Read inputVariables from nodes
        pref = (*mppref);

        //LocalExpressions
        Av = 0.785398*Power(Dv,2);
        w = pi*Dv*frac*Sin(phi);
        p3 = c3;
        p4 = c4;

        //Initializing variable vector for Newton-Raphson
        stateVark[0] = xv;
        stateVark[1] = q2;
        stateVark[2] = p1;
        stateVark[3] = p2;

        //Iterative solution using Newton-Rapshson
        for(iter=1;iter<=mNoiter;iter++)
        {
         //PressureControlValveG
         //Differential-algebraic system of equation parts

          //Assemble differential-algebraic equations
          systemEquations[0] =xv - limit((Av*mTimestep*(p3 - p4 - \
pref))/(2*Bv + ks*mTimestep + 2*Cq*mTimestep*(p1 - p2)*w*Cos(phi)) - \
delayedPart[1][1],0.,Xvmax);
          systemEquations[1] =q2 - \
1.4142135623730951*Cq*Sqrt(1/rho)*w*xv*signedSquareL(p1 - p2,p0);
          systemEquations[2] =p1 - lowLimit(c1 - q2*Zc1*onPositive(p1),0);
          systemEquations[3] =p2 - lowLimit(c2 + q2*Zc2*onPositive(p2),0);

          //Jacobian matrix
          jacobianMatrix[0][0] = 1;
          jacobianMatrix[0][1] = 0;
          jacobianMatrix[0][2] = (2*Av*Cq*Power(mTimestep,2)*(p3 - p4 - \
pref)*w*Cos(phi)*dxLimit((Av*mTimestep*(p3 - p4 - pref))/(2*Bv + ks*mTimestep \
+ 2*Cq*mTimestep*(p1 - p2)*w*Cos(phi)) - \
delayedPart[1][1],0.,Xvmax))/Power(2*Bv + ks*mTimestep + 2*Cq*mTimestep*(p1 - \
p2)*w*Cos(phi),2);
          jacobianMatrix[0][3] = (-2*Av*Cq*Power(mTimestep,2)*(p3 - p4 - \
pref)*w*Cos(phi)*dxLimit((Av*mTimestep*(p3 - p4 - pref))/(2*Bv + ks*mTimestep \
+ 2*Cq*mTimestep*(p1 - p2)*w*Cos(phi)) - \
delayedPart[1][1],0.,Xvmax))/Power(2*Bv + ks*mTimestep + 2*Cq*mTimestep*(p1 - \
p2)*w*Cos(phi),2);
          jacobianMatrix[1][0] = \
-1.4142135623730951*Cq*Sqrt(1/rho)*w*signedSquareL(p1 - p2,p0);
          jacobianMatrix[1][1] = 1;
          jacobianMatrix[1][2] = \
-1.4142135623730951*Cq*Sqrt(1/rho)*w*xv*dxSignedSquareL(p1 - p2,p0);
          jacobianMatrix[1][3] = \
1.4142135623730951*Cq*Sqrt(1/rho)*w*xv*dxSignedSquareL(p1 - p2,p0);
          jacobianMatrix[2][0] = 0;
          jacobianMatrix[2][1] = Zc1*dxLowLimit(c1 - \
q2*Zc1*onPositive(p1),0)*onPositive(p1);
          jacobianMatrix[2][2] = 1;
          jacobianMatrix[2][3] = 0;
          jacobianMatrix[3][0] = 0;
          jacobianMatrix[3][1] = -(Zc2*dxLowLimit(c2 + \
q2*Zc2*onPositive(p2),0)*onPositive(p2));
          jacobianMatrix[3][2] = 0;
          jacobianMatrix[3][3] = 1;
//==This code has been autogenerated using Compgen==

          //Solving equation using LU-faktorisation
          mpSolver->solve(jacobianMatrix, systemEquations, stateVark, iter);
          xv=stateVark[0];
          q2=stateVark[1];
          p1=stateVark[2];
          p2=stateVark[3];
          //Expressions
          q1 = -q2;
          q3 = 0.;
          q4 = 0.;
        }

        //Calculate the delayed parts
        delayParts1[1] = (-(Av*mTimestep*p3) + Av*mTimestep*p4 + \
Av*mTimestep*pref - 2*Bv*xv + ks*mTimestep*xv + \
2*Cq*mTimestep*p1*w*xv*Cos(phi) - 2*Cq*mTimestep*p2*w*xv*Cos(phi))/(2*Bv + \
ks*mTimestep + 2*Cq*mTimestep*p1*w*Cos(phi) - 2*Cq*mTimestep*p2*w*Cos(phi));

        delayedPart[1][1] = delayParts1[1];
        delayedPart[2][1] = delayParts2[1];
        delayedPart[3][1] = delayParts3[1];
        delayedPart[4][1] = delayParts4[1];

        //Write new values to nodes
        //Port P1
        (*mpND_p1)=p1;
        (*mpND_q1)=q1;
        (*mpND_dE1)=dE1;
        //Port P2
        (*mpND_p2)=p2;
        (*mpND_q2)=q2;
        (*mpND_dE2)=dE2;
        //Port P3
        (*mpND_p3)=p3;
        (*mpND_q3)=q3;
        (*mpND_dE3)=dE3;
        //Port P4
        (*mpND_p4)=p4;
        (*mpND_q4)=q4;
        (*mpND_dE4)=dE4;
        //outputVariables
        (*mpxv)=xv;

        //Update the delayed variabels
        mDelayedPart11.update(delayParts1[1]);

     }
    void deconfigure()
    {
        delete mpSolver;
    }
};
#endif // HYDRAULICPRESSURECONTROLVALVEG_HPP_INCLUDED
