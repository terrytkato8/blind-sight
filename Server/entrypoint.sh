#!/usr/bin/env bash
set -euo pipefail

# BS_SERVER_KEY authorises this process to write progression. It must arrive from the
# orchestrator's secret store, never baked into the image.
if [[ -z "${BS_SERVER_KEY:-}" ]]; then
  echo "WARNING: BS_SERVER_KEY unset — this server will run but nothing will persist." >&2
fi

# Allocation platforms inject the public address so clients can reach us through NAT.
# Agones: use the SDK; GameLift/Edgegap/Hathora: environment variables.
if [[ -z "${BS_ADVERTISE_ADDR:-}" ]]; then
  BS_ADVERTISE_ADDR="$(curl -fsS --max-time 3 https://api.ipify.org || echo 127.0.0.1):${BS_PORT}"
  export BS_ADVERTISE_ADDR
fi

echo "Blind Sight dedicated server"
echo "  map      : ${BS_MAP}"
echo "  mode     : ${BS_MODE}"
echo "  region   : ${BS_REGION}"
echo "  advertise: ${BS_ADVERTISE_ADDR}"
echo "  backend  : ${BS_BACKEND_URL:-<default from ini>}"

ARGS=(
  "${BS_MAP}?game=${BS_MODE}?listen"
  -server
  -log
  -port="${BS_PORT}"
  -BSAdvertise="${BS_ADVERTISE_ADDR}"
  -nosteam
)

# Allow overriding the backend URL without rebuilding the pak.
if [[ -n "${BS_BACKEND_URL:-}" ]]; then
  ARGS+=( -ini:Game:[/Script/BlindSight.BSGameSettings]:BackendBaseUrl="${BS_BACKEND_URL}" )
fi

exec ./BlindSightServer.sh "${ARGS[@]}"
