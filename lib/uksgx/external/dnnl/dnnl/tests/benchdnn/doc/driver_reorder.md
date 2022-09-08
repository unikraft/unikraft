# Reorder Driver

## Usage
``` sh
    ./benchdnn --reorder [benchdnn-knobs] [reorder-knobs] [reorder-desc] ...
```

where *reorder-knobs* are:

 - `--sdt={f32 [default], s32, s8, u8, bf16, f16}` -- src data type.
            Refer to the common glossary in README.md for details.
 - `--ddt={f32 [default], s32, s8, u8, bf16, f16}` -- dst data type.
            Refer to the common glossary in README.md for details.
 - `--stag={nchw [default], ...}` -- physical src memory layout.
            Refer to the common glossary in README.md for details.
 - `--dtag={nchw [default], ...}` -- physical dst memory layout.
            Refer to the common glossary in README.md for details.
 - `--attr="attr_str"` -- primitive attributes, default `""` (no attributes).
            Refer to knobs_attr.md for details.
 - `--def-scales={N1[,N2][,N3]...}` -- input scales, separated by ','.
            Example: 0.125, 0.25, 0.5, 1, 2, 4, 8
 - `--alg={reference [default], bootstrap}` -- reorder algorithm. TBA.
 - `--oflag={none [default], conv_s8s8, gconv_s8s8}` -- reorder output flag.
            Works only in combination with `--alg=bootstrap`. TBA.

and *reorder-desc* is a problem descriptor. The canonical form is:
```
    NxNxNxNxN
```
where N is an integer number. This represents a 3D spatial problem with the
following logical dimensions: N, C, D, H, W. Consider removing each `xN` from
the end to specify fewer dimensions.


## Essence of Testing
TBA.


## Examples

Run the reorder set from an input file with the default settings:
``` sh
    ./benchdnn --reorder --batch=inputs/reorder/test_reorder_all
```

Run two specific reorders with s8 src and dst data type, bootstrap algorithm,
and specific input and output physical memory layouts. First problem without
a flag; second problem with the `conv_s8s8` flag:
``` sh
    ./benchdnn --reorder --alg=bootstrap --sdt=s8 --ddt=s8 \
               --stag=hwio --dtag=OIhw4i16o4i 32x32x3x3 \
               --oflag=conv_s8s8 16x32x7x5
```

More examples with different driver options can be found at
inputs/reorder/test_reorder_all. Examples with different benchdnn options can be
found at driver_conv.md.
