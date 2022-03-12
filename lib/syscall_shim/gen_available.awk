BEGIN {print "/* Automatically generated file; DO NOT EDIT */"}

NR==FNR {
    lines[$1]=$0; next
}

lines[substr($2,6)]!="" {
    print lines[substr($2,6)]
}
