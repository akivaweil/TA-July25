# PlatformIO pre-script (extra_scripts = pre:lib/LineLink/tools/flash_request.py).
# Before an OTA upload, ask the machine for an upload slot (flash interlock,
# lib/LineLink/src/LineLinkFlashGate.h) and wait until it is ready:
#   - Stage 1 starts no new cycle once the request is in;
#   - the pieces already in the line keep moving; the upload starts when this
#     machine and every machine before it (Stage 1 -> TA -> Stage 2 -> Router)
#     are idle; machines after it finish their cycles.
# Firmware without the /api/flash endpoint is uploaded directly (old behaviour).

Import("env")  # noqa: F821 (provided by PlatformIO / SCons)

import json
import time
import urllib.error
import urllib.request

FLASH_WAIT_S = 900       # give up (and cancel the request) after this long
POLL_S = 1.0             # status poll period
HTTP_TIMEOUT_S = 3


def _flash_api(host, query=""):
    with urllib.request.urlopen(f"http://{host}/api/flash{query}", timeout=HTTP_TIMEOUT_S) as r:
        body = json.loads(r.read().decode())
    if not isinstance(body, dict) or "machine" not in body:
        raise ValueError("not an upload-slot reply")
    return body


def request_upload_slot(source, target, env):
    if env.subst("$UPLOAD_PROTOCOL") != "espota":
        return
    host = env.subst("$UPLOAD_PORT")
    try:
        status = _flash_api(host, "?request=1")
    except (urllib.error.HTTPError, ValueError, json.JSONDecodeError):
        print("[flash] firmware has no upload-slot endpoint: uploading directly")
        return
    except Exception as e:  # unreachable: let espota report it
        print(f"[flash] {host} not reachable for an upload slot ({e}); trying the upload anyway")
        return

    print(f"[flash] upload slot requested on {status['machine']}: Stage 1 starts no new cycle")
    start = time.time()
    last = None
    while not status.get("ready"):
        waiting = status.get("waitingFor") or "the line to settle"
        if waiting != last:
            print(f"[flash] waiting for {waiting} ...")
            last = waiting
        if time.time() - start > FLASH_WAIT_S:
            try:
                _flash_api(host, "?cancel=1")
            except Exception:
                pass
            print(f"[flash] not ready after {FLASH_WAIT_S} s: request cancelled")
            env.Exit(1)
        time.sleep(POLL_S)
        try:
            status = _flash_api(host)
        except Exception as e:
            print(f"[flash] status read failed ({e}); retrying")
    print(f"[flash] {status['machine']} ready after {time.time() - start:.0f} s - uploading")


env.AddPreAction("upload", request_upload_slot)  # noqa: F821
