/*
 * Runtime Feature Check Utility
 *
 * Provides a command-line interface for shell scripts to query
 * RDK_isDbgSrvUnlocked() from common utilities.
 *
 * Exit status:
 *   0 - runtime feature access is enabled
 *   1 - runtime feature access is disabled
 */

#include "common_device_api.h"
#include "rdkv_cdl_log_wrapper.h"

int main(void)
{
    bool enabled = RDK_isDbgSrvUnlocked();

    COMMONUTILITIES_INFO("runtime_feature_check invoked: runtimeFeatureEnabled=%s\n",
                         enabled ? "true" : "false");

    return enabled ? 0 : 1;
}
