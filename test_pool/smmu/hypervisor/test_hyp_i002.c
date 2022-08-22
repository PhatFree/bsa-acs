/** @file
 * Copyright (c) 2016-2018, 2021-2022 Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

#include "val/include/bsa_acs_val.h"
#include "val/include/val_interface.h"

#include "val/include/bsa_acs_pe.h"
#include "val/include/bsa_acs_smmu.h"
#include "val/include/bsa_acs_pcie.h"

#define TEST_NUM   (ACS_SMMU_HYP_TEST_NUM_BASE + 2)
#define TEST_RULE  "B_SMMU_18 "
#define TEST_DESC  "SMMU Revision,S-EL2 support Hyp       "

static
void
payload()
{
  /* Check SMMU Revision & S-EL2 Support for Hypervisor */
  uint32_t num_smmu;
  uint32_t index;
  uint32_t s_el2;
  uint32_t smmu_rev;
  uint32_t minor;
  uint32_t s2ts, s2p;

  index = val_pe_get_index_mpid(val_pe_get_mpid());
  s_el2 = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 36, 39);

  num_smmu = val_smmu_get_info(SMMU_NUM_CTRL, 0);
  if (num_smmu == 0) {
      val_print(ACS_PRINT_ERR, "\n       No SMMU Controllers are discovered ", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM, 1));
      return;
  }

  if (s_el2) {
      val_print(ACS_PRINT_DEBUG, "\n       S-EL2 implemented...Skipping", 0);
      val_set_status(index, RESULT_SKIP(TEST_NUM, 2));
      return;
  }

  while (num_smmu--) {
      smmu_rev = val_smmu_get_info(SMMU_CTRL_ARCH_MAJOR_REV, num_smmu);

      if (smmu_rev == 2) {
          s2ts = VAL_EXTRACT_BITS(val_smmu_read_cfg(SMMUv2_IDR0, num_smmu), 29, 29);
          // Stage 2 translation functionality cannot be provided by SMMU v2 revision
          if (!s2ts) {
              val_print(ACS_PRINT_ERR,
                        "\n       SMMUv2 not providing Stage2 functionality", 0);
              val_set_status(index, RESULT_FAIL(TEST_NUM, 1));
              return;
          }
      }
      else if (smmu_rev == 3) {
          minor = VAL_EXTRACT_BITS(val_smmu_read_cfg(SMMUv3_AIDR, num_smmu), 0, 3);
          s2p = VAL_EXTRACT_BITS(val_smmu_read_cfg(SMMUv3_IDR0, num_smmu), 0, 0);
          // Stage 2 translation functionality cannot be provided by SMMU v3.0/3.1 revisions
          if (!s2p) {
              val_print(ACS_PRINT_ERR,
                        "\n       SMMUv3.%d not providing Stage2 functionality", minor);
              val_set_status(index, RESULT_FAIL(TEST_NUM, 2));
              return;
          }
      }
      if (smmu_rev < 2) {
          val_print(ACS_PRINT_ERR, "\n       SMMU revision must be at least v2  ", 0);
          val_set_status(index, RESULT_FAIL(TEST_NUM, 3));
          return;
      }
  }

  val_set_status(index, RESULT_PASS(TEST_NUM, 1));
}

uint32_t
hyp_i002_entry(uint32_t num_pe)
{

  uint32_t status = ACS_STATUS_FAIL;

  num_pe = 1;  //This test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
  if (status != ACS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);

  val_report_status(0, BSA_ACS_END(TEST_NUM), NULL);

  return status;
}
