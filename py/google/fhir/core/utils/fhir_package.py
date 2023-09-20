#
# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Utility class for abstracting over FHIR definitions."""

import abc
import dataclasses
import decimal
import json
import os.path
import tarfile
from typing import Any, BinaryIO, Callable, Collection, Dict, Generic, Iterable, Iterator, List, Optional, Tuple, Type, TypeVar, Union
import zipfile

import logging

from google.protobuf import message
from google.fhir.core.internal import primitive_handler
from google.fhir.core.internal.json_format import _json_parser

# TODO(b/201107372): Add structural typing to constrain version-agnostic types.
_T = TypeVar('_T')

_StructDefT = TypeVar('_StructDefT')
_SearchParameterT = TypeVar('_SearchParameterT')
_CodeSystemT = TypeVar('_CodeSystemT')
_ValueSetT = TypeVar('_ValueSetT')

# Type for a source of FHIR package data, which can either be a file name or
# a factory-like callable that returns the package data itself.
PackageSource = Union[str, Callable[[], BinaryIO]]


# TODO(b/235876918): Consider deprecating internal "zip" format entirely.
def _read_fhir_package_zip(zip_file: BinaryIO) -> Iterator[Tuple[str, str]]:
  """Yields the file entries for JSON resources in `zip_file` and their contents.

  Args:
    zip_file: The file-like of a `.zip` file that should be parsed.

  Yields:
    A tuple of filename and the raw JSON contained in that file.
  """
  with zipfile.ZipFile(zip_file, mode='r') as f:
    for entry_name in f.namelist():
      if entry_name.endswith('.json'):
        yield entry_name, f.read(entry_name).decode('utf-8')
      else:
        logging.info('Skipping .zip entry: %s.', entry_name)


def _read_fhir_package_npm(npm_file: BinaryIO) -> Iterator[Tuple[str, str]]:
  """Yields the file entries for JSON resources in `npm_file` and their contents.

  Args:
    npm_file: The `.tar.gz` or `.tgz` file following NPM conventions that should
      be parsed.

  Yields:
    A tuple of filename and the raw JSON contained in that file.
  """
  with tarfile.open(fileobj=npm_file, mode='r:gz') as f:
    for member in f.getmembers():
      if member.name.endswith('.json'):
        content = f.extractfile(member)
        if content is not None:
          # Note that decoding as utf-8-sig works for both utf-8-sig and normal
          # utf-8.
          yield member.name, content.read().decode('utf-8-sig')
      else:
        logging.info('Skipping  entry: %s.', member.name)


def _parse_ig_info(json_obj: Dict[str, Any]) -> 'IgInfo':
  """Creates an IgInfo object given the contents of a package.json file."""
  return IgInfo(
      name=json_obj['name'],
      version=json_obj['version'],
      canonical=json_obj.get('canonical'),
      title=json_obj.get('title'),
      description=json_obj.get('description'),
      dependencies=tuple(
          IgDependency(url=url, version=version)
          for url, version in json_obj.get('dependencies', {}).items()
      ),
  )


