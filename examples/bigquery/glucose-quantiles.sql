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
  cc.code.value loinc_code,
  cc.display.value loinc_name,
  approx_quantiles(round(o.value.quantity.value.value,1),4) as quantiles,
  count(*) as num_obs
FROM
  synthea.Observation o, o.code.coding cc
WHERE
  cc.system.value like '%loinc%' and lower(cc.display.value) like '%cholesterol%'
GROUP BY 1,2
ORDER BY 4 desc

