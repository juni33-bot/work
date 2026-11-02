#!/bin/sh

if [ ! $APP_PATH ]; then
APP_PATH=.
fi

# clear app option
if [ "${1:-}" = "-reset" ]; then
  if ps | grep -q '[a]pp_vcodec'; then
    killall app_vcodec || true
    sleep 1
fi
  if ps | grep -q '[m]ediamtx'; then
    killall mediamtx || true
    sleep 1
  fi
  shift
fi


IP_CONFIG_FILE="/etc/myconfig/ip_config.conf"
IFACE="eth0"

if [ ! $MODULE_INI ]; then
    MODULE_INI=${APP_PATH}/misc/nc_enc_app_config.ini
fi

NETCFG_SH="./set_network_ip.sh"

mode=$(
awk -F'=' '
  BEGIN{ IGNORECASE=1; sec="" }
  NR==1 { sub(/^\xef\xbb\xbf/,"") }                  
  /^\s*[;#]/ || /^\s*$/ { next }                        
  /^\s*\[.*\]\s*$/ {                                    
      sec=$0; gsub(/^\s*\[|\]\s*$/,"",sec); sec=tolower(sec); next
  }
  sec=="app" {
      k=$1; v=$2
      gsub(/^[ \t]+|[ \t]+$/,"",k)
      gsub(/^[ \t]+|[ \t]+$/,"",v)
      k=tolower(k); v=tolower(v)
      if (k=="mode") { print v; exit }
  }
' "$MODULE_INI" 
)

if [ "$mode" = "rtsp" ]; then
  echo "[INFO] RTSP mode detected"

  # 1) network setting
  CONFIG_INI="$MODULE_INI" IFACE="$IFACE" "$NETCFG_SH"

  # 2) run mediamtx
  cd "${APP_PATH}/misc/vcodec"
  if [ ! -x ./mediamtx ]; then
    echo "[app] mediamtx binary not found or not executable in $(pwd)" >&2
    exit 1
  fi
  ./mediamtx &
  cd ../../
  sleep 3
else
  log "mode=$mode (skip netcfg + mediamtx)"
fi
  
# vcodec clk divide
# devmem 0x6004600 32 0x06      # CODEC_BPU_CLKDIV    (PLL : 2400 Mhz, MAX : 400 Mhz)
# devmem 0x6004604 32 0x04      # CODEC_CORE_CLKDIV   (PLL : 2000 Mhz, MAX : 500 Mhz)
# devmem 0x6004608 32 0x06      # HWA_AXI_CLKDIV      (PLL : 2000 Mhz, MAX : 500 Mhz)
# devmem 0x600460c 32 0x10      # HWA_APB_CLKDIV      (PLL : 2000 Mhz, MAX : 200 Mhz)

export LD_LIBRARY_PATH=${APP_PATH}/misc/lib
# export VIDEO_CODEC_FIRMWARE_FILE=/lib/wave_fw/chagall.bin

insmod ${APP_PATH}/../modules/vpu.ko

${APP_PATH}/app_vcodec