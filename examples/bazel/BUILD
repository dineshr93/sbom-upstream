genrule(
    name = "app",
    srcs = ["main.txt"],
    outs = ["app.txt"],
    cmd = "cat $(location main.txt) > $@",
)
