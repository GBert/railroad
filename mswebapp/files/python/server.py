"""
THE BEER-WARE LICENSE (Revision 42)

<mende.r@hotmail.de> wrote this file. As long as you retain this notice you can do whatever you want with this
stuff. If we meet someday, and you think this stuff is worth it, you can
buy me a beer in return.
Ralf Mende
"""

import os
import socket
import threading
import queue
import json
import argparse
import logging
import time
from enum import IntEnum, Enum
from flask import Flask, request, jsonify, render_template, Response, stream_with_context, send_from_directory

logging.basicConfig(level=logging.INFO, format='[%(asctime)s] %(levelname)s %(message)s')
logger = logging.getLogger('mswebapp')

try:
    from . import __version__ as _MSW_VERSION  # type: ignore
except Exception:
    _MSW_VERSION = "0.0.0"

BASE_DIR = os.path.abspath(os.path.dirname(__file__))
FRONTEND_DIR = os.path.abspath(os.path.join(BASE_DIR, '..', 'frontend'))

# Create Flask app with static/templates pointing to frontend folders
app = Flask(
    __name__,
    static_folder=os.path.join(FRONTEND_DIR, 'static'),
    template_folder=os.path.join(FRONTEND_DIR, 'templates')
)
# Keep effective frontend dir configurable at runtime (overridable via --www)
app.config['FRONTEND_DIR'] = FRONTEND_DIR

subscribers = set()
subs_lock = threading.Lock()
loc_data_lock = threading.Lock()
loc_list = []
switch_list = {}

loco_state = {}
switch_state = [0] * 64  # 64 switches, default state 0
icon_overrides: dict[str, str] = {}

class SystemState(str, Enum):
    STOPPED = 'stopped'
    RUNNING = 'running'
    HALTED  = 'halted'

system_state = SystemState.STOPPED

path_config_files = 'var'
PUBLIC_CONFIG_BASE = '/cfg'
UDP_IP = '127.0.0.1'
UDP_PORT_TX = 15731
UDP_PORT_RX = 15730
DEVICE_UID = 0

K_STATE = 'state'
K_LOCO_ID = 'loco_id'
K_SPEED = 'speed'
K_DIRECTION = 'direction'
K_FUNCTION = 'function'
K_VALUE = 'value'
_listener_started = False
_stop_evt = threading.Event()

class Command(IntEnum):
    SYSTEM = 0
    DISCOVERY = 1
    BIND = 2
    VERIFY = 3
    SPEED = 4
    DIRECTION = 5
    FUNCTION = 6
    READ_CONFIG = 7
    WRITE_CONFIG = 8
    SWITCH = 11

class SystemSubCmd(IntEnum):
    STOP = 0x00
    GO = 0x01
    HALT = 0x02
    LOCO_EMERGENCY_STOP = 0x03
    LOCO_CYCLE_STOP = 0x04
    LOCO_DATA_PROTOCOL = 0x05
    ACCESSORY_SWITCH_TIME = 0x06
    FAST_READ_MFX_SID = 0x07
    TRACK_PROTOCOL_ENABLE = 0x08
    MFX_REENROLL_COUNTER = 0x09
    SYSTEM_OVERLOAD = 0x0A
    SYSTEM_STATUS = 0x0B
    DEVICE_ID = 0x0C
    MFX_SEEK = 0x30
    SYSTEM_RESET = 0x80

class Direction(IntEnum):
    KEEP = 0
    FORWARD = 1
    REVERSE = 2
    TOGGLE = 3

FUNCTION_MIN = 0
FUNCTION_MAX = 31

# --- Event/SSE ---

def _require_json() -> dict:
    return request.get_json(silent=True) or {}

def publish_event(ev: dict):
    try:
        data = json.dumps(ev, separators=(',', ':'))
    except Exception:
        return
    with subs_lock:
        dead = []
        for q in list(subscribers):
            try:
                q.put_nowait(data)
            except Exception:
                dead.append(q)
        for q in dead:
            subscribers.discard(q)

