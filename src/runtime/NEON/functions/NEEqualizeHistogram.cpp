/*
 * Copyright (c) 2016, 2017 ARM Limited.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "arm_compute/runtime/NEON/functions/NEEqualizeHistogram.h"

#include "arm_compute/core/ITensor.h"
#include "arm_compute/core/TensorInfo.h"
#include "arm_compute/core/Types.h"
#include "arm_compute/core/Validate.h"
#include "arm_compute/runtime/NEON/NEScheduler.h"

using namespace arm_compute;

NEEqualizeHistogram::NEEqualizeHistogram()
    : _histogram_kernel(), _border_histogram_kernel(), _cd_histogram_kernel(), _map_histogram_kernel(), _hist(nr_bins, 0, max_range), _cum_dist(nr_bins, 0, max_range), _cd_lut(nr_bins, DataType::U8),
      _run_border_hist(false)
{
}

void NEEqualizeHistogram::configure(const IImage *input, IImage *output)
{
    ARM_COMPUTE_ERROR_ON_TENSOR_NOT_2D(input);
    ARM_COMPUTE_ERROR_ON_TENSOR_NOT_2D(output);
    ARM_COMPUTE_ERROR_ON_DATA_TYPE_CHANNEL_NOT_IN(input, 1, DataType::U8);
    ARM_COMPUTE_ERROR_ON_DATA_TYPE_CHANNEL_NOT_IN(output, 1, DataType::U8);
    ARM_COMPUTE_ERROR_ON_MSG((input->info()->dimension(0) % 16) != 0, "Currently the width of the image must be a multiple of 16");

    // Configure kernels
    _histogram_kernel.configure(input, &_hist);

    //COMPMID-196: Figure out how to handle the border part
    ARM_COMPUTE_UNUSED(_run_border_hist);
#if 0
    _run_border_hist = (input->info()->dimension(0) % _histogram_kernel.num_elems_processed_per_iteration()) != 0U;

    if(_run_border_hist)
    {
        _border_histogram_kernel.configure(input, &_hist, _histogram_kernel.num_elems_processed_per_iteration());
    }
#endif

    _cd_histogram_kernel.configure(input, &_hist, &_cum_dist, &_cd_lut);
    _map_histogram_kernel.configure(input, &_cd_lut, output);
}

void NEEqualizeHistogram::run()
{
    // Calculate histogram of input.
    NEScheduler::get().multithread(&_histogram_kernel);

    // Calculate remaining pixels when image is not multiple of the elements of histogram kernel
    //COMPMID-196: Figure out how to handle the border part
#if 0
    if(_run_border_hist)
    {
        _border_histogram_kernel.run(_border_histogram_kernel.window());
    }
#endif

    // Calculate cumulative distribution of histogram and create LUT.
    _cd_histogram_kernel.run(_cd_histogram_kernel.window());

    // Map input to output using created LUT.
    NEScheduler::get().multithread(&_map_histogram_kernel);
}