class ResourceCollection(Iterable[_T]):
  """A collection of FHIR resources of a given type.

  Attributes:
    archive_file: The zip or tar file path or a function returning a file-like
    proto_cls: The class of the proto this collection contains.
    handler: The FHIR primitive handler used for resource parsing.
    resource_time_zone: The time zone code to parse resource dates into.
    resources_by_uri: A map between URIs and the resource for that URI. Elements
      begin as JSON and are replaced with protos of type _T as they are
      accessed. For resources inside bundles, the JSON will be the bundle's JSON
      rather than the resource itself.
  """

  @classmethod
  def from_iterable(
      cls,
      resources: Iterable[_T],
      proto_cls: Type[_T],
      handler: primitive_handler.PrimitiveHandler,
      resource_time_zone: str,
  ) -> 'ResourceCollection[_T]':
    """Creates a resource collection containing the protos in `resources`."""
    collection = cls(proto_cls, handler, resource_time_zone)
    for resource in resources:
      collection.resources_by_uri[resource.url.value] = resource
    return collection

  def __init__(
      self,
      proto_cls: Type[_T],
      handler: primitive_handler.PrimitiveHandler,
      resource_time_zone: str,
  ) -> None:
    self.proto_cls = proto_cls
    self.handler = handler
    self.resource_time_zone = resource_time_zone
    self.resources_by_uri: Dict[str, Union[Optional[_T], Dict[str, Any]]] = {}

  def put(
      self,
      resource_json: Dict[str, Any],
      parent_bundle: Optional[Dict[str, Any]] = None,
  ) -> None:
    """Puts the given resource into this collection.

    Adds the resource represented by `resource_json` found inside
    `parent_bundle` into this collection for subsequent lookup via the Get
    method. `parent_bundle` may be None if `resource_json` is not located inside
    a bundle.

    Args:
      resource_json: The JSON object representing the resource.
      parent_bundle: The bundle `resource_json` is located inside, if any.
    """
    if parent_bundle is None:
      self.resources_by_uri[resource_json['url']] = resource_json
    else:
      self.resources_by_uri[resource_json['url']] = parent_bundle

  def get(self, uri: str) -> Optional[_T]:
    """Retrieves a protocol buffer for the resource with the given uri.

    Args:
      uri: URI of the resource to retrieve.

    Returns:
      A protocol buffer for the resource or `None` if the `uri` is not present
      in the ResourceCollection.

    Raises:
      RuntimeError: The resource could not be found or the retrieved resource
      did not have the expected URL. The .zip file may have changed on disk.
    """
    resource = self.resources_by_uri.get(uri)
    if resource is None:
      return None

    # See if the resource has already been parsed into a proto.
    if isinstance(resource, self.proto_cls):
      return resource

    # The resource needs to be parsed from JSON into a proto.
    parsed = self._parse_resource(uri, resource)
    self.resources_by_uri[uri] = parsed
    return parsed

  def _parse_resource(self, uri: str, json_obj: Dict[str, Any]) -> Optional[_T]:
    """Parses a protocol buffer for the given JSON object.

    Args:
      uri: The URI of the resource to parse.
      json_obj: The JSON object to parse into a proto.

    Returns:
      The protocol buffer for the resource or `None` if it can not be found.
    """
    json_parser = _json_parser.JsonParser(self.handler, self.resource_time_zone)
    resource_type = json_obj.get('resourceType')
    if resource_type is None:
      raise ValueError(f'JSON for URI {uri} does not have a resource type.')

    if resource_type == 'Bundle':
      json_value = _find_resource_in_bundle(uri, json_obj)
      if json_value is None:
        return None
      else:
        target = self.proto_cls()
        json_parser.merge_value(json_value, target)
        return target
    else:
      target = self.proto_cls()
      json_parser.merge_value(json_obj, target)
      return target

  def __iter__(self) -> Iterator[_T]:
    for uri in self.resources_by_uri:
      resource = self.get(uri)
      if resource is not None:
        yield resource

  def __len__(self) -> int:
    return len(self.resources_by_uri)


class FhirPackageAccessor(
    Generic[_StructDefT, _SearchParameterT, _CodeSystemT, _ValueSetT], abc.ABC
):
  """Interface implemented by FhirPackage and FhirPackageManager."""

  @abc.abstractmethod
  def get_structure_definition(self, uri: str) -> Optional[_StructDefT]:
    pass

  @abc.abstractmethod
  def get_search_parameter(self, uri: str) -> Optional[_SearchParameterT]:
    pass

  @abc.abstractmethod
  def get_code_system(self, uri: str) -> Optional[_CodeSystemT]:
    pass

  @abc.abstractmethod
  def get_value_set(self, uri: str) -> Optional[_ValueSetT]:
    pass


@dataclasses.dataclass(frozen=True)
class IgInfo:
  """Metadata parsed from a package's package.json file.

  See documentation for the package.json file at:
  https://confluence.hl7.org/pages/viewpage.action?pageId=35718629#NPMPackageSpecification-Packagemanifest
  """

  name: str
  version: str
  description: Optional[str] = None
  canonical: Optional[str] = None
  title: Optional[str] = None
  dependencies: Collection['IgDependency'] = ()


@dataclasses.dataclass(frozen=True)
class IgDependency:
  """The URL and version of a package dependency."""

  url: str
  version: str