@app.get('/api/events')
def sse_events():
    q = queue.Queue(maxsize=1000)
    with subs_lock:
        subscribers.add(q)

    def stream():
        try:
            try:
                status = 1 if system_state == SystemState.RUNNING else 0
                q.put_nowait(json.dumps({'type': 'system', 'status': status}, separators=(',', ':')))
                # Take a snapshot to avoid concurrent modification during iteration
                with loc_data_lock:
                    loco_items = list(loco_state.items())
                for uid, st in loco_items:
                    try:
                        spd = int(st.get('speed', 0))
                        dirv = int(st.get('direction', 1))
                        q.put_nowait(json.dumps({'type': 'speed', 'loc_id': uid, 'value': spd}, separators=(',', ':')))
                        q.put_nowait(json.dumps({'type': 'direction', 'loc_id': uid, 'value': dirv}, separators=(',', ':')))
                        fnmap = st.get('functions') or {}
                        for fn_idx, active in fnmap.items():
                            try:
                                fn_i = int(fn_idx)
                                fn_v = 1 if bool(active) else 0
                                q.put_nowait(json.dumps({'type': 'function', 'loc_id': uid, 'fn': fn_i, 'value': fn_v}, separators=(',', ':')))
                            except Exception:
                                continue
                    except Exception:
                        continue
                try:
                    for idx, val in enumerate(switch_state):
                        q.put_nowait(json.dumps({'type': 'switch', 'idx': idx, 'value': int(val)}, separators=(',', ':')))
                except Exception:
                    try:
                        for idx in range(64):
                            v = int(switch_state.get(idx, 0))
                            q.put_nowait(json.dumps({'type': 'switch', 'idx': idx, 'value': v}, separators=(',', ':')))
                    except Exception:
                        pass
            except Exception:
                pass

            while True:
                data = q.get()
                yield f'data: {data}\n\n'
        except GeneratorExit:
            pass
        finally:
            with subs_lock:
                subscribers.discard(q)

    headers = {
        "Cache-Control": "no-cache",
        "X-Accel-Buffering": "no"
    }
    return Response(stream_with_context(stream()), mimetype='text/event-stream', headers=headers)

# --- System state ---

def _payload_system_state(device_uid: int, running: bool) -> bytes:
    b = bytearray()
    b.extend(device_uid.to_bytes(4, 'big'))
    b.append(1 if running else 0)
    return b

def set_system_state(new_state):
    global system_state
    if system_state != new_state:
        system_state = new_state
        state_wif = 1 if system_state == SystemState.RUNNING else 0
        publish_event({'type': 'system', 'status': state_wif})

@app.route('/api/stop_button', methods=['POST'])
def toggle():
    data = _require_json()
    running = bool(data.get(K_STATE, False))
    payload = {K_LOCO_ID: DEVICE_UID, K_STATE: running}
    set_system_state(SystemState.RUNNING if running else SystemState.STOPPED)
    return send_cs2_udp(payload, [K_STATE], Command.SYSTEM, _payload_system_state, 5)

# --- Loco state ---

def _ensure_loco_state(uid: int):
    st = loco_state.get(uid)
    if st is None:
        st = {'speed': 0, 'direction': 1, 'functions': {}}
        loco_state[uid] = st
    return st

def _payload_speed(loco_uid: int, speed: int) -> bytes:
    b = bytearray()
    b.extend(loco_uid.to_bytes(4, 'big'))
    b.extend(int(speed).to_bytes(2, 'big'))
    return b

def _payload_direction(loco_uid: int, direction: int) -> bytes:
    b = bytearray()
    b.extend(loco_uid.to_bytes(4, 'big'))
    b.append(direction & 255)
    return b

def _payload_function(loco_uid: int, function: int, value: int) -> bytes:
    b = bytearray()
    b.extend(loco_uid.to_bytes(4, 'big'))
    b.append(function & 255)
    b.append(value & 255)
    return b

def _get_first(data: dict, *keys):
    for k in keys:
        if k in data:
            return data.get(k)
    return None

def set_loco_state_speed(loc_id, speed):
    with loc_data_lock:
        st = _ensure_loco_state(int(loc_id))
        st['speed'] = int(speed)
    publish_event({'type': 'speed', 'loc_id': loc_id, 'value': speed})

def set_loco_state_direction(loc_id, direction):
    with loc_data_lock:
        st = _ensure_loco_state(int(loc_id))
        st['direction'] = int(direction)
    publish_event({'type': 'direction', 'loc_id': loc_id, 'value': direction})

def set_loco_state_function(loc_id, fn_no, fn_val):
    with loc_data_lock:
        st = _ensure_loco_state(int(loc_id))
        st['functions'][int(fn_no)] = bool(fn_val)
    publish_event({'type': 'function', 'loc_id': loc_id, 'fn': fn_no, 'value': fn_val})

