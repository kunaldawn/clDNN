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

#include "softmax_kernel_ref.h"
#include "kernel_selector_utils.h" 
 
namespace KernelSelector 
{
    ParamsKey SoftmaxKernelRef::GetSupportedKey() const
    {
        ParamsKey k;
        k.EnableInputDataType(Datatype::F16);
        k.EnableInputDataType(Datatype::F32);
        k.EnableOutputDataType(Datatype::F16);
        k.EnableOutputDataType(Datatype::F32);
        k.EnableInputLayout(DataLayout::bfyx);
        k.EnableInputLayout(DataLayout::yxfb);
        k.EnableInputLayout(DataLayout::bf);
        k.EnableInputLayout(DataLayout::fb);
        k.EnableOutputLayout(DataLayout::bfyx);
        k.EnableOutputLayout(DataLayout::yxfb);
        k.EnableOutputLayout(DataLayout::bf);
        k.EnableOutputLayout(DataLayout::fb);
        k.EnableSoftmaxDim(SoftmaxDim::X);
        k.EnableSoftmaxDim(SoftmaxDim::Y);
        k.EnableSoftmaxDim(SoftmaxDim::FEATURE);
        k.EnableTensorOffset();
        k.EnableTensorPitches();
        k.EnableBatching();
        return k;
    }

    KernelsData SoftmaxKernelRef::GetKernelsData(const Params& params, const OptionalParams& options) const
    {
        assert(params.GetType() == KernelType::SOFT_MAX);

        KernelData kd = KernelData::Default<SoftmaxParams>(params);

        SoftmaxParams& newParams = *static_cast<SoftmaxParams*>(kd.params.get());
        const auto& out = newParams.output;
        auto& kernel = kd.kernels[0];
        const std::string kernel_id = GetEntryPoint(kernelName, params.layerID, options);
        auto jit = GetBaseJit(newParams, kernel_id);
        switch (newParams.smParams.dim)
        {
        case SoftmaxDim::X:
            jit +=
                "#define INPUT_OTHER0_PITCH     INPUT_Y_PITCH\n"
                "#define INPUT_OTHER1_PITCH     INPUT_FEATURE_PITCH\n"
                "#define INPUT_CLASS_PITCH      INPUT_X_PITCH\n"
                "#define INPUT_CLASS_NUM        INPUT_SIZE_X\n"
                "#define OUTPUT_OTHER0_PITCH    OUTPUT_Y_PITCH\n"
                "#define OUTPUT_OTHER1_PITCH    OUTPUT_FEATURE_PITCH\n"
                "#define OUTPUT_CLASS_PITCH     OUTPUT_X_PITCH\n";
            kernel.workGroups.global = { out.Y().v, out.Feature().v, out.Batch().v };
            break;
        case SoftmaxDim::Y:
            jit +=
                "#define INPUT_OTHER0_PITCH     INPUT_X_PITCH\n"
                "#define INPUT_OTHER1_PITCH     INPUT_FEATURE_PITCH\n"
                "#define INPUT_CLASS_PITCH      INPUT_Y_PITCH\n"
                "#define INPUT_CLASS_NUM        INPUT_SIZE_Y\n"
                "#define OUTPUT_OTHER0_PITCH    OUTPUT_X_PITCH\n"
                "#define OUTPUT_OTHER1_PITCH    OUTPUT_FEATURE_PITCH\n"
                "#define OUTPUT_CLASS_PITCH     OUTPUT_Y_PITCH\n";
            kernel.workGroups.global = { out.X().v, out.Feature().v, out.Batch().v };
            break;
        case SoftmaxDim::FEATURE:
            jit +=
                "#define INPUT_OTHER0_PITCH     INPUT_X_PITCH\n"
                "#define INPUT_OTHER1_PITCH     INPUT_Y_PITCH\n"
                "#define INPUT_CLASS_PITCH      INPUT_FEATURE_PITCH\n"
                "#define INPUT_CLASS_NUM        INPUT_FEATURE_NUM\n"
                "#define OUTPUT_OTHER0_PITCH    OUTPUT_X_PITCH\n"
                "#define OUTPUT_OTHER1_PITCH    OUTPUT_Y_PITCH\n"
                "#define OUTPUT_CLASS_PITCH     OUTPUT_FEATURE_PITCH\n";
            kernel.workGroups.global = { out.X().v, out.Y().v, out.Batch().v };
            break;
        default:
            break;
        }

        // TODO: W/A - currently using low precision accumulator type. (for testing only)
        if (newParams.output.GetDType() == Datatype::F16)
        {
            jit += "#define COUNTER_TYPE_F16\n";
        }

        kernel.workGroups.local = GetOptimalLocalWorkGroupSizes(kernel.workGroups.global);
        kernel.kernelString = GetKernelString(kernelName, jit, kernel_id);
        kernel.arguments = GetArgumentDesc(1, false, false);

        kd.estimatedTime = DONT_USE_IF_HAVE_SOMETHING_ELSE;

        return{ kd };
    }
}