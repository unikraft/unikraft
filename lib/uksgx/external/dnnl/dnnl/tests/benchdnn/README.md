# benchdnn

**benchdnn** is a standalone correctness and performance benchmark for
[Deep Neural Network Library (DNNL)](
/intel/mkl-dnn).

The purpose of the benchmark is extended and robust correctness verification of
the primitives provided by DNNL.
**benchdnn** itself is a harness for different primitive-specific drivers.
So far it supports and uses the following drivers:
* [batch normalization](doc/driver_bnorm.md)
* [concatenation](doc/driver_concat.md)
* [convolution](doc/driver_conv.md)
* [deconvolution](doc/driver_conv.md)
* [element-wise](doc/driver_eltwise.md)
* [inner product](doc/driver_ip.md)
* [layer normalization](doc/driver_lnorm.md)
* [local response normalization (LRN)](doc/driver_lrn.md)
* [pooling](doc/driver_pool.md)
* [reorder](doc/driver_reorder.md)
* [recurrent neural network (RNN)](doc/driver_rnn.md)
* [shuffle](doc/driver_shuffle.md)
* [softmax](doc/driver_softmax.md)
* [sum](doc/driver_sum.md)

## License
**benchdnn** is licensed under
[Apache License Version 2.0](http://www.apache.org/licenses/LICENSE-2.0).

## Harness Usage
``` sh
    ./benchdnn --DRIVER [--engine=ENGINE_KIND] [--mode=MODE] [--reset] \
               [--max-ms-per-prb=INT] [--fix-times-per-prb=INT] \
               [-vINT|--verbose=INT] [--skip-impl=SKIP_IMPL] \
               [--allow-unimpl=BOOL] [--perf-template=PERF_TEMPLATE] \
               [DRIVER-OPTS] PROBLEM-DESCRIPTION [--batch=FILE]
```

where:

 - `--DRIVER` -- is either `bnorm`, `concat`, `conv` [default], `deconv`,
            `eltwise`, `ip`, `lrn`, `pool`, `reorder`, `rnn`, `shuffle`,
            `softmax`, or `sum`.
 - `--engine=ENGINE_KIND` -- specifies the engine kind to use for the benchmark.
            Can be `cpu` [default] or `gpu`.
 - `--mode=MODE` -- string that contains flags for benchmark mode.
            `C`, `c` for correctness [default], `P`, `p` for performance, `L`,
            `l` to list all tests without running them.
 - `--reset` -- reset all the parameters set previously to the default.
 - `--max-ms-per-prb=INT` -- time spent per problem in milliseconds.
            Available range [1e2, 60e3]. Default is `3e3`.
 - `--fix-times-per-prb=INT` -- number of iterations run per problem, N must be
            non-negative. Default is `0` (not applied, time criterion is used).
 - `-vINT, --verbose=INT` -- verbose level; use for printing additional
            information. Default is `0`.
 - `--skip-impl="str1[:str2]..."` -- skip a specific implementation
            (see dnnl_query_impl_info_str), default `""`.
 - `--allow-unimpl=true|false` -- do not treat unimplemented configuration
            as an error. Default is `false`.
 - `--perf-template={def [default], csv, CUSTOM_TEMPLATE}` -- A template to
            provide the output for a performance run. Refer to
            [performace report](doc/knobs_perf_report.md) for details.
 - `DRIVER-OPTS` -- each driver has a customized list of options. Refer to
            the corresponding driver_DRIVER.md for detailed information.
 - `PROBLEM-DESCRIPTION` -- each driver requires a specific problem format.
            Refer to the corresponding driver_DRIVER.md for detailed
            information.
 - `--batch=file` -- use options from the given file, can handle any options.

Returns `0` on success (all tests passed) or non-zero in case of any error.

## Common Glossary

|Abbreviation   | Description
|:---           |:---
| src           | Source image (input image for forward convolution)
| wei           | Weights (aka filter)
| bia           | Bias
| dst           | Destination image (output image for forward convolution)
| acc           | Accumulation (typically in terms of data type)
| ic, oc        | Input/Output channels (aka feature maps)
| id, ih, iw    | Input depth, height and width
| od, oh, ow    | Output depth, height and width
| kd, kh, kw    | Kernel (filter, weights) depth, height and width
| sd, sh, sw    | Convolution stride over depth, height and width
| pd, ph, pw    | Convolution front, top and left padding
| mb            | Minibatch (amount of images processed at once)
| g             | Groups (a way to reduce the amount of computations, see Alexnet topology)

|Prop kind      | Description
|:---           |:---
| FWD_B         | dnnl_forward_training w/ bias
| FWD_D         | dnnl_forward_training w/o bias
| FWD_I         | dnnl_forward_inference
| BWD_D         | dnnl_backward_data
| BWD_W         | dnnl_backward_weights
| BWD_DW        | dnnl_backward_data + dnnl_backward_weights
| BWD_WB        | dnnl_backward_weights + dnnl_backward_bias

|Data type/cfg  | Description
|:---           |:---
| f32           | standard float
| s32           | standard int
| s8            | standard char
| u8            | standard unsigned char
| f16           | 2-byte float (5 bits exp, 10 bits mantissa, 1 bit sign)
| bf16          | 2-byte float (8 bits exp,  7 bits mantissa, 1 bit sign)

|Format tags    | Description
|:---           |:---
| Plain:        |
|  abcd         | Standard de-facto for training in CNN (aka nchw).
|  acdb         | Standard de-facto for int8 inference in CNN (aka nhwc).
| Blocked:      |
|  aBcd8b       | Internal blocked format for AVX2 systems and below.
|  aBcd16b      | Internal blocked format for AVX512VL systems and above.
|  ...          | and some others...
| Special:      |
|  any          | dnnl_format_tag_any. Let the library decide, which layout should be used.
|  undef        | dnnl_format_tag_undef. Make a driver omit dst, letting the library to deduce it.


## Issues and Contributions

We welcome community contributions to **benchdnn** as well as to DNNL.
If you have any ideas or issues, please submit an issue or pull request. For
clarity, please include ''benchdnn: '' in the title.


## Inspiration

This work is inspired by [benchFFT](http://www.fftw.org/benchfft/) project
developed by Matteo Frigo and Steven G. Johnson as a benchmark for
Discrete Fourier Transform (DFT) implementations.
