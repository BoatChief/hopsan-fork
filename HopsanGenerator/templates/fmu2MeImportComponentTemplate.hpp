/*-----------------------------------------------------------------------------

 Copyright 2017 Hopsan Group

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.


 The full license is available in the file LICENSE.
 For details about the 'Hopsan Group' or information about Authors and
 Contributors see the HOPSANGROUP and AUTHORS files that are located in
 the Hopsan source code root directory.

-----------------------------------------------------------------------------*/

#ifndef <<<headerguard>>>
#define <<<headerguard>>>

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stddef.h>
#include <sstream>
#include <iostream>

#include "ComponentEssentials.h"
#include "ComponentUtilities.h"

#include "FMI/fmi_import_context.h"
#include "FMI1/fmi1_import.h"
#include "FMI2/fmi2_import.h"
#include "JM/jm_portability.h"


void localLogger(jm_callbacks *c, jm_string module, jm_log_level_enu_t log_level, jm_string message) { }

namespace hopsan {

//!
//! @brief
//! @ingroup HydraulicComponents
//!
class <<<className>>> : public ComponentSignal
{

private:
    <<<localvars>>>

    fmi2_callback_functions_t callBackFunctions;
    jm_callbacks callbacks;
    fmi_import_context_t* context;
    fmi_version_enu_t version;
    jm_status_enu_t status;
    fmi2_status_t fmistatus;
    int k;

    double tlast;

    fmi2_real_t hcur;
    size_t n_states;
    size_t n_event_indicators;
    fmi2_real_t* states;
    fmi2_real_t* states_der;
    fmi2_real_t* event_indicators;
    fmi2_real_t* event_indicators_prev;
    fmi2_boolean_t callEventUpdate;
    fmi2_boolean_t terminateSimulation;
    fmi2_boolean_t toleranceControlled;
    fmi2_real_t relativeTolerance;
    fmi2_event_info_t eventInfo;

    fmi2_import_t* fmu;

public:
    static Component *Creator()
    {
        return new <<<className>>>();
    }

    void configure()
    {
        //Add constants
        <<<addconstants>>>

        //Add input variables
        <<<addinputs>>>

        //Add output variables
        <<<addoutputs>>>

        //Add ports
        <<<addports>>>
    }

    void initialize()
    {
        <<<setnodedatapointers>>>

        addInfoMessage("Initializing FMU 2.0 import");

        fmi2_boolean_t terminateSimulation = fmi2_false;
        fmi2_boolean_t toleranceControlled = fmi2_true;
        fmi2_real_t relativeTolerance = 0.001;

        const char* FMUPath = "<<<fmupath>>>";
        const char* tmpPath = "<<<temppath>>>";

        callbacks.malloc = malloc;
        callbacks.calloc = calloc;
        callbacks.realloc = realloc;
        callbacks.free = free;
        callbacks.logger = localLogger;
        callbacks.log_level = jm_log_level_debug;
        callbacks.context = 0;

        context = fmi_import_allocate_context(&callbacks);

        version = fmi_import_get_fmi_version(context, FMUPath, tmpPath);

        if(version != fmi_version_2_0_enu)
        {
            addErrorMessage("The code only supports version 2.0\n");
            stopSimulation();
            return;
        }

        fmu = fmi2_import_parse_xml(context, tmpPath, 0);

        if(!fmu)
        {
            addErrorMessage("Error parsing XML, exiting\n");
            stopSimulation();
            return;
        }

        if(fmi2_import_get_fmu_kind(fmu) != fmi2_fmu_kind_me)
        {
            addErrorMessage("Only ME 2.0 is supported by this code\n");
            stopSimulation();
             return;
        }

        callBackFunctions.logger = fmi2_log_forwarding;
        callBackFunctions.allocateMemory = calloc;
        callBackFunctions.freeMemory = free;
        callBackFunctions.componentEnvironment = fmu;

        status = fmi2_import_create_dllfmu(fmu, fmi2_fmu_kind_me, &callBackFunctions);
        if (status == jm_status_error)
        {
            std::stringstream ss;
            ss << "Could not create the DLL loading mechanism(C-API) (error: " << fmi2_import_get_last_error(fmu) << ").";
            addErrorMessage(ss.str().c_str());
            stopSimulation();
            return;
        }

        n_states = fmi2_import_get_number_of_continuous_states(fmu);
        n_event_indicators = fmi2_import_get_number_of_event_indicators(fmu);

        states = new fmi2_real_t[n_states];
        states_der = new fmi2_real_t[n_states];
        event_indicators = new fmi2_real_t[n_event_indicators];
        event_indicators_prev = new fmi2_real_t[n_event_indicators];

        //Instantiate FMU
        fmi2_string_t instanceName = "Test CS model instance";
        fmi2_string_t fmuLocation = "";
        fmi2_boolean_t visible = fmi2_false;
        jm_status_enu_t jmstatus = fmi2_import_instantiate(fmu, instanceName, fmi2_cosimulation, fmuLocation, visible);
        if (jmstatus == jm_status_error)
        {
            addErrorMessage("fmi2_import_instantiate() failed!");
            stopSimulation();
            return;
        }

        //Setup experiment
        fmi2_real_t relativeTol = 1e-4;
        fmistatus = fmi2_import_setup_experiment(fmu, fmi2_true, relativeTol, mTime, fmi2_false, 10);
        if(fmistatus != fmi2_status_ok)
        {
            addErrorMessage("fmi2_import_setup_experiment() failed!");
            stopSimulation();
            return;
        }

        //Enter initialization mode
        fmistatus = fmi2_import_enter_initialization_mode(fmu);
        if(fmistatus != fmi2_status_ok)
        {
            addErrorMessage("fmi2_import_enter_initialization_mode() failed!");
            stopSimulation();
            return;
        }

        //Set parameters
        fmi2_value_reference_t vr;
>>>setpars>>>        vr = <<<vr>>>;
        fmistatus = <<<setparfunction>>>(fmu, &vr, 1, &<<<var>>>);
        <<<setpars<<<

        //Exit initialization mode
        fmistatus = fmi2_import_exit_initialization_mode(fmu);
        if(fmistatus != fmi2_status_ok)
        {
            addErrorMessage("fmi2_import_exit_initialization_mode() failed!");
            stopSimulation();
            return;
        }

        callEventUpdate = fmi2_false;

        eventInfo.newDiscreteStatesNeeded           = fmi2_false;
        eventInfo.terminateSimulation               = fmi2_false;
        eventInfo.nominalsOfContinuousStatesChanged = fmi2_false;
        eventInfo.valuesOfContinuousStatesChanged   = fmi2_true;
        eventInfo.nextEventTimeDefined              = fmi2_false;
        eventInfo.nextEventTime                     = -0.0;

        /* fmiExitInitializationMode leaves FMU in event mode */
        do_event_iteration(fmu, &eventInfo);
        fmi2_import_enter_continuous_time_mode(fmu);

        fmistatus = fmi2_import_get_continuous_states(fmu, states, n_states);
        fmistatus = fmi2_import_get_event_indicators(fmu, event_indicators, n_event_indicators);
    }


