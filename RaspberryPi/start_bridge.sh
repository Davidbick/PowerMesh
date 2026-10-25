
#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PYTHON="$SCRIPT_DIR/.venv/bin/python"
BRIDGE="$SCRIPT_DIR/tun_uart_bridge.py"

find_serial_port() {
  local port
  local -a preferred=() by_id=() acm=()

  shopt -s nullglob

  for port in /dev/serial/by-id/*; do
    [[ -e "$port" ]] || continue
    by_id+=("$port")

    case "$(basename -- "$port")" in
      *STMicroelectronics*|*STLink*|*STLINK*|*ST-LINK*)
        preferred+=("$port")
        ;;
    esac
  done

  if (( ${#preferred[@]} == 1 )); then
    printf '%s\n' "${preferred[0]}"
    return 0
  fi

  if (( ${#by_id[@]} == 1 )); then
    printf '%s\n' "${by_id[0]}"
    return 0
  fi

  for port in /dev/ttyACM*; do
    if [[ -e "$port" ]]; then
      acm+=("$port")
    fi
  done

  if (( ${#acm[@]} == 1 )); then
    printf '%s\n' "${acm[0]}"
    return 0
  fi

  return 1
}

if [[ ! -x "$PYTHON" ]]; then
  echo "Python environment missing. Run: bash RaspberryPi/install.sh" >&2
  exit 1
fi

if [[ -n "${POWERMESH_SERIAL:-}" ]]; then
  PORT="$POWERMESH_SERIAL"
  echo "Using configured STM32 serial device: $PORT"

  while [[ ! -e "$PORT" ]]; do
    sleep 2
  done
else
  echo "Waiting for an STM32 USB serial device..."

  until PORT="$(find_serial_port)"; do
    sleep 2
  done

  echo "Found STM32 serial device: $PORT"
fi


exec "$PYTHON" -u "$BRIDGE" "$PORT"