class FhirPackage(
    FhirPackageAccessor[
        _StructDefT, _SearchParameterT, _CodeSystemT, _ValueSetT
    ]
):
  """Represents a FHIR Proto package.

  The FHIR Proto package is constructed from a `.zip` file containing defining
  resources and a `PackageInfo` proto, as generated by the `fhir_package` rule
  in `protogen.bzl`.

  Attributes:
    ig_info: Metadata on the implementation guide defined by the package.
    structure_definitions: The structure definitions defined by the package.
    search_parameters: The search parameters defined by the package.
    code_systems: The code systems defined by the package.
    value_sets: The value sets defined by the package.
  """

  ig_info: IgInfo
  structure_definitions: ResourceCollection[_StructDefT]
  search_parameters: ResourceCollection[_SearchParameterT]
  code_systems: ResourceCollection[_CodeSystemT]
  value_sets: ResourceCollection[_ValueSetT]

  @classmethod
  def load(
      cls,
      archive_file: PackageSource,
      handler: primitive_handler.PrimitiveHandler,
      struct_def_class: Type[_StructDefT],
      search_param_class: Type[_SearchParameterT],
      code_system_class: Type[_CodeSystemT],
      value_set_class: Type[_ValueSetT],
      resource_time_zone: str = 'Z',
  ) -> 'FhirPackage[_StructDefT, _SearchParameterT, _CodeSystemT, _ValueSetT]':
    """Instantiates and returns a new `FhirPackage` from a `.zip` file.

    Most users should not use this directly, but rather use the load methods
    in FHIR version-specific packages.

    Args:
      archive_file: A path to the `.zip`, `.tar.gz` or `.tgz` file containing
        the `FhirPackage` contents.
      handler: The FHIR primitive handler used for resource parsing.
      struct_def_class: The StructureDefinition proto class to use.
      search_param_class: The SearchParameter proto class to use.
      code_system_class: The CodeSystem proto class to use.
      value_set_class: The Valueset proto class to use.
      resource_time_zone: The time zone code to parse resource dates into.

    Returns:
      An instance of `FhirPackage`.

    Raises:
      ValueError: In the event that the file or contents are invalid.
    """
    collections_per_resource_type = {
        'StructureDefinition': ResourceCollection[_StructDefT](
            struct_def_class, handler, resource_time_zone
        ),
        'SearchParameter': ResourceCollection[_SearchParameterT](
            search_param_class, handler, resource_time_zone
        ),
        'CodeSystem': ResourceCollection[_CodeSystemT](
            code_system_class, handler, resource_time_zone
        ),
        'ValueSet': ResourceCollection[_ValueSetT](
            value_set_class, handler, resource_time_zone
        ),
    }

    with _open_path_or_factory(archive_file) as fd:
      # Default to zip if there is no file name for compatibility.
      if not isinstance(fd.name, str) or fd.name.endswith('.zip'):
        json_files = _read_fhir_package_zip(fd)
      elif fd.name.endswith('.tar.gz') or fd.name.endswith('.tgz'):
        json_files = _read_fhir_package_npm(fd)
      else:
        raise ValueError(f'Unsupported file type from {fd.name}')

      ig_info: Optional[IgInfo] = None
      for file_name, raw_json in json_files:
        json_obj = json.loads(
            raw_json, parse_float=decimal.Decimal, parse_int=decimal.Decimal
        )

        if os.path.basename(file_name) == 'package.json':
          ig_info = _parse_ig_info(json_obj)

        _add_resource_to_collection(
            json_obj, json_obj, collections_per_resource_type
        )

    if ig_info is None:
      raise ValueError(
          f'Package {fd.name} does not contain a package.json '
          'file stating its URL and version.'
      )

    return FhirPackage(
        ig_info=ig_info,
        structure_definitions=collections_per_resource_type[
            'StructureDefinition'
        ],
        search_parameters=collections_per_resource_type['SearchParameter'],
        code_systems=collections_per_resource_type['CodeSystem'],
        value_sets=collections_per_resource_type['ValueSet'],
    )

  # TODO(b/201107372): Constrain version-agnostic types with structural typing.
  def __init__(
      self,
      *,
      ig_info: IgInfo,
      structure_definitions: ResourceCollection[_StructDefT],
      search_parameters: ResourceCollection[_SearchParameterT],
      code_systems: ResourceCollection[_CodeSystemT],
      value_sets: ResourceCollection[_ValueSetT],
  ) -> None:
    """Creates a new instance of `FhirPackage`. Callers should favor `load`."""
    self.ig_info = ig_info
    self.structure_definitions = structure_definitions
    self.search_parameters = search_parameters
    self.code_systems = code_systems
    self.value_sets = value_sets

  def get_resource(self, uri: str) -> Optional[message.Message]:
    """Retrieves a protocol buffer representation of the given resource.

    Args:
      uri: The URI of the resource to retrieve.

    Returns:
      Protocol buffer for the resource or `None` if the `uri` can not be found.
    """
    for collection in (
        self.structure_definitions,
        self.search_parameters,
        self.code_systems,
        self.value_sets,
    ):
      resource = collection.get(uri)
      if resource is not None:
        return resource

    return None

  def get_structure_definition(self, uri: str) -> Optional[_StructDefT]:
    return self.structure_definitions.get(uri)

  def get_search_parameter(self, uri: str) -> Optional[_SearchParameterT]:
    return self.search_parameters.get(uri)

  def get_code_system(self, uri: str) -> Optional[_CodeSystemT]:
    return self.code_systems.get(uri)

  def get_value_set(self, uri: str) -> Optional[_ValueSetT]:
    return self.value_sets.get(uri)


