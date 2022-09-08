Inner Product {#dev_guide_inner_product}
========================================

>
> API reference: [C](@ref c_api_inner_product), [C++](@ref cpp_api_inner_product)
>

The inner product primitive (sometimes called fully connected) treats each
activation in the minibatch as a vector and computes its product with a
weights 2D tensor producing a 2D tensor as an output.

More precisely, let \f$src\f$, \f$weights\f$, \f$bias\f$ and \f$dst\f$ be \f$N
\times IC\f$, \f$OC \times IC\f$, \f$OC\f$, \f$N \times OC\f$ tensors (the
variable names follow the standard @ref dev_guide_conventions). Then:

\f[dst(n, oc) = bias(oc) + \sum_{ic=0}^{IC-1} src(n, ic) \cdot weights(oc, ic)\f]

In case when the \f$src\f$ tensor has spatial dimension it is flattened to 2D.
For example, if it is a 4D \f$N \times IC' \times IH \times IW\f$ tensor, then
the formula above is applied with \f$IC = IC' \cdot IH \cdot IW\f$.

#### Difference Between [Forward Training](#dnnl::forward_training) and [Forward Inference](#dnnl::forward_inference)

There is no difference between the @ref dnnl::forward_training
and @ref dnnl::forward_inference propagation kinds.

### Backward

The backward propagation computes \f$diff\_src\f$
based on \f$diff\_dst\f$ and \f$weights\f$.

The weights update computes \f$diff\_weights\f$ and \f$diff\_bias\f$
based on \f$diff\_dst\f$ and \f$src\f$.

@note The *optimized* memory formats \f$src\f$ and \f$weights\f$ might be
different on forward propagation, backward propagation, and weights update.

## Implementation Details

### General Notes

N/A.

### Data Types

Inner product primitive supports the following combination of data types for
source, destination, weights, and bias:

| Propagation        | Source    | Weights   | Destination      | Bias             |
| :--                | :--       | :--       | :--              | :--              |
| forward / backward | f32       | f32       | f32              | f32              |
| forward            | f16       | f16       | f16              | f16              |
| forward            | u8, s8    | s8        | u8, s8, s32, f32 | u8, s8, s32, f32 |
| forward            | bf16      | bf16      | f32, bf16        | f32, bf16        |
| backward           | f32, bf16 | bf16      | bf16             |                  |
| weights update     | bf16      | f32, bf16 | bf16             | f32, bf16        |

### Data Representation

Like other CNN primitives, the inner product primitive expects the following
tensors:

| Spatial | Source                                      | Destination      | Weights
| :--     | :--                                         | :--              | :--
| 1D      | \f$N \times C \times W\f$                   | \f$N \times C\f$ | \f$OC \times IC \times KW\f$
| 2D      | \f$N \times C \times H \times W\f$          | \f$N \times C\f$ | \f$OC \times IC \times KH \times KW\f$
| 3D      | \f$N \times C \times D \times H \times W\f$ | \f$N \times C\f$ | \f$OC \times IC \times KD \times KH \times KW\f$

Memory format of data and weights memory objects is critical for inner
product primitive performance. In the DNNL programming model, inner
product primitive is one of the few primitives that support the placeholder
format #dnnl::memory::format_tag::any (shortened to `any` from
now on) and can define data and weight memory objects formats based on the
primitive parameters. When using `any` it is necessary to first create an
inner product primitive descriptor and then query it for the actual data and
weight memory objects formats.

The table below shows the combinations for which **plain** memory formats the
inner product primitive is optimized for. For the destination tensor (which is
always \f$N \times C\f$) the memory format is always
#dnnl::memory::format_tag::nc (#dnnl::memory::format_tag::ab).

| Spatial | Source / Weights logical tensor | Implementation optimized for memory formats
| :--     | :--                             | :--
| 0D      | NC / OI                         | #dnnl_nc (#dnnl_ab) / #dnnl_oi (#dnnl_ab)
| 0D      | NC / OI                         | #dnnl_nc (#dnnl_ab) / #dnnl_io (#dnnl_ba)
| 1D      | NCW / OIW                       | #dnnl_ncw (#dnnl_abc) / #dnnl_oiw (#dnnl_abc)
| 1D      | NCW / OIW                       | #dnnl_nwc (#dnnl_acb) / #dnnl_wio (#dnnl_cba)
| 2D      | NCHW / OIHW                     | #dnnl_nchw (#dnnl_abcd) / #dnnl_oihw (#dnnl_abcd)
| 2D      | NCHW / OIHW                     | #dnnl_nhwc (#dnnl_acdb) / #dnnl_hwio (#dnnl_cdba)
| 3D      | NCDHW / OIDHW                   | #dnnl_ncdhw (#dnnl_abcde) / #dnnl_oidhw (#dnnl_abcde)
| 3D      | NCDHW / OIDHW                   | #dnnl_ndhwc (#dnnl_acdeb) / #dnnl_dhwio (#dnnl_cdeba)

### Post-ops and Attributes

Post-ops and attributes enable you to modify the behavior of the inner product
primitive by chaining certain operations after the inner product operation.
The following post-ops are supported by inner product primitives:

| Propagation | Type    | Operation | Description
| :--         | :--     | :--       | :--
| forward     | post-op | eltwise   | Applies an @ref c_api_eltwise operation to the result

## Implementation Limitations

1. Check @ref dev_guide_data_types.


## Performance Tips

- Use #dnnl::memory::format_tag::any for source, weights,
  and destinations memory format tags when create an inner product primitive
  to allow the library to choose the most appropriate memory format.
