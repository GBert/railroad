// THE BEER-WARE LICENSE (Revision 42)
// <mende.r@hotmail.de> wrote this file. As long as you retain this notice you can do whatever you want with this
// stuff. If we meet someday, and you think this stuff is worth it, you can buy me a beer in return. Ralf Mende

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <thread>
#include <atomic>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <optional>
#include <filesystem>
#include <condition_variable>
#include <cstring>
#include <cctype>

#include "httplib.h" // yhirose/cpp-httplib single header (HTTP + SSE via chunked)

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <fcntl.h>
#endif

namespace fs = std::filesystem;

// ---- Basic types/state ----
struct Loco {
    int uid = 0;
    std::string name;
    std::string icon;
    int symbol = 0;
    int tachomax = 0;
    std::map<int,int> fn_typ; // function index -> type id
};
static std::map<int, Loco> g_locos; // uid -> loco
static std::map<int, int> g_loco_speed; // uid -> 0..1023
static std::map<int, int> g_loco_dir;   // uid -> 0/1/2
static std::map<int, std::map<int,bool>> g_loco_fn; // uid -> fn->bool
struct SwitchEntry {
    int uid = -1;            // computed uid
    std::string name;
    std::string typ;
    std::string dectyp;     // decoder type (mm2/dcc/sx1/...) used to compute uid
    int switch_delay = 0;   // switching time (ms) parsed as integer
};
static std::vector<SwitchEntry> g_switches; // index corresponds to switch slot
static std::map<int, int> g_switch_state; // idx -> value
static std::atomic<bool> g_running{false};
static std::string g_config_dir = "var";
static std::string g_public_cfg = "/cfg";
static std::string g_udp_ip = "127.0.0.1";
static int g_udp_tx = 15731;
static int g_udp_rx = 15730;
static int g_http_port = 6020;
static std::string g_bind_host = "0.0.0.0";
static int g_device_uid = 0; // sender uid for CAN id hash
static bool g_verbose = false;
static std::string g_frontend_dir_override; // optional path to frontend dir (templates/static)
static std::map<int, std::string> g_icon_overrides; // uid -> icon name (stem)
static bool g_enable_bind_timer = false; // enable CMD_BIND timer only when --bind is passed
static int g_bind_timeout_ms = 1000;     // timeout for MFX-BIND timer in milliseconds (default 1s)
static bool g_enable_precompressed_gzip = true; // serve .gz files for static assets when available

// ---- Utilities ----
static std::string trim(const std::string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static std::string json_escape(const std::string &in) {
    std::string out; out.reserve(in.size()+8);
    for (char c : in) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                char buf[7]; std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                out += buf;
            } else {
                out += c;
            }
        }
    }
    return out;
}

// parse integer from string supporting decimal and 0x... hex
static int parse_int_auto(const std::string &s) {
    std::string v = trim(s);
    if (v.size() > 2 && (v[0]=='0') && (v[1]=='x' || v[1]=='X')) {
        try { return std::stoi(v, nullptr, 16); } catch (...) { return 0; }
    }
    try { return std::stoi(v); } catch(...) { return 0; }
}

// Handle --bind flag with optional numeric timeout argument
static void handle_bind_flag(int &i, int argc, char** argv) {
    g_enable_bind_timer = true;
    if (i + 1 < argc) {
        std::string nv = argv[i + 1];
        if (!nv.empty() && (std::isdigit((unsigned char)nv[0]) || (nv.size() > 2 && nv[0] == '0' && (nv[1] == 'x' || nv[1] == 'X')))) {
            ++i;
            int v = parse_int_auto(nv);
            if (v > 0) g_bind_timeout_ms = v;
        }
    }
}

// Resolve a hostname (or numeric IP) to an IPv4 dotted string; on failure, return input unchanged
static std::string resolve_hostname_to_ipv4(const std::string &host) {
    if (host.empty()) return host;
    addrinfo hints{}; hints.ai_family = AF_INET; // IPv4 only
    addrinfo *res = nullptr;
    int rc = getaddrinfo(host.c_str(), nullptr, &hints, &res);
    if (rc != 0 || !res) {
        return host; // leave as-is if resolution fails
    }
    char buf[INET_ADDRSTRLEN] = {0};
    const sockaddr_in *sa = reinterpret_cast<const sockaddr_in*>(res->ai_addr);
    const void *addr_ptr = &(sa->sin_addr);
    const char *ntop = inet_ntop(AF_INET, addr_ptr, buf, sizeof(buf));
    std::string out = ntop ? std::string(ntop) : host;
    freeaddrinfo(res);
    return out;
}

static std::string loco_list_json() {
    std::ostringstream os; os << "{";
    bool first = true;
    for (auto &kv : g_locos) {
        if (!first) os << ","; first = false;
        const Loco &l = kv.second;
        os << '"' << kv.first << '"' << ":{";
        os << "\"uid\":" << l.uid << ",";
        os << "\"name\":\"" << json_escape(l.name) << "\",";
        // Apply icon override if present
        auto itov = g_icon_overrides.find(l.uid);
        std::string icon_eff = (itov != g_icon_overrides.end()) ? itov->second : l.icon;
        os << "\"icon\":\"" << json_escape(icon_eff) << "\",";
        os << "\"tachomax\":" << l.tachomax;
        os << ",\"symbol\":" << l.symbol;
        if (!l.fn_typ.empty()) {
            os << ",\"funktionen\":{";
            bool ffirst = true;
            for (auto &fk : l.fn_typ) {
                if (!ffirst) os << ","; ffirst=false;
                os << '"' << fk.first << '"' << ":{\"typ\":" << fk.second << "}";
            }
            os << "}";
        }
        os << "}";
    }
    os << "}";
    return os.str();
}
// Persistent overrides JSON path under config_dir/config
static fs::path overrides_path() {
    return fs::path(g_config_dir)/"config"/"mswebapp_overrides.json";
}

static void load_overrides() {
    g_icon_overrides.clear();
    std::ifstream f(overrides_path());
    if (!f) return;
    std::ostringstream buf; buf << f.rdbuf();
    std::string s = buf.str();
    // Very minimal JSON parse (expect {"icon_overrides": {"uid":"icon"}})
    auto pos = s.find("\"icon_overrides\""); if (pos==std::string::npos) return;
    pos = s.find('{', pos); if (pos==std::string::npos) return; size_t end = s.find('}', pos); if (end==std::string::npos) return;
    std::string mapstr = s.substr(pos+1, end-pos-1);
    size_t i=0; while (i<mapstr.size()) {
        while (i<mapstr.size() && (mapstr[i]==','||isspace((unsigned char)mapstr[i]))) i++;
        if (i>=mapstr.size() || mapstr[i]!='"') break; size_t j = mapstr.find('"', i+1); if (j==std::string::npos) break;
        std::string key = mapstr.substr(i+1, j-(i+1)); i = j+1; size_t colon = mapstr.find(':', i); if (colon==std::string::npos) break; i = colon+1;
        while (i<mapstr.size() && isspace((unsigned char)mapstr[i])) i++; if (i>=mapstr.size()) break;
        std::string val;
        if (mapstr[i]=='"') { size_t k = mapstr.find('"', i+1); if (k==std::string::npos) break; val = mapstr.substr(i+1, k-(i+1)); i = k+1; }
        else { size_t k=i; while (k<mapstr.size() && mapstr[k]!=',' ) k++; val = trim(mapstr.substr(i, k-i)); i = k; }
        int uid = 0; try { uid = std::stoi(key); } catch(...) { continue; }
        g_icon_overrides[uid] = val;
    }
}

