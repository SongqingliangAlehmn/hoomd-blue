// Copyright (c) 2009-2016 The Regents of the University of Michigan
// This file is part of the HOOMD-blue project, released under the BSD 3-Clause License.


// Maintainer: ksil

#include "OPLSDihedralForceCompute.h"
#include "OPLSDihedralForceGPU.cuh"
#include "hoomd/Autotuner.h"

/*! \file OPLSDihedralForceComputeGPU.h
    \brief Declares the OPLSDihedralForceComputeGPU class
*/

#ifdef NVCC
#error This header cannot be compiled by nvcc
#endif

#ifndef __OPLSDIHEDRALFORCECOMPUTEGPU_H__
#define __OPLSDIHEDRALFORCECOMPUTEGPU_H__

//! Computes OPLS-style dihedral potentials on the GPU
/*! Calculates the OPLS type dihedral force on the GPU

    The GPU kernel for calculating this can be found in OPLSDihedralForceComputeGPU.cu
    \ingroup computes
*/
class OPLSDihedralForceComputeGPU : public OPLSDihedralForceCompute
    {
    public:
        //! Constructs the compute
        OPLSDihedralForceComputeGPU(boost::shared_ptr<SystemDefinition> sysdef);

        //! Destructor
        virtual ~OPLSDihedralForceComputeGPU() { }

        //! Set autotuner parameters
        /*! \param enable Enable/disable autotuning
            \param period period (approximate) in time steps when returning occurs
        */
        virtual void setAutotunerParams(bool enable, unsigned int period)
            {
            OPLSDihedralForceCompute::setAutotunerParams(enable, period);
            m_tuner->setPeriod(period);
            m_tuner->setEnabled(enable);
            }

    private:
        boost::scoped_ptr<Autotuner> m_tuner; //!< Autotuner for block size

        virtual void computeForces(unsigned int timestep);
    };

//! Exports the OPLSDihedralForceComputeGPU class to python
void export_OPLSDihedralForceComputeGPU();

#endif