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

import contextlib
import json
import os
import tempfile
from typing import Callable, Iterable, Sequence, Tuple
from unittest import mock
import zipfile

from absl import flags
from google.protobuf import message
from absl.testing import absltest
from absl.testing import parameterized
from proto.google.fhir.proto import annotations_pb2
from proto.google.fhir.proto import profile_config_pb2
from proto.google.fhir.proto.r4.core.resources import value_set_pb2
from google.fhir.core.utils import fhir_package
from google.fhir.core.utils import proto_utils

FLAGS = flags.FLAGS

_R4_DEFINITIONS_COUNT = 653
_R4_CODESYSTEMS_COUNT = 1062
_R4_VALUESETS_COUNT = 1316
_R4_SEARCH_PARAMETERS_COUNT = 1385


def _parameterized_with_package_factories(func):
  parameters = [
      {
          'testcase_name': 'WithFilePath',
          'package_factory': fhir_package.FhirPackage.load,
      },
      {
          'testcase_name': 'WithFileFactory',
          'package_factory': _package_factory_using_file_factory
      },
  ]
  wrapper = parameterized.named_parameters(*parameters)
  return wrapper(func)


def _package_factory_using_file_factory(path: str) -> fhir_package.FhirPackage:
  return fhir_package.FhirPackage.load(lambda: open(path, 'rb'))


class FhirPackageTest(parameterized.TestCase):

  def testFhirPackageEquality_withEqualOperands_succeeds(self):
    lhs = fhir_package.FhirPackage(
        package_info=profile_config_pb2.PackageInfo(proto_package='Foo'),
        structure_definitions=[],
        search_parameters=[],
        code_systems=[],
        value_sets=[])
    rhs = fhir_package.FhirPackage(
        package_info=profile_config_pb2.PackageInfo(proto_package='Foo'),
        structure_definitions=[],
        search_parameters=[],
        code_systems=[],
        value_sets=[])
    self.assertEqual(lhs, rhs)

  def testFhirPackageEquality_withNonEqualOperands_succeeds(self):
    lhs = fhir_package.FhirPackage(
        package_info=profile_config_pb2.PackageInfo(proto_package='Foo'),
        structure_definitions=[],
        search_parameters=[],
        code_systems=[],
        value_sets=[])
    rhs = fhir_package.FhirPackage(
        package_info=profile_config_pb2.PackageInfo(proto_package='Bar'),
        structure_definitions=[],
        search_parameters=[],
        code_systems=[],
        value_sets=[])
    self.assertNotEqual(lhs, rhs)

  @_parameterized_with_package_factories
  def testFhirPackageLoad_withValidFhirPackage_succeeds(
      self, package_factory: Callable[[str], fhir_package.FhirPackage]):
    package_filepath = os.path.join(
        FLAGS.test_srcdir, 'com_google_fhir/spec/fhir_r4_package.zip')
    package = package_factory(package_filepath)

    self.assertEqual(package.package_info.proto_package, 'google.fhir.r4.core')
    self.assertLen(package.structure_definitions, _R4_DEFINITIONS_COUNT)
    self.assertLen(package.code_systems, _R4_CODESYSTEMS_COUNT)
    self.assertLen(package.value_sets, _R4_VALUESETS_COUNT)
    self.assertLen(package.search_parameters, _R4_SEARCH_PARAMETERS_COUNT)

  @_parameterized_with_package_factories
  def testFhirPackageLoad_withValidFhirPackage_isReadable(
      self, package_factory: Callable[[str], fhir_package.FhirPackage]):
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
    package_info = profile_config_pb2.PackageInfo(
        proto_package='Foo', fhir_version=annotations_pb2.R4)

    # Create a zip file containing the resources and our bundle.
    zipfile_contents = [
        ('package_info.prototxt', repr(package_info)),
        ('sd1.json', json.dumps(structure_definition_1)),
        ('sp1.json', json.dumps(search_parameter_1)),
        ('cs1.json', json.dumps(code_system_1)),
        ('vs1.json', json.dumps(value_set_1)),
        ('bundle.json', json.dumps(bundle)),
    ]
    with zipfile_containing(zipfile_contents) as temp_file:
      package = package_factory(temp_file.name)

      # Ensure all resources can be found by get_resource calls
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
        found_resource = package.get_resource(resource['url'])
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

  def testFhirPackageGetResource_forMissingUri_isNone(self):
    """Ensure we return None when requesting non-existent resource URIs."""
    package = fhir_package.FhirPackage(
        package_info=profile_config_pb2.PackageInfo(proto_package='Foo'),
        structure_definitions=fhir_package.ResourceCollection(''),
        search_parameters=fhir_package.ResourceCollection(''),
        code_systems=fhir_package.ResourceCollection(''),
        value_sets=fhir_package.ResourceCollection(''),
    )
    self.assertIsNone(package.get_resource('some_uri'))


