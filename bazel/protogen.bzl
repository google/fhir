"""Rules for generating Protos from Profiles and StructureDefinitions"""

def zip_file(name, srcs = []):
    native.genrule(
        name = name,
        srcs = srcs,
        outs = [name],
        cmd = "zip --quiet -j $@ $(SRCS)",
    )
