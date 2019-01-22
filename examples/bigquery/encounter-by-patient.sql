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
  encounter_class,
  APPROX_QUANTILES(num_encounters, 4) num_encounters_quantiles
FROM (
  SELECT
    class.code encounter_class,
    subject.patientId patient_id,
    COUNT(DISTINCT id) AS num_encounters
  FROM
    synthea.Encounter
  GROUP BY
    1,2
  )
GROUP BY 1
ORDER BY 1