class FhirPackageManager(
    FhirPackageAccessor[
        _StructDefT, _SearchParameterT, _CodeSystemT, _ValueSetT
    ]
):
  """Manages access to a collection of FhirPackage instances.

  Allows users to add packages to the package manager and then search all of
  them for a particular resource.

  Attributes:
    packages: The packages added to the package manager.
  """

  packages: List[
      FhirPackage[_StructDefT, _SearchParameterT, _CodeSystemT, _ValueSetT]
  ]

  def __init__(
      self,
      packages: Optional[
          List[
              FhirPackage[
                  _StructDefT, _SearchParameterT, _CodeSystemT, _ValueSetT
              ]
          ]
      ] = None,
  ) -> None:
    self.packages = packages or []

  def add_package(
      self,
      package: FhirPackage[
          _StructDefT, _SearchParameterT, _CodeSystemT, _ValueSetT
      ],
  ) -> None:
    """Adds the given package to the package manager."""
    self.packages.append(package)

  def get_resource(self, uri: str) -> Optional[message.Message]:
    """Retrieves a protocol buffer representation of the given resource.

    Searches the packages added to the package manger for the resource with the
    given URI. If multiple packages contain the same resource, the package
    consulted will be non-deterministic.

    Args:
      uri: The URI of the resource to retrieve.

    Returns:
      Protocol buffer for the resource or `None` if the `uri` can not be found.
    """
    for package in self.packages:
      resource = package.get_resource(uri)
      if resource is not None:
        return resource

    return None

  def get_structure_definition(self, uri: str) -> Optional[_StructDefT]:
    for package in self.packages:
      resource = package.get_structure_definition(uri)
      if resource is not None:
        return resource

  def get_search_parameter(self, uri: str) -> Optional[_SearchParameterT]:
    for package in self.packages:
      resource = package.get_search_parameter(uri)
      if resource is not None:
        return resource

  def get_code_system(self, uri: str) -> Optional[_CodeSystemT]:
    for package in self.packages:
      resource = package.get_code_system(uri)
      if resource is not None:
        return resource

  def get_value_set(self, uri: str) -> Optional[_ValueSetT]:
    for package in self.packages:
      resource = package.get_value_set(uri)
      if resource is not None:
        return resource

  def iter_structure_definitions(self) -> Iterator[_StructDefT]:
    for package in self.packages:
      yield from package.structure_definitions


def _add_resource_to_collection(
    parent_resource: Dict[str, Any],
    resource_json: Dict[str, Any],
    collections_per_resource_type: Dict[str, ResourceCollection],
) -> None:
  """Adds an entry for the given resource to the appropriate collection.

  Adds the resource described by `resource_json` found within `parent_resource`
  to the appropriate ResourceCollection of the given `fhir_package`. Allows the
  resource to subsequently be retrieved by its URL from the FhirPackage. In the
  case where `resource_json` is located inside a bundle, `parent_resource` will
  be the bundle containing the resource. Otherwise, `resource_json` and
  `parent_resource` will be the same JSON object. If the JSON is not a FHIR
  resource, or not a resource type tracked by the PackageManager, does nothing.

  Args:
    parent_resource: The bundle `resource_json` can be found inside, or the
      resource itself if it is not part of a bundle.
    resource_json: The parsed JSON representation of the resource to add.
    collections_per_resource_type: The set of `ResourceCollection`s to add the
      resource to.
  """
  resource_type = resource_json.get('resourceType')

  if resource_type in collections_per_resource_type:
    collections_per_resource_type[resource_type].put(
        resource_json, parent_resource
    )
  elif resource_type == 'Bundle':
    for entry in resource_json.get('entry', ()):
      bundle_resource = entry.get('resource')
      if bundle_resource:
        _add_resource_to_collection(
            parent_resource, bundle_resource, collections_per_resource_type
        )


def _find_resource_in_bundle(
    uri: str, bundle_json: Dict[str, Any]
) -> Optional[Dict[str, Any]]:
  """Finds the JSON object for the resource with `uri` inside `bundle_json`.

  Args:
    uri: The resource URI to search for.
    bundle_json: Parsed JSON object for the bundle to search

  Returns:
    Parsed JSON object for the resource or `None` if it can not be found.
  """
  for entry in bundle_json.get('entry', ()):
    resource = entry.get('resource', {})
    if resource.get('url') == uri:
      return resource
    elif resource.get('resourceType') == 'Bundle':
      bundled_resource = _find_resource_in_bundle(uri, resource)
      if bundled_resource is not None:
        return bundled_resource

  return None


def _open_path_or_factory(path_or_factory: PackageSource) -> BinaryIO:
  """Either opens the file at the path or calls the factory to create a file."""
  if isinstance(path_or_factory, str):
    return open(path_or_factory, 'rb')
  else:
    return path_or_factory()