    void simulateOneTimestep()
    {
        //Read inputs
        fmi2_value_reference_t vr;
        double value;
>>>readvars>>>        vr = <<<vr>>>;
        value = (*<<<var>>>);
        fmistatus = fmi2_import_set_real(fmu, &vr, 1, &value);
        <<<readvars<<<

        double time = mTime-mTimestep;
        double tStop = mTime;

        while(time < tStop)
        {
            size_t k;
            fmi2_real_t tlast;
            int zero_crossing_event = 0;

            fmistatus = fmi2_import_set_time(fmu, time);

            { /* Swap event_indicators and event_indicators_prev so that we can get new indicators */
                fmi2_real_t *temp = event_indicators;
                event_indicators = event_indicators_prev;
                event_indicators_prev = temp;
            }
            fmistatus = fmi2_import_get_event_indicators(fmu, event_indicators, n_event_indicators);

              /* Check if an event indicator has triggered */
            for (k = 0; k < n_event_indicators; k++)
            {
                if ((event_indicators[k] > 0) != (event_indicators_prev[k] > 0))
                {
                    zero_crossing_event = 1;
                    break;
                }
            }

            /* Handle any events */
            if (callEventUpdate || zero_crossing_event ||
                    (eventInfo.nextEventTimeDefined && time == eventInfo.nextEventTime))
            {
                fmistatus = fmi2_import_enter_event_mode(fmu);
                do_event_iteration(fmu, &eventInfo);
                fmistatus = fmi2_import_enter_continuous_time_mode(fmu);

                fmistatus = fmi2_import_get_continuous_states(fmu, states, n_states);
                fmistatus = fmi2_import_get_event_indicators(fmu, event_indicators, n_event_indicators);
            }

            /* Calculate next time step */
            tlast = time;
            time = tStop;
            if (eventInfo.nextEventTimeDefined && (time >= eventInfo.nextEventTime))
            {
                time = eventInfo.nextEventTime;
            }
            hcur = time - tlast;

            /* Integrate a step */
            fmistatus = fmi2_import_get_derivatives(fmu, states_der, n_states);
            for (k = 0; k < n_states; k++)
            {
                states[k] = states[k] + hcur*states_der[k];
            }

            /* Set states */
            fmistatus = fmi2_import_set_continuous_states(fmu, states, n_states);

            /* Step is complete */
            fmistatus = fmi2_import_completed_integrator_step(fmu, fmi2_true, &callEventUpdate,
                                                              &terminateSimulation);
        }


        double rValue;
>>>writevars>>>        vr = <<<vr>>>;
        fmistatus = fmi2_import_get_real(fmu, &vr, 1, &rValue);
        (*<<<var>>>) = rValue;
        <<<writevars<<<
    }


    void finalize()
    {
        fmistatus = fmi2_import_terminate(fmu);

        fmi2_import_free_instance(fmu);

        fmi2_import_destroy_dllfmu(fmu);

        fmi2_import_free(fmu);
        fmi_import_free_context(context);
    }


    void do_event_iteration(fmi2_import_t *fmu, fmi2_event_info_t *eventInfo)
    {
        eventInfo->newDiscreteStatesNeeded = fmi2_true;
        eventInfo->terminateSimulation     = fmi2_false;
        while (eventInfo->newDiscreteStatesNeeded && !eventInfo->terminateSimulation)
        {
            fmi2_import_new_discrete_states(fmu, eventInfo);
        }
    }
};
}

#endif