@app.route('/api/loco_list')
def get_locs():
    with loc_data_lock:
        items = list(loc_list)
    result: dict[str, dict] = {}
    for loco in items:
        try:
            uid_s = str(loco['uid'])
            entry = dict(loco)
            ov = icon_overrides.get(uid_s)
            if ov:
                entry['icon'] = ov
            result[uid_s] = entry
        except Exception:
            continue
    return jsonify(result)

@app.route('/api/loco_state')
def get_state():
    uid = request.args.get('loco_id', type=int)
    if uid is None:
        with loc_data_lock:
            snapshot = {str(k): v for k, v in loco_state.items()}
        return jsonify(snapshot)
    with loc_data_lock:
        st = dict(loco_state.get(uid, {}))
    return jsonify(st)

@app.route('/api/control_event', methods=['POST'])
def control_event():
    data = _require_json()
    if data.get(K_LOCO_ID) is None:
        return jsonify({'status': 'error', 'message': 'loco_id required'}), 400

    if K_SPEED in data:
        set_loco_state_speed(data.get(K_LOCO_ID), data.get(K_SPEED))
        return send_cs2_udp(data, [K_SPEED], Command.SPEED, _payload_speed, 6)

    if K_DIRECTION in data:
        set_loco_state_direction(data.get(K_LOCO_ID), data.get(K_DIRECTION))
        return send_cs2_udp(data, [K_DIRECTION], Command.DIRECTION, _payload_direction, 5)

    fn = _get_first(data, K_FUNCTION, 'fn')
    if fn is not None:
        val = _get_first(data, K_VALUE, 'val', 'on')
        data[K_FUNCTION] = fn
        data[K_VALUE] = val
        set_loco_state_function(data.get(K_LOCO_ID), fn, val)
        return send_cs2_udp(data, [K_FUNCTION, K_VALUE], Command.FUNCTION, _payload_function, 6)

    return jsonify({'status': 'error', 'message': 'no control field (speed/direction/function)'}), 400

# --- Switch state ---

def _ensure_switch_state(uid: int):
    st = switch_state.get(uid)
    if st is None:
        st = {'value': 0}
        switch_state[uid] = st
    return st

def _payload_switch(loco_uid: int, switch_state_v: int) -> bytes:
    b = bytearray()
    b.extend(loco_uid.to_bytes(4, 'big'))
    b.append(switch_state_v & 255)
    b.append(0x01)
    return b

@app.route('/api/switch_list')
def get_switch_list():
    return jsonify(switch_list)

@app.route('/api/switch_state')
def get_switch_state():
    return jsonify({'switch_state': switch_state})

def set_switch_state(idx, value):
    idx = int(idx)
    value = int(value)
    if 0 <= idx < 64:
        switch_state[idx] = value
        publish_event({'type': 'switch', 'idx': idx, 'value': value})

@app.route('/api/keyboard_event', methods=['POST'])
def keyboard_event():
    data = _require_json()
    idx = data.get('idx')
    value = data.get('value')
    if idx is None or value is None:
        return jsonify({'status': 'error', 'message': 'idx and value required'}), 400
    try:
        idx = int(idx)
        value = int(value)
    except Exception:
        return jsonify({'status': 'error', 'message': 'idx and value must be integers'}), 400

    set_switch_state(idx, value)

    uid = idx
    artikel = switch_list.get('artikel') if isinstance(switch_list, dict) else None
    if isinstance(artikel, list) and idx < len(artikel):
        entry = artikel[idx]
        uid = entry.get('uid', idx)

    payload = {K_LOCO_ID: uid, K_VALUE: value}
    return send_cs2_udp(payload, [K_VALUE], Command.SWITCH, _payload_switch, 6)

# --- CS2 interaction ---

def generate_hash(uid: int) -> int:
    hi = uid >> 16 & 65535
    lo = uid & 65535
    h = hi ^ lo
    h = h << 3 & 65280 | 768 | h & 127
    return h & 65535

def build_can_id(uid: int, command: int, prio: int = 0, resp: int = 0) -> int:
    hash16 = generate_hash(uid)
    return prio << 25 | (command << 1 | resp & 1) << 16 | hash16

def _pad_to_8(data: bytearray | bytes) -> bytes:
    b = bytearray(data)
    while len(b) < 8:
        b.append(0)
    return bytes(b)

def _udp_send_frame(can_id: int, payload: bytes | bytearray, dlc: int) -> None:
    data_bytes = _pad_to_8(payload)
    send_bytes = bytearray()
    send_bytes.extend(can_id.to_bytes(4, 'big'))
    send_bytes.append(dlc & 255)
    send_bytes.extend(data_bytes)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        target_ip = app.config.get('UDP_IP', UDP_IP)
        target_port = app.config.get('UDP_PORT_TX', UDP_PORT_TX)
    except Exception:
        target_ip = UDP_IP
        target_port = UDP_PORT_TX
    sock.sendto(send_bytes, (target_ip, target_port))

