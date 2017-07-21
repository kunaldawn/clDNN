﻿/*
// Copyright (c) 2016 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "concatenation_kernel_depth_bfyx_no_pitch.h"
#include "kernel_selector_utils.h"

namespace KernelSelector 
{

    ParamsKey ConcatenationKernel_depth_bfyx_no_pitch::GetSupportedKey() const
    {
        ParamsKey k;
        k.EnableInputDataType(Datatype::F32);
        k.EnableOutputDataType(Datatype::F32);
        k.EnableInputLayout(DataLayout::bfyx);
        k.EnableInputLayout(DataLayout::bf);
        k.EnableOutputLayout(DataLayout::bfyx);
        k.EnableOutputLayout(DataLayout::bf);
        k.EnableTensorOffset();
        k.EnableBatching();
        k.EnableConcatAxis(ConcatAxis::FEATURE);
        return k;
    }

    ConcatenationKernelBase::DispatchData ConcatenationKernel_depth_bfyx_no_pitch::SetDefault(const ConcatenationParams& params) const
    {
        DispatchData runInfo = ConcatenationKernelBase::SetDefault(params);
        const auto& input = params.inputs[0];
        const auto batch = input.Batch().v;
        runInfo.gws0 = batch;
        runInfo.gws1 = Align(std::max((size_t)1, input.LogicalSize() / batch / 8), 16);
        runInfo.gws2 = 1;

        runInfo.lws0 = 1;
        runInfo.lws1 = 16;
        runInfo.lws2 = 1;
        
        runInfo.effiency = FORCE_PRIORITY_9;

        return runInfo;
    }

    KernelsData ConcatenationKernel_depth_bfyx_no_pitch::GetKernelsData(const Params& params, const OptionalParams& optParams) const
    {
        return GetCommonKernelsData(params, optParams);
    }
}