static bool save_overrides() {
    fs::path p = overrides_path();
    std::error_code ec; fs::create_directories(p.parent_path(), ec);
    std::ostringstream os; os << "{\"icon_overrides\":{";
    bool first=true; for (auto &kv : g_icon_overrides) { if (!first) os << ","; first=false; os << '"' << kv.first << '"' << ":\"" << json_escape(kv.second) << "\""; }
    os << "}}";
    std::string data = os.str();
    fs::path tmp = p; tmp += ".tmp";
    std::ofstream f(tmp, std::ios::binary|std::ios::trunc); if (!f) return false; f.write(data.data(), data.size()); f.close();
    std::error_code ec2; fs::rename(tmp, p, ec2); if (ec2) return false; return true;
}

static std::string loco_state_json() {
    std::ostringstream os; os << "{";
    bool first = true;
    for (auto &kv : g_locos) {
        int uid = kv.first;
        if (!first) os << ","; first = false;
        int spd = g_loco_speed[uid];
        int dir = g_loco_dir[uid];
        os << '"' << uid << '"' << ":{";
        os << "\"speed\":" << spd << ",\"direction\":" << dir << ",\"functions\":{";
        bool f2 = true;
        for (auto &fk : g_loco_fn[uid]) {
            if (!f2) os << ","; f2 = false;
            os << '"' << fk.first << '"' << ":" << (fk.second ? 1 : 0);
        }
        os << "}}";
    }
    os << "}";
    return os.str();
}

// List icons in icons/ directory under g_config_dir
static std::string icons_list_json() {
    std::vector<std::pair<std::string,std::string>> items; // {name, file}
    fs::path dir = fs::path(g_config_dir) / "icons";
    std::error_code ec;
    if (fs::exists(dir, ec) && fs::is_directory(dir, ec)) {
        for (auto &e : fs::directory_iterator(dir, ec)) {
            if (ec) break;
            if (!e.is_regular_file(ec)) continue;
            auto p = e.path();
            auto ext = p.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return (char)std::tolower(c); });
            if (ext==".png" || ext==".jpg" || ext==".jpeg" || ext==".gif" || ext==".webp") {
                auto file = p.filename().string();
                auto stem = p.stem().string();
                items.emplace_back(stem, file);
            }
        }
    }
    std::sort(items.begin(), items.end(), [](auto &a, auto &b){
        std::string af=a.second, bf=b.second; std::transform(af.begin(),af.end(),af.begin(),::tolower); std::transform(bf.begin(),bf.end(),bf.begin(),::tolower); return af<bf;
    });
    std::ostringstream os; os << "[";
    for (size_t i=0;i<items.size();++i) {
        if (i) os << ",";
        os << "{\"name\":\"" << json_escape(items[i].first) << "\",\"file\":\"" << json_escape(items[i].second) << "\"}";
    }
    os << "]";
    return os.str();
}

