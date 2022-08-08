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

import decimal
import json
import tarfile
from typing import Any, BinaryIO, Callable, Dict, Iterable, Iterator, Optional, Tuple, Union, Type, TypeVar
import zipfile

import logging

from google.protobuf import message
from google.fhir.core.internal import primitive_handler
from google.fhir.core.internal.json_format import _json_parser

# TODO: Add structural typing to constrain version-agnostic types.
_T = TypeVar('_T')

# Type for a source of FHIR package data, which can either be a file name or
# a factory-like callable that returns the package data itself.
PackageSource = Union[str, Callable[[], BinaryIO]]


# TODO: Consider deprecating internal "zip" format entirely.
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
          yield member.name, content.read().decode('utf-8')
      else:
        logging.info('Skipping  entry: %s.', member.name)


class ResourceCollection(Iterable[_T]):
  """A collection of FHIR resources of a given type.

  Attributes:
    archive_file: The zip or tar file path or a function returning a file-like
    proto_cls: The class of the proto this collection contains.
    handler: The FHIR primitive handler used for resource parsing.
    resource_time_zone: The time zone code to parse resource dates into.
    parsed_resources: A cache of resources which have been parsed into protocol
      buffers.
    resource_paths_for_uris: A mapping of URIs to tuples of the path within the
      zip file containing the resource JSON and the type of that resource.
  """

  def __init__(self, archive_file: PackageSource, proto_cls: Type[_T],
               handler: primitive_handler.PrimitiveHandler,
               resource_time_zone: str) -> None:
    self.archive_file = archive_file
    self.proto_cls = proto_cls
    self.handler = handler
    self.resource_time_zone = resource_time_zone

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
      parsed = self._parse_proto_from_file(uri, fd, path)
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

  def _parse_proto_from_file(self, uri: str, archive_file: BinaryIO,
                             path: str) -> Optional[_T]:
    """Parses a protocol buffer from the resource at the given path.

    Args:
      uri: The URI of the resource to parse.
      archive_file: The file-like of a zip or tar file containing resource
        definitions.
      path: The path within the zip file to the resource file to parse.

    Returns:
      The protocol buffer for the resource or `None` if it can not be found.
    """
    json_parser = _json_parser.JsonParser(self.handler, self.resource_time_zone)
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

    json_obj = json.loads(
        raw_json, parse_float=decimal.Decimal, parse_int=decimal.Decimal)
    resource_type = json_obj.get('resourceType')
    if resource_type is None:
      raise ValueError(f'JSON at {path} does not have a resource type.')

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
  def load(cls,
           archive_file: PackageSource,
           handler: primitive_handler.PrimitiveHandler,
           struct_def_class: Type[message.Message],
           search_param_class: Type[message.Message],
           code_system_class: Type[message.Message],
           value_set_class: Type[message.Message],
           resource_time_zone: str = 'Z') -> 'FhirPackage':
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
        'StructureDefinition':
            ResourceCollection(archive_file, struct_def_class, handler,
                               resource_time_zone),
        'SearchParameter':
            ResourceCollection(archive_file, search_param_class, handler,
                               resource_time_zone),
        'CodeSystem':
            ResourceCollection(archive_file, code_system_class, handler,
                               resource_time_zone),
        'ValueSet':
            ResourceCollection(archive_file, value_set_class, handler,
                               resource_time_zone),
    }

    with _open_path_or_factory(archive_file) as fd:
      # Default to zip if there is no file name for compatibility.
      if not isinstance(fd.name, str) or fd.name.endswith('.zip'):
        json_files = _read_fhir_package_zip(fd)
      elif fd.name.endswith('.tar.gz') or fd.name.endswith('.tgz'):
        json_files = _read_fhir_package_npm(fd)
      else:
        raise ValueError(f'Unsupported file type from {fd.name}')

      for file_name, raw_json in json_files:
        json_obj = json.loads(
            raw_json, parse_float=decimal.Decimal, parse_int=decimal.Decimal)
        _add_resource_to_collection(json_obj, collections_per_resource_type,
                                    file_name)

    return FhirPackage(
        structure_definitions=collections_per_resource_type[
            'StructureDefinition'],
        search_parameters=collections_per_resource_type['SearchParameter'],
        code_systems=collections_per_resource_type['CodeSystem'],
        value_sets=collections_per_resource_type['ValueSet'])

  # TODO: Constrain version-agnostic types with structural typing.
  def __init__(self, *, structure_definitions: ResourceCollection,
               search_parameters: ResourceCollection,
               code_systems: ResourceCollection,
               value_sets: ResourceCollection) -> None:
    """Creates a new instance of `FhirPackage`. Callers should favor `load`."""
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


def _open_path_or_factory(path_or_factory: PackageSource) -> BinaryIO:
  """Either opens the file at the path or calls the factory to create a file."""
  if isinstance(path_or_factory, str):
    return open(path_or_factory, 'rb')
  else:
    return path_or_factory()
