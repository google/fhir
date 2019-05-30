"""Build rules to create C++ code from an Antlr4 grammar."""

def antlr4_cc_lexer(name, src, namespaces = None, imports = None, deps = None, lib_import = None):
    """Generates the C++ source corresponding to an antlr4 lexer definition.

    Args:
      name: The name of the package to use for the cc_library.
      src: The antlr4 g4 file containing the lexer rules.
      namespaces: The namespace used by the generated files. Uses an array to
        support nested namespaces. Defaults to [name].
      imports: A list of antlr4 source imports to use when building the lexer.
      deps: Dependencies for the generated code.
      lib_import: Optional target for importing grammar and token files.
    """
    namespaces = namespaces or [name]
    imports = imports or []
    deps = deps or []
    if not src.endswith(".g4"):
        fail("Grammar must end with .g4", "src")
    if (any([not imp.endswith(".g4") for imp in imports])):
        fail("Imported files must be Antlr4 grammar ending with .g4", "imports")
    file_prefix = src[:-3]
    base_file_prefix = _strip_end(file_prefix, "Lexer")
    out_files = [
        "%sLexer.h" % base_file_prefix,
        "%sLexer.cpp" % base_file_prefix,
    ]

    native.java_binary(
        name = "antlr_tool",
        jvm_flags = ["-Xmx256m"],
        main_class = "org.antlr.v4.Tool",
        runtime_deps = ["@maven//:org_antlr_antlr4_4_7_1"],
    )

    command = ";\n".join([
        # Use the first namespace, we'll add the others afterwards.
        _make_tool_invocation_command(namespaces[0], lib_import),
        _make_namespace_adjustment_command(namespaces, out_files),
    ])

    native.genrule(
        name = name + "_source",
        srcs = [src] + imports,
        outs = out_files,
        cmd = command,
        heuristic_label_expansion = 0,
        tools = ["antlr_tool"],
    )
    native.cc_library(
        name = name,
        srcs = [f for f in out_files if f.endswith(".cpp")],
        hdrs = [f for f in out_files if f.endswith(".h")],
        deps = ["@antlr_cc_runtime//:antlr4_runtime"] + deps,
        copts = [
            "-fexceptions",
        ],
        features = ["-use_header_modules"],  # Incompatible with -fexceptions.
    )

def antlr4_cc_parser(
        name,
        src,
        namespaces = None,
        token_vocab = None,
        imports = None,
        listener = True,
        visitor = False,
        deps = None,
        lib_import = None):
    """Generates the C++ source corresponding to an antlr4 parser definition.

    Args:
      name: The name of the package to use for the cc_library.
      src: The antlr4 g4 file containing the parser rules.
      namespaces: The namespace used by the generated files. Uses an array to
        support nested namespaces. Defaults to [name].
      token_vocab: The antlr g4 file containing the lexer tokens.
      imports: A list of antlr4 source imports to use when building the parser.
      listener: Whether or not to include listener generated files.
      visitor: Whether or not to include visitor generated files.
      deps: Dependencies for the generated code.
      lib_import: Optional target for importing grammar and token files.
    """
    suffixes = ()
    if listener:
        suffixes += (
            "%sBaseListener.cpp",
            "%sListener.cpp",
            "%sBaseListener.h",
            "%sListener.h",
        )
    if visitor:
        suffixes += (
            "%sBaseVisitor.cpp",
            "%sVisitor.cpp",
            "%sBaseVisitor.h",
            "%sVisitor.h",
        )
    namespaces = namespaces or [name]
    imports = imports or []
    deps = deps or []
    if not src.endswith(".g4"):
        fail("Grammar must end with .g4", "src")
    if token_vocab != None and not token_vocab.endswith(".g4"):
        fail("Token Vocabulary must end with .g4", "token_vocab")
    if (any([not imp.endswith(".g4") for imp in imports])):
        fail("Imported files must be Antlr4 grammar ending with .g4", "imports")
    file_prefix = src[:-3]
    base_file_prefix = _strip_end(file_prefix, "Parser")
    out_files = [
        "%sParser.h" % base_file_prefix,
        "%sParser.cpp" % base_file_prefix,
    ] + _make_outs(file_prefix, suffixes)
    if token_vocab:
        imports.append(token_vocab)
    command = ";\n".join([
        # Use the first namespace, we'll add the others afterwardsm thi .
        _make_tool_invocation_command(namespaces[0], lib_import, listener, visitor),
        _make_namespace_adjustment_command(namespaces, out_files),
    ])

    native.genrule(
        name = name + "_source",
        srcs = [src] + imports,
        outs = out_files,
        cmd = command,
        heuristic_label_expansion = 0,
        tools = [
            ":antlr_tool",
        ],
    )
    native.cc_library(
        name = name,
        srcs = [f for f in out_files if f.endswith(".cpp")],
        hdrs = [f for f in out_files if f.endswith(".h")],
        deps = ["@antlr_cc_runtime//:antlr4_runtime"] + deps,
        copts = [
            "-fexceptions",
            # FIXME: antlr generates broken C++ code that attempts to construct
            # a std::string from nullptr. It's not clear whether the relevant
            # constructs are reachable.
            "-Wno-nonnull",
        ],
        features = ["-use_header_modules"],  # Incompatible with -fexceptions.
    )

def _make_outs(file_prefix, suffixes):
    return [file_suffix % file_prefix for file_suffix in suffixes]

def _strip_end(text, suffix):
    if not text.endswith(suffix):
        return text
    return text[:len(text) - len(suffix)]

def _to_c_macro_name(filename):
    # Convert the filenames to a format suitable for C preprocessor definitions.
    char_list = [filename[i].upper() for i in range(len(filename))]
    return "ANTLR4_GEN_" + "".join(
        [a if (("A" <= a) and (a <= "Z")) else "_" for a in char_list],
    )

def _make_tool_invocation_command(package, lib_import, listener = False, visitor = False):
    return "$(location :antlr_tool) " + \
           "$(SRCS)" + \
           (" -visitor" if visitor else " -no-visitor") + \
           (" -listener" if listener else " -no-listener") + \
           (" -lib $$(dirname $(location " + lib_import + "))" if lib_import else "") + \
           " -Dlanguage=Cpp" + \
           " -package " + package + \
           " -o $(@D)" + \
           " -Xexact-output-dir"

def _make_namespace_adjustment_command(namespaces, out_files):
    if len(namespaces) == 1:
        return "true"
    commands = []
    extra_header_namespaces = "\\\n".join(["namespace %s {" % namespace for namespace in namespaces[1:]])
    for filepath in out_files:
        if filepath.endswith(".h"):
            commands.append("sed -i '/namespace %s {/ a%s' $(@D)/%s" % (namespaces[0], extra_header_namespaces, filepath))
            for namespace in namespaces[1:]:
                commands.append("sed -i '/}  \/\/ namespace %s/i}  \/\/ namespace %s' $(@D)/%s" % (namespaces[0], namespace, filepath))
        else:
            commands.append("sed -i 's/using namespace %s;/using namespace %s;/' $(@D)/%s" % (namespaces[0], "::".join(namespaces), filepath))
    return ";\n".join(commands)
