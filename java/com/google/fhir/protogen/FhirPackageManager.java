//    Copyright 2022 Google LLC
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        https://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

package com.google.fhir.protogen;

import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.r4.core.CodeSystem;
import com.google.fhir.r4.core.SearchParameter;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.fhir.r4.core.ValueSet;
import com.google.protobuf.Message;
import java.util.ArrayList;
import java.util.Optional;

/**
 * Manages access to a collection of FhirPackage instances.
 *
 * <p>Callers can add packages to the `FhirPackageManager` and search across them all for specific
 * resources.
 */
public class FhirPackageManager {
  public void addPackage(FhirPackage fhirPackage) {
    for (FhirPackage existingPackage : packages) {
      if (fhirPackage.equals(existingPackage)) {
        throw new IllegalArgumentException(
            "Already managing package: " + fhirPackage.packageInfo.getProtoPackage());
      }
    }
    packages.add(fhirPackage);
  }

  /**
   * Returns the `StructureDefinition` from the packages being managed with the specified `uri` or
   * an empty `Optional` if none of the packages contain a resource for the `uri`.
   *
   * @param uri of the `StructureDefinition` to retrieve as defined in the package the resource was
   *     loaded from.
   * @throws InvalidFhirException when the resource with the provided `uri` cannot be deserialized
   *     as a `StructureDefinition`.
   */
  public Optional<StructureDefinition> getStructureDefinition(String uri)
      throws InvalidFhirException {
    return getResource(p -> p.getStructureDefinition(uri));
  }

  /**
   * Returns the `SearchParameter` from the packages being managed with the specified `uri` or an
   * empty `Optional` if none of the packages contain a resource for the `uri`.
   *
   * @param uri of the `SearchParameter` to retrieve as defined in the package the resource was
   *     loaded from.
   * @throws InvalidFhirException when the resource with the provided `uri` cannot be deserialized
   *     as a `SearchParameter`.
   */
  public Optional<SearchParameter> getSearchParameter(String uri) throws InvalidFhirException {
    return getResource(p -> p.getSearchParameter(uri));
  }

  /**
   * Returns the `CodeSystem` from the packages being managed with the specified `uri` or an empty
   * `Optional` if none of the packages contain a resource for the `uri`.
   *
   * @param uri of the `CodeSystem` to retrieve as defined in the package the resource was loaded
   *     from.
   * @throws InvalidFhirException when the resource with the provided `uri` cannot be deserialized
   *     as a `CodeSystem`.
   */
  public Optional<CodeSystem> getCodeSystem(String uri) throws InvalidFhirException {
    return getResource(p -> p.getCodeSystem(uri));
  }

  /**
   * Returns the `ValueSet` from the packages being managed with the specified `uri` or an empty
   * `Optional` if none of the packages contain a resource for the `uri`.
   *
   * @param uri of the `ValueSet` to retrieve as defined in the package the resource was loaded
   *     from.
   * @throws InvalidFhirException when the resource with the provided `uri` cannot be deserialized
   *     as a `ValueSet`.
   */
  public Optional<ValueSet> getValueSet(String uri) throws InvalidFhirException {
    return getResource(p -> p.getValueSet(uri));
  }

  private interface ResourceAccessor<T> {
    Optional<T> run(FhirPackage fhirPackage) throws InvalidFhirException;
  }

  private <T extends Message> Optional<T> getResource(ResourceAccessor<T> resourceAccessor)
      throws InvalidFhirException {
    Optional<T> resource = Optional.empty();
    for (FhirPackage p : packages) {
      resource = resourceAccessor.run(p);
      if (resource.isPresent()) {
        break;
      }
    }
    return resource;
  }

  private final ArrayList<FhirPackage> packages = new ArrayList<>();
}
