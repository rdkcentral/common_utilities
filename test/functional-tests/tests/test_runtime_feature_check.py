import os
import shutil
import subprocess

import pytest


DEVICE_PROPERTIES = "/etc/device.properties"
RUNTIME_FEATURE_CHECK = next(
    (
        path
        for path in (
            shutil.which("runtime_feature_check"),
            "/usr/local/bin/runtime_feature_check",
            "/usr/bin/runtime_feature_check",
        )
        if path and os.path.exists(path)
    ),
    None,
)
SECURE_DEBUG_STATE = "/opt/enable_secure_dbg"


BACKUP = "/tmp/device.properties.runtime_feature_l2.bak"
STATE_BACKUP = "/tmp/enable_secure_dbg.runtime_feature_l2.bak"



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



def write_secure_debug_state(enabled):
    with open(SECURE_DEBUG_STATE, "w") as fp:
        fp.write("{}\n".format("1" if enabled else "0"))


def set_signedlab_state(enabled):
    replace_property("LABSIGNED_ENABLED", "true")
    set_build_type("signedlab")
    write_secure_debug_state(enabled)


def run_runtime_feature_check():
    assert RUNTIME_FEATURE_CHECK is not None, (
        "runtime_feature_check is not installed or could not be located"
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

    state_existed = os.path.exists(SECURE_DEBUG_STATE)

    if state_existed:
        shutil.copy2(SECURE_DEBUG_STATE, STATE_BACKUP)

    yield

    shutil.copy2(BACKUP, DEVICE_PROPERTIES)

    if state_existed:
        shutil.copy2(STATE_BACKUP, SECURE_DEBUG_STATE)
    elif os.path.exists(SECURE_DEBUG_STATE):
        os.remove(SECURE_DEBUG_STATE)

    if os.path.exists(BACKUP):
        os.remove(BACKUP)

    if os.path.exists(STATE_BACKUP):
        os.remove(STATE_BACKUP)


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