static bool update_lok_icon_file(const fs::path &file, int uid, const std::string &iconName) {
    std::ifstream in(file);
    if (!in) return false;
    std::vector<std::string> lines; std::string line;
    while (std::getline(in, line)) lines.push_back(line + (line.size() && line.back()=='\n' ? "" : "\n"));
    in.close();
    bool changed = false; size_t n = lines.size();
    for (size_t i=0; i<n; ) {
        std::string s = trim(lines[i]);
        if (s == "lokomotive") {
            size_t blk_start = i, j = i + 1; int cur_uid = -1; int icon_idx = -1; int name_idx = -1; int uid_idx = -1;
            for (; j<n && trim(lines[j]) != "lokomotive"; ++j) {
                std::string t = trim(lines[j]);
                if (!t.empty() && t[0]=='.' && t.size()>=2 && t[1] != '.' && t.find('=')!=std::string::npos) {
                    auto pos = t.find('=');
                    std::string key = trim(t.substr(1, pos-1));
                    std::string val = trim(t.substr(pos+1));
                    if (key == "uid") { cur_uid = parse_int_auto(val); uid_idx = (int)j; }
                    else if (key == "icon") { icon_idx = (int)j; }
                    else if (key == "name") { name_idx = (int)j; }
                }
            }
            if (cur_uid == uid) {
                std::string new_line = " .icon=" + iconName + "\n";
                if (icon_idx >= 0) {
                    if (lines[(size_t)icon_idx] != new_line) { lines[(size_t)icon_idx] = new_line; changed = true; }
                } else {
                    size_t insert_at = (name_idx >= 0 ? (size_t)name_idx + 1 : (uid_idx >= 0 ? (size_t)uid_idx + 1 : blk_start + 1));
                    if (insert_at > lines.size()) insert_at = lines.size();
                    lines.insert(lines.begin() + insert_at, new_line);
                    changed = true; n = lines.size(); j++; // advance end
                }
            }
            i = j; continue;
        }
        ++i;
    }
    if (changed) {
        std::ofstream out(file, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        for (auto &l : lines) out << l;
        out.close();
    }
    return changed;
}

static void parse_lokomotive_cs2(const fs::path &p) {
    g_locos.clear();
    std::ifstream f(p);
    if (!f) return;
    std::string line;
    Loco cur; bool in = false;
    bool in_fn = false; int fn_nr = -1; int fn_typ = -1;
    while (std::getline(f, line)) {
        line = trim(line);
        if (line == "lokomotive") {
            if (in && cur.uid) g_locos[cur.uid] = cur;
            cur = Loco{}; in = true; in_fn = false; fn_nr = -1; fn_typ = -1;
        } else if (in) {
            if (!line.empty() && line.rfind(".funktionen", 0) == 0) {
                // Start a function entry; next lines contain ..nr and ..typ
                in_fn = true; fn_nr = -1; fn_typ = -1; continue;
            }
            if (in_fn && line.rfind("..", 0) == 0 && line.find('=')!=std::string::npos) {
                auto pos = line.find('=');
                std::string key = trim(line.substr(2, pos-2));
                std::string val = trim(line.substr(pos+1));
                if (key == "nr") {
                    fn_nr = parse_int_auto(val);
                } else if (key == "typ" || key == "type") {
                    fn_typ = parse_int_auto(val);
                }
                if (fn_nr >= 0 && fn_typ >= 0) { cur.fn_typ[fn_nr] = fn_typ; fn_nr = -1; fn_typ = -1; in_fn = false; }
                continue;
            }
            if (!line.empty() && line[0]=='.' && line.find('=')!=std::string::npos && (line.size()<2 || line[1] != '.')) {
                auto pos = line.find('=');
                std::string key = line.substr(1, pos-1);
                std::string val = line.substr(pos+1);
                key = trim(key); val = trim(val);
                if (key == "uid") {
                    cur.uid = parse_int_auto(val);
                } else if (key == "name") {
                    cur.name = val;
                } else if (key == "icon") {
                    cur.icon = val;
                } else if (key == "symbol") {
                    cur.symbol = parse_int_auto(val);
                } else if (key == "tachomax") {
                    cur.tachomax = parse_int_auto(val);
                } /*else if (key == "vmax") {
                    int vmax = parse_int_auto(val);
                    if (cur.tachomax <= 0 && vmax > 0) cur.tachomax = vmax; //Workaround for MS where there is no tachomax
                }*/
            }
        }
    }
    if (in && cur.uid) g_locos[cur.uid] = cur;
}

static void parse_magnetartikel_cs2(const fs::path &p) {
    g_switch_state.clear();
    g_switches.clear();
    g_switches.resize(64);
    std::ifstream f(p);
    if (!f) return;
    std::string line; bool in = false; int idx = 0; int cur_id = 0; std::string dectyp; std::string cur_name; std::string cur_typ; int cur_time = 0;
    while (std::getline(f, line)) {
        line = trim(line);
        if (line == "artikel") {
            in = true; idx++; cur_id = 0; dectyp.clear(); cur_name.clear();
            continue;
        }
        if (in && !line.empty() && line[0] == '.' && line.find('=') != std::string::npos) {
            auto pos = line.find('=');
            std::string key = trim(line.substr(1, pos - 1));
            std::string val = trim(line.substr(pos + 1));
            if (key == "id") {
                try { cur_id = parse_int_auto(val); } catch (...) { cur_id = 0; }
            } else if (key == "name") {
                cur_name = val;
            } else if (key == "typ") {
                cur_typ = val;
            } else if (key == "schaltzeit") {
                cur_time = parse_int_auto(val);
            } else if (key == "dectyp") {
                std::transform(val.begin(), val.end(), val.begin(), [](unsigned char c){ return (char)std::tolower(c); });
                dectyp = val;
            }
            if (idx >= 1 && idx <= 64) {
                int vec_idx = idx - 1;
                g_switch_state[vec_idx] = 0;
                // Compute UID (recompute each time as in previous logic)
                int id_int = cur_id - 1;
                int uid = -1;
                if (dectyp == "mm2") uid = 0x3000 | (id_int & 0x3FF);
                else if (dectyp == "dcc") uid = 0x3800 | (id_int & 0x3FF);
                else if (dectyp == "sx1") uid = 0x2800 | (id_int & 0x3FF);
                else uid = (id_int & 0x3FF);
                g_switches[vec_idx].uid = uid;
                if (!cur_name.empty()) g_switches[vec_idx].name = cur_name;
                if (!cur_typ.empty()) g_switches[vec_idx].typ = cur_typ;
                g_switches[vec_idx].switch_delay = cur_time;
                if (!dectyp.empty()) g_switches[vec_idx].dectyp = dectyp;
            }
        }
    }
}

// Forward declarations for SSE/event helpers used in UDP listener
static void publish_event(const std::string &msg);
static std::string system_event_json();
static std::string speed_event_json(int uid, int spd);
static std::string direction_event_json(int uid, int dir);
static std::string function_event_json(int uid, int fn, bool val);
static std::string switch_event_json(int idx, int val);

// ---- UDP send/recv stubs ----
// ---- UDP CS2 helpers ----
enum Command {
    CMD_SYSTEM = 0,
    CMD_DISCOVERY = 1,
    CMD_BIND = 2,
    CMD_VERIFY = 3,
    CMD_SPEED = 4,
    CMD_DIRECTION = 5,
    CMD_FUNCTION = 6,
    CMD_READ_CONFIG = 7,
    CMD_WRITE_CONFIG = 8,
    CMD_SWITCH = 11
};

enum SystemSubCmd {
    SYS_STOP = 0x00,
    SYS_GO = 0x01,
    SYS_HALT = 0x02
};

static uint16_t generate_hash(uint32_t uid) {
    uint16_t hi = (uid >> 16) & 0xFFFF;
    uint16_t lo = uid & 0xFFFF;
    uint16_t h = hi ^ lo;
    h = (uint16_t)(((h << 3) & 0xFF00) | 0x300 | (h & 0x7F));
    return h & 0xFFFF;
}

static uint32_t build_can_id(uint32_t uid, int command, int prio = 0, int resp = 0) {
    uint16_t hash16 = generate_hash(uid);
    return (uint32_t(prio) << 25) | (uint32_t(((command << 1) | (resp & 1))) << 16) | hash16;
}

static std::vector<uint8_t> pad_to_8(const std::vector<uint8_t> &in) {
    std::vector<uint8_t> b = in;
    while (b.size() < 8) b.push_back(0);
    if (b.size() > 8) b.resize(8);
    return b;
}

static std::vector<uint8_t> payload_speed(uint32_t loco_uid, int speed) {
    std::vector<uint8_t> b;
    b.push_back((loco_uid >> 24) & 0xFF);
    b.push_back((loco_uid >> 16) & 0xFF);
    b.push_back((loco_uid >> 8) & 0xFF);
    b.push_back(loco_uid & 0xFF);
    int spd = std::max(0, std::min(1023, speed));
    b.push_back((spd >> 8) & 0xFF);
    b.push_back(spd & 0xFF);
    return b;
}

static std::vector<uint8_t> payload_direction(uint32_t loco_uid, int dir) {
    std::vector<uint8_t> b;
    b.push_back((loco_uid >> 24) & 0xFF);
    b.push_back((loco_uid >> 16) & 0xFF);
    b.push_back((loco_uid >> 8) & 0xFF);
    b.push_back(loco_uid & 0xFF);
    b.push_back(dir & 0xFF);
    return b;
}

static std::vector<uint8_t> payload_function(uint32_t loco_uid, int fn, int val) {
    std::vector<uint8_t> b;
    b.push_back((loco_uid >> 24) & 0xFF);
    b.push_back((loco_uid >> 16) & 0xFF);
    b.push_back((loco_uid >> 8) & 0xFF);
    b.push_back(loco_uid & 0xFF);
    b.push_back(fn & 0xFF);
    b.push_back(val & 0xFF);
    return b;
}

static std::vector<uint8_t> payload_switch(uint32_t uid, int pos, int cur) {
    std::vector<uint8_t> b;
    b.push_back((uid >> 24) & 0xFF);
    b.push_back((uid >> 16) & 0xFF);
    b.push_back((uid >> 8) & 0xFF);
    b.push_back(uid & 0xFF);
    b.push_back(pos & 0xFF);
    b.push_back(cur & 0xFF);
    return b;
}

static std::vector<uint8_t> payload_system_state(uint32_t device_uid, bool running) {
    std::vector<uint8_t> b;
    b.push_back((device_uid >> 24) & 0xFF);
    b.push_back((device_uid >> 16) & 0xFF);
    b.push_back((device_uid >> 8) & 0xFF);
    b.push_back(device_uid & 0xFF);
    b.push_back(running ? 1 : 0);
    return b;
}

static void udp_send_frame(uint32_t can_id, const std::vector<uint8_t> &payload, int dlc) {
     auto data = pad_to_8(payload);
#ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) return;
    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons((u_short)g_udp_tx);
    addr.sin_addr.s_addr = inet_addr(g_udp_ip.c_str());
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        // inet_addr returns INADDR_NONE both on error and for 255.255.255.255; try inet_pton
        inet_pton(AF_INET, g_udp_ip.c_str(), &addr.sin_addr);
    }
    // Enable broadcast (safe for unicast too)
    BOOL bc = TRUE; setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*)&bc, sizeof(bc));
