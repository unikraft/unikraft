Sum {#dev_guide_sum}
====================

>
> API reference: [C](@ref c_api_sum), [C++](@ref cpp_api_sum)
>

The sum primitive sums \f$N\f$ tensors:

\f[
    dst(\overline{x}) =
        \sum\limits_{i = 1}^{N}
        scales(i) \cdot
        src_i(\overline{x})
\f]

The sum primitive doesn't have a notion of forward or backward propagations.
The backward propagation for the sum operation is simply an identity operation.

## Implementation Details

### General Notes

 * The \f$dst\f$ memory format can be either specified by a user or derived
   the most appropriate one by the primitive. The recommended way is to allow
   the primitive to choose the appropriate format.

 * The sum primitive requires all source and destination tensors to have the
   same shape.
   Implicit broadcasting is not supported.

### Post-ops and Attributes

The sum primitive doesn't support any post-ops or attributes.

### Data Types Support

The sum primitive supports arbitrary data types for source and destination
tensors according to the @ref dev_guide_data_types page.

### Data Representation

#### Sources, Destination

The sum primitive works with arbitrary data tensors. There is no special
meaning associated with any logical dimensions.


## Implementation Limitations

1. No primitive specific limitations. Refer to @ref dev_guide_data_types for
   limitations related to data types support.


## Performance Tips

 * Whenever possible do not specify the destination memory format so that the
   primitive is able to choose the most appropriate one.

 * The sum primitive is highly optimized for the cases when all source tensors
   have same memory format and data type matches the destination tensor data
   type. For other cases more general but slower code is working. Consider
   reordering sources to the same data format before the sum primitive.
