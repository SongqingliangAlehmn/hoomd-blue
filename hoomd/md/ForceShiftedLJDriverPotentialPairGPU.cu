// Copyright (c) 2009-2018 The Regents of the University of Michigan
// This file is part of the HOOMD-blue project, released under the BSD 3-Clause License.

/*! \file ForceShiftedLJDriverPotentialPairGPU.cu
    \brief Defines the driver functions for computing all types of pair forces on the GPU
*/

#include "EvaluatorPairForceShiftedLJ.h"
#include "AllDriverPotentialPairGPU.cuh"

cudaError_t gpu_compute_force_shifted_lj_forces(const pair_args_t & args,
                                                const Scalar2 *d_params)
    {
    return gpu_compute_pair_forces<EvaluatorPairForceShiftedLJ>(args,
                                                                d_params);
    }

