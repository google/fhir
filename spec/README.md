# FHIR package definitions

This directory contains downloads of various versions of the FHIR spec, as well as some downloaded implementation guides. The directory structure within each package is defined by the [FHIR NPM package spec](http://wiki.hl7.org/index.php?title=FHIR_NPM_Package_Spec). Individual packages are located in directories named by the package name and version number. For example, the STU3 core spec can be found in the hl7.fhir.core/3.0.1/ directory.

In the future, these files will most likely be removed from this github repository, and instead be downloaded directly by bazel workspace rules. However, doing so requires the target files to be immutable, and that's not yet the case with the package specs on the FHIR site.
