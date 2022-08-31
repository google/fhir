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
"""Test fhir_package functionality."""
# TODO: Add tests that can run without version-specific protos.

import abc
import contextlib
import io
import json
import pickle
import tarfile
import tempfile
from typing import Any, Callable, Iterable, Sequence, Tuple, cast
from unittest import mock
import zipfile

from absl import flags

from google.protobuf import message
from absl.testing import absltest
from absl.testing import parameterized
from google.fhir.core.utils import fhir_package
from google.fhir.core.utils import proto_utils

FLAGS = flags.FLAGS


def _parameterized_with_package_sources(func):
  """Test parameters for package sources."""
  parameters = [
      {
          'testcase_name': 'WithFilePath',
          'package_source_fn': _package_source_direct_path,
      },
      {
          'testcase_name': 'WithFileFactory',
          'package_source_fn': _package_source_callable,
      },
  ]
  wrapper = parameterized.named_parameters(*parameters)
  return wrapper(func)


PackageSourceFn = Callable[[str], fhir_package.PackageSource]


def _package_source_direct_path(path: str) -> fhir_package.PackageSource:
  return path


def _package_source_callable(path: str) -> fhir_package.PackageSource:
  return lambda: open(path, 'rb')


# Metaclass so base tests can inherit both ABC and parameterized.Testcase.
class _FhirPackageTestMeta(abc.ABCMeta, type(parameterized.TestCase)):
  pass


