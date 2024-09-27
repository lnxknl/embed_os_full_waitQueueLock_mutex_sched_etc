/**
 * @file traps_core.h
 * @brief
 * @author Denis Deryugin <deryugin.denis@gmail.com>
 * @version 0.1
 * @date 2015-08-17
 */

#ifndef ASM_HAL_ENV_TRAPS_CORE_H_
#define ASM_HAL_ENV_TRAPS_CORE_H_

#include <stdint.h>

/** Defines handler for traps_dispatcher in arm archecture */
typedef int (*__trap_handler)(uint32_t nr, void *data);

/** Defines traps environment for arm structure */
typedef struct __traps_env {
} __traps_env_t;

#endif /* ASM_HAL_ENV_TRAPS_CORE_H_ */
