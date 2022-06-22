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

import tarfile
from typing import Any, BinaryIO, Callable, Dict, Iterable, Iterator, Optional, Tuple, TypeVar, Union
import zipfile

import logging

from google.protobuf import message
from google.protobuf import text_format

from proto.google.fhir.proto import annotations_pb2
from proto.google.fhir.proto import profile_config_pb2
from proto.google.fhir.proto.r4.core.resources import code_system_pb2
from proto.google.fhir.proto.r4.core.resources import search_parameter_pb2
from proto.google.fhir.proto.r4.core.resources import structure_definition_pb2
from proto.google.fhir.proto.r4.core.resources import value_set_pb2
from google.fhir.r4 import json_format

_T = TypeVar('_T', structure_definition_pb2.StructureDefinition,
             search_parameter_pb2.SearchParameter, code_system_pb2.CodeSystem,
             value_set_pb2.ValueSet)
_PROTO_CLASSES_FOR_TYPES = {
    'ValueSet': value_set_pb2.ValueSet,
    'CodeSystem': code_system_pb2.CodeSystem,
    'StructureDefinition': structure_definition_pb2.StructureDefinition,
    'SearchParameter': search_parameter_pb2.SearchParameter,
}

# TODO: Currently only supports R4. Make FHIR-version agnostic.


# TODO: Consider deprecating internal "zip" format entirely.
def _read_fhir_package_zip(
    zip_file: BinaryIO,
) -> Tuple[profile_config_pb2.PackageInfo, Dict[str, str]]:
  """Indexes the file entries in `zip_file` and returns package info.

  Args:
    zip_file: The file-like of a `.zip` file that should be parsed.

  Returns:
    A tuple of (package info, JSON entries), detailing the package information
    and associated `.json` files indexed by filename, respectively.

  Raises:
    ValueError: In the event that the `PackgeInfo` is invalid.
  """
  package_info: Optional[profile_config_pb2.PackageInfo] = None
  json_files: Dict[str, str] = {}
  with zipfile.ZipFile(zip_file, mode='r') as f:
    for entry_name in f.namelist():
      if (entry_name.endswith('package_info.textproto') or
          entry_name.endswith('package_info.prototxt')):
        data = f.read(entry_name)
        package_info: profile_config_pb2.PackageInfo = (
            text_format.Parse(data, profile_config_pb2.PackageInfo()))
        if not package_info.proto_package:
          raise ValueError(
              f'Missing proto_package from PackageInfo in: {zip_file.name}.')
        if package_info.fhir_version != annotations_pb2.R4:
          raise ValueError(
              f'Unsupported FHIR version: {package_info.fhir_version}.')
      elif entry_name.endswith('.json'):
        json_files[entry_name] = f.read(entry_name).decode('utf-8')
      else:
        logging.info('Skipping .zip entry: %s.', entry_name)

  if package_info is None:
    raise ValueError('FhirPackage does not have a valid '
                     f'`package_info.prototext`: {zipfile}.')

  return (package_info, json_files)


def _read_fhir_package_npm(npm_file: BinaryIO) -> Dict[str, str]:
  """Indexes the file entries at `npm_file_path` and returns package info.

  Args:
    npm_file: The `.tar.gz` or `.tgz` file following NPM conventions that should
      be parsed.

  Returns:
    A dictionary JSON entries indexed by filename.
  """
  json_files: Dict[str, str] = {}
  with tarfile.open(fileobj=npm_file, mode='r:gz') as f:
    for member in f.getmembers():
      if member.name.endswith('.json'):
        content = f.extractfile(member)
        if content is not None:
          json_files[member.name] = content.read().decode('utf-8')
      else:
        logging.info('Skipping  entry: %s.', member.name)

  return json_files


