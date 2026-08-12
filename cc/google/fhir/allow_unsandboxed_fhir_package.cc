// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// NOLINTBEGIN(whitespace/line_length)
// Definition of the poison symbol required to link fhir_package in unsandboxed
// mode. This is restricted to allowed unsandboxed users to prevent accidental
// backsliding.

extern "C" int
    DO_NOT_OVERWRITE____FHIR_PACKAGE_UNSANDBOXED_USE_REQUIRES_APPROVAL____SEE_cl_924576836 =
        1;
// NOLINTEND(whitespace/line_length)
