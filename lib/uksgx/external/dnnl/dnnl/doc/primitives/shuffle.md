Shuffle {#dev_guide_shuffle}
============================

>
> API reference: [C](@ref c_api_shuffle), [C++](@ref cpp_api_shuffle)
>

The shuffle primitive shuffles data along the shuffle axis (here is designated
as \f$C\f$) with the group parameter \f$G\f$. Namely, the shuffle axis is
thought to be a 2D tensor of size \f$(\frac{C}{G} \times G)\f$ and it is being
transposed to \f$(G \times \frac{C}{G})\f$.

The formal definition is shown below:

### Forward

\f[
    dst(\overline{ou}, c, \overline{in}) =
    src(\overline{ou}, c', \overline{in})
\f]

where

- \f$c\f$ dimension is called a shuffle axis,
- \f$G\f$ is a `group_size`,
- \f$\overline{ou}\f$ is the outermost indices (to the left from shuffle axis),
- \f$\overline{in}\f$ is the innermost indices (to the right from shuffle axis), and
- \f$c'\f$ and \f$c\f$ relate to each other as define by the system:

\f[
    \begin{cases}
        c  &= u + v\frac{C}{G}, \\
        c' &= uG + v, \\
    \end{cases}
\f]

Here, \f$u \in [0, \frac{C}{G})\f$ and \f$v \in [0, G)\f$.

#### Difference Between [Forward Training](#dnnl_forward_training) and [Forward Inference](#dnnl_forward_inference)

There is no difference between the #dnnl_forward_training
and #dnnl_forward_inference propagation kinds.

### Backward

The backward propagation computes
\f$diff\_src(ou, c, in)\f$,
based on
\f$diff\_dst(ou, c, in)\f$.

Essentially, backward propagation is the same as forward propagation with
\f$g\f$ replaced by \f$C / g\f$.

## Implementation Details

### General Notes

1. The memory format and data type for `src` and `dst` are assumed to be the
   same, and in the API are typically referred as `data` (e.g., see `data_desc`
   in dnnl::shuffle_forward::desc::desc()). The same holds for
   `diff_src` and `diff_dst`. The corresponding memory descriptors are referred
   to as `diff_data_desc`.

## Data Types

The shuffle primitive supports the following combinations of data types:

| Propagation        | Source / Destination
| :--                | :--
| forward / backward | f32, bf16
| forward            | s32, s8, u8

@warning
    There might be hardware and/or implementation specific restrictions.
    Check [Implementation Limitations](@ref dg_shuffle_impl_limits) section
    below.

## Data Layouts

The shuffle primitive works with arbitrary data tensors. There is no special
meaning associated with any logical dimensions. However, the shuffle axis is
typically referred to as channels (hence in formulas we use \f$c\f$).

Shuffle operation typically appear in CNN topologies. Hence, in the library the
shuffle primitive is optimized for the corresponding memory formats:

| Spatial | Logical tensor | Shuffle Axis | Implementations optimized for memory formats                               |
| :--     | :--            | :--          | :--                                                                        |
| 2D      | NCHW           | 1 (C)        | #dnnl_nchw (#dnnl_abcd), #dnnl_nhwc (#dnnl_acdb), *optimized^*     |
| 3D      | NCDHW          | 1 (C)        | #dnnl_ncdhw (#dnnl_abcde), #dnnl_ndhwc (#dnnl_acdeb), *optimized^* |

Here *optimized^* means the format that
[comes out](@ref memory_format_propagation_cpp)
of any preceding compute-intensive primitive.

### Post-ops and Attributes

The shuffle primitive doesn't support any post-ops or attributes.


@anchor dg_shuffle_impl_limits
## Implementation Limitations

1. Refer to @ref dev_guide_data_types for limitations related to data types
   support.


## Performance Tips

N/A