class FhirPackageTest(
    parameterized.TestCase, abc.ABC, metaclass=_FhirPackageTestMeta):
  """Base class for testing the FhirPackage class."""

  @property
  @abc.abstractmethod
  def _primitive_handler(self):
    raise NotImplementedError('Subclasses must implement _primitive_handler')

  @property
  @abc.abstractmethod
  def _valueset_cls(self):
    raise NotImplementedError('Subclasses must implement _valueset_cls')

  @abc.abstractmethod
  def _load_package(
      self,
      package_source: fhir_package.PackageSource) -> fhir_package.FhirPackage:
    raise NotImplementedError('Subclasses must implement _load_package')

  def empty_collection(self) -> fhir_package.ResourceCollection:
    return fhir_package.ResourceCollection('', self._valueset_cls,
                                           self._primitive_handler, 'Z')

  @_parameterized_with_package_sources
  def testFhirPackageLoad_withValidFhirPackage_isReadable(
      self, package_source_fn: PackageSourceFn):
    """Ensure we can read resources following a load."""
    # Define a bunch of fake resources.
    structure_definition_1 = {
        'resourceType': 'StructureDefinition',
        'url': 'http://sd1',
        'name': 'sd1',
        'kind': 'complex-type',
        'abstract': False,
        'type': 'Extension',
        'status': 'draft',
    }
    structure_definition_2 = {
        'resourceType': 'StructureDefinition',
        'url': 'http://sd2',
        'name': 'sd2',
        'kind': 'complex-type',
        'abstract': False,
        'type': 'Extension',
        'status': 'draft',
    }

    search_parameter_1 = {
        'resourceType': 'SearchParameter',
        'url': 'http://sp1',
        'name': 'sp1',
        'status': 'draft',
        'description': 'sp1',
        'code': 'facility',
        'base': ['Claim'],
        'type': 'reference',
    }
    search_parameter_2 = {
        'resourceType': 'SearchParameter',
        'url': 'http://sp2',
        'name': 'sp2',
        'status': 'draft',
        'description': 'sp2',
        'code': 'facility',
        'base': ['Claim'],
        'type': 'reference',
    }

    code_system_1 = {
        'resourceType': 'CodeSystem',
        'url': 'http://cs1',
        'name': 'cs1',
        'status': 'draft',
        'content': 'complete',
    }
    code_system_2 = {
        'resourceType': 'CodeSystem',
        'url': 'http://cs2',
        'name': 'cs2',
        'status': 'draft',
        'content': 'complete',
    }

    value_set_1 = {
        'resourceType': 'ValueSet',
        'url': 'http://vs1',
        'name': 'vs1',
        'status': 'draft'
    }
    value_set_2 = {
        'resourceType': 'ValueSet',
        'url': 'http://vs2',
        'name': 'vs2',
        'status': 'draft'
    }

    # create a bundle for half of the resources
    bundle = {
        'resourceType':
            'Bundle',
        'entry': [
            {
                'resource': structure_definition_2
            },
            {
                'resource': search_parameter_2
            },
            # ensure we handle bundles containing other bundles
            {
                'resource': {
                    'resourceType':
                        'Bundle',
                    'entry': [{
                        'resource': code_system_2
                    }, {
                        'resource': value_set_2
                    }]
                }
            }
        ]
    }

    # Create zip and npm files containing the resources and our bundle.
    fhir_resource_contents = [
        ('sd1.json', json.dumps(structure_definition_1)),
        ('sp1.json', json.dumps(search_parameter_1)),
        ('cs1.json', json.dumps(code_system_1)),
        ('vs1.json', json.dumps(value_set_1)),
        ('bundle.json', json.dumps(bundle)),
    ]

    npm_package_info = {
        'name': 'example',
        'fhirVersions': ['4.0.1'],
        'license': 'Apache',
        'canonical': 'http://example.com/fhir',
    }
    npmfile_contents = fhir_resource_contents + [
        ('package.json', json.dumps(npm_package_info))
    ]

    # Helper to check contents for both zip and NPM/tar packages.
    def check_contents(package):
      for resource in (
          structure_definition_1,
          structure_definition_2,
          search_parameter_1,
          search_parameter_2,
          code_system_1,
          code_system_2,
          value_set_1,
          value_set_2,
      ):
        found_resource = cast(Any, package.get_resource(resource['url']))
        self.assertEqual(resource['url'], found_resource.url.value)
        self.assertEqual(resource['name'], found_resource.name.value)

      # Ensure we can iterate over all resources for each collection.
      self.assertCountEqual(
          [resource.url.value for resource in package.structure_definitions],
          [structure_definition_1['url'], structure_definition_2['url']])

      self.assertCountEqual(
          [resource.url.value for resource in package.search_parameters],
          [search_parameter_1['url'], search_parameter_2['url']])

      self.assertCountEqual(
          [resource.url.value for resource in package.code_systems],
          [code_system_1['url'], code_system_2['url']])

      self.assertCountEqual(
          [resource.url.value for resource in package.value_sets],
          [value_set_1['url'], value_set_2['url']])

    with zipfile_containing(fhir_resource_contents) as temp_file:
      package = self._load_package(package_source_fn(temp_file.name))
      check_contents(package)

    with npmfile_containing(npmfile_contents) as temp_file:
      package = self._load_package(package_source_fn(temp_file.name))
      check_contents(package)

  def testFhirPackageGetResource_forMissingUri_isNone(self):
    """Ensure we return None when requesting non-existent resource URIs."""
    package = fhir_package.FhirPackage(
        structure_definitions=self.empty_collection(),
        search_parameters=self.empty_collection(),
        code_systems=self.empty_collection(),
        value_sets=self.empty_collection())
    self.assertIsNone(package.get_resource('some_uri'))

  def testFhirPackage_pickle_isSuccessful(self):
    """Ensure FhirPackages are pickle-able."""
    package = fhir_package.FhirPackage(
        structure_definitions=self.empty_collection(),
        search_parameters=self.empty_collection(),
        code_systems=self.empty_collection(),
        value_sets=self.empty_collection())
    pickle.dumps(package)