def _require_int(data: dict, key: str, err_msg: str) -> int:
    try:
        return int(data.get(key))
    except (TypeError, ValueError):
        raise ValueError(err_msg)

def _clamp_speed10(x: int) -> int:
    return max(0, min(1023, int(x)))


def _coerce_direction(val) -> int:
    try:
        if isinstance(val, (int, float)):
            i = int(val)
            return i
        s = str(val).strip().lower()
        mapping = {'keep': 0, 'forward': 1, 'fwd': 1, 'reverse': 2, 'rev': 2, 'toggle': 3}
        if s in mapping:
            return mapping[s]
        return int(s)
    except Exception:
        return 0

def _coerce_bool(val) -> int:
    if isinstance(val, bool):
        return 1 if val else 0
    try:
        if isinstance(val, (int, float)) and not isinstance(val, bool):
            return 1 if int(val) != 0 else 0
        s = str(val).strip().lower()
        return 1 if s in ('1', 'true', 'on', 'yes') else 0
    except Exception:
        return 0

def send_cs2_udp(data, key_map, can_command, payload_func, dlc):
    try:
        uid_int = _require_int(data, K_LOCO_ID, 'invalid loco_id')
    except ValueError as e:
        return (jsonify(status='error', message=str(e)), 400)
    try:
        values = [data.get(k) for k in key_map]
        if can_command == Command.SPEED:
            values[0] = _clamp_speed10(int(values[0]))
        elif can_command == Command.DIRECTION:
            values[0] = _coerce_direction(values[0])
        elif can_command == Command.FUNCTION:
            values[0] = int(values[0])
            values[1] = _coerce_bool(values[1])
        elif can_command == Command.SWITCH:
            values[0] = int(values[0])
    except (TypeError, ValueError):
        return (jsonify(status='error', message=f'invalid {key_map}'), 400)
    can_id = build_can_id(DEVICE_UID, can_command, prio=0, resp=0)
    data_bytes = payload_func(uid_int, *values)
    data_bytes = _pad_to_8(data_bytes)
    try:
        _udp_send_frame(can_id, data_bytes, dlc=dlc)
        return jsonify(status='ok')
    except Exception as e:
        return (jsonify(status='error', message=str(e)), 500)


def listen_cs2_udp(host: str = '', port: int = UDP_PORT_RX, stop_event: threading.Event | None = None):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        s.bind((host, port))
    except OSError as e:
        publish_event({'type': 'error', 'message': f'UDP bind failed: {e}'})
        return
    s.settimeout(1.0)
    try:
        while not (stop_event and stop_event.is_set()):
            try:
                pkt, _ = s.recvfrom(2048)
            except socket.timeout:
                continue
            except Exception as e:
                publish_event({'type': 'error', 'message': f'UDP recv error: {e}'})
                continue
            if len(pkt) != 13:
                continue
            can_id = int.from_bytes(pkt[0:4], 'big')
            dlc = pkt[4]
            data = pkt[5:13]
            cmd_resp = can_id >> 16 & 511
            command = cmd_resp >> 1 & 255
            resp_bit = cmd_resp & 1
            if resp_bit == 1:
                if command == Command.SYSTEM and dlc >= 5:
                    sub = data[4]
                    if sub in (0, 1, 2):
                        state = {0: SystemState.STOPPED, 1: SystemState.RUNNING, 2: SystemState.HALTED}[sub]
                        set_system_state(state)
                elif command == Command.SPEED and dlc >= 6:
                    loc_id = int.from_bytes(data[0:4], 'big')
                    speed = int.from_bytes(data[4:6], 'big')
                    set_loco_state_speed(loc_id, speed)
                elif command == Command.DIRECTION and dlc >= 5:
                    loc_id = int.from_bytes(data[0:4], 'big')
                    direction = data[4]
                    set_loco_state_direction(loc_id, direction)
                elif command == Command.FUNCTION and dlc >= 5:
                    loc_id = int.from_bytes(data[0:4], 'big')
                    fn_no = data[4]
                    fn_val = data[5] if dlc >= 6 else 1
                    set_loco_state_function(loc_id, fn_no, fn_val)
                elif command == Command.SWITCH and dlc >= 6:
                    idx = int.from_bytes(data[3:4], 'big')
                    value = data[4]
                    set_switch_state(idx, value)
    except Exception as e:
        publish_event({'type': 'error', 'message': f'UDP listener crashed: {e}'})
    finally:
        try:
            s.close()
        except Exception:
            pass

