# Shared file UI controls

The wallpaper picker uses HnThumbnailDelegate around its existing asynchronous
thumbnail provider and HnPreviewFrame around its selected image. Shared navigation
is retained; loading and empty feedback use shared controls. The controller remains
the sole owner of pending selection, display targeting, Apply gating and conflict
resolution. Favorites and folder management remain in WallpaperModel. No File Browser
dependency is introduced.

Acceptance covers folder/favorites navigation, keyboard selection, unavailable images,
pending-versus-applied state, display targeting and external configuration conflicts.
Native visual/theme/fractional-scale acceptance uses host-built binaries.

Verification results and compatible provider revisions are recorded in the umbrella
[initiative ledger](../../../../docs/initiatives/shared-file-ui-controls/TASKS.md).
