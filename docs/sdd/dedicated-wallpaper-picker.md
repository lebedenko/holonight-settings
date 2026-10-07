# Dedicated Wallpaper Picker

The Settings process owns two independently opened, decorated windows. Running
`holonight-settings --wallpaper` opens only the Wallpaper picker; Settings models
and the audio service are initialized when Settings is first opened. The desktop
launcher provides **Change Wallpaper**, Appearance provides **Change…**, and the
shell desktop menu opens the picker for its connector.

D-Bus activation retains `org.holonight.Settings` and the freedesktop Application
interface. `Activate` opens Settings; existing page actions retain their meaning.
`ActivateAction("wallpaper", [connector], platformData)` opens or restores the
picker. Requests queue independently before each window exists. Repeated requests
retain pending selections; closing the last window exits the process.

The browser recursively discovers raster images in standard data locations under
`wallpapers/holonight`, Pictures, and added folders. It deduplicates canonical file
paths, excludes animation/vector images, skips directory symlink traversal, and
sorts by filename then path. Worker scans use generations to reject obsolete
results. Watched directories/files trigger debounced rescans. Thumbnails decode
on QThreadPool with a 32 MiB cache keyed by path, modification time, and bounded
requested size. Preview images use bounded asynchronous Qt decoding.

Folders and favorites persist immediately in
`$XDG_CONFIG_HOME/holonight/settings-wallpapers.ini`. The sidebar shows actual
folders, All Wallpapers, and Favorites through the same NavPanel as Settings.
Add Folder requests the desktop portal FileChooser with directory selection;
portal failures are shown inline. No artwork or external library is added.

The controller has an independent shell configuration document watcher and
conflict-aware writer. Only `[background].images` changes; string arrays use TOML
escaping and comments/unknown fields/unrelated concurrent changes survive saves.
Malformed configuration blocks Apply. Conflicting image-array edits retain the
pending assignment and require **Reload** or **Keep pending**.

Displays follow `QGuiApplication::screens()` order. Individual edits materialize
last-image fallback for connected displays and preserve trailing entries. An empty
configuration materializes empty paths (solid backgrounds) for untouched displays.
Apply to all displays uses one image and the shell's fallback for future displays.
Removing the target requires explicit reselection. Screen reordering keeps the
shell's existing positional reassignment behavior.

Selection changes the pending preview. Apply validates readable image files and
keeps the picker open after success. Closing discards assignments but keeps folder
and favorite preferences. Previews show the whole image without cropping; Apply
only writes image paths. The background service owns wallpaper rendering. Slideshow, fit modes,
curated categories, and workspace assignments are deferred.

Verification covers string escaping, empty/fallback/trailing arrays, isolated
storage failures, concurrent edits/conflicts, malformed documents, recursive
source deduplication, corrupt/deleted files, favorites persistence, thumbnail
invalidation, activation queues, both opening orders, and independent closing.
