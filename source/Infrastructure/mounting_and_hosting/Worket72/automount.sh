#!/bin/bash

# Define label-to-mount mappings
declare -A mount_points
mount_points=(
  [INFRASTORE]="/INFRASTORE"
  [TRADER]="/TRADER"
  [NSE_OPT_TBT]="/NSE_OPT_TBT"
  [NSE_STK_OPT_TBT]="/NSE_STK_OPT_TBT"
)

# Function to check and mount drives
mount_drive() {
  local label="$1"
  local mount_point="$2"
  local device
  
  # Find the device by label
  device=$(lsblk -o NAME,LABEL -nr | awk -v lbl="$label" '$2 == lbl {print "/dev/"$1}')
  
  if [[ -n "$device" ]]; then
    # Create mount point if not exists
    [[ -d "$mount_point" ]] || mkdir -p "$mount_point"
    
    # Check if already mounted
    if ! mount | grep -q "${mount_point}"; then
      mount "$device" "$mount_point" && echo "$label mounted at $mount_point"
    else
      echo "$label is already mounted at $mount_point"
    fi
  else
    echo "Device with label $label not found"
  fi
}

# Iterate through all labels and mount them
for label in "${!mount_points[@]}"; do
  mount_drive "$label" "${mount_points[$label]}"
done
