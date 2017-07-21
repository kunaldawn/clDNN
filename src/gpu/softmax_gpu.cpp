/*
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

#include "softmax_inst.h"
#include "kernel.h"
#include "implementation_map.h"
#include "kernel_selector_helper.h"

using namespace cldnn;

namespace neural
{

struct softmax_gpu : typed_primitive_impl<softmax>
{
    const softmax_node& outer;
    gpu::kernel _kernel;


    softmax_gpu(const softmax_node& arg, const kernel_selector::kernel_data& kd)
        : outer(arg)
        , _kernel(arg.get_program().get_engine()->get_context(), kd.kernels[0].kernelString)
    {
        _kernel_data = kd;
    }

    event_impl::ptr execute_impl(const std::vector<event_impl::ptr>& events, softmax_inst& instance) override
    {
        gpu::kernel::kernel_arguments_data args;
        args.scalars = &_kernel_data.kernels[0].scalars;
        args.inputs = { &instance.input_memory() };
        args.output = &instance.output_memory();

        return _kernel.run(_kernel_data.kernels[0], events, args);
    }

    
    static primitive_impl* create(const softmax_node& arg) 
    {
        auto sm_params = get_default_params<kernel_selector::softmax_params>(arg);
        auto sm_optional_params = get_default_optional_params<kernel_selector::softmax_optional_params>(arg.get_program());

        auto& input = sm_params.inputs[0];
        auto& output = sm_params.output;
        auto& sm = sm_params.smParams;
        const auto primitive = arg.get_primitive();

        switch (primitive->dimension)
        {
        case softmax::normalize_x:
            sm.dim = kernel_selector::softmax_dim::X;
            break;
        case softmax::normalize_y:
            sm.dim = kernel_selector::softmax_dim::Y;
            break;
        case softmax::normalize_fyx: /* fallthru */
            // W/A for bf/bx issue of cldnn
            input = input.FlattenFeatureAndSpatials();
            output = output.FlattenFeatureAndSpatials();
        case softmax::normalize_f:
            sm.dim = kernel_selector::softmax_dim::FEATURE;
            break;
        case softmax::normalize_bfyx: /* fallthru */
        case softmax::normalize_yx:   /* fallthru */
        case softmax::normalize_b:    /* fallthru */
        default:
            throw std::runtime_error("Wrong API - no such softmax");
        }

        auto& kernel_selector = kernel_selector::softmax_kernel_selector::Instance();
        auto best_kernels = kernel_selector.GetBestKernels(sm_params, sm_optional_params);

        if (best_kernels.empty())
        {
            throw std::runtime_error("Cannot find a proper kernel for " + arg.id() +" with this arguments");
        }

        auto softmax_node = new softmax_gpu(arg, best_kernels[0]);

        return softmax_node;
    };
};

namespace {
    struct attach {
        attach() {
            auto val_fw = softmax_gpu::create;
            implementation_map<softmax>::add(std::make_tuple(cldnn::engine_types::ocl, data_types::f32, format::yxfb), val_fw);
            implementation_map<softmax>::add(std::make_tuple(cldnn::engine_types::ocl, data_types::f16, format::yxfb), val_fw);
            implementation_map<softmax>::add(std::make_tuple(cldnn::engine_types::ocl, data_types::f32, format::bfyx), val_fw);
            implementation_map<softmax>::add(std::make_tuple(cldnn::engine_types::ocl, data_types::f16, format::bfyx), val_fw);
        }
        ~attach() {}
    };
}
attach attach_impl;
} // namespace neural