# --- Info API ---

@app.route('/api/info_events', methods=['POST'])
def srseii_commands():
    data = _require_json()
    loco_id = data.get('loco_id')
    fn_no = data.get('function', 0)
    value = data.get('value', 1)
    if loco_id is None:
        return jsonify({'status': 'error', 'message': 'loco_id fehlt'}), 400
    payload = {K_LOCO_ID: loco_id, K_FUNCTION: fn_no, K_VALUE: value}
    return send_cs2_udp(payload, [K_FUNCTION, K_VALUE], Command.FUNCTION, _payload_function, 6)

# --- Config parse ---

def parse_value(val):
    val = val.strip()
    if val.startswith('0x'):
        try:
            return int(val, 16)
        except ValueError:
            return val
    if val.isdigit():
        return int(val)
    return val

def magnetartikel_uid(id_int, dectyp):
    id_int = id_int - 1
    if dectyp == 'mm2':
        return 0x3000 | (id_int & 0x3FF)
    elif dectyp == 'dcc':
        return 0x3800 | (id_int & 0x3FF)
    elif dectyp == 'sx1':
        return 0x2800 | (id_int & 0x3FF)
    else:
        return (id_int & 0x3FF)

def parse_lokomotive_cs2(file_path):
    locomotives = []
    current_locomotive = None
    current_functions = {}
    current_function_key = None
    parsing_function = False
    with open(file_path, 'r', encoding='utf-8', errors='ignore') as file:
        for line in file:
            line = line.strip()
            if line == 'lokomotive':
                if current_locomotive:
                    if current_functions:
                        current_locomotive['funktionen'] = current_functions
                    locomotives.append(current_locomotive)
                current_locomotive = {}
                current_functions = {}
                current_function_key = None
                parsing_function = False
            elif current_locomotive is not None:
                if '.funktionen' in line:
                    parsing_function = True
                    current_function_key = None
                elif parsing_function and line.startswith('..nr='):
                    current_function_key = line.split('=', 1)[1].strip()
                    current_functions[current_function_key] = {}
                elif parsing_function and line.startswith('..') and ('=' in line) and current_function_key:
                    key, value = line[2:].split('=', 1)
                    current_functions[current_function_key][key.strip()] = parse_value(value)
                elif line.startswith('.') and '=' in line and (not line.startswith('..')):
                    key, value = line[1:].split('=', 1)
                    current_locomotive[key.strip()] = parse_value(value)
                    parsing_function = False
        if current_locomotive:
            if current_functions:
                current_locomotive['funktionen'] = current_functions
            locomotives.append(current_locomotive)
    return locomotives

def parse_magnetartikel_cs2(file_path):
    articles = {}
    current_section = None
    current_entry = {}
    with open(file_path, 'r', encoding='utf-8') as file:
        for line in file:
            line = line.strip()
            if not line:
                continue
            if not line.startswith('.'):
                if line == 'artikel':
                    current_section = 'artikel'
                    current_entry = {}
                    if 'artikel' not in articles:
                        articles['artikel'] = []
                    articles['artikel'].append(current_entry)
            else:
                key_value = line.lstrip('.').split('=', 1)
                if len(key_value) == 2:
                    key, value = key_value
                    value = parse_value(value)
                    if current_section == 'artikel':
                        current_entry[key] = value
    for entry in articles.get('artikel', []):
        id_val = entry.get('id')
        dectyp = str(entry.get('dectyp', '')).lower()
        if id_val is not None:
            try:
                id_int = int(id_val)
            except Exception:
                continue
            entry['uid'] = magnetartikel_uid(id_int, dectyp)
    return articles

# --- Routes / Main ---

@app.route('/')
def index():
    # Provide CONFIG_PATH (HTTP base, e.g. '/cfg') to template for asset building.
    # The real filesystem directory comes from CONFIG_FS_PATH and is exposed via
    # the /cfg/<path:filename> route (see below). We NEVER leak the raw
    # filesystem path to the browser.
    cfg = app.config.get('CONFIG_PATH', PUBLIC_CONFIG_BASE).rstrip('/')
    return render_template('index.html', config_path=cfg)

@app.route('/info')
def info():
    cfg = app.config.get('CONFIG_PATH', PUBLIC_CONFIG_BASE).rstrip('/')
    return render_template('info.html', config_path=cfg)

