import os
import shutil
import subprocess
import time

import pytest


DEVICE_PROPERTIES = "/etc/device.properties"
RUNTIME_FEATURE_CHECK = "/usr/bin/runtime_feature_check"
SECURE_DEBUG_STATE = "/opt/enable_secure_dbg"

DEVICE_TYPE_RFC = (
    "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Identity.DeviceType"
)
DBG_SERVICES_RFC = (
    "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Identity.DbgServices.Enable"
)

BACKUP = "/tmp/device.properties.runtime_feature_l2.bak"


def run_command(command):
    return subprocess.run(
        command,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )


def replace_property(key, value):
    with open(DEVICE_PROPERTIES, "r") as fp:
        lines = fp.readlines()

    updated = []
    found = False

    for line in lines:
        if line.startswith(key + "="):
            updated.append("{}={}\n".format(key, value))
            found = True
        else:
            updated.append(line)

    if not found:
        updated.append("{}={}\n".format(key, value))

    with open(DEVICE_PROPERTIES, "w") as fp:
        fp.writelines(updated)


def set_build_type(build_type):
    replace_property("BUILD_TYPE", build_type)


def set_rfc(parameter, value, data_type):
    cmd = (
        "tr181 -d -s -t {} -v {} {}"
        .format(data_type, value, parameter)
    )

    result = run_command(cmd)

    assert result.returncode == 0, (
        "Failed to set RFC {}={}.\nstdout={}\nstderr={}"
        .format(parameter, value, result.stdout, result.stderr)
    )


def wait_for_runtime_state(expected, timeout=10):
    end = time.time() + timeout

    while time.time() < end:
        if os.path.exists(SECURE_DEBUG_STATE):
            with open(SECURE_DEBUG_STATE, "r") as fp:
                if fp.read().strip() == expected:
                    return

        time.sleep(0.5)

    raise AssertionError(
        "{} did not become {}".format(
            SECURE_DEBUG_STATE,
            expected
        )
    )


def set_signedlab_state(enabled):
    replace_property("LABSIGNED_ENABLED", "true")
    set_build_type("signedlab")

    set_rfc(
        DEVICE_TYPE_RFC,
        "test",
        "string"
    )

    set_rfc(
        DBG_SERVICES_RFC,
        "true" if enabled else "false",
        "bool"
    )

    wait_for_runtime_state(
        "1" if enabled else "0"
    )


def run_runtime_feature_check():
    assert os.path.exists(RUNTIME_FEATURE_CHECK), (
        "{} is not installed".format(RUNTIME_FEATURE_CHECK)
    )

    return subprocess.run(
        [RUNTIME_FEATURE_CHECK],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )


@pytest.fixture(autouse=True)
def preserve_device_properties():
    shutil.copy2(DEVICE_PROPERTIES, BACKUP)

    yield

    shutil.copy2(BACKUP, DEVICE_PROPERTIES)

    if os.path.exists(BACKUP):
        os.remove(BACKUP)


def test_runtime_feature_dev_enabled():
    set_build_type("dev")

    result = run_runtime_feature_check()

    assert result.returncode == 0


def test_runtime_feature_prod_disabled():
    set_build_type("prod")

    result = run_runtime_feature_check()

    assert result.returncode == 1


def test_runtime_feature_unknown_disabled():
    set_build_type("unknown")

    result = run_runtime_feature_check()

    assert result.returncode == 1


def test_runtime_feature_signedlab_enabled():
    set_signedlab_state(True)

    result = run_runtime_feature_check()

    assert result.returncode == 0


def test_runtime_feature_signedlab_disabled():
    set_signedlab_state(False)

    result = run_runtime_feature_check()

    assert result.returncode == 1
