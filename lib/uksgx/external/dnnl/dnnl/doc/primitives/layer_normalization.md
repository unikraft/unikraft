Layer Normalization {#dev_guide_layer_normalization}
====================================================

>
> API reference: [C](@ref c_api_layer_normalization), [C++](@ref cpp_api_layer_normalization)
>

The layer normalization primitive performs a forward or backward layer
normalization operation on 2-5D data tensor.

The layer normalization operation performs normalization over the last logical
axis of data tensor and is defined by the following formulas. We show
formulas only for 3D data which are straightforward to generalize to
cases of higher dimensions. Variable names follow the standard
@ref dev_guide_conventions.

### Forward

\f[
    dst(t, n, c) =
       \gamma(c) \cdot
       \frac{src(t, n, c) - \mu(t, n)} {\sqrt{\sigma^2(t, n) + \varepsilon}}
       + \beta(c),
\f]

where

- \f$\gamma(c), \beta(c)\f$ are optional scale and shift for a channel
(see #dnnl_use_scaleshift flag),

- \f$\mu(t, n), \sigma^2(t, n)\f$ are computed at run-time or provided by a user
mean and variance (see #dnnl_use_global_stats flag),
and

- \f$\varepsilon\f$ is a constant to improve numerical stability.

When mean and variance are computed at a run-time the following formulas are
used:

- \f$\mu(t, n) = \frac{1}{C} \sum\limits_{c} src(t, n, c)_{}\f$,

- \f$\sigma^2(t, n) = \frac{1}{C} \sum\limits_{c} {}_{} (src(t, n, c) - \mu(t, n))^2\f$.

The \f$\gamma(c)\f$ and \f$\beta(c)\f$ tensors are considered learnable.

#### Difference Between [Forward Training](#dnnl_forward_training) and [Forward Inference](#dnnl_forward_inference)

 * If mean and variance are computed at run-time (i.e., #dnnl_use_global_stats
   is not set), they become outputs for the propagation kind
   #dnnl_forward_training (since they would be required during the backward
   propagation). Data layout for mean and variance must be specified when
   initializing layer normalization descriptor by passing memory descriptor
   for statistics (e.g., by passing stat_desc in
   dnnl::layer_normalization_forward::desc::desc()). Mean and variance are
   not exposed for the propagation kind #dnnl_forward_inference.

### Backward

The backward propagation computes
\f$diff\_src(t, n, c)\f$,
\f$diff\_\gamma(c)^*\f$, and \f$diff\_\beta(c)^*\f$
based on
\f$diff\_dst(t, n, c)\f$, \f$src(t, n, c)\f$, \f$\mu(t, n)\f$,
\f$\sigma^2(t, n)\f$, \f$\gamma(c) ^*\f$, and \f$\beta(c) ^*\f$.

The tensors marked with an asterisk are used only when the primitive is
configured to use \f$\gamma(c)\f$, and \f$\beta(c)\f$ (i.e.,
#dnnl_use_scaleshift is set).

## Execution Arguments

Depending on the [flags](@ref dnnl_normalization_flags_t) and
[propagation kind](@ref dnnl_prop_kind_t), the layer normalization primitive
requires different inputs and outputs. For clarity, the summary table is shown
below.

| Propagation             | Flag | Input                                | Output
| :--                     | :--  | :--                                  | :--
| forward inference       |      | src                                  | dst
| forward inference       | S    | src, scaleshift                      | dst
| forward inference       | G    | src, mean, variance, scaleshift      | dst
| forward training        |      | src                                  | dst, mean, variance
| forward training        | S    | src, scaleshift                      | dst, mean, variance
| forward training        | G    | src, mean, variance, scaleshift      | dst
| backward data           |      | src, mean, variance, diff_dst        | diff_src
| backward data & weights | S    | src, mean, variance, diff_dst, scaleshift | diff_src, diff_scaleshift


## Implementation Details

### General Notes

1. The different flavors of the primitive are partially controlled by the @p
   flags parameter that is passed to the operation descriptor initialization
   function (e.g., dnnl::layer_normalization_forward::desc::desc()). Multiple
   flags can be set using the bitwise OR operator (`|`).

2. For forward propagation, the mean and variance might be either computed at
   run-time (in which case they are outputs of the primitive) or provided by
   a user (in which case they are inputs). In the latter case, a user must set
   the #dnnl_use_global_stats flag. For the backward propagation, the mean and
   variance are always input parameters.

3. The memory format and data type for `src` and `dst` are assumed to be the
   same, and in the API are typically referred as `data` (e.g., see `data_desc`
   in dnnl::layer_normalization_forward::desc::desc()). The same holds for
   `diff_src` and `diff_dst`. The corresponding memory descriptors are referred
   to as `diff_data_desc`.

4. Both forward and backward propagation support in-place operations, meaning
   that `src` can be used as input and output for forward propagation, and
   `diff_dst` can be used as input and output for backward propagation. In case
   of in-place operation, the original data will be overwritten.

### Data Type Support

The operation supports the following combinations of data types:

| Propagation        | Source / Destination | Mean / Variance / ScaleShift
| :--                | :--                  | :--
| forward / backward | f32                  | f32

### Data Representation

#### Mean and Variance

The mean (\f$\mu\f$) and variance (\f$\sigma^2\f$) are
separate tensors with number of dimensions equal to (\f$data\_ndims - 1\f$) and size
\f$(data\_dim[0], data\_dim[1], ..., data\_dim[ndims - 2]).\f$

Corresponding memory object can have arbitrary memory format. Unless mean and
variance are computed at runtime and not exposed (i.e. propagation kind is
#dnnl_forward_inference and #dnnl_use_global_stats is not set), user should
provide memory descriptor for statistics when initializing layer normalization
descriptor. For best performance it is adviced to use memory format that follows
the data memory format, i.e. data format is #dnnl_tnc, best performance can be
expected for statistics with #dnnl_tn format and suboptimal for statistics with
#dnnl_nc format.

#### Scale and Shift

If used, the scale (\f$\gamma\f$) and shift (\f$\beta\f$) are
combined in a single 2D tensor of shape \f$2 \times C\f$.

The format of the corresponding memory object must be #dnnl_nc (#dnnl_ab).

#### Source, Destination, and Their Gradients

Layer normalization primitive works with arbitrary data tensor, however it was
designed for RNN data tensors(i.e. #dnnl_nc, #dnnl_tnc, #dnnl_ldnc).
Unlike CNN data tensors, RNN data tensors have a single feature dimension.
Layer normalization performs normalization over the last logical dimension
(feature dimension for RNN tensors) across non-feature dimensions.

The layer normalization primitive is optimized for the following memory formats:

| Logical tensor | Implementations optimized for memory formats
| :--            | :--
| NC             | #dnnl_nc (#dnnl_ab)
| TNC            | #dnnl_tnc (#dnnl_abc), #dnnl_ntc (#dnnl_bac)
| LDNC           | #dnnl_ldnc (#dnnl_abcd)

## Performance Tips
1. For data tensors (`src`, `dst`, `diff_src`, `diff_dst`) use memory
   formats for which last logical axis is the last in the physical memory layout.

2. For `mean`/`variance` use memory format that follows the data memory format, i.e.
   data format is #dnnl_tnc, best performance can be expected for statistics
   with #dnnl_tn and suboptimal for statistics with #dnnl_nc format.

3. For backward propagation, use the same memory format for `src`, `diff_dst`,
   and `diff_src` (the format of the `diff_dst` and `diff_src` are always the
   same because of the API). Different formats are functionally supported but
   lead to highly suboptimal performance.

4. Use in-place operations whenever possible.