#else
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;
    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons(g_udp_tx);
    inet_pton(AF_INET, g_udp_ip.c_str(), &addr.sin_addr);
    // Enable broadcast (safe for unicast too)
    int yes = 1; setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
#endif
    std::vector<uint8_t> send_bytes;
    send_bytes.reserve(4+1+8);
    send_bytes.push_back((can_id >> 24) & 0xFF);
    send_bytes.push_back((can_id >> 16) & 0xFF);
    send_bytes.push_back((can_id >> 8) & 0xFF);
    send_bytes.push_back(can_id & 0xFF);
    send_bytes.push_back(dlc & 0xFF);
    send_bytes.insert(send_bytes.end(), data.begin(), data.end());
    if (g_verbose) {
        auto print_hex = [](uint32_t v){ char b[16]; std::snprintf(b, sizeof(b), "0x%08X", v); return std::string(b); };
        std::ostringstream dbg;
        dbg << "UDP -> " << g_udp_ip << ":" << g_udp_tx << " CANID=" << print_hex(can_id) << " DLC=" << dlc << " DATA=";
        for (size_t i=0;i<data.size();++i) { char b[4]; std::snprintf(b, sizeof(b), "%02X", data[i]); dbg << b; if (i+1<data.size()) dbg << ' '; }
        dbg << "\n";
#ifdef _WIN32
        OutputDebugStringA(dbg.str().c_str());
#endif
        fprintf(stderr, "%s", dbg.str().c_str());
    }
#ifdef _WIN32
    int rc = sendto(sock, reinterpret_cast<const char*>(send_bytes.data()), (int)send_bytes.size(), 0, (sockaddr*)&addr, sizeof(addr));
    if (rc == SOCKET_ERROR && g_verbose) {
        fprintf(stderr, "sendto() failed: %d\n", WSAGetLastError());
    }
    closesocket(sock);
#else
    ssize_t rc = sendto(sock, send_bytes.data(), send_bytes.size(), 0, (sockaddr*)&addr, sizeof(addr));
    if (rc < 0 && g_verbose) {
        perror("sendto failed");
    }
    close(sock);
#endif
}

static void udp_listener_thread(std::atomic<bool>& stop_flag) {
    using Clock = std::chrono::steady_clock;
    bool mfx_bind_pending = false;
    Clock::time_point mfx_bind_deadline{};

#ifdef _WIN32
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return;
    BOOL yes = TRUE; setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons((u_short)g_udp_rx); addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) { closesocket(s); return; }
    DWORD timeout = 1000; setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) return;
    int yes = 1; setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons(g_udp_rx); addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(s, (sockaddr*)&addr, sizeof(addr)) < 0) { close(s); return; }
    struct timeval tv; tv.tv_sec = 1; tv.tv_usec = 0; setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

    while (!stop_flag.load()) {
        // Optional MFX-BIND timer only active if --bind flag set
        if (g_enable_bind_timer && mfx_bind_pending && Clock::now() >= mfx_bind_deadline) {
            mfx_bind_pending = false;
            int loco_id = 1; int fn = 0; int value = 1;
            uint32_t can_id = build_can_id((uint32_t)g_device_uid, CMD_FUNCTION, 0, 0);
            udp_send_frame(can_id, payload_function((uint32_t)loco_id, fn, (value!=0)?1:0), 6);
            if (g_verbose) fprintf(stderr, "[MFX-BIND] Timer expired -> Requesting update of Lokomotive.cs2\n");        
        }

        uint8_t buf[64];
#ifdef _WIN32
        sockaddr_in src{}; int srclen = sizeof(src);
        int recvd = recvfrom(s, reinterpret_cast<char*>(buf), (int)sizeof(buf), 0, (sockaddr*)&src, &srclen);
        if (recvd == SOCKET_ERROR) { continue; }
#else
        sockaddr_in src{}; socklen_t srclen = sizeof(src);
        int recvd = recvfrom(s, buf, sizeof(buf), 0, (sockaddr*)&src, &srclen);
        if (recvd < 0) { continue; }
#endif
        if (recvd != 13) continue; //Only 13-Byte Frames

        uint32_t can_id = (uint32_t(buf[0])<<24) | (uint32_t(buf[1])<<16) | (uint32_t(buf[2])<<8) | uint32_t(buf[3]);
        uint8_t dlc = buf[4];
        const uint8_t *data = buf + 5;
        int cmd_resp = (can_id >> 16) & 0x1FF; // 9 bits (command + resp bit)
        int command = (cmd_resp >> 1) & 0xFF;
        int resp_bit = cmd_resp & 1;

        if ((command == CMD_BIND || command == CMD_READ_CONFIG) && g_enable_bind_timer) {
            // (Re)start MFX-Bind timer on each BIND or READ_CONFIG
            int ms = (g_bind_timeout_ms > 0) ? g_bind_timeout_ms : 1000;
            mfx_bind_deadline = Clock::now() + std::chrono::milliseconds(ms);
            mfx_bind_pending = true;
            if (g_verbose) fprintf(stderr, "[MFX-BIND] Received -> restart %d ms timer\n", ms);
        }

        if (resp_bit == 1) {
            if (command == CMD_SYSTEM && dlc >= 5) {
                uint8_t sub = data[4];
                if (sub == SYS_GO) { g_running.store(true); publish_event(system_event_json()); }
                else if (sub == SYS_STOP || sub == SYS_HALT) { g_running.store(false); publish_event(system_event_json()); }
            } else if (command == CMD_SPEED && dlc >= 6) {
                int uid = (int(data[0])<<24)|(int(data[1])<<16)|(int(data[2])<<8)|int(data[3]);
                int spd = (int(data[4])<<8)|int(data[5]);
                g_loco_speed[uid] = spd; publish_event(speed_event_json(uid, spd));
            } else if (command == CMD_DIRECTION && dlc >= 5) {
                int uid = (int(data[0])<<24)|(int(data[1])<<16)|(int(data[2])<<8)|int(data[3]);
                int dir = int(data[4]); g_loco_dir[uid] = dir; publish_event(direction_event_json(uid, dir));
            } else if (command == CMD_FUNCTION && dlc >= 5) {
                int uid = (int(data[0])<<24)|(int(data[1])<<16)|(int(data[2])<<8)|int(data[3]);
                int fn = int(data[4]); int val = (dlc >= 6) ? int(data[5]) : 1;
                g_loco_fn[uid][fn] = (val != 0); publish_event(function_event_json(uid, fn, g_loco_fn[uid][fn]));
            } else if (command == CMD_SWITCH && dlc >= 6) {
                // Data layout: [0..3]=UID (big-endian), [4]=value, [5]=0x01
                int uid = (int(data[0])<<24)|(int(data[1])<<16)|(int(data[2])<<8)|int(data[3]);
                int value = int(data[4]);
                // Map UID back to index if known
                int idx = -1;
                for (size_t i = 0; i < g_switches.size(); ++i) {
                    if (g_switches[i].uid == uid) { idx = int(i); break; }
                }
                if (idx < 0) {
                    // Fallback: use low 10 bits as simple index if unknown mapping
                    idx = uid & 0x3FF; if (idx >= 64) idx = idx % 64;
                }
                g_switch_state[idx] = value; publish_event(switch_event_json(idx, value));
            }
        }
    }

