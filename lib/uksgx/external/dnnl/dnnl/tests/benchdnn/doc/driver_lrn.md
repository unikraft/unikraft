# Local Response Normalization Driver

## Usage
``` sh
    ./benchdnn --lrn [benchdnn-knobs] [lrn-knobs] [lrn-desc] ...
```

where *lrn-knobs* are:

 - `--dir={FWD_D [default], BWD_D}` -- dnnl_prop_kind_t.
            Refer to the common glossary in README.md for details.
 - `--dt={f32 [default], bf16, f16}` -- src and dst data types.
            Refer to the common glossary in README.md for details.
 - `--tag={nchw [default], ...}` -- physical src and dst memory layout.
            Refer to the common glossary in README.md for details.
 - `--alg={ACROSS [default], WITHIN}` -- lrn algorithm.
            `ACROSS` is dnnl_lrn_across_channels;
            `WITHIN` is dnnl_lrn_within_channel;
            Refer to ``doc/primitives/lrn.md`` for details.
 - `--mb=INT` -- override minibatch size specified in the problem description.
             When set to `0`, use minibatch size as defined by the individual
             problem descriptor. The default is `0`.

and *lrn-desc* is a problem descriptor. The canonical form is:
```
    mbXicX_idXihXiwX_lsX_alphaY_betaY_kY_nS
```
Here X is an integer number, Y is a real number, and S is a string (n stands for
name). The special symbol `_` is ignored, so it may be used as a delimiter.
`ls` is lrn local_size, `alpha` is lrn_alpha, `beta` is lrn_beta, and `k` is
lrn_k. Refer to the common glossary in README.md for the entity name and
description.

There are default values for some entities in case they were not specified:
 - mb = 2;
 - ls = 5;
 - alpha = 0.0001;
 - beta = 0.75;
 - k = 1;

There are also implicit rules:
 - Values for smaller dimensions may be copied from the biggest.


## Essence of Testing
Fill input data with integers so that an output will not overflow in the f16 or
bf16 data types.


## Examples

Run a set of lrns from an input file with the default settings:
``` sh
    ./benchdnn --lrn --batch=inputs/lrn/lrn_2d_all
```

Run a named problem with single precision src/dst, iterating by:
1) memory layouts, plain and blocked, where channel blocking equals 8 and 16,
2) both forward training and backward by data,
3) both algorithms supported:
``` sh
    ./benchdnn --lrn --dt=f32 --tag=nchw,nChw8c,nChw16c \
               --dir=FWD_D,BWD_D --alg=ACROSS,WITHIN \
               mb256ic96_ih55n"alexnet:norm1"
```

More examples with different driver options can be found at
inputs/lrn/test_lrn_all. Examples with different driver descriptors can be
found at inputs/lrn/lrn_***. Examples with different benchdnn options can be
found at driver_conv.md.