class ResourceCollectionTest(absltest.TestCase, abc.ABC):
  """Base class for testing ResourceCollections."""

  @property
  @abc.abstractmethod
  def _primitive_handler(self):
    raise NotImplementedError('Subclasses must implement _primitive_handler')

  @property
  @abc.abstractmethod
  def _valueset_cls(self):
    raise NotImplementedError('Subclasses must implement _valueset_cls')

  def testResourceCollection_addGetResource(self):
    """Ensure we can add and then get a resource."""
    uri = 'http://hl7.org/fhir/ValueSet/example-extensional'
    zipfile_contents = [('a_value_set.json',
                         json.dumps({
                             'resourceType': 'ValueSet',
                             'url': uri,
                             'id': 'example-extensional',
                             'status': 'draft',
                         }))]
    with zipfile_containing(zipfile_contents) as temp_file:
      collection = fhir_package.ResourceCollection(temp_file.name,
                                                   self._valueset_cls,
                                                   self._primitive_handler, 'Z')
      collection.add_uri_at_path(uri, 'a_value_set.json', 'ValueSet')
      resource = collection.get_resource(uri)

      self.assertIsNotNone(resource)
      self.assertTrue(proto_utils.is_message_type(resource, self._valueset_cls))
      self.assertEqual(resource.id.value, 'example-extensional')
      self.assertEqual(resource.url.value, uri)

  def testResourceCollection_getBundles(self):
    """Ensure we can add and then get a resource from a bundle."""
    file_contents = [('a_bundle.json',
                      json.dumps({
                          'resourceType':
                              'Bundle',
                          'entry': [{
                              'resource': {
                                  'resourceType': 'ValueSet',
                                  'id': 'example-extensional',
                                  'url': 'http://value-in-a-bundle',
                                  'status': 'draft',
                              }
                          }]
                      }))]

    def check_resources(collection):
      resource = collection.get_resource('http://value-in-a-bundle')

      self.assertIsNotNone(resource)
      self.assertTrue(proto_utils.is_message_type(resource, self._valueset_cls))
      self.assertEqual(resource.id.value, 'example-extensional')
      self.assertEqual(resource.url.value, 'http://value-in-a-bundle')

    with zipfile_containing(file_contents) as temp_file:
      collection = fhir_package.ResourceCollection(temp_file.name,
                                                   self._valueset_cls,
                                                   self._primitive_handler, 'Z')
      collection.add_uri_at_path('http://value-in-a-bundle', 'a_bundle.json',
                                 'Bundle')
      check_resources(collection)

    with npmfile_containing(file_contents) as temp_file:
      collection = fhir_package.ResourceCollection(temp_file.name,
                                                   self._valueset_cls,
                                                   self._primitive_handler, 'Z')
      collection.add_uri_at_path('http://value-in-a-bundle',
                                 'package/a_bundle.json', 'Bundle')
      check_resources(collection)

  def testResourceCollection_getMissingResource(self):
    """Ensure we return None when requesing missing resources."""
    collection = fhir_package.ResourceCollection('missing_file.zip',
                                                 self._valueset_cls,
                                                 self._primitive_handler, 'Z')
    resource = collection.get_resource('missing-uri')

    self.assertIsNone(resource)

  @mock.patch.object(
      fhir_package.ResourceCollection, '_parse_proto_from_file', autospec=True)
  def testResourceCollection_getResourceWithUnexpectedUrl(
      self, mock_parse_proto_from_file):
    """Ensure we raise an error for bad state when retrieving resources."""
    uri = 'http://hl7.org/fhir/ValueSet/example-extensional'
    zipfile_contents = [('a_value_set.json',
                         json.dumps({
                             'resourceType': 'ValueSet',
                             'url': uri,
                             'id': 'example-extensional',
                             'status': 'draft',
                         }))]
    with zipfile_containing(zipfile_contents) as temp_file:
      collection = fhir_package.ResourceCollection(temp_file.name,
                                                   self._valueset_cls,
                                                   self._primitive_handler, 'Z')
      collection.add_uri_at_path(uri, 'a_value_set.json', 'ValueSet')

      # The resource unexpectedly has the wrong url
      mock_parse_proto_from_file.return_value.url.value = 'something unexpected'
      with self.assertRaises(RuntimeError):
        collection.get_resource(uri)

      # The resource is unexpectedly missing.
      mock_parse_proto_from_file.return_value = None
      with self.assertRaises(RuntimeError):
        collection.get_resource(uri)

  def testResourceCollection_cachedResource(self):
    """Ensure we cache the first access to a resource."""
    uri = 'http://hl7.org/fhir/ValueSet/example-extensional'
    zipfile_contents = [('a_value_set.json',
                         json.dumps({
                             'resourceType': 'ValueSet',
                             'url': uri,
                             'id': 'example-extensional',
                             'status': 'draft',
                         }))]
    with zipfile_containing(zipfile_contents) as temp_file:
      collection = fhir_package.ResourceCollection(temp_file.name,
                                                   self._valueset_cls,
                                                   self._primitive_handler, 'Z')
      collection.add_uri_at_path(uri, 'a_value_set.json', 'ValueSet')
      # Get the resource for the first time to cache it
      resource = collection.get_resource(uri)

    # The temp file is now gone, so this shouldn't succeed without a cache.
    cached_resource = collection.get_resource(uri)

    self.assertIsNotNone(cached_resource)
    self.assertEqual(cached_resource, resource)


