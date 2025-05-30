#!/bin/bash

CONFIG_FILE="/home/pengine/prod/live_configs/ind_to_ip_map.cfg"
MODE="$1"

declare -A data_map

if [[ ! -f "$CONFIG_FILE" ]]; then
  echo "Configuration file not found!"
  exit 1
fi

print_map() {
  local -n map_ref=$1
  echo "Contents of the map:"
  for key in "${!map_ref[@]}"; do
    echo "$key -> ${map_ref[$key]}"
  done
}

case "$MODE" in
  SIMPLE)
    while read -r col1 col2 _; do
      data_map["$col1"]="$col2"
    done < "$CONFIG_FILE"

    print_map data_map
    ;;
  NSE)
    while read -r col1 col2 col3 _; do
      if [[ "$col3" == "N" ]]; then
        data_map["$col1"]="$col2"
      fi
    done < "$CONFIG_FILE"

    print_map data_map
    ;;
  BSE)
    while read -r col1 col2 col3 _; do
      if [[ "$col3" == "B" ]]; then
        data_map["$col1"]="$col2"
      fi
    done < "$CONFIG_FILE"

    print_map data_map
    ;;
  CBOE)
    while read -r col1 col2 col3 _; do
      if [[ "$col3" == "I" ]]; then
        data_map["$col1"]="$col2"
      fi
    done < "$CONFIG_FILE"

    print_map data_map
    ;;
  LOCAL)
    while read -r col1 col2 col3 _; do
      if [[ "$col3" == "L" ]]; then
        data_map["$col1"]="$col2"
      fi
    done < "$CONFIG_FILE"

    print_map data_map
    ;;
  *)
    echo "Unknown mode: $MODE"
    exit 1
    ;;
esac


## Use the map accordingly in code...
