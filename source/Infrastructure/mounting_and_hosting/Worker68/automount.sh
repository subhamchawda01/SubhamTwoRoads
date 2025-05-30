#!/bin/bash

# Define label-to-mount mappings
declare -A mount_points
mount_points=(
  [NSE_TBT_OPT]="/NSE_TBT_OPT"
  [USER]="/USER"
  [TRADING_DF]="/TRADING_DF"
  [NSE_L1]="/NSEL1Data"
)

# Define UUID-to-mount mappings
declare -A uuid_mount_points
uuid_mount_points=(
  [dfa6d305-73d5-4807-a467-5b02c1831785]="/NAS6"
  [574cf2ac-7d29-4c98-86ef-ac3d541eeab9]="/NAS8"
  [2dc95849-183d-4978-a91b-9a3990ed9741]="/NAS7"
  [b031362a-4157-487f-b354-4e7d5c8b4903]="/NAS9"
)

# Function to check and mount drives by label
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

# Function to check and mount drives by UUID
mount_drive_uuid() {
  local uuid="$1"
  local mount_point="$2"
  
  # Create mount point if not exists
  [[ -d "$mount_point" ]] || mkdir -p "$mount_point"
  
  # Check if already mounted
  if ! mount | grep -q "${mount_point}"; then
    mount -U "$uuid" "$mount_point" && echo "UUID $uuid mounted at $mount_point"
  else
    echo "UUID $uuid is already mounted at $mount_point"
  fi
}

# Iterate through all labels and mount them
for label in "${!mount_points[@]}"; do
  mount_drive "$label" "${mount_points[$label]}"
done

# Iterate through all UUIDs and mount them
for uuid in "${!uuid_mount_points[@]}"; do
  mount_drive_uuid "$uuid" "${uuid_mount_points[$uuid]}"
done