#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}
// ---- SSE broker ----
struct Subscriber {
    std::mutex mtx;
    std::condition_variable cv;
    std::vector<std::string> queue;
    bool closed = false;
};
static std::mutex g_subs_mtx;
static std::set<Subscriber*> g_subscribers;
static const size_t kMaxSseClients = 32; // hard cap to prevent thread-pool starvation

static void publish_event(const std::string &msg) {
    std::lock_guard<std::mutex> lk(g_subs_mtx);
    for (auto *s : g_subscribers) {
        std::unique_lock<std::mutex> lk2(s->mtx);
        s->queue.push_back(msg);
        lk2.unlock();
        s->cv.notify_one();
    }
}

static std::string system_event_json() {
    std::ostringstream os;
    os << "{\"type\":\"system\",\"status\":" << (g_running.load() ? 1 : 0) << "}";
    return os.str();
}

static std::string speed_event_json(int uid, int spd) {
    std::ostringstream os;
    os << "{\"type\":\"speed\",\"loc_id\":" << uid << ",\"value\":" << spd << "}";
    return os.str();
}

static std::string direction_event_json(int uid, int dir) {
    std::ostringstream os;
    os << "{\"type\":\"direction\",\"loc_id\":" << uid << ",\"value\":" << dir << "}";
    return os.str();
}

static std::string function_event_json(int uid, int fn, bool val) {
    std::ostringstream os;
    os << "{\"type\":\"function\",\"loc_id\":" << uid << ",\"fn\":" << fn << ",\"value\":" << (val?1:0) << "}";
    return os.str();
}

static std::string switch_event_json(int idx, int val) {
    std::ostringstream os;
    os << "{\"type\":\"switch\",\"idx\":" << idx << ",\"value\":" << val << "}";
    return os.str();
}

