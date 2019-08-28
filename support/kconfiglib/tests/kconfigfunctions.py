def add(kconf, name, *args):
    return str(sum(map(int, args)))

def one(kconf, name, s):
    return name + 2*s

def one_or_more(kconf, name, arg, *args):
    return arg + " + " + ",".join(args)

functions = {
    "add":         (add,         0, None),
    "one":         (one,         1,    1),
    "one-or-more": (one_or_more, 1, None),
}