@app.route('/sw.js')
def service_worker():
    fe = app.config.get('FRONTEND_DIR', FRONTEND_DIR)
    return send_from_directory(os.path.join(fe), 'sw.js', mimetype='application/javascript')


# --- Dynamic serving of user provided config/asset directory ---
# The CLI option --config provides a LOCAL filesystem directory that contains
# subfolders like icons/, fcticons/, config/ (with *.cs2 files), etc.
# Browsers cannot access raw filesystem paths, so we expose that directory
# readonly under the fixed URL prefix /cfg/.  Templates/JS only ever see the
# HTTP path (CONFIG_PATH) while server-side parsing uses CONFIG_FS_PATH.
@app.route(f"{PUBLIC_CONFIG_BASE}/<path:filename>")
def serve_config_assets(filename):
    base = app.config.get('CONFIG_FS_PATH')
    if not base:
        from flask import abort
        return abort(404)
    fs_path = os.path.join(base, filename)
    norm_base = os.path.abspath(base)
    norm_file = os.path.abspath(fs_path)
    if not norm_file.startswith(norm_base):
        from flask import abort
        logger.warning('[cfg] traversal attempt: %s', filename)
        return abort(403)
    if os.path.isdir(norm_file):
        from flask import abort
        logger.debug('[cfg] directory access blocked: %s', filename)
        return abort(404)
    if os.path.isfile(norm_file):
        try:
            from flask import send_file
            return send_file(norm_file)
        except Exception as e:
            import traceback
            traceback.print_exc()
            logger.exception('[cfg] send error %s: %s', filename, e)
            from flask import abort
            return abort(500)
    fe = app.config.get('FRONTEND_DIR', FRONTEND_DIR)
    pkg_static = os.path.join(fe, 'static')
    fallback_path = os.path.join(pkg_static, filename)
    if os.path.isfile(fallback_path):
        try:
            from flask import send_file
            return send_file(fallback_path)
        except Exception as e:
            import traceback
            traceback.print_exc()
            logger.exception('[cfg] fallback send error %s: %s', filename, e)
            from flask import abort
            return abort(500)
    logger.warning('[cfg] not found: %s (looked in %s)', filename, base)
    from flask import abort
    return abort(404)


# --- Icon listing and assignment ---
def _list_icon_files(base_dir: str):
    icons_dir = os.path.join(base_dir or '', 'icons')
    out = []
    try:
        for fn in os.listdir(icons_dir):
            if not isinstance(fn, str):
                continue
            lower = fn.lower()
            if lower.endswith(('.png', '.jpg', '.jpeg', '.gif', '.webp')):
                name, _ = os.path.splitext(fn)
                out.append({'name': name, 'file': fn})
    except Exception:
        pass
    # stable sort
    out.sort(key=lambda x: x['file'].lower())
    return out


@app.get('/api/icons')
def api_list_icons():
    base = app.config.get('CONFIG_FS_PATH')
    if not base:
        return jsonify([])
    return jsonify(_list_icon_files(base))


def _update_lokomotive_icon_file(file_path: str, uid: int, icon_name: str) -> bool:
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
    except Exception:
        return False
    out_lines = list(lines)
    n = len(lines)
    i = 0
    changed = False
    while i < n:
        line = lines[i].strip()
        if line == 'lokomotive':
            # scan block
            blk_start = i
            blk_end = i + 1
            cur_uid = None
            icon_idx = None
            name_idx = None
            uid_idx = None
            j = i + 1
            while j < n and lines[j].strip() != 'lokomotive':
                s = lines[j].strip()
                if s.startswith('.') and '=' in s and not s.startswith('..'):
                    key, val = s[1:].split('=', 1)
                    key = key.strip(); val = val.strip()
                    if key == 'uid':
                        try:
                            cur_uid = parse_value(val)
                            uid_idx = j
                        except Exception:
                            cur_uid = cur_uid
                    elif key == 'icon':
                        icon_idx = j
                    elif key == 'name':
                        name_idx = j
                j += 1
            blk_end = j
            if cur_uid is not None and int(cur_uid) == int(uid):
                # apply change within out_lines
                new_line = f" .icon={icon_name}\n"
                if icon_idx is not None:
                    if out_lines[icon_idx] != new_line:
                        out_lines[icon_idx] = new_line
                        changed = True
                else:
                    # insert after name, else after uid, else right after blk_start
                    insert_at = None
                    for idx in (name_idx, uid_idx):
                        if idx is not None:
                            insert_at = idx + 1
                            break
                    if insert_at is None:
                        insert_at = blk_start + 1
                    out_lines.insert(insert_at, new_line)
                    # adjust counters
                    delta = 1
                    n += delta
                    i += delta
                    changed = True
                # don't break; continue scanning to allow multiple same uids (unlikely)
            i = blk_end
            continue
        i += 1
    if changed:
        try:
            with open(file_path, 'w', encoding='utf-8') as f:
                f.writelines(out_lines)
        except Exception:
            return False
    return changed