int main(int argc, char** argv) {
#ifdef _WIN32
    WSADATA wsaData; WSAStartup(MAKEWORD(2,2), &wsaData);
#endif
    // Parse CLI (very light)
    for (int i=1;i<argc;i++) {
        std::string a = argv[i];
        auto next = [&](int &i){ return (i+1<argc)? std::string(argv[++i]) : std::string(); };
        if (a == "--config") g_config_dir = next(i);
        else if (a == "--udp-ip") g_udp_ip = next(i);
        else if (a == "--host") g_bind_host = next(i);
        else if (a == "--port") g_http_port = std::stoi(next(i));
        else if (a == "--www") { g_frontend_dir_override = next(i); }
        else if (a == "--bind") { handle_bind_flag(i, argc, argv); }
        else if (a.rfind("--bind=", 0) == 0) {
            g_enable_bind_timer = true;
            std::string nv = a.substr(7);
            int v = parse_int_auto(nv); if (v > 0) g_bind_timeout_ms = v;
        }
        else if (a == "--verbose" || a == "-v") { g_verbose = true; }
        else if (a == "--no-gzip") { g_enable_precompressed_gzip = false; }
        else if (a == "--help" || a == "-h") {
            printf("Usage: mswebapp_cpp [options]\n");
            printf("  --config <dir>     Path to config directory (contains config/, icons/, fcticons/, ...)\n");
            printf("  --udp-ip <ip|host> UDP target ip/host (default Gleisbox)\n");
            printf("  --host <addr>      HTTP bind host (default 0.0.0.0)\n");
            printf("  --port <port>      HTTP port (default 6020)\n");
            printf("  --www <dir>        Frontend directory containing templates/ and static/\n");
            printf("  --bind[=<ms>]      Enable requesting new loco config after MFX-BIND command automatically; optional timeout in ms (default %d)\n", g_bind_timeout_ms);
            printf("  --no-gzip          Disable serving precompressed .gz files (default is enabled)\n");
            printf("  --verbose          Verbose logging\n");
            return 0;
        }
    }

    // Resolve UDP target (hostname or numeric) to IPv4 dotted string
    g_udp_ip = resolve_hostname_to_ipv4(g_udp_ip);

    // Load config files
    parse_lokomotive_cs2(fs::path(g_config_dir)/"config"/"lokomotive.cs2");
    parse_magnetartikel_cs2(fs::path(g_config_dir)/"config"/"magnetartikel.cs2");
    load_overrides();

    // Initialize state maps
    for (auto &kv : g_locos) {
        g_loco_speed[kv.first] = 0;
        g_loco_dir[kv.first] = 1;
        g_loco_fn[kv.first] = {};
    }

    // Start UDP listener stub
    std::atomic<bool> stop_flag{false};
    std::thread rx(udp_listener_thread, std::ref(stop_flag));

    httplib::Server svr;
    // Increase worker threads to avoid starvation with long-lived SSE requests
    svr.new_task_queue = [] { return new httplib::ThreadPool(32); };

    // Static assets from src/frontend
    // If --frontend is provided, use it. Otherwise derive from executable path.
    fs::path frontend_dir;
    if (!g_frontend_dir_override.empty()) {
        frontend_dir = fs::path(g_frontend_dir_override);
    } else {
        // Executable is typically at src/backend_cpp/build-*/.../mswebapp_cpp
        // Go up 4 levels to reach src then append frontend
        auto base_dir = fs::path(argv[0]).parent_path().parent_path().parent_path().parent_path();
        frontend_dir = base_dir / "frontend";
    }
    fs::path static_dir = frontend_dir / "static";
    fs::path templates_dir = frontend_dir / "templates";

    // Root: serve template with simple replacement and explicit no-cache
    auto serve_template = [&](const fs::path &p, httplib::Response &res){
        std::ifstream f(p, std::ios::binary);
        if (!f) { res.status = 404; return; }
        std::ostringstream buf; buf << f.rdbuf();
        std::string html = buf.str();
        // Replace {{ config_path }} occurrences
        const std::string needle = "{{ config_path }}";
        size_t pos = 0;
        while ((pos = html.find(needle, pos)) != std::string::npos) {
            html.replace(pos, needle.size(), g_public_cfg);
            pos += g_public_cfg.size();
        }
        res.set_content(html, "text/html; charset=utf-8");
        res.set_header("Cache-Control", "no-cache");
    };

    svr.Get("/", [&](const httplib::Request&, httplib::Response &res){
        serve_template(templates_dir/"index.html", res);
    });
    // Info page removed: info is now shown via in-page modal

    // Service worker (no-cache to ensure updates roll out)
    svr.Get("/sw.js", [&](const httplib::Request&, httplib::Response &res){
        std::ifstream f(frontend_dir/"sw.js", std::ios::binary);
        if (!f) { res.status = 404; return; }
        std::ostringstream buf; buf << f.rdbuf();
        res.set_content(buf.str(), "application/javascript");
        res.set_header("Cache-Control", "no-cache");
    });

    // Static: custom handler with ETag, long cache, and optional precompressed .gz delivery
    auto mime_of = [](const std::string& ext)->const char*{
        if (ext==".css") return "text/css";
        if (ext==".js")  return "application/javascript";
        if (ext==".png") return "image/png";
        if (ext==".jpg"||ext==".jpeg") return "image/jpeg";
        if (ext==".webp") return "image/webp";
        if (ext==".svg") return "image/svg+xml";
        if (ext==".ico") return "image/x-icon";
        if (ext==".json") return "application/json";
        return "application/octet-stream";
    };
    auto compute_etag = [](const fs::path& p)->std::string{
        std::error_code ec;
        uintmax_t sz = fs::file_size(p, ec);
        auto mt = fs::last_write_time(p, ec).time_since_epoch().count();
        return "\"" + std::to_string(static_cast<unsigned long long>(sz)) + "-" +
               std::to_string(static_cast<long long>(mt)) + "\"";
    };
    svr.Get(R"(/static/(.*))", [&](const httplib::Request& req, httplib::Response &res){
        std::string rel = req.matches[1].str();
        fs::path wanted = static_dir / rel;
        // Prefer precompressed .gz if client accepts gzip
        bool accept_gzip = req.has_header("Accept-Encoding") && req.get_header_value("Accept-Encoding").find("gzip") != std::string::npos;
        fs::path gz = wanted; gz += ".gz";
        bool use_gz = g_enable_precompressed_gzip && accept_gzip && fs::exists(gz);
        fs::path file = use_gz ? gz : wanted;
        std::error_code ec;
        if (!fs::exists(file, ec) || fs::is_directory(file, ec)) { res.status = 404; return; }

        std::string etag = compute_etag(file);
        auto inm = req.get_header_value("If-None-Match");
        if (!inm.empty() && inm == etag) {
            res.status = 304;
            res.set_header("ETag", etag.c_str());
            res.set_header("Cache-Control", "public, max-age=86400");
            res.set_header("Vary", "Accept-Encoding");
            return;
        }

        std::ifstream f(file, std::ios::binary);
        if (!f) { res.status = 500; return; }
        std::ostringstream buf; buf << f.rdbuf();
        std::string ext = wanted.extension().string();
        res.set_content(buf.str(), mime_of(ext));
        res.set_header("ETag", etag.c_str());
        res.set_header("Cache-Control", "public, max-age=86400");
        res.set_header("Vary", "Accept-Encoding");
        if (use_gz) res.set_header("Content-Encoding", "gzip");
    });

    // Config assets under /cfg => map to g_config_dir
    svr.Get((g_public_cfg + "/(.*)").c_str(), [&](const httplib::Request& req, httplib::Response &res){
        auto rel = req.matches[1];
        fs::path wanted = fs::path(g_config_dir)/rel.str();
        if (fs::is_directory(wanted)) { res.status = 404; return; }
        if (fs::exists(wanted)) {
            std::ifstream f(wanted, std::ios::binary);
            if (!f) { res.status = 500; return; }
            std::ostringstream buf; buf << f.rdbuf();
            // naive content-type
            std::string ct = "application/octet-stream";
            auto ext = wanted.extension().string();
            if (ext == ".png") ct = "image/png"; else if (ext == ".jpg"||ext==".jpeg") ct = "image/jpeg"; else if (ext == ".css") ct = "text/css"; else if (ext==".js") ct = "application/javascript";
            res.set_content(buf.str(), ct.c_str());
            res.set_header("Cache-Control", "no-cache");
        } else {
            // fallback to packaged static
            fs::path fallback = static_dir/rel.str();
            if (fs::exists(fallback)) {
                std::ifstream f(fallback, std::ios::binary);
                std::ostringstream buf; buf << f.rdbuf();
                res.set_content(buf.str(), "application/octet-stream");
                res.set_header("Cache-Control", "public, max-age=86400");
            } else {
                res.status = 404;
            }
        }
    });

    // API: loco list
    svr.Get("/api/loco_list", [&](const httplib::Request&, httplib::Response &res){
        res.set_content(loco_list_json(), "application/json");
    });

    // API: loco state (single or all)
    svr.Get("/api/loco_state", [&](const httplib::Request& req, httplib::Response &res){
        auto it = req.params.find("loco_id");
        if (it == req.params.end()) {
            res.set_content(loco_state_json(), "application/json");
            return;
        }
        int uid = 0; try { uid = std::stoi(it->second); } catch (...) { uid = 0; }
        std::ostringstream os; os << "{";
        int spd = g_loco_speed[uid]; int dir = g_loco_dir[uid];
        os << "\"speed\":" << spd << ",\"direction\":" << dir << ",\"functions\":{";
        bool first=true; for (auto &fk : g_loco_fn[uid]) { if (!first) os << ","; first=false; os << '"' << fk.first << '"' << ":" << (fk.second?1:0); }
        os << "}}";
        res.set_content(os.str(), "application/json");
    });

    // API: switch list (structure with artikel array of names)
    svr.Get("/api/switch_list", [&](const httplib::Request&, httplib::Response &res){
        // Build from parsed magnetartikel.cs2 if available
        int maxIdx = 63; for (auto &kv : g_switch_state) { if (kv.first > maxIdx) maxIdx = kv.first; }
        std::ostringstream os; os << "{\"artikel\":[";
        for (int i=0;i<=maxIdx;i++) {
            if (i>0) os << ",";
            std::string name = (i < (int)g_switches.size()) ? g_switches[i].name : std::string();
            int uid = (i < (int)g_switches.size() && g_switches[i].uid >= 0) ? g_switches[i].uid : i;
            os << "{\"name\":\"" << json_escape(name) << "\",\"uid\":" << uid;
            if (i < (int)g_switches.size() && !g_switches[i].dectyp.empty()) {
                os << ",\"typ\":\"" << json_escape(g_switches[i].dectyp) << "\"";
            }
            os << "}";
        }
        os << "]}";
        res.set_content(os.str(), "application/json");
    });

    // API: switch state array
    svr.Get("/api/switch_state", [&](const httplib::Request&, httplib::Response &res){
        std::ostringstream os; os << "{\"switch_state\":[";
        int maxIdx = -1; for (auto &kv : g_switch_state) { if (kv.first > maxIdx) maxIdx = kv.first; }
        if (maxIdx < 0) maxIdx = 63; // default
        for (int i=0;i<=maxIdx;i++) {
            if (i>0) os << ",";
            int val = 0; auto it = g_switch_state.find(i); if (it != g_switch_state.end()) val = it->second;
            os << val;
        }
        os << "]}";
        res.set_content(os.str(), "application/json");
    });

    // API: icons list
    svr.Get("/api/icons", [&](const httplib::Request&, httplib::Response &res){
        res.set_content(icons_list_json(), "application/json");
    });

    // API: set loco icon
    svr.Post("/api/loco_icon", [&](const httplib::Request& req, httplib::Response &res){
        // Expect JSON with loco_id and icon
        int uid = -1; std::string icon;
        auto body = req.body;
        auto find_num = [&](const char* key)->int{
            auto pos = body.find(key); if (pos==std::string::npos) return -1; pos = body.find(':', pos); if (pos==std::string::npos) return -1; size_t j=pos+1; while (j<body.size() && (body[j]==' '||body[j]=='"')) j++; size_t k=j; while (k<body.size() && (isdigit((unsigned char)body[k])||body[k]=='-')) k++; if (k>j) return std::stoi(body.substr(j,k-j)); return -1; };
        auto find_str = [&](const char* key)->std::string{
            auto pos = body.find(key); if (pos==std::string::npos) return std::string(); pos = body.find(':', pos); if (pos==std::string::npos) return std::string(); size_t j=pos+1; while (j<body.size() && (body[j]==' ')) j++; if (j<body.size() && body[j]=='"') { j++; size_t k=j; while (k<body.size() && body[k] != '"') k++; if (k>j) return body.substr(j,k-j); } else { size_t k=j; while (k<body.size() && (isalnum((unsigned char)body[k]) || body[k]=='_' || body[k]=='-' || body[k]==' ')) k++; if (k>j) return body.substr(j,k-j); } return std::string(); };
        uid = find_num("loco_id"); icon = find_str("icon");
        if (uid <= 0 || icon.empty()) { res.status=400; res.set_content("{\"status\":\"error\",\"message\":\"loco_id and icon required\"}", "application/json"); return; }
        // Persist override and notify clients; do not change lokomotive.cs2
        g_icon_overrides[uid] = icon;
        if (!save_overrides()) { res.status=500; res.set_content("{\"status\":\"error\",\"message\":\"failed to save override\"}", "application/json"); return; }
        publish_event("{\"type\":\"loco_list_reloaded\"}");
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    // API: control_event
    svr.Post("/api/control_event", [&](const httplib::Request& req, httplib::Response &res){
        int uid=0; int speed=-1; int direction=-1; int fn=-1; int val=-1;
        auto body = req.body;
        auto find_num = [&](const char* key)->int{
            auto pos = body.find(key);
            if (pos==std::string::npos) return -1;
            pos = body.find(':', pos);
            if (pos==std::string::npos) return -1;
            size_t j = pos+1; while (j<body.size() && (body[j]==' '||body[j]=='"')) j++;
            size_t k=j; while (k<body.size() && (isdigit((unsigned char)body[k])||body[k]=='-')) k++;
            if (k>j) return std::stoi(body.substr(j,k-j));
            return -1;
        };
        uid = find_num("loco_id");
        speed = find_num("speed");
        direction = find_num("direction");
        fn = find_num("function"); if (fn<0) fn = find_num("fn");
        val = find_num("value"); if (val<0) val = find_num("val");
        if (uid<=0) { res.status=400; res.set_content("{\"status\":\"error\",\"message\":\"loco_id required\"}","application/json"); return; }
        if (speed>=0) {
            int spd = std::max(0,std::min(1023,speed));
            g_loco_speed[uid] = spd; publish_event(speed_event_json(uid, spd));
            uint32_t can_id = build_can_id((uint32_t)g_device_uid, CMD_SPEED, 0, 0);
            udp_send_frame(can_id, payload_speed((uint32_t)uid, spd), 6);
        } else if (direction>=0) {
            g_loco_dir[uid] = direction; publish_event(direction_event_json(uid, g_loco_dir[uid]));
            uint32_t can_id = build_can_id((uint32_t)g_device_uid, CMD_DIRECTION, 0, 0);
            udp_send_frame(can_id, payload_direction((uint32_t)uid, direction), 5);
        } else if (fn>=0) {
            bool on = (val!=0);
            g_loco_fn[uid][fn] = on; publish_event(function_event_json(uid, fn, on));
            uint32_t can_id = build_can_id((uint32_t)g_device_uid, CMD_FUNCTION, 0, 0);
            udp_send_frame(can_id, payload_function((uint32_t)uid, fn, on?1:0), 6);
        }
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    // API: keyboard_event (switch)
    svr.Post("/api/keyboard_event", [&](const httplib::Request& req, httplib::Response &res){
        int idx=-1, pos=-1;
        auto body = req.body;
        auto find_num = [&](const char* key)->int{
            auto pos = body.find(key);
            if (pos==std::string::npos) return -1;
            pos = body.find(':', pos);
            if (pos==std::string::npos) return -1;
            size_t j = pos+1; while (j<body.size() && (body[j]==' '||body[j]=='"')) j++;
            size_t k=j; while (k<body.size() && (isdigit((unsigned char)body[k])||body[k]=='-')) k++;
            if (k>j) return std::stoi(body.substr(j,k-j));
            return -1;
        };
        idx = find_num("idx"); pos = find_num("pos");
        if (idx<0 || pos<0) { res.status=400; res.set_content("{\"status\":\"error\",\"message\":\"idx and pos required\"}","application/json"); return; }
        g_switch_state[idx] = pos;
        publish_event(switch_event_json(idx, pos));
        {
            // Map index to UID if available from parsed magnetartikel.cs2
            int uid = idx;
            if (idx >= 0 && idx < (int)g_switches.size() && g_switches[idx].uid >= 0) {
                uid = g_switches[idx].uid;
            }
            uint32_t can_id = build_can_id((uint32_t)g_device_uid, CMD_SWITCH, 0, 0);
            udp_send_frame(can_id, payload_switch((uint32_t)uid, pos, 1), 6);
            int switch_delay_ms = 200; // default delay before auto-reset
            if (g_switches[idx].switch_delay > 0) { switch_delay_ms = g_switches[idx].switch_delay; }
            std::thread([can_id, uid, pos, switch_delay_ms]{
                std::this_thread::sleep_for(std::chrono::milliseconds(switch_delay_ms));
                udp_send_frame(can_id, payload_switch((uint32_t)uid, pos, 0), 6);
            }).detach();
        }
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    // API: stop_button
    svr.Post("/api/stop_button", [&](const httplib::Request& req, httplib::Response &res){
        // expects { state: true/false }
        bool state = req.body.find("\"state\":true") != std::string::npos || req.body.find("\"state\":1") != std::string::npos;
        g_running.store(state);
        publish_event(system_event_json());
        {
            uint32_t can_id = build_can_id((uint32_t)g_device_uid, CMD_SYSTEM, 0, 0);
            udp_send_frame(can_id, payload_system_state((uint32_t)g_device_uid, state), 5);
        }
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    // API: info_events (maps to a function trigger)
    svr.Post("/api/info_events", [&](const httplib::Request& req, httplib::Response &res){
        int loco_id = -1; int fn = 0; int value = 1;
        auto body = req.body;
        auto get_num = [&](const char* key, int defv)->int{
            auto pos = body.find(key); if (pos==std::string::npos) return defv;
            pos = body.find(':', pos); if (pos==std::string::npos) return defv;
            size_t j=pos+1; while (j<body.size() && (body[j]==' '||body[j]=='"')) j++;
            size_t k=j; while (k<body.size() && (isdigit((unsigned char)body[k])||body[k]=='-')) k++;
            if (k>j) return std::stoi(body.substr(j,k-j)); return defv;
        };
        loco_id = get_num("loco_id", -1);
        fn = get_num("function", 0);
        value = get_num("value", 1);
        if (loco_id < 0) { res.status=400; res.set_content("{\"status\":\"error\",\"message\":\"loco_id fehlt\"}", "application/json"); return; }
        // Reuse control_event logic
        uint32_t can_id = build_can_id((uint32_t)g_device_uid, CMD_FUNCTION, 0, 0);
        udp_send_frame(can_id, payload_function((uint32_t)loco_id, fn, (value!=0)?1:0), 6);
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    // API: events (SSE)
    svr.Get("/api/events", [&](const httplib::Request&, httplib::Response &res){
        res.set_header("Cache-Control", "no-cache");
        res.set_header("Content-Type", "text/event-stream");
        res.set_header("Connection", "keep-alive");
        // Simple admission control: if too many SSE clients are already connected,
        // reject to avoid exhausting the server's worker threads.
        {
            std::lock_guard<std::mutex> lk(g_subs_mtx);
            if (g_subscribers.size() >= kMaxSseClients) {
                res.status = 503;
                res.set_content("too many SSE clients\n", "text/plain");
                return;
            }
        }
        auto *sub = new Subscriber();
        {
            std::lock_guard<std::mutex> lk(g_subs_mtx);
            g_subscribers.insert(sub);
        }
        // initial snapshot
        {
            // Queue initial snapshot directly to this subscriber to avoid racing global queues
            std::unique_lock<std::mutex> lk(sub->mtx);
            sub->queue.push_back(system_event_json());
            for (auto &kv : g_locos) {
                int uid=kv.first;
                sub->queue.push_back(speed_event_json(uid, g_loco_speed[uid]));
                sub->queue.push_back(direction_event_json(uid, g_loco_dir[uid]));
                for (auto &fkv : g_loco_fn[uid]) sub->queue.push_back(function_event_json(uid, fkv.first, fkv.second));
            }
            for (auto &skv : g_switch_state) sub->queue.push_back(switch_event_json(skv.first, skv.second));
            lk.unlock();
            sub->cv.notify_one();
        }

        res.set_chunked_content_provider("text/event-stream", [sub](size_t, httplib::DataSink &sink) {
            std::unique_lock<std::mutex> lk(sub->mtx);
            while (true) {
                if (!sub->queue.empty()) {
                    auto msg = sub->queue.front(); sub->queue.erase(sub->queue.begin());
                    lk.unlock();
                    std::string line = "data: " + msg + "\n\n";
                    if (!sink.write(line.data(), line.size())) return false;
                    lk.lock();
                } else {
                    // Wait for new events, but also send a periodic keepalive to
                    // detect broken connections and keep intermediaries happy.
                    auto status = sub->cv.wait_for(lk, std::chrono::seconds(15));
                    if (sub->closed) return false;
                    if (status == std::cv_status::timeout && sub->queue.empty()) {
                        lk.unlock();
                        static const char ka[] = ": keepalive\n\n"; // SSE comment line
                        if (!sink.write(ka, sizeof(ka) - 1)) return false;
                        lk.lock();
                    }
                }
            }
            return true;
        }, [sub](bool /*success*/){ // done
            {
                std::lock_guard<std::mutex> lk(g_subs_mtx);
                sub->closed = true; g_subscribers.erase(sub);
            }
            delete sub;
        });
    });

    // API: health
    svr.Get("/api/health", [&](const httplib::Request&, httplib::Response &res){
        std::ostringstream os; os << "{"
            << "\"status\":\"ok\"," 
            << "\"system_state\":\"" << (g_running.load()?"running":"stopped") << "\"," 
            << "\"loco_count\":" << g_locos.size() << ","
            << "\"switch_count\":" << g_switch_state.size() << ","
            << "\"udp_target\":\"" << g_udp_ip << ":" << g_udp_tx << "\"}";
        res.set_content(os.str(), "application/json");
    });

    // Bind and run
    int http_port = svr.bind_to_port(g_bind_host.c_str(), g_http_port);
    if (http_port <= 0) {
    fprintf(stderr, "Failed to bind HTTP server on %s:%d\n", g_bind_host.c_str(), g_http_port);
    stop_flag.store(true);
    if (rx.joinable()) rx.join();
#ifdef _WIN32
    WSACleanup();
#endif
    return 1;
    }
    printf("Listening on %s:%d (config=%s)\n", g_bind_host.c_str(), g_http_port, g_config_dir.c_str());
    if (g_verbose) {
        printf("UDP target %s tx=%d rx=%d device_uid=%d (hash=%04X)\n", g_udp_ip.c_str(), g_udp_tx, g_udp_rx, g_device_uid, generate_hash((uint32_t)g_device_uid));
    }

    // Watcher thread for lokomotive.cs2 reloads (inotify on Linux/macOS, disabled on Windows)
    std::thread watcher([&](){
        fs::path lokfile = fs::path(g_config_dir)/"config"/"lokomotive.cs2";
#ifdef _WIN32
        (void)lokfile;
#else
        int fd = inotify_init1(IN_NONBLOCK);
        if (fd < 0) return;
        int wd = inotify_add_watch(fd, lokfile.string().c_str(), IN_CLOSE_WRITE | IN_MOVED_TO | IN_DELETE_SELF | IN_ATTRIB);
        if (wd < 0) { close(fd); return; }
        char buf[4096];
        while (!stop_flag.load()) {
            ssize_t n = read(fd, buf, sizeof(buf));
            if (n <= 0) { std::this_thread::sleep_for(std::chrono::milliseconds(200)); continue; }
            try {
                parse_lokomotive_cs2(lokfile);
                // Reinit state maps for new/removed locos preserving values where possible
                std::map<int,int> new_speed; std::map<int,int> new_dir; std::map<int,std::map<int,bool>> new_fn;
                for (auto &kv : g_locos) {
                    int uid = kv.first;
                    new_speed[uid] = g_loco_speed.count(uid)? g_loco_speed[uid] : 0;
                    new_dir[uid]   = g_loco_dir.count(uid)? g_loco_dir[uid] : 1;
                    new_fn[uid]    = g_loco_fn.count(uid)? g_loco_fn[uid] : std::map<int,bool>{};
                }
                g_loco_speed.swap(new_speed);
                g_loco_dir.swap(new_dir);
                g_loco_fn.swap(new_fn);
                publish_event("{\"type\":\"loco_list_reloaded\"}");
            } catch (...) {
            }
        }
        close(fd);
#endif
    });

    svr.listen_after_bind();

    stop_flag.store(true);
    stop_flag.store(true);
    if (watcher.joinable()) watcher.join();
    rx.join();

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
