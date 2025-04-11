#!/bin/bash

# Get nodes with all three role labels: amd, gmc-cp, and worker
NODES=$(oc get nodes -o json | jq -r '.items[] | select(
  .metadata.labels["node-role.kubernetes.io/amd"] != null and
  .metadata.labels["node-role.kubernetes.io/gmc-cp"] != null and
  .metadata.labels["node-role.kubernetes.io/worker"] != null
) | .metadata.name')

# Loop over each matching node
for NODE in $NODES; do
  echo "🔧 Processing $NODE..."

  # Run oc debug and create the tar.gz inside the node's host context
  oc debug node/$NODE -- chroot /host tar -czf /tmp/cgroup_snapshot.tar.gz -C /sys/fs/cgroup . || {
    echo "❌ Failed to archive on $NODE"
    continue
  }

  # Copy the tar.gz file from the node's host to the local machine
  echo "📦 Copying tarball from $NODE to local..."
  oc debug node/$NODE -- chroot /host cat /tmp/cgroup_snapshot.tar.gz > /tmp/cgroup_snapshot_$NODE.tar.gz || {
    echo "❌ Failed to copy from $NODE"
    continue
  }

  echo "✅ Finished $NODE: /tmp/cgroup_snapshot_$NODE.tar.gz"
done