class FhirPackageManagerTest(absltest.TestCase, abc.ABC):
  """"Base class for testing FhirPackageManager."""

  @property
  @abc.abstractmethod
  def _valueset_cls(self):
    raise NotImplementedError('Subclasses must implement _valueset_cls')

  def testGetResource_withAddedPackages_retrievesResource(self):
    """Test getting resources added to packages."""
    vs_1 = self._valueset_cls()
    vs_1.url.value = 'vs1'

    vs_2 = self._valueset_cls()
    vs_2.url.value = 'vs2'

    package_1 = fhir_package.FhirPackage(
        structure_definitions=mock_resource_collection_containing([]),
        search_parameters=mock_resource_collection_containing([]),
        code_systems=mock_resource_collection_containing([]),
        value_sets=mock_resource_collection_containing([vs_1]),
    )
    package_2 = fhir_package.FhirPackage(
        structure_definitions=mock_resource_collection_containing([]),
        search_parameters=mock_resource_collection_containing([]),
        code_systems=mock_resource_collection_containing([]),
        value_sets=mock_resource_collection_containing([vs_2]),
    )

    manager = fhir_package.FhirPackageManager()
    manager.add_package(package_1)
    manager.add_package(package_2)

    self.assertEqual(manager.get_resource('vs1'), vs_1)
    self.assertEqual(manager.get_resource('vs2'), vs_2)


@contextlib.contextmanager
def zipfile_containing(file_contents: Sequence[Tuple[str, str]]):
  """Builds a temp file containing a zip file with the given contents.

  Args:
    file_contents: Sequence of (file_name, file_contents) tuples to be written
      to the zip file.

  Yields:
    A tempfile.NamedTemporaryFile for the written zip file.
  """
  with tempfile.NamedTemporaryFile(suffix='.zip') as temp_file:
    with zipfile.ZipFile(temp_file, 'w') as zip_file:
      for file_name, contents in file_contents:
        zip_file.writestr(file_name, contents)
    temp_file.flush()
    yield temp_file


@contextlib.contextmanager
def npmfile_containing(file_contents: Sequence[Tuple[str, str]]):
  """Builds a temp file containing a NPM .tar.gz file with the given contents.

  Args:
    file_contents: Sequence of (file_name, file_contents) tuples to be written
      to the NPM file.

  Yields:
    A tempfile.NamedTemporaryFile for the written zip file.
  """
  with tempfile.NamedTemporaryFile(suffix='.tgz') as temp_file:
    with tarfile.open(fileobj=temp_file, mode='w:gz') as tar_file:
      for file_name, contents in file_contents:
        # NPM package contents live in the package/ directory.
        info = tarfile.TarInfo(name=f'package/{file_name}')
        info.size = len(contents)
        tar_file.addfile(
            tarinfo=info, fileobj=io.BytesIO(contents.encode('utf-8')))
    temp_file.flush()
    yield temp_file


def mock_resource_collection_containing(
    resources: Iterable[message.Message]) -> mock.MagicMock:
  """Builds a mock for a ResourceCollection containing the given resources."""
  mock_collection = mock.MagicMock(spec=fhir_package.ResourceCollection)
  resources = {
      cast(Any, resource).url.value: resource for resource in resources
  }

  def mock_get_resource(uri: str) -> message.Message:
    return resources.get(uri)

  mock_collection.get_resource.side_effect = mock_get_resource

  return mock_collection
