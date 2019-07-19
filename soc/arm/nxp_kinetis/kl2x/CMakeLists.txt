# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_SOC_MKL25Z4 soc_kl25z.c)
zephyr_sources_ifdef(CONFIG_SOC_MKL27Z644 soc_kl27z.c)