class ResourceCollection(Iterable[_T]):
  """A collection of FHIR resources of a given type.

  Attributes:
    archive_file: The zip or tar file path or a function returning a file-like
      containing resources represented by this collection.
    parsed_resources: A cache of resources which have been parsed into protocol
      buffers.
    resource_paths_for_uris: A mapping of URIs to tuples of the path within the
      zip file containing the resource JSON and the type of that resource.
  """

  def __init__(self, archive_file: Union[str, Callable[[], BinaryIO]]) -> None:
    self.archive_file = archive_file

    self.parsed_resources: Dict[str, _T] = {}
    self.resource_paths_for_uris: Dict[str, Tuple[str, str]] = {}

  def add_uri_at_path(self, uri: str, path: str, resource_type: str) -> None:
    """Adds a resource with the given URI located at the given path.

    Args:
      uri: URI of the resource.
      path: The path inside self.zip_file_path containing the JSON file for the
        resource.
      resource_type: The type of the resource located at `path`.
    """
    self.resource_paths_for_uris[uri] = (path, resource_type)

  def get_resource(self, uri: str) -> Optional[_T]:
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
    cached = self.parsed_resources.get(uri)
    if cached is not None:
      return cached

    path, resource_type = self.resource_paths_for_uris.get(uri, (None, None))
    if path is None and resource_type is None:
      return None

    with _open_path_or_factory(self.archive_file) as fd:
      parsed = self._parse_proto_from_file(uri, fd, path, resource_type)
    if parsed is None:
      raise RuntimeError(
          'Resource for url %s could no longer be found. The resource .zip '
          'file may have changed on disk.' % uri)
    elif parsed.url.value != uri:
      raise RuntimeError(
          'url for the retrieved resource did not match the url passed to '
          '`get_resource`. The resource .zip file may have changed on disk. '
          'Expected: %s, found: %s' % (uri, parsed.url.value))
    else:
      self.parsed_resources[uri] = parsed
      return parsed

  @staticmethod
  def _parse_proto_from_file(uri: str, archive_file: BinaryIO, path: str,
                             resource_type: str) -> Optional[_T]:
    """Parses a protocol buffer from the resource at the given path.

    Args:
      uri: The URI of the resource to parse.
      archive_file: The file-like of a zip or tar file containing resource
        definitions.
      path: The path within the zip file to the resource file to parse.
      resource_type: The type of the resource contained at `path`.

    Returns:
      The protocol buffer for the resource or `None` if it can not be found.
    """
    # Default to zip files for compatiblity.
    if (not isinstance(archive_file.name, str) or
        archive_file.name.endswith('.zip')):
      with zipfile.ZipFile(archive_file, mode='r') as f:
        raw_json = f.read(path).decode('utf-8')
    elif (archive_file.name.endswith('.tar.gz') or
          archive_file.name.endswith('.tgz')):
      with tarfile.open(fileobj=archive_file, mode='r:gz') as f:
        io_bytes = f.extractfile(path)
        if io_bytes is None:
          return None
        raw_json = io_bytes.read().decode('utf-8')
    else:
      raise ValueError(f'Unsupported file type from {archive_file.name}')

    if resource_type in _PROTO_CLASSES_FOR_TYPES:
      return json_format.json_fhir_string_to_proto(
          raw_json, _PROTO_CLASSES_FOR_TYPES[resource_type])
    elif resource_type == 'Bundle':
      json_value = _find_resource_in_bundle(uri,
                                            json_format.load_json(raw_json))
      if json_value is None:
        return None
      else:
        return json_format.json_fhir_object_to_proto(
            json_value, _PROTO_CLASSES_FOR_TYPES[json_value['resourceType']])
    else:
      logging.warning(
          'Unhandled JSON entry: %s for unexpected resource type %s.', path,
          resource_type)
      return None

  def __iter__(self) -> Iterator[_T]:
    for uri in self.resource_paths_for_uris:
      resource = self.get_resource(uri)
      if resource is not None:
        yield resource

  def __len__(self) -> int:
    return len(self.resource_paths_for_uris)


