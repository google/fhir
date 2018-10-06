#
# Copyright 2018 Google LLC
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
"""Client for FHIR bulk-data protocol.

See https://github.com/smart-on-fhir/fhir-bulk-data-docs for details on this
API.
Possible bulkdata servers to read from:
 * https://bulk-data.smarthealthit.org
 * https://test.fhir.org/r3
 * https://fhir-open.stagingcerner.com/r4/a758f80e-aa74-4118-80aa-98cc75846c76/

Example client. Not for production use. No support for authentication.
"""

from multiprocessing.dummy import Pool as ThreadPool
import sys
import tempfile
import time
from absl import app
from absl import flags
import requests

FLAGS = flags.FLAGS
flags.DEFINE_string("server", None, "URL of FHIR server")
flags.DEFINE_string("group_id", None, "Group ID. If none, fetch all patients")
flags.DEFINE_string("epic_client_id", None, "Client ID for Epic servers")
flags.DEFINE_boolean("debug", False, "Print debug info")


def download(url, resource_type):
  headers = {"Accept": "application/fhir+json"}
  if FLAGS.epic_client_id:
    headers["Epic-Client-ID"] = FLAGS.epic_client_id

  if FLAGS.debug:
    print "GET %s" % url
  r = requests.get(url, headers=headers)
  if r.status_code == 200:
    if FLAGS.debug:
      print "DONE %s" % url
    return (resource_type, r.content)
  else:
    if FLAGS.debug:
      print "ERROR: ", r
    return ""


def main(argv):
  del argv  # Unused.
  headers = {"Prefer": "respond-async", "Accept": "application/fhir+json"}
  if FLAGS.epic_client_id:
    headers["Epic-Client-ID"] = FLAGS.epic_client_id

  url = FLAGS.server
  if FLAGS.group_id:
    url += "/Group/%s/$export" % FLAGS.group_id
  else:
    url += "/Patient/$export"

  if FLAGS.debug:
    print "GET ", url, headers
  wait = requests.get(url=url, headers=headers)
  if FLAGS.debug:
    print "HTTP ", wait.status_code
    print("response header: %s" % "\n".join(
        ["%s: %s" % (k, v) for k, v in wait.headers.iteritems()]))
  poll_url = wait.headers["Content-Location"]

  links = []
  poll_headers = {"Accept": "application/json"}
  if FLAGS.epic_client_id:
    poll_headers["Epic-Client-ID"] = FLAGS.epic_client_id

  s = requests.session()
  sys.stdout.write("polling ")
  while True:
    done = s.get(url=poll_url, headers=poll_headers)
    print done.status_code
    if done.status_code == 200:
      links = done.json().get("output", [])
      break
    sys.stdout.write(".")
    sys.stdout.flush()
    time.sleep(0.1)
  print "\n"

  pool = ThreadPool(len(links))
  results = pool.map(lambda d: download(d["url"], d["type"]), links)

  pool.close()
  pool.join()
  outputs = {}
  for k, v in results:
    if k not in outputs:
      outputs[k] = ""
    outputs[k] += v

  tempdir = tempfile.mkdtemp()
  for fname, content in outputs.iteritems():
    f = open("%s/%s.ndjson" % (tempdir, fname), "w")
    f.write("\n".join(content.splitlines()))
    f.close()
  print "FHIR data written to %s" % tempdir


if __name__ == "__main__":
  flags.mark_flag_as_required("server")
  app.run(main)