def _overrides_path() -> str:
    base = app.config.get('CONFIG_FS_PATH') or ''
    return os.path.join(base, 'config', 'mswebapp_overrides.json')

def _load_overrides() -> None:
    global icon_overrides
    try:
        with open(_overrides_path(), 'r', encoding='utf-8') as f:
            data = json.load(f)
        d = data.get('icon_overrides') if isinstance(data, dict) else None
        if isinstance(d, dict):
            icon_overrides = {str(k): str(v) for k, v in d.items() if isinstance(k, (str, int))}
        else:
            icon_overrides = {}
    except Exception:
        icon_overrides = {}

def _save_overrides() -> bool:
    path = _overrides_path()
    try:
        os.makedirs(os.path.dirname(path), exist_ok=True)
        tmp = path + '.tmp'
        data = {'icon_overrides': icon_overrides}
        with open(tmp, 'w', encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, separators=(',', ':'))
        os.replace(tmp, path)
        return True
    except Exception:
        return False

@app.post('/api/loco_icon')
def api_set_loco_icon():
    data = _require_json()
    uid = data.get('loco_id')
    icon_name = data.get('icon')
    if uid is None or not icon_name:
        return jsonify({'status': 'error', 'message': 'loco_id and icon required'}), 400
    try:
        uid = int(uid)
    except Exception:
        return jsonify({'status': 'error', 'message': 'invalid loco_id'}), 400
    icon_overrides[str(uid)] = str(icon_name)
    if not _save_overrides():
        return jsonify({'status': 'error', 'message': 'failed to save override'}), 500
    try:
        publish_event({'type': 'loco_list_reloaded'})
    except Exception:
        pass
    return jsonify({'status': 'ok'})


@app.get('/api/health')
def health():
    """Lightweight health/status endpoint for monitoring."""
    try:
        udp_ip = app.config.get('UDP_IP', UDP_IP)
        udp_tx = app.config.get('UDP_PORT_TX', UDP_PORT_TX)
        loco_cnt = len(loco_state)
        try:
            switch_cnt = len(switch_state) if isinstance(switch_state, (list, dict)) else 0
        except Exception:
            switch_cnt = 0
        return jsonify({
            'status': 'ok',
            'system_state': system_state,
            'loco_count': loco_cnt,
            'switch_count': switch_cnt,
            'udp_target': f"{udp_ip}:{udp_tx}",
            'version': _MSW_VERSION
        })
    except Exception as e:
        return jsonify({'status': 'error', 'error': str(e)}), 500


def parse_args():
    parser = argparse.ArgumentParser(description='MobileStationWebApp Server')
    parser.add_argument('--config', dest='config_path', default=path_config_files, help='Path to CS2 configuration files')
    parser.add_argument('--udp-ip', dest='udp_ip', default=UDP_IP, help='IP address or hostname of the CS2 UDP target')
    parser.add_argument('--host', dest='host', default='0.0.0.0', help='Bind host for Flask')
    parser.add_argument('--port', dest='port', type=int, default=6020, help='Port for Flask')
    parser.add_argument('--www', dest='www', default=None, help='Path to frontend directory (contains static/ and templates/)')
    args = parser.parse_args()
    # If udp_ip is a hostname, resolve it to IP
    try:
        args.udp_ip = socket.gethostbyname(args.udp_ip)
    except Exception:
        pass  # leave as-is if resolution fails
    return args


