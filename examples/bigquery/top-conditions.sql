-- Copyright 2018 Google LLC
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     https://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.

SELECT
  cc.code snomed_ct,
  c.code.text as diagnosis,
  COUNT(DISTINCT subject.patientId) AS num_patients,
  COUNT(DISTINCT context.encounterId) AS num_encounters
FROM
  synthea.Condition c,
  c.code.coding cc
WHERE
  cc.system LIKE '%snomed%'
GROUP BY
  1,2
ORDER BY
  3 DESC
LIMIT 20
