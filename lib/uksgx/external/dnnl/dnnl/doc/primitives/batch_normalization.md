Batch Normalization {#dev_guide_batch_normalization}
====================================================

>
> API reference: [C](@ref c_api_batch_normalization), [C++](@ref cpp_api_batch_normalization)
>

The batch normalization primitive performs a forward or backward batch
normalization operation on 0D, 2D, or 3D spatial data.

The batch normalization operation is defined by the following formulas. We show
formulas only for 2D spatial data which are straightforward to generalize to
cases of higher and lower dimensions. Variable names follow the standard
@ref dev_guide_conventions.

### Forward

\f[
    dst(n, c, h, w) =
       \gamma(c) \cdot
       \frac{src(n, c, h, w) - \mu(c)} {\sqrt{\sigma^2(c) + \varepsilon}}
       + \beta(c),
\f]

where

- \f$\gamma(c), \beta(c)\f$ are optional scale and shift for a channel
(see #dnnl_use_scaleshift flag),

- \f$\mu(c), \sigma^2(c)\f$ are computed at run-time or provided by a user
mean and variance for channel (see #dnnl_use_global_stats flag),
and

- \f$\varepsilon\f$ is a constant to improve numerical stability.

When mean and variance are computed at a run-time the following formulas are
used:

- \f$\mu(c) = \frac{1}{NHW} \sum\limits_{nhw} src(n, c, h, w)_{}\f$,

- \f$\sigma^2(c) = \frac{1}{NHW} \sum\limits_{nhw} {}_{} (src(n, c, h, w) - \mu(c))^2\f$.

The \f$\gamma(c)\f$ and \f$\beta(c)\f$ tensors are considered learnable.

In training mode the primitive also optionally supports fusion with ReLU
activation with zero negative slope applied to the result
(see #dnnl_fuse_norm_relu flag).

@note
* The batch normalization primitive computes population mean and variance and
  not their sample or unbiased versions that are typically used to compute
  running mean and variance.
* Using the mean and variance computed by the batch normalization primitive,
  running mean and variance \f$\hat\mu\f$ and \f$\hat\sigma^2\f$ can be
  computed as \f[
    \hat\mu := \alpha \cdot \hat\mu + (1 - \alpha) \cdot \mu, \\
    \hat\sigma^2 := \alpha \cdot \hat\sigma^2 + (1 - \alpha) \cdot \sigma^2.
  \f]


#### Difference Between [Forward Training](#dnnl_forward_training) and [Forward Inference](#dnnl_forward_inference)

 * If mean and variance are computed at run-time (i.e., #dnnl_use_global_stats
   is not set), they become outputs for the propagation kind
   #dnnl_forward_training (since they would be required during the backward
   propagation) and are not exposed for the propagation kind
   #dnnl_forward_inference.

 * If batch normalization is created with ReLU fusion (i.e.,
   #dnnl_fuse_norm_relu is set), for the propagation kind
   #dnnl_forward_training the primitive would produce a `workspace`
   memory as one extra output. This memory is required to compute the backward
   propagation. When the primitive is executed with propagation kind
   #dnnl_forward_inference, the workspace is not produced. Behavior would
   be the same as creating a batch normalization primitive with ReLU as a
   post-op (see section below).

### Backward

The backward propagation computes
\f$diff\_src(n, c, h, w)\f$,
\f$diff\_\gamma(c)^*\f$, and \f$diff\_\beta(c)^*\f$
based on
\f$diff\_dst(n, c, h, w)\f$, \f$src(n, c, h, w)\f$, \f$\mu(c)\f$,
\f$\sigma^2(c)\f$, \f$\gamma(c) ^*\f$, and \f$\beta(c) ^*\f$.

The tensors marked with an asterisk are used only when the primitive is
configured to use \f$\gamma(c)\f$, and \f$\beta(c)\f$ (i.e.,
#dnnl_use_scaleshift is set).

## Execution Arguments

Depending on the [flags](@ref dnnl_normalization_flags_t) and
[propagation kind](@ref dnnl_prop_kind_t), the batch normalization primitive
requires different inputs and outputs.  For clarity, the summary table is shown
below.

> TODO: add?

## Implementation Details

### General Notes

1. The different flavors of the primitive are partially controlled by the @p
   flags parameter that is passed to the operation descriptor initialization
   function (e.g., dnnl::batch_normalization_forward::desc::desc()). Multiple
   flags can be set using the bitwise OR operator (`|`).

2. For forward propagation, the mean and variance might be either computed at
   run-time (in which case they are outputs of the primitive) or provided by
   a user (in which case they are inputs). In the latter case, a user must set
   the #dnnl_use_global_stats flag. For the backward propagation, the mean and
   variance are always input parameters.

3. The memory format and data type for `src` and `dst` are assumed to be the
   same, and in the API are typically referred as `data` (e.g., see `data_desc`
   in dnnl::batch_normalization_forward::desc::desc()). The same holds for
   `diff_src` and `diff_dst`. The corresponding memory descriptors are referred
   to as `diff_data_desc`.

4. Both forward and backward propagation support in-place operations, meaning
   that `src` can be used as input and output for forward propagation, and
   `diff_dst` can be used as input and output for backward propagation. In case
   of in-place operation, the original data will be overwritten.

5. As mentioned above, the batch normalization primitive can be fused with
   ReLU activation even in the training mode. In this case, on the forward
   propagation the primitive has one additional output, `workspace`, that
   should be passed during the backward propagation.

### Data Type Support

The operation supports the following combinations of data types:

| Propagation        | Source / Destination | Mean / Variance / ScaleShift
| :--                | :--                  | :--
| forward / backward | f32, bf16            | f32
| forward            | f16                  | f32
| forward            | s8                   | f32

@warning
    There might be hardware and/or implementation specific restrictions.
    Check [Implementation Limitations](@ref dg_bnorm_impl_limits) section below.

### Data Representation

#### Mean and Variance

The mean (\f$\mu\f$) and variance (\f$\sigma^2\f$) are
separate 1D tensors of size \f$C\f$.

The format of the corresponding memory object must be #dnnl_x (#dnnl_a).

#### Scale and Shift

If used, the scale (\f$\gamma\f$) and shift (\f$\beta\f$) are
combined in a single 2D tensor of shape \f$2 \times C\f$.

The format of the corresponding memory object must be #dnnl_nc (#dnnl_ab).

#### Source, Destination, and Their Gradients

Like other CNN primitives, the batch normalization primitive expects data
to be \f$N \times C \times SP_n \times \cdots \times SP_0\f$ tensor.

The batch normalization primitive is optimized for the following memory formats:

| Spatial | Logical tensor | Implementations optimized for memory formats
| :--     | :--            | :--
| 0D      | NC             | #dnnl_nc (#dnnl_ab)
| 2D      | NCHW           | #dnnl_nchw (#dnnl_abcd), #dnnl_nhwc (#dnnl_acdb), *optimized^*
| 3D      | NCDHW          | #dnnl_ncdhw (#dnnl_abcde), #dnnl_ndhwc (#dnnl_acdeb), *optimized^*

Here *optimized^* means the format that
[comes out](@ref memory_format_propagation_cpp)
of any preceding compute-intensive primitive.

### Post-ops and Attributes

Post-ops and attributes enable you to modify the behavior of the batch
normalization primitive by chaining certain operations after the batch
normalization operation. The following post-ops are supported by batch
normalization primitives:

| Propagation | Type    | Operation | Description
| :--         | :--     | :--       | :--
| forward     | post-op | eltwise   | Applies an @ref c_api_eltwise operation to the result (currently only #dnnl_eltwise_relu algorithm is supported)

@note As mentioned in @ref dev_guide_attributes, the post-ops should be used
for inference only. For instance, using ReLU as a post-op would not produce an
additional output `workspace` that is required to compute backward propagation
correctly. Hence, in case of training one should use the #dnnl_fuse_norm_relu
directly.

@anchor dg_bnorm_impl_limits
## Implementation Limitations

1. Refer to @ref dev_guide_data_types for limitations related to data types
   support.

2. For the data types that have forward propagation support only, mean and
   variance must be provided by a user (i.e., #dnnl_use_global_stats
   is not set).


## Performance Tips

1. For backward propagation, use the same memory format for `src`, `diff_dst`,
   and `diff_src` (the format of the `diff_dst` and `diff_src` are always the
   same because of the API). Different formats are functionally supported but
   lead to highly suboptimal performance.

2. Use in-place operations whenever possible.