def run_server(udp_ip: str = UDP_IP, config_path: str = path_config_files, host: str = '0.0.0.0', port: int = 6020, frontend_dir: str | None = None):
    global loc_list, switch_list
    try:
        resolved_ip = socket.gethostbyname(udp_ip)
    except Exception:
        resolved_ip = udp_ip  # fallback to original value if resolution fails
    app.config['UDP_IP'] = resolved_ip
    app.config['UDP_PORT_TX'] = UDP_PORT_TX
    app.config['UDP_PORT_RX'] = UDP_PORT_RX
    # Store the raw filesystem path separately (never exposed directly to client)
    app.config['CONFIG_FS_PATH'] = config_path
    # Public HTTP base path for those assets (mounted via /cfg/<path>)
    app.config['CONFIG_PATH'] = PUBLIC_CONFIG_BASE
    # Frontend dir override
    try:
        effective_fe = str(frontend_dir) if frontend_dir else app.config.get('FRONTEND_DIR', FRONTEND_DIR)
    except Exception:
        effective_fe = FRONTEND_DIR
    app.config['FRONTEND_DIR'] = effective_fe
    # Update Flask static/template paths to the effective frontend dir
    try:
        from jinja2 import FileSystemLoader
        app.static_folder = os.path.join(effective_fe, 'static')
        app.template_folder = os.path.join(effective_fe, 'templates')
        app.jinja_loader = FileSystemLoader(app.template_folder)
    except Exception:
        pass

    try:
        loc_list = parse_lokomotive_cs2(os.path.join(config_path, 'config', 'lokomotive.cs2'))
    except Exception as e:
        loc_list = []
        logger.warning("Error loading lokomotive.cs2 file: %s", e)

    # Load persistent overrides (icon overrides, etc.)
    try:
        _load_overrides()
    except Exception:
        pass

    try:
        switch_list = parse_magnetartikel_cs2(os.path.join(config_path, 'config', 'magnetartikel.cs2'))
    except Exception as e:
        switch_list = []
        logger.warning("Error loading magnetartikel.cs2 file: %s", e)

    try:
        with loc_data_lock:
            # reset and pre-initialize loco_state so keys exist for all locos
            loco_state.clear()
            for loco in loc_list:
                _ensure_loco_state(int(loco.get('uid') if isinstance(loco, dict) else loco['uid']))
    except Exception:
        pass

    # Background watcher for lokomotive.cs2 - reload on change and reinit state
    def _watch_lokomotive(file_path: str, poll_sec: float = 1.0):
        last_mtime = None
        try:
            if os.path.isfile(file_path):
                last_mtime = os.path.getmtime(file_path)
        except Exception:
            last_mtime = None
        while not _stop_evt.is_set():
            try:
                time.sleep(poll_sec)
                if not os.path.isfile(file_path):
                    continue
                mtime = os.path.getmtime(file_path)
                if last_mtime is None:
                    last_mtime = mtime
                    continue
                if mtime != last_mtime:
                    last_mtime = mtime
                    try:
                        new_list = parse_lokomotive_cs2(file_path)
                        logger.info("lokomotive.cs2 changed, reloading %d locomotives", len(new_list))
                    except Exception as e:
                        logger.warning("Failed to reload lokomotive.cs2: %s", e)
                        continue
                    # Swap loc_list and reinitialize loco_state safely
                    try:
                        with loc_data_lock:
                            # Update global loc_list
                            globals()['loc_list'] = new_list
                            # Rebuild loco_state preserving prior values when possible
                            old_state = dict(loco_state)
                            loco_state.clear()
                            for loco in new_list:
                                try:
                                    uid = int(loco.get('uid') if isinstance(loco, dict) else loco['uid'])
                                except Exception:
                                    continue
                                st = old_state.get(uid, {'speed': 0, 'direction': 1, 'functions': {}})
                                loco_state[uid] = {
                                    'speed': int(st.get('speed', 0)),
                                    'direction': int(st.get('direction', 1)),
                                    'functions': dict(st.get('functions') or {})
                                }
                    except Exception as e:
                        logger.exception("Error applying new loco list: %s", e)
                        continue
                    # Inform clients about refreshed data (send a system 'refresh' via events)
                    try:
                        publish_event({'type': 'system', 'status': 1 if system_state == SystemState.RUNNING else 0})
                        # also emit a synthetic info that locos reloaded; frontend will resubscribe/refresh naturally
                        publish_event({'type': 'loco_list_reloaded'})
                    except Exception:
                        pass
            except Exception:
                # Never crash the watcher
                continue

    lok_file = os.path.join(config_path, 'config', 'lokomotive.cs2')
    threading.Thread(target=_watch_lokomotive, args=(lok_file,), daemon=True).start()

    t = threading.Thread(target=listen_cs2_udp, args=('', UDP_PORT_RX, _stop_evt), daemon=True)
    t.start()

    app.run(host=host, port=port, threaded=True, use_reloader=False)

if __name__ == '__main__':
    args = parse_args()
    run_server(udp_ip=args.udp_ip, config_path=args.config_path, host=args.host, port=args.port, frontend_dir=args.www)