class FhirPackage:
  """Represents a FHIR Proto package.

  The FHIR Proto package is constructed from a `.zip` file containing defining
  resources and a `PackageInfo` proto, as generated by the `fhir_package` rule
  in `protogen.bzl`.
  """

  @classmethod
  def load(cls, archive_file: Union[str, Callable[[],
                                                  BinaryIO]]) -> 'FhirPackage':
    """Instantiates and returns a new `FhirPackage` from a `.zip` file.

    Args:
      archive_file: The zip or tar file path or a function returning a file-like
        containing resources represented by this collection.

    Returns:
      An instance of `FhirPackage`.

    Raises:
      ValueError: In the event that the file or contents are invalid.
    """
    with _open_path_or_factory(archive_file) as fd:
      # Default to zip if there is no file name for compatibility.
      if not isinstance(fd.name, str) or fd.name.endswith('.zip'):
        package_info, json_files = _read_fhir_package_zip(fd)
      elif fd.name.endswith('.tar.gz') or fd.name.endswith('.tgz'):
        json_files = _read_fhir_package_npm(fd)
        package_info = None
      else:
        raise ValueError(f'Unsupported file type from {fd.name}')

    collections_per_resource_type = {
        'StructureDefinition':
            ResourceCollection[structure_definition_pb2.StructureDefinition]
            (archive_file),
        'SearchParameter':
            ResourceCollection[search_parameter_pb2.SearchParameter]
            (archive_file),
        'CodeSystem':
            ResourceCollection[code_system_pb2.CodeSystem](archive_file),
        'ValueSet':
            ResourceCollection[value_set_pb2.ValueSet](archive_file),
    }

    for file_name, raw_json in json_files.items():
      resource_json = json_format.load_json(raw_json)
      _add_resource_to_collection(resource_json, collections_per_resource_type,
                                  file_name)

    return FhirPackage(
        package_info=package_info,
        structure_definitions=collections_per_resource_type[
            'StructureDefinition'],
        search_parameters=collections_per_resource_type['SearchParameter'],
        code_systems=collections_per_resource_type['CodeSystem'],
        value_sets=collections_per_resource_type['ValueSet'])

  # TODO: Consider deprecating package_info here entirely, since
  # there is not a clean counterpart from NPM-produced packages.
  def __init__(self, *, package_info: Optional[profile_config_pb2.PackageInfo],
               structure_definitions: ResourceCollection[
                   structure_definition_pb2.StructureDefinition],
               search_parameters: ResourceCollection[
                   search_parameter_pb2.SearchParameter],
               code_systems: ResourceCollection[code_system_pb2.CodeSystem],
               value_sets: ResourceCollection[value_set_pb2.ValueSet]) -> None:
    """Creates a new instance of `FhirPackage`. Callers should favor `load`."""
    self.package_info = package_info
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
    for collection in (self.structure_definitions, self.search_parameters,
                       self.code_systems, self.value_sets):
      resource = collection.get_resource(uri)
      if resource is not None:
        return resource

    return None

  def __eq__(self, o: Any) -> bool:
    if not isinstance(o, FhirPackage):
      return False
    return self.package_info.proto_package == o.package_info.proto_package

  def __hash__(self) -> int:
    return hash(self.package_info.proto_package)


class FhirPackageManager:
  """Manages access to a collection of FhirPackage instances.

  Allows users to add packages to the package manager and then search all of
  them for a particular resource.

  Attributes:
    packages: The packages added to the package manager.
  """

  def __init__(self) -> None:
    self.packages = []

  def add_package(self, package: FhirPackage) -> None:
    """Adds the given package to the package manager."""
    self.packages.append(package)

  def add_package_at_path(self, path: str) -> None:
    """Loads the package at `path` and adds it to the package manager."""
    self.add_package(FhirPackage.load(path))

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


def _add_resource_to_collection(resource_json: Dict[str, Any],
                                collections_per_resource_type: Dict[
                                    str, ResourceCollection],
                                path: str,
                                is_in_bundle: bool = False) -> None:
  """Adds an entry for the given resource to the appropriate collection.

  Args:
    resource_json: Parsed JSON object of the resource to add.
    collections_per_resource_type: Set of `ResourceCollection`s to add the
      resource to.
    path: Path within the `ResourceCollection` zip file leading to the resource.
    is_in_bundle: Used in recursive calls to indicate the resource is located in
      a bundle file.
  """
  resource_type = resource_json.get('resourceType')
  uri = resource_json.get('url')

  if resource_type in collections_per_resource_type:
    # If a resource doesn't have a URI, ignore it as the resource will
    # subsequently be impossible to look up.
    if not uri:
      logging.warning(
          'JSON entry: %s does not have a url. Skipping loading of the resource.',
          path)
    else:
      # If the resource is a member of a bundle, pass the bundle's resource
      # type to ensure the file is handled correctly later.
      collections_per_resource_type[resource_type].add_uri_at_path(
          uri, path, 'Bundle' if is_in_bundle else resource_type)
  elif resource_type == 'Bundle':
    for entry in resource_json.get('entry', ()):
      resource = entry.get('resource')
      if resource:
        _add_resource_to_collection(
            resource, collections_per_resource_type, path, is_in_bundle=True)
  else:
    logging.warning('Unhandled JSON entry: %s for unexpected resource type %s.',
                    path, resource_type)


def _find_resource_in_bundle(
    uri: str, bundle_json: Dict[str, Any]) -> Optional[Dict[str, Any]]:
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


def _open_path_or_factory(
    path_or_factory: Union[str, Callable[[], BinaryIO]]) -> BinaryIO:
  """Either opens the file at the path or calls the factory to create a file."""
  if isinstance(path_or_factory, str):
    return open(path_or_factory, 'rb')
  else:
    return path_or_factory()
