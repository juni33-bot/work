#!/bin/sh

set -euo pipefail

# CONFIG_INI: [network] 섹션에서 ip/netmask/gateway 읽기
CONFIG_INI="${CONFIG_INI:-nc_enc_app_config.ini}"
IFACE="${IFACE:-eth0}"
APP_PATH="${APP_PATH:-.}"
CONFIG_FILE="${CONFIG_FILE:-/etc/myconfig/ip_config.conf}"

log() { echo "[netcfg] $*"; }

# --- INI parser ---
ini_get() {
  awk -F'=' -v key="$1" '
    /^\[network\]/{sec=1; next}
    /^\[/{sec=0}
    sec && tolower($1) ~ tolower(key) {
      gsub(/\r/,"",$2)
      gsub(/^[ \t]+|[ \t]+$/,"",$2)
      print $2
      exit
    }
  ' "$CONFIG_INI" 2>/dev/null || echo ""
}

# --- netmask -> CIDR(prefix) ---
mask_to_cidr() {
  MASK="$1"
  IFS=.
  set -- $MASK
  COUNT=0
  for OCTET in "$@"; do
    case $OCTET in
      255) COUNT=$((COUNT + 8));;
      254) COUNT=$((COUNT + 7));;
      252) COUNT=$((COUNT + 6));;
      248) COUNT=$((COUNT + 5));;
      240) COUNT=$((COUNT + 4));;
      224) COUNT=$((COUNT + 3));;
      192) COUNT=$((COUNT + 2));;
      128) COUNT=$((COUNT + 1));;
      0)   ;;
      *) echo "[netcfg] Invalid netmask: $MASK" >&2; return 1;;
    esac
  done
  echo "$COUNT"
}

# --- load from INI ---
IP_ADDR="$(ini_get ip)"
NETMASK="$(ini_get netmask)"
GATEWAY="$(ini_get gateway)"

IP_ADDR="${IP_ADDR:-192.168.13.246}"
NETMASK="${NETMASK:-255.255.255.0}"
GATEWAY="${GATEWAY:-192.168.13.254}"

CIDR="$(mask_to_cidr "$NETMASK")"
DESIRED_ADDR="$IP_ADDR/$CIDR"

# --- current kernel addr/gw ---
CUR_ADDR="$(ip -o -f inet addr show dev "$IFACE" 2>/dev/null | awk '{print $4}' | head -n1 || true)"
CUR_GW="$(ip route show default 2>/dev/null | awk '/^default/ {print $3; exit}' || true)"

if [ "${CUR_ADDR:-}" != "$DESIRED_ADDR" ] || [ "${CUR_GW:-}" != "$GATEWAY" ]; then
  log "Applying: addr=$DESIRED_ADDR gw=$GATEWAY dev=$IFACE"

  ip -4 addr flush dev "$IFACE"
  ip link set "$IFACE" up
  ip route del default 2>/dev/null || true

  ip addr add "$DESIRED_ADDR" dev "$IFACE"
  ip route add default via "$GATEWAY" dev "$IFACE"

  log "Network configured."
else
  log "No network changes needed."
fi

# --- write ip_config.conf ---
mkdir -p "$(dirname "$CONFIG_FILE")"
{
  echo "IP_ADDR=$IP_ADDR"
  echo "NETMASK=$NETMASK"
  echo "GATEWAY_ADDR=$GATEWAY"
} > "$CONFIG_FILE" 2>/dev/null || true

log "Wrote $CONFIG_FILE"