class ResourceCollectionTest(absltest.TestCase):

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
      collection = fhir_package.ResourceCollection(temp_file.name)
      collection.add_uri_at_path(uri, 'a_value_set.json', 'ValueSet')
      resource = collection.get_resource(uri)

      self.assertIsNotNone(resource)
      self.assertTrue(
          proto_utils.is_message_type(resource, value_set_pb2.ValueSet))
      self.assertEqual(resource.id.value, 'example-extensional')
      self.assertEqual(resource.url.value, uri)

  def testResourceCollection_getBundles(self):
    """Ensure we can add and then get a resource from a bundle."""
    zipfile_contents = [('a_bundle.json',
                         json.dumps({
                             'resourceType':
                                 'Bundle',
                             'url':
                                 'http://bundles.com',
                             'entry': [{
                                 'resource': {
                                     'resourceType': 'ValueSet',
                                     'id': 'example-extensional',
                                     'url': 'http://value-in-a-bundle',
                                     'status': 'draft',
                                 }
                             }]
                         }))]
    with zipfile_containing(zipfile_contents) as temp_file:
      collection = fhir_package.ResourceCollection(temp_file.name)
      collection.add_uri_at_path('http://value-in-a-bundle', 'a_bundle.json',
                                 'Bundle')
      resource = collection.get_resource('http://value-in-a-bundle')

      self.assertIsNotNone(resource)
      self.assertTrue(
          proto_utils.is_message_type(resource, value_set_pb2.ValueSet))
      self.assertEqual(resource.id.value, 'example-extensional')
      self.assertEqual(resource.url.value, 'http://value-in-a-bundle')

  def testResourceCollection_getMissingResource(self):
    """Ensure we return None when requesing missing resources."""
    collection = fhir_package.ResourceCollection('missing_file.zip')
    resource = collection.get_resource('missing-uri')

    self.assertIsNone(resource)

  # A bug present in the Python version running tests in kokoro prevents us
  # from using auto-spec on static methods.
  @mock.patch.object(
      fhir_package.ResourceCollection,
      '_parse_proto_from_file',
      spec=fhir_package.ResourceCollection._parse_proto_from_file)
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
      collection = fhir_package.ResourceCollection(temp_file.name)
      collection.add_uri_at_path(uri, 'a_value_set.json', 'ValueSet')

      # The resource unexpectedly has the wrong url
      mock_parse_proto_from_file().url.value = 'something unexpected'
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
      collection = fhir_package.ResourceCollection(temp_file.name)
      collection.add_uri_at_path(uri, 'a_value_set.json', 'ValueSet')
      # Get the resource for the first time to cache it
      resource = collection.get_resource(uri)

    # The temp file is now gone, so this shouldn't succeed without a cache.
    cached_resource = collection.get_resource(uri)

    self.assertIsNotNone(cached_resource)
    self.assertEqual(cached_resource, resource)


class FhirPackageManagerTest(absltest.TestCase):

  def testGetResource_withAddedPackages_retrievesResource(self):
    vs_1 = value_set_pb2.ValueSet()
    vs_1.url.value = 'vs1'

    vs_2 = value_set_pb2.ValueSet()
    vs_2.url.value = 'vs2'

    package_1 = fhir_package.FhirPackage(
        package_info=mock.MagicMock(),
        structure_definitions=mock_resource_collection_containing([]),
        search_parameters=mock_resource_collection_containing([]),
        code_systems=mock_resource_collection_containing([]),
        value_sets=mock_resource_collection_containing([vs_1]),
    )
    package_2 = fhir_package.FhirPackage(
        package_info=mock.MagicMock(),
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
def zipfile_containing(
    file_contents: Sequence[Tuple[str, str]]) -> tempfile.NamedTemporaryFile:
  """Builds a temp file containing a zip file with the given contents.

  Args:
    file_contents: Sequence of (file_name, file_contents) tuples to be written
      to the zip file.

  Yields:
    A tempfile.NamedTemporaryFile for the written zip file.
  """
  with tempfile.NamedTemporaryFile() as temp_file:
    with zipfile.ZipFile(temp_file, 'w') as zip_file:
      for file_name, contents in file_contents:
        zip_file.writestr(file_name, contents)
    temp_file.flush()
    yield temp_file


def mock_resource_collection_containing(
    resources: Iterable[message.Message]) -> mock.MagicMock:
  """Builds a mock for a ResourceCollection containing the given resources."""
  mock_collection = mock.MagicMock(spec=fhir_package.ResourceCollection)
  resources = {resource.url.value: resource for resource in resources}

  def mock_get_resource(uri: str) -> message.Message:
    return resources.get(uri)

  mock_collection.get_resource.side_effect = mock_get_resource

  return mock_collection


if __name__ == '__main__':
  absltest.main()
