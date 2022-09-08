# Element-wise Driver

## Usage
``` sh
    ./benchdnn --eltwise [benchdnn-knobs] [eltwise-knobs] [eltwise-desc] ...
```

where *eltwise-knobs* are:

 - `--dir={FWD_D [default], BWD_D}` -- dnnl_prop_kind_t. Refer to the common
            glossary in README.md for details.
 - `--dt={f32 [default], bf16, f16, s32, s8}` -- src and dst data type.
            Refer to the common glossary in README.md for details.
 - `--tag={nchw [default], ...}` -- physical src and dst memory layout.
            Refer to the common glossary in README.md for details.
 - `--alg={RELU [default], ...}` -- dnnl_eltwise algorithm.
            Refer to ``doc/primitives/eltwise.md`` for details.
 - `--alpha=FLOAT` -- float value corresponding to algorithm operation.
            Refer to ``Scales`` below.
 - `--beta=FLOAT` -- float value corresponding to algorithm operation.
            Refer to ``Scales`` below.
 - `--mb=INT` -- override minibatch size specified in the problem description.
             When set to `0`, use minibatch size as defined by the individual
             problem descriptor. The default is `0`.
 - `--inplace=BOOL` -- memory mode for the primitive. If `true`, it uses input
            memory as output, otherwise, input and output are separate.
            The default is `true`.

and *eltwise-desc* is a problem descriptor. The canonical form is:
```
    NxNxNxNxN
```
where N is an integer number. This represents a 3D spatial problem with the
following logical dimensions: N, C, D, H, W. Consider removing each `xN` from
the end to specify fewer dimensions.


## Scales
Some algorithms support `alpha` scale multiplication such as RELU, ELU, BRELU,
and LINEAR. LINEAR also supports `beta` scale. All other algorithms will
silently skip the run if `alpha` or `beta` are specified but their values are
not equal to `0`. The same works for algorithms that support `alpha` but do not
support `beta`. The default set of scales for `alpha` and `beta` is
{0, 0.25, 2}.


## Essence of Testing
Fill input data in four ranges: positive/negative integers up to 10 and
positive/negative fractions up to 1.0. This covers special areas of all
algorithm kinds. There is a general threshold; however, it cannot be applied
everywhere. That is why there are some special cases. For details, refer to
``eltwise/eltwise.cpp::compare()``.


## Examples

Run the eltwise set from an input file with the default settings:
``` sh
    ./benchdnn --eltwise --batch=inputs/eltwise/test_eltwise_all
```

Run a specific eltwise problem with the f32 data type and in-place memory mode,
iterating over memory layouts and forward and backward prop kinds:
``` sh
    ./benchdnn --eltwise --dir=FWD_D,BWD_D --dt=f32 --tag=nchw,nChw16c \
               --inplace=true 50x192x55x55
```

More examples with different driver options can be found at
inputs/eltwise/test_eltwise_all. Examples with different benchdnn options can be
found at driver_conv.md.
