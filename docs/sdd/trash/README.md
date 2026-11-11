# Trash settings

The Trash page edits automatic deletion, the retention period in whole days
and the NFS exports included in cleanup. The page shows active NFS exports
and saved offline opt-ins. Settings uses the provider's parser and serializer
for `$XDG_CONFIG_HOME/holonight/trash.toml` and replaces the file atomically on
Save. Discard reloads the policy. If the file changed on disk, the footer
reports the conflict and preserves pending edits.

The default is disabled with seven days suggested. The allowed range is
one to 365 days. Only explicitly selected NFS mount point and export source
pairs participate in retention